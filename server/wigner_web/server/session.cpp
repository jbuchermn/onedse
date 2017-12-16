#include <json.hpp>

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

    void Session::process_initialize(const json& data, json& resp){
        if(!data.count("basis")) throw process_error("Basis missing!");
        if(!data.count("initial_state")) throw process_error("Initial state missing");
        
        basis = Basis::factory(data["basis"]);
        state = DensityOperator::factory(data["initial_state"], basis);

        resp["state"] = "OK";
    }

    void Session::process_get_wigner(const json& data, json& resp){
        if(basis == nullptr || state == nullptr) throw process_error("Not initialized");
        
        if(!data.count("points")) throw process_error("Points missing");

        WignerFunction wigner{ *state, data["points"] };
        
        wigner.to_json(resp["wigner"]);
        resp["state"] = "OK";

    }
        
    void Session::process_get_wavefunctions(const nlohmann::json& data, nlohmann::json& resp){
        if(basis == nullptr || state == nullptr) throw process_error("Not initialized");

        if(!data.count("points")) throw process_error("Points missing");
        state->to_json(resp["wavefunctions"], data["points"]);
        resp["state"] = "OK";
    }

    std::string Session::process(const std::string& request){
        json req = json::parse(request);
        json resp;

        try{
            if(!req.count("key") || !req.count("data")) throw process_error("Key or data missing!");
            
            if(req["key"] == "initialize"){
                process_initialize(req["data"], resp["data"]);
            }else if(req["key"] == "get_wigner"){
                process_get_wigner(req["data"], resp["data"]);
            }else if(req["key"] == "get_wavefunctions"){
                process_get_wavefunctions(req["data"], resp["data"]);
            }else{
                throw process_error("Invalid key!");
            }
        }catch(process_error& ex){
            resp["error"] = ex.what();
        }

        return resp.dump();
    }
}
