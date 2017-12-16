#include <memory>
#include <thread>
#include <iostream>
#include <fstream>

#include <server_http.hpp>

#include "wigner_web/server/session.h"

#define INDEX_PATH "../../webapp/www/index.html"

using namespace std;
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

int main(int argc, char* argv[]){
    wigner_web::server::Session session;

    HttpServer server;
    server.config.port = 8080;

    // TODO: Only works when invoked in the same directory
    std::string index_path = INDEX_PATH;


    server.resource["^/internal$"]["POST"] = [&session](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){
        string resp = session.process(request->content.string());
        *response << "HTTP/1.1 200 OK\r\n"
                  << "Content-Length: " << resp.length() << "\r\n\r\n"
                  << resp;
    };

    server.default_resource["GET"] = [&index_path](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){

        auto ifs = make_shared<ifstream>();
        ifs->open(index_path, ifstream::in | ios::binary | ios::ate);

        if(*ifs) {
            auto length = ifs->tellg();
            ifs->seekg(0, ios::beg);

            SimpleWeb::CaseInsensitiveMultimap header;
            header.emplace("Content-Length", to_string(length));
            response->write(header);

            // Trick to define a recursive function within this scope (for example purposes)
            class FileServer {
                public:
                    static void read_and_send(const shared_ptr<HttpServer::Response> &response, const shared_ptr<ifstream> &ifs) {
                        // Read and send 128 KB at a time
                        static vector<char> buffer(131072); // Safe when server is running on one thread
                        streamsize read_length;
                        if((read_length = ifs->read(&buffer[0], static_cast<streamsize>(buffer.size())).gcount()) > 0) {
                            response->write(&buffer[0], read_length);
                            if(read_length == static_cast<streamsize>(buffer.size())) {
                                response->send([response, ifs](const SimpleWeb::error_code &ec) {
                                        if(!ec) read_and_send(response, ifs);
                                        else cerr << "Connection interrupted" << endl;
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

    thread server_thread([&server](){
        server.start();
    });

    server_thread.join();
    return 0;
}
