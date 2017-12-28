#include <iostream>
#include <json.hpp>
#include <map>
#include <string>
#include <fstream>
#include <streambuf>
#include <experimental/filesystem>
#include <LuaContext.hpp>

#include "server/session/session.h"
#include "core/discretization/basis.h"
#include "core/state/density_operator.h"
#include "core/state/wigner_function.h"

namespace fs = std::experimental::filesystem;
using json = nlohmann::json;

namespace server::session{
    Session::process_error::process_error(std::string message): std::runtime_error(message){}

    int Session::id_counter = 0;

    Session::Session(std::string arg0_): id(id_counter), arg0(arg0_){ id_counter++; }

    std::string Session::lua_file() const{
        fs::path executable_path(arg0);
        fs::path lua_folder = executable_path.parent_path();

        while(!fs::exists(lua_folder / "lua")){
            lua_folder /= "..";
        }

        lua_folder /= "lua";

        if(!fs::exists(lua_folder)){
            fs::create_directory(lua_folder);
        }

        lua_folder = fs::canonical(lua_folder);

        fs::path lua = lua_folder / fs::path("session"+std::to_string(id)+".lua");

        if(!fs::exists(lua)){
            std::ofstream writer(lua.string());
            writer<<"-- New Session! "<<std::endl<<"print(\"Hello world!\")";
        }

        return lua.string();

    }

    std::string Session::read_lua() const{
        std::ifstream input(lua_file());
        return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    }

    void Session::save_lua(std::string lua) const{
        std::ofstream output(lua_file());
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
