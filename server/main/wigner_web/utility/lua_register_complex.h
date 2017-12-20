#include <LuaContext.hpp>
#include <complex>

namespace wigner_web::utility{

    template<typename NumberType>
    void lua_register_complex(LuaContext& lua){
        lua.writeFunction("complex", [](NumberType re, NumberType im){ return std::complex<NumberType>{ re, im }; });
        lua.registerFunction<void (std::complex<NumberType>::*)(NumberType)>("set_re", [](std::complex<NumberType>& object, NumberType val) { object.real(val); });
        lua.registerFunction<NumberType (std::complex<NumberType>::*)()>("get_re", [](std::complex<NumberType>& object) { return object.real(); });
        lua.registerFunction<void (std::complex<NumberType>::*)(NumberType)>("set_im", [](std::complex<NumberType>& object, NumberType val) { object.imag(val); });
        lua.registerFunction<NumberType (std::complex<NumberType>::*)()>("get_im", [](std::complex<NumberType>& object) { return object.imag(); });

    }
}
