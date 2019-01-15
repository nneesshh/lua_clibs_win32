@echo "---------------- start debugging ----------------"
SET ZBS=D:\www\_nneesshh_git\ZeroBraneStudio53
SET LUA_PATH=./?.lua;./src/?.lua;%ZBS%/lualibs/?/?.lua;%ZBS%/lualibs/?.lua
SET LUA_CPATH=%ZBS%/bin/?.dll;%ZBS%/bin/clibs/?.dll
lua.exe ".\lua\test_pb.lua"