#include <iostream>
#include <json.hpp>
#include <map>
#include <string>
#include <LuaContext.hpp>

#include "wigner_web/server/session.h"
#include "wigner_web/discretization/basis.h"
#include "wigner_web/state/density_operator.h"
#include "wigner_web/state/wigner_function.h"

using json = nlohmann::json;
using Basis = wigner_web::discretization::Basis;
using DensityOperator = wigner_web::state::DensityOperator;
using WignerFunction = wigner_web::state::WignerFunction;

namespace wigner_web::server{
    Session::process_error::process_error(std::string message): std::runtime_error(message){}


    std::string Session::process(const std::string& request){
        json req = json::parse(request);
        json resp;

        try{
            if(!req.count("lua")) throw process_error("No code given!");
            try{
                system.execute(req["lua"], resp["data"]);
            }catch(const LuaContext::ExecutionErrorException& ex){
                throw process_error(std::string("Exception in Lua code: ")+ex.what());
            }catch(const LuaContext::SyntaxErrorException& ex){
                throw process_error(std::string("Syntax error in Lua code: ")+ex.what());
            }catch(const LuaContext::WrongTypeException& ex){
                throw process_error(std::string("Wrong type in Lua code: ")+ex.what());
            }
        }catch(process_error& ex){
            resp["error"] = ex.what();
        }

        return resp.dump();
    }
}
