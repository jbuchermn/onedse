#include <memory>
#include <thread>
#include <iostream>
#include <fstream>
#include <experimental/filesystem>

#include <server_http.hpp>
#include "server/session/session.h"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using Session = server::session::Session;

namespace fs = std::experimental::filesystem;

int main(int argc, char* argv[]){
    std::string arg0(argv[0]);

    Session session(arg0);

    HttpServer server;
    server.config.port = 8080;

    fs::path executable_path(arg0);
    fs::path index_html = executable_path.parent_path();

    while(!fs::exists(index_html / "js")){
        index_html /= "..";
    }

    index_html = fs::canonical(index_html / "js" / "www" / "index.html");
    std::string index_path = index_html.string();


    server.resource["^/internal$"]["POST"] = [&session](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request){
        std::string resp = session.process(request->content.string());
        *response << "HTTP/1.1 200 OK\r\n"
                  << "Content-Length: " << resp.length() << "\r\n\r\n"
                  << resp;
    };

    server.default_resource["GET"] = [&index_path](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request){

        auto ifs = std::make_shared<std::ifstream>();
        ifs->open(index_path, std::ifstream::in | std::ios::binary | std::ios::ate);

        if(*ifs) {
            auto length = ifs->tellg();
            ifs->seekg(0, std::ios::beg);

            SimpleWeb::CaseInsensitiveMultimap header;
            header.emplace("Content-Length", to_string(length));
            response->write(header);

            // Trick to define a recursive function within this scope (for example purposes)
            class FileServer {
                public:
                    static void read_and_send(const std::shared_ptr<HttpServer::Response> &response, const std::shared_ptr<std::ifstream> &ifs) {
                        // Read and send 128 KB at a time
                        static std::vector<char> buffer(131072); // Safe when server is running on one thread
                        std::streamsize read_length;
                        if((read_length = ifs->read(&buffer[0], static_cast<std::streamsize>(buffer.size())).gcount()) > 0) {
                            response->write(&buffer[0], read_length);
                            if(read_length == static_cast<std::streamsize>(buffer.size())) {
                                response->send([response, ifs](const SimpleWeb::error_code &ec) {
                                        if(!ec) read_and_send(response, ifs);
                                        else std::cerr << "Connection interrupted" << std::endl;
                                });
                            }
                        }
                    }
            };
            FileServer::read_and_send(response, ifs);    
        }else{
            std::string message = "Could not find index.html: " + index_path;
            *response << "HTTP/1.1 404 NOT FOUND\r\n"
                      << "Content-Length: " << message.length() << "\r\n\r\n"
                      << message;
        }
    };

    std::cout << "Starting server on port " << server.config.port << "... " << std::endl;
    server.start();
    return 0;
}
