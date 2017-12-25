#pragma once

#include <string>

namespace wigner_web::utility{
    struct files{
        static std::string arg0;

        /**
         * Find index.html
         */
        static std::string index_html();

        /**
         * Find the lua file to store the session in.
         * Creates directories if necessary
         */
        static std::string session_lua(int session_id);
    };
}
