--require('mobdebug').start('192.168.1.110') --<-- only insert this line

--
package.path = package.path .. ";./lua/?.lua;./lua/protobuf/?.lua;./lua/proto_pb/?.lua"
package.cpath = package.cpath .. ";./?.dll"

require "pb"
require "protobuf"
require "PVE_pb"

--[[ test protobuf message ]]
local stest = PVE_pb.PVEBattleService
local tmsg1 = PVE_pb.PVEBattleService()

--local file = io.open("debug.pb", "rb");
local file = io.open("responseError.pb", "rb");
local pbdata = file:read("*a")
file:close()
tmsg1:ParseFromString(pbdata)


local pa = require "utils.serpent".line
local str = pa(tmsg1)
print(str)

print("tmsg1 = ", tmsg1)
return tmsg1
