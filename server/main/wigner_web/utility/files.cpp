#include "wigner_web/utility/files.h"

#include <experimental/filesystem>
#include <iostream>
#include <fstream>
#include <string>

namespace fs = std::experimental::filesystem;

namespace wigner_web::utility{
    std::string files::arg0 = "./wigner_webd";

    std::string files::index_html(){
        fs::path executable_path(arg0);
        fs::path index_html = executable_path.parent_path();

        while(!fs::exists(index_html / "webapp")){
            index_html /= "..";
        }

        index_html = fs::canonical(index_html / "webapp" / "www" / "index.html");
        return index_html.string();
    }
    
    std::string files::session_lua(int session_id){
        fs::path executable_path(arg0);
        fs::path lua_folder = executable_path.parent_path();

        while(!fs::exists(lua_folder / "webapp")){
            lua_folder /= "..";
        }

        lua_folder /= "lua";

        if(!fs::exists(lua_folder)){
            fs::create_directory(lua_folder);
        }

        lua_folder = fs::canonical(lua_folder);

        fs::path lua = lua_folder / fs::path("session"+std::to_string(session_id)+".lua");

        if(!fs::exists(lua)){
            std::ofstream writer(lua.string());
            writer<<"-- Session "<<session_id;
        }

        return lua.string();
    }
}
