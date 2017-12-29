#include <iostream>
#include <fstream>
#include <streambuf>
#include <vector>
#include <stdlib.h>
#include <string>
#include <chrono>
#include <experimental/filesystem>
#include <json.hpp>

namespace fs = std::experimental::filesystem;
using json = nlohmann::json;

#include "core/lua/system.h"

using System = core::lua::System;

int main(int argc, char* argv[]){
    /*
     * Parse arguments
     */
    std::vector<std::string> args;
    for(int i=0; i<argc; i++) args.push_back(std::string(argv[i]));

    if(args.size()<2){
        std::cout<<"Usage: >onedse script.lua"<<std::endl;
        return 1;
    }

    fs::path script(args[1]);
    if(!fs::exists(script)){
        std::cout<<"Could not open "<<script.string()<<std::endl;
        return 2;
    }

    script = fs::canonical(script);

    /*
     * Read script
     */
    std::cout<<"Reading "<<script.string()<<"..."<<std::endl;
    std::ifstream input(script.string());
    std::string lua((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    /*
     * Execute it
     */
    System system;
    json result;

    std::cout<<"Executing code..."<<std::endl;
    system.execute(lua, result);

    /*
     * Find/Create result folder
     */
    fs::path result_folder = script.parent_path()/script.stem();
    if(!fs::exists(result_folder)){
        fs::create_directory(result_folder);
    }

    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

    result_folder/=std::to_string(ms);
    fs::create_directory(result_folder);

    std::cout<<"Writing result to "<<result_folder.string()<<std::endl;

    /*
     * Write result
     */
    std::ofstream output((result_folder/"data.json").string());
    output << result.dump();

    /*
     * Find python script
     */
    fs::path executable_path(args[0]);
    fs::path python = executable_path.parent_path();

    while(!fs::exists(python / "python")){
        python /= "..";
    }

    python/="python";
    python/="plot.py";

    /*
     * Run Python
     */
    std::cout<<"Running plot script..."<<std::endl;
    ::system(("python3 "+python.string()+" "+result_folder.string()).c_str());


    std::cout<<"Finishing..."<<std::endl;
    return 0;
}
