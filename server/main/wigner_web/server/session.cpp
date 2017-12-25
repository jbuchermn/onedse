#include <iostream>
#include <json.hpp>
#include <map>
#include <string>
#include <fstream>
#include <streambuf>
#include <LuaContext.hpp>

#include "wigner_web/server/session.h"
#include "wigner_web/discretization/basis.h"
#include "wigner_web/state/density_operator.h"
#include "wigner_web/state/wigner_function.h"
#include "wigner_web/utility/files.h"

using json = nlohmann::json;

namespace wigner_web::server{
    Session::process_error::process_error(std::string message): std::runtime_error(message){}

    int Session::id_counter = 0;

    Session::Session(): id(id_counter){ id_counter++; }


    std::string Session::read_lua() const{
        std::ifstream input(wigner_web::utility::files::session_lua(id));
        return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    }

    void Session::save_lua(std::string lua) const{
        std::ofstream output(wigner_web::utility::files::session_lua(id));
        output<<lua;
    }

    std::string Session::process(const std::string& request){
        json req = json::parse(request);
        json resp;

        try{
            if(!req.count("command")) throw process_error("No command given!");

            if(req["command"] == "execute"){
                if(!req.count("lua")) throw process_error("No code given!");
                try{
                    save_lua(req["lua"]);
                    system.execute(req["lua"], resp["data"]);
                }catch(const LuaContext::ExecutionErrorException& ex){
                    throw process_error(std::string("Exception in Lua code: ")+ex.what());
                }catch(const LuaContext::SyntaxErrorException& ex){
                    throw process_error(std::string("Syntax error in Lua code: ")+ex.what());
                }catch(const LuaContext::WrongTypeException& ex){
                    throw process_error(std::string("Wrong type in Lua code: ")+ex.what());
                }


            }else if(req["command"] == "get"){
                resp["data"]["lua"] = read_lua();
            }
        }catch(process_error& ex){
            resp["error"] = ex.what();
        }

        return resp.dump();
    }
}
