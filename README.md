# README for cLuaJIT 2.0.4
==========================

forked by [LuaJIT(v2.0.4)](https://github.com/LuaJIT/LuaJIT)

Lua script convert to binary(for LuaJIT version)
glue code with srlua
[using to file hosting service](http://webserver2.tecgraf.puc-rio.br/~lhf/ftp/lua/#srlua)(ver 5.1)
[original possibly](https://github.com/LuaDist/srlua)

# usage

```
$ echo 'print("hello world")' > hello.lua

$ lua hello.lua
hello world

$ cluajit hello.lua
hello world

$ cluajit hello.lua hello.o -link
$ ./hello.o
hello world
```

# binaries

[Windwos binaries](https://github.com/wordijp/cLuaJIT/releases)

# about LuaJIT

LuaJIT is a Just-In-Time (JIT) compiler for the Lua programming language.

Project Homepage: http://luajit.org/

LuaJIT is Copyright (C) 2005-2016 Mike Pall.
LuaJIT is free software, released under the MIT license.
See full Copyright Notice in the COPYRIGHT file or in luajit.h.

Documentation for LuaJIT is available in HTML format.
Please point your favorite browser to:

 doc/luajit.html
