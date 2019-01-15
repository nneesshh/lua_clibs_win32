--require('mobdebug').start('192.168.1.110') --<-- only insert this line

--
package.path = package.path .. ";./lua/?.lua;./lua/protobuf/?.lua;./lua/proto_pb/?.lua"
package.cpath = package.cpath .. ";./?.dll"

local lua_code_str = 'do return end' -- < fill it
local file = io.open("test_stack.txt", "r");
lua_code_str = file:read("*a")
file:close()

require('lcf.workshop.base')
local get_ast = request('!.lua.code.get_ast')
local get_formatted_code = request('!.lua.code.ast_as_code')

local result_str = get_formatted_code(get_ast(lua_code_str))
local file2 = io.open("test_stack.lua", "w");
file2:write(result_str)
file2:close()