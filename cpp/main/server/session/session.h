#pragma once

#include <memory>
#include <string>
#include <exception>
#include <json.hpp>

#include "core/lua/system.h"

namespace server::session{
    class Session{
        class process_error: public std::runtime_error{
        public:
            process_error(std::string msg);
        };

        const std::string arg0;

        static int id_counter;
        int id;

        std::string lua_file() const;
        std::string read_lua() const;
        void save_lua(std::string lua) const;

        core::lua::System system;
    public:
        Session(std::string arg0_);

        std::string process(const std::string& request);
    };
}
