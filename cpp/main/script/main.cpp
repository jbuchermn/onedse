#include <iostream>
#include <fstream>
#include <streambuf>
#include <vector>
#include <string>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "core/lua/system.h"
#include "core/utility/paths.h"

namespace fs = std::filesystem;
using json = nlohmann::json;
using System = core::lua::System;

namespace{
    int usage(){
        std::cout << "Usage: onedse [options] script.lua\n"
                  << "\n"
                  << "Runs a Lua script and writes its plots to <script>/<timestamp>/data.json\n"
                  << "\n"
                  << "Options:\n"
                  << "  --out DIR   write data.json to DIR instead\n"
                  << "  --plot      render the results with matplotlib (python/plot.py)\n"
                  << "  --check     only run the script, do not write anything\n"
                  << "  --quiet     do not echo the script's output\n";
        return 1;
    }
}

int main(int argc, char* argv[]){
    bool check = false, plot = false, quiet = false;
    std::string out;
    std::string script_arg;

    for(int i=1; i<argc; i++){
        std::string arg(argv[i]);
        if(arg == "--check") check = true;
        else if(arg == "--plot") plot = true;
        else if(arg == "--quiet") quiet = true;
        else if(arg == "--out" && i+1<argc) out = argv[++i];
        else if(arg == "--help" || arg == "-h") return usage();
        else if(script_arg.empty() && arg.rfind("--", 0) != 0) script_arg = arg;
        else return usage();
    }
    if(script_arg.empty()) return usage();

    fs::path script(script_arg);
    if(!fs::exists(script)){
        std::cerr << "Could not open " << script.string() << std::endl;
        return 2;
    }
    script = fs::canonical(script);

    std::ifstream input(script.string());
    std::string lua((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    /*
     * Execute
     */
    json result;
    result["plots"] = json::array();
    std::string stdout_text, stderr_text;
    size_t plot_count = 0;

    System system([&](const json& message){
        const std::string type = message.at("type");
        if(type == "stdout"){
            if(!quiet) std::cout << message.at("text").get<std::string>() << std::flush;
            stdout_text += message.at("text").get<std::string>();
        }else if(type == "stderr"){
            std::cerr << message.at("text").get<std::string>() << std::flush;
            stderr_text += message.at("text").get<std::string>();
        }else if(type == "plot"){
            plot_count++;
            if(!check) result["plots"].push_back(message);
        }
    });

    auto start = std::chrono::steady_clock::now();
    bool ok = system.execute(lua);
    double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    std::cerr << (ok ? "Finished" : "FAILED") << " after " << elapsed << "s, " << plot_count << " plot frames" << std::endl;
    if(check) return ok ? 0 : 3;

    /*
     * Write result
     */
    fs::path result_folder;
    if(!out.empty()){
        result_folder = out;
    }else{
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        result_folder = fs::current_path() / script.stem() / std::to_string(ms);
    }
    fs::create_directories(result_folder);

    result["stdout"] = stdout_text;
    result["stderr"] = stderr_text;
    std::ofstream output((result_folder/"data.json").string());
    output << result.dump();
    output.close();
    std::cerr << "Results written to " << (result_folder/"data.json").string() << std::endl;

    if(plot){
        try{
            fs::path python = core::utility::data_dir("python") / "plot.py";
            std::string command = "python3 \"" + python.string() + "\" \"" + result_folder.string() + "\"";
            if(::system(command.c_str()) != 0) std::cerr << "Plot script failed" << std::endl;
        }catch(const std::exception& e){
            std::cerr << e.what() << std::endl;
        }
    }

    return ok ? 0 : 3;
}
