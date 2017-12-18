#pragma once

#include <memory>
#include <string>
#include <exception>
#include <json.hpp>

#include "wigner_web/system/system.h"

namespace wigner_web::server{
    class Session{
        class process_error: public std::runtime_error{
        public:
            process_error(std::string msg);
        };

        wigner_web::system::System system;
    public:
        std::string process(const std::string& request);
    };
}
