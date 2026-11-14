#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

namespace core::utility{
    /// Base64 of raw bytes
    inline std::string base64(const unsigned char* data, size_t len){
        static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        out.reserve((len+2)/3*4);
        for(size_t i=0; i<len; i+=3){
            uint32_t v = data[i] << 16;
            if(i+1<len) v |= data[i+1] << 8;
            if(i+2<len) v |= data[i+2];
            out.push_back(table[(v>>18)&63]);
            out.push_back(table[(v>>12)&63]);
            out.push_back(i+1<len ? table[(v>>6)&63] : '=');
            out.push_back(i+2<len ? table[v&63] : '=');
        }
        return out;
    }

    /**
     * Numeric arrays are shipped to the frontends as base64 encoded little-endian
     * float32 - a factor of ~5 smaller than JSON number arrays and much faster to parse.
     */
    inline std::string encode_f32(const std::vector<float>& values){
        return base64(reinterpret_cast<const unsigned char*>(values.data()), values.size()*sizeof(float));
    }

    template<class Iterable>
    std::string encode_f32_from(const Iterable& values){
        std::vector<float> tmp;
        tmp.reserve(values.size());
        for(int i=0; i<values.size(); i++) tmp.push_back(static_cast<float>(values(i)));
        return encode_f32(tmp);
    }
}
