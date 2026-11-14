Vendored from https://github.com/ahupowerdns/luawrapper (commit 5ded6194, BSD license, see LICENSE.md)
with one local fix in `LuaContext::callRaw`: errors raised by Lua functions that are called
from C++ callbacks were reported as "Trying to cast a lua variable from string to exception_ptr"
instead of the actual Lua error message.
