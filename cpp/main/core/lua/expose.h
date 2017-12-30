#include <LuaContext.hpp>
#include <complex>
#include <memory>
#include <sstream>
#include <iomanip>

namespace core::lua{

    template<class VectorClass, typename ScalarType>
    void expose_linear_space_operators(LuaContext& lua){
        lua.registerFunction<void (VectorClass::*)(std::shared_ptr<VectorClass>)>("iadd", [](VectorClass& self, std::shared_ptr<VectorClass> other){ self += *other; }); 
        lua.registerFunction<void (VectorClass::*)(std::shared_ptr<VectorClass>)>("isub", [](VectorClass& self, std::shared_ptr<VectorClass> other){ self -= *other; }); 
        lua.registerFunction<void (VectorClass::*)(ScalarType)>("imul", [](VectorClass& self, ScalarType scalar){ self *= scalar; }); 
        lua.registerFunction<void (VectorClass::*)(ScalarType)>("idiv", [](VectorClass& self, ScalarType scalar){ self /= scalar; }); 

        lua.registerFunction<std::shared_ptr<VectorClass> (VectorClass::*)(std::shared_ptr<VectorClass>)>("add", [](VectorClass& self, std::shared_ptr<VectorClass> other){ return std::make_shared<VectorClass>(self + (*other)); }); 
        lua.registerFunction<std::shared_ptr<VectorClass> (VectorClass::*)(std::shared_ptr<VectorClass>)>("sub", [](VectorClass& self, std::shared_ptr<VectorClass> other){ return std::make_shared<VectorClass>(self - (*other)); }); 
        lua.registerFunction<std::shared_ptr<VectorClass> (VectorClass::*)(ScalarType)>("mul", [](VectorClass& self, ScalarType scalar){ return std::make_shared<VectorClass>(self * scalar); }); 
        lua.registerFunction<std::shared_ptr<VectorClass> (VectorClass::*)(ScalarType)>("div", [](VectorClass& self, ScalarType scalar){ return std::make_shared<VectorClass>(self / scalar); }); 

    }

    template<typename NumberType>
    void expose_complex(LuaContext& lua){
        lua.writeFunction("complex", [](NumberType re, NumberType im){ return std::complex<NumberType>{ re, im }; });
        lua.registerFunction<void (std::complex<NumberType>::*)(NumberType)>("set_re", [](std::complex<NumberType>& object, NumberType val) { object.real(val); });
        lua.registerFunction<NumberType (std::complex<NumberType>::*)()>("get_re", [](std::complex<NumberType>& object) { return object.real(); });
        lua.registerFunction<void (std::complex<NumberType>::*)(NumberType)>("set_im", [](std::complex<NumberType>& object, NumberType val) { object.imag(val); });
        lua.registerFunction<NumberType (std::complex<NumberType>::*)()>("get_im", [](std::complex<NumberType>& object) { return object.imag(); });

        lua.registerFunction<void (std::complex<NumberType>::*)(std::complex<NumberType>)>("iadd", [](std::complex<NumberType>& self, std::complex<NumberType> other){ self += other; }); 
        lua.registerFunction<void (std::complex<NumberType>::*)(std::complex<NumberType>)>("isub", [](std::complex<NumberType>& self, std::complex<NumberType> other){ self -= other; }); 
        lua.registerFunction<void (std::complex<NumberType>::*)(std::complex<NumberType>)>("imul", [](std::complex<NumberType>& self, std::complex<NumberType> other){ self *= other; }); 
        lua.registerFunction<void (std::complex<NumberType>::*)(std::complex<NumberType>)>("idiv", [](std::complex<NumberType>& self, std::complex<NumberType> other){ self /= other; }); 

        lua.registerFunction<std::complex<NumberType> (std::complex<NumberType>::*)(std::complex<NumberType>)>("add", [](std::complex<NumberType>& self, std::complex<NumberType> other){ return (self + other); }); 
        lua.registerFunction<std::complex<NumberType> (std::complex<NumberType>::*)(std::complex<NumberType>)>("sub", [](std::complex<NumberType>& self, std::complex<NumberType> other){ return (self - other); }); 
        lua.registerFunction<std::complex<NumberType> (std::complex<NumberType>::*)(std::complex<NumberType>)>("mul", [](std::complex<NumberType>& self, std::complex<NumberType> other){ return self * other; }); 
        lua.registerFunction<std::complex<NumberType> (std::complex<NumberType>::*)(std::complex<NumberType>)>("div", [](std::complex<NumberType>& self, std::complex<NumberType> other){ return self / other; }); 
        
        lua.registerToStringFunction<std::string (std::complex<NumberType>::*)()>([](std::complex<NumberType>& self){ 
                std::ostringstream ss; 
                ss << std::setprecision(15) << self;
                return ss.str();
        });

        lua.registerEqFunction<bool (std::complex<NumberType>::*)(std::complex<NumberType>)>([](std::complex<NumberType>& self, std::complex<NumberType> other){ return self==other; });

    }


    template<class T, class BaseClass, typename ... Types>
    void expose_constructor(LuaContext& lua, std::string name){
        lua.writeFunction(name, [](Types ... args) -> std::shared_ptr<BaseClass>{ return std::make_shared<T>(args...); });
    }

    template<class T, class BaseClass, class ParentClass, typename ... Types>
    void expose_constructor_as_member(LuaContext& lua, std::string name){
        lua.registerFunction<std::shared_ptr<BaseClass> (ParentClass::*)(Types ...)>(name, [](ParentClass& self, Types ... args){ return std::make_shared<T>(self, args...); });
    }
}
