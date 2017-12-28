#include <LuaContext.hpp>
#include <complex>
#include <memory>

namespace wigner_web::utility{

    template<typename NumberType>
    void lua_expose_complex(LuaContext& lua){
        lua.writeFunction("complex", [](NumberType re, NumberType im){ return std::complex<NumberType>{ re, im }; });
        lua.registerFunction<void (std::complex<NumberType>::*)(NumberType)>("set_re", [](std::complex<NumberType>& object, NumberType val) { object.real(val); });
        lua.registerFunction<NumberType (std::complex<NumberType>::*)()>("get_re", [](std::complex<NumberType>& object) { return object.real(); });
        lua.registerFunction<void (std::complex<NumberType>::*)(NumberType)>("set_im", [](std::complex<NumberType>& object, NumberType val) { object.imag(val); });
        lua.registerFunction<NumberType (std::complex<NumberType>::*)()>("get_im", [](std::complex<NumberType>& object) { return object.imag(); });
    }

    template<class VectorClass, typename ScalarType>
    void lua_expose_linear_space_operators(LuaContext& lua){
        lua.registerFunction<void (VectorClass::*)(std::shared_ptr<VectorClass>)>("add", [](VectorClass& self, std::shared_ptr<VectorClass> other){ self += *other; }); 
        lua.registerFunction<void (VectorClass::*)(std::shared_ptr<VectorClass>)>("sub", [](VectorClass& self, std::shared_ptr<VectorClass> other){ self -= *other; }); 
        lua.registerFunction<void (VectorClass::*)(ScalarType)>("mul", [](VectorClass& self, ScalarType scalar){ self *= scalar; }); 
        lua.registerFunction<void (VectorClass::*)(ScalarType)>("div", [](VectorClass& self, ScalarType scalar){ self /= scalar; }); 
    }

    template<class T, class BaseClass, typename ... Types>
    void lua_expose_constructor(LuaContext& lua, std::string name){
        lua.writeFunction(name, [](Types ... args) -> std::shared_ptr<BaseClass>{ return std::make_shared<T>(args...); });
    }

    template<class T, class BaseClass, class ParentClass, typename ... Types>
    void lua_expose_constructor_as_member(LuaContext& lua, std::string name){
        lua.registerFunction<std::shared_ptr<BaseClass> (ParentClass::*)(Types ...)>(name, [](ParentClass& self, Types ... args){ return std::make_shared<T>(self, args...); });
    }
}
