#pragma once

#include <cstdlib>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace core::utility{
    /**
     * Locate a data directory ("web", "lua", "python"): $ONEDSE_ROOT, then the
     * install prefix, then the source tree this binary was built from.
     */
    inline std::filesystem::path data_dir(const std::string& name){
        namespace fs = std::filesystem;
        std::vector<fs::path> candidates;
        if(const char* root = std::getenv("ONEDSE_ROOT")) candidates.push_back(fs::path(root));
        candidates.push_back(fs::path(ONEDSE_DATA_DIR));
        candidates.push_back(fs::path(ONEDSE_SOURCE_DIR));

        std::error_code ec;
        for(auto& c: candidates){
            if(fs::is_directory(c / name, ec)) return fs::canonical(c / name);
        }
        throw std::runtime_error("Could not find the '" + name + "' directory, set ONEDSE_ROOT to the directory containing it");
    }
}
