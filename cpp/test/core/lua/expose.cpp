#include <gtest/gtest.h>
#include <LuaContext.hpp>
#include <string>


#include "core/lua/expose.h"


TEST(lua_expose, complex_double){
    LuaContext lua;
    core::lua::expose_complex<double>(lua);

    std::complex<double> tmp;
    lua.writeVariable("a", tmp);
    lua.executeCode("a:set_re(1.5)\n a:set_im(2.4)");

    EXPECT_DOUBLE_EQ(1.5, lua.executeCode<double>("return a:get_re()"));
    EXPECT_DOUBLE_EQ(2.4, lua.executeCode<double>("return a:get_im()"));

    auto val = lua.executeCode<std::complex<double>>("return complex(3.6, 4.9)");

    EXPECT_DOUBLE_EQ(3.6, val.real());
    EXPECT_DOUBLE_EQ(4.9, val.imag());

    lua.executeCode("a = complex(2.4, 5.6)\nb=complex(5.2, 7.1)\nc=a:add(b)");
    auto a = lua.readVariable<std::complex<double>>("a");
    auto b = lua.readVariable<std::complex<double>>("b");
    auto c = lua.readVariable<std::complex<double>>("c");

    EXPECT_DOUBLE_EQ(2.4, a.real());
    EXPECT_DOUBLE_EQ(5.6, a.imag());
    EXPECT_DOUBLE_EQ(5.2, b.real());
    EXPECT_DOUBLE_EQ(7.1, b.imag());
    EXPECT_DOUBLE_EQ(7.6, c.real());
    EXPECT_DOUBLE_EQ(12.7, c.imag());

    auto astring = lua.executeCode<std::string>("return tostring(a)");
    EXPECT_EQ(astring, "(2.4,5.6)");

    lua.executeCode("a = complex(1, -1)\nb = complex(1, -1)\nc = complex(1, 1)");
    auto r1 = lua.executeCode<bool>("return a==b");
    auto r2 = lua.executeCode<bool>("return a==c");

    EXPECT_EQ(r1, true);
    EXPECT_EQ(r2, false);
}
