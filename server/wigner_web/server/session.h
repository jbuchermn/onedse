#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <string>
#include <exception>
#include <json.hpp>


#include "wigner_web/discretization/basis.h"
#include "wigner_web/state/density_operator.h"

namespace wigner_web::server{
    class Session{
        class process_error: public std::runtime_error{
        public:
            process_error(std::string msg);
        };


        std::shared_ptr<wigner_web::discretization::Basis> basis;
        std::shared_ptr<wigner_web::state::DensityOperator> state;

        void process_initialize(const nlohmann::json& data, nlohmann::json& resp);
        void process_get_wigner(const nlohmann::json& data, nlohmann::json& resp);
        void process_get_wavefunctions(const nlohmann::json& data, nlohmann::json& resp);

    public:
        std::string process(const std::string& request);
    };
}

#endif
