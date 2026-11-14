#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "core/lua/system.h"
#include "core/utility/interrupt.h"
#include "core/utility/paths.h"

namespace fs = std::filesystem;
using json = nlohmann::json;
using System = core::lua::System;

namespace{
    /**
     * A running script. The interpreter thread pushes NDJSON lines, the HTTP
     * thread streams them to the browser as they arrive.
     */
    struct Job{
        std::mutex mutex;
        std::condition_variable cv;
        std::deque<std::string> lines;
        bool done = false;
        bool abandoned = false;
        std::thread thread;

        static constexpr size_t max_queue = 32;

        void push(const json& message, bool force=false){
            std::unique_lock<std::mutex> lock(mutex);
            // Backpressure: do not compute faster than the browser can receive
            cv.wait(lock, [&]{ return force || lines.size()<max_queue || abandoned || core::utility::interrupt_flag.load(); });
            if(abandoned) return;
            lines.push_back(message.dump() + "\n");
            cv.notify_all();
        }
    };

    std::mutex job_mutex;
    std::shared_ptr<Job> current_job;

    void stop_current_job(){
        // job_mutex is held by the caller
        if(!current_job) return;
        core::utility::interrupt_flag = true;
        current_job->cv.notify_all();
        if(current_job->thread.joinable()) current_job->thread.join();
        core::utility::interrupt_flag = false;
        current_job.reset();
    }

    std::shared_ptr<Job> start_job(const std::string& code){
        std::lock_guard<std::mutex> lock(job_mutex);
        stop_current_job();

        auto job = std::make_shared<Job>();
        job->thread = std::thread([job, code]{
            auto start = std::chrono::steady_clock::now();
            bool ok = false;
            try{
                System system([&job](const json& message){ job->push(message); }, /* sandbox */ true);
                ok = system.execute(code);
            }catch(const std::exception& e){
                job->push({ {"type", "stderr"}, {"text", std::string(e.what()) + "\n"} }, true);
            }
            double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            job->push({ {"type", "done"}, {"ok", ok}, {"elapsed", elapsed} }, true);

            std::lock_guard<std::mutex> lock(job->mutex);
            job->done = true;
            job->cv.notify_all();
        });
        current_job = job;
        return job;
    }

    /// The first comment block of an example is its title and description
    json describe_example(const fs::path& path){
        std::ifstream input(path.string());
        std::string code((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

        std::string title, description;
        std::istringstream lines(code);
        std::string line;
        while(std::getline(lines, line)){
            if(line.rfind("--", 0) != 0) break;
            std::string text = line.substr(2);
            text.erase(0, text.find_first_not_of(" \t"));
            if(title.empty()) title = text;
            else description += (description.empty() ? "" : " ") + text;
        }
        if(title.empty()) title = path.stem().string();

        return { {"name", path.stem().string()}, {"title", title}, {"description", description}, {"code", code} };
    }

    bool same_origin(const httplib::Request& req){
        // Scripts are executed on this machine: never accept requests triggered by other web sites
        if(!req.has_header("Origin")) return true;
        std::string origin = req.get_header_value("Origin");
        auto pos = origin.find("://");
        return pos != std::string::npos && origin.substr(pos+3) == req.get_header_value("Host");
    }
}

int main(int argc, char* argv[]){
    std::string host = "127.0.0.1";
    int port = 8080;

    for(int i=1; i<argc; i++){
        std::string arg(argv[i]);
        if(arg == "--port" && i+1<argc) port = std::stoi(argv[++i]);
        else if(arg == "--host" && i+1<argc) host = argv[++i];
        else{
            std::cout << "Usage: onedse_server [--host 127.0.0.1] [--port 8080]\n";
            return 1;
        }
    }

    fs::path web_dir, lua_dir;
    try{
        web_dir = core::utility::data_dir("web");
        lua_dir = core::utility::data_dir("lua");
    }catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
        return 2;
    }

    httplib::Server server;
    server.set_mount_point("/", web_dir.string());
    server.set_default_headers({ {"Cache-Control", "no-store"} });

    server.Get("/api/examples", [&lua_dir](const httplib::Request&, httplib::Response& res){
        std::vector<fs::path> files;
        for(auto& entry: fs::directory_iterator(lua_dir)){
            if(entry.path().extension() == ".lua") files.push_back(entry.path());
        }
        std::sort(files.begin(), files.end());

        json examples = json::array();
        for(auto& f: files) examples.push_back(describe_example(f));
        res.set_content(examples.dump(), "application/json");
    });

    server.Post("/api/run", [](const httplib::Request& req, httplib::Response& res){
        if(!same_origin(req)){
            res.status = 403;
            return;
        }

        std::string code;
        try{
            code = json::parse(req.body).at("lua").get<std::string>();
        }catch(const std::exception& e){
            res.status = 400;
            res.set_content(std::string("Bad request: ") + e.what(), "text/plain");
            return;
        }

        auto job = start_job(code);
        res.set_chunked_content_provider("application/x-ndjson",
            [job](size_t, httplib::DataSink& sink){
                std::deque<std::string> lines;
                bool done;
                {
                    std::unique_lock<std::mutex> lock(job->mutex);
                    job->cv.wait(lock, [&]{ return !job->lines.empty() || job->done; });
                    lines.swap(job->lines);
                    done = job->done;
                    job->cv.notify_all();
                }
                for(auto& line: lines){
                    if(!sink.write(line.data(), line.size())) return false;
                }
                if(done) sink.done();
                return true;
            },
            [job](bool success){
                // Browser went away (or stream is complete): make sure the interpreter does not block
                std::lock_guard<std::mutex> lock(job->mutex);
                job->abandoned = true;
                job->cv.notify_all();
            });
    });

    server.Post("/api/stop", [](const httplib::Request& req, httplib::Response& res){
        if(!same_origin(req)){
            res.status = 403;
            return;
        }
        // Only raise the flag: the job reports "done" through its own stream
        if(current_job) core::utility::interrupt_flag = true;
        res.set_content("{}", "application/json");
    });

    if(!server.bind_to_port(host, port)){
        std::cerr << "Could not bind to " << host << ":" << port << " (already in use? try --port)" << std::endl;
        return 3;
    }
    std::cout << "onedse is running on http://" << (host == "0.0.0.0" ? "localhost" : host) << ":" << port << std::endl;
    if(host != "127.0.0.1" && host != "localhost"){
        std::cout << "WARNING: listening on " << host << " - everybody who can reach this port can run Lua scripts on this machine" << std::endl;
    }
    server.listen_after_bind();

    {
        std::lock_guard<std::mutex> lock(job_mutex);
        stop_current_job();
    }
    return 0;
}
