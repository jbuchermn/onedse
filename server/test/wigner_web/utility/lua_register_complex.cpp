#include <gtest/gtest.h>
#include <LuaContext.hpp>


#include "wigner_web/utility/lua_register_complex.h"


TEST(lua_complex, read_write){
    LuaContext lua;
    wigner_web::utility::lua_register_complex<double>(lua);

    std::complex<double> tmp;
    lua.writeVariable("a", tmp);
    lua.executeCode("a:set_re(1.5)\n a:set_im(2.4)");

    EXPECT_DOUBLE_EQ(1.5, lua.executeCode<double>("return a:get_re()"));
    EXPECT_DOUBLE_EQ(2.4, lua.executeCode<double>("return a:get_im()"));

    auto val = lua.executeCode<std::complex<double>>("return complex(3.6, 4.9)");
    
    EXPECT_DOUBLE_EQ(3.6, val.real());
    EXPECT_DOUBLE_EQ(4.9, val.imag());
}
