# README for cLuaJIT 2.0.4
==========================

forked by [LuaJIT(v2.0.4)](https://github.com/LuaJIT/LuaJIT)  
Lua script convert to binary(for LuaJIT version)

glue code with [LuaDist/srlua](https://github.com/LuaDist/srlua)([using to uLua - 秀丸マクロ.net](http://xn--pckzexbx21r8q9b.net/?page=nobu_mod_the_ulua_src))

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

[Windows binaries](https://github.com/wordijp/cLuaJIT/releases)

# about LuaJIT

LuaJIT is a Just-In-Time (JIT) compiler for the Lua programming language.

Project Homepage: http://luajit.org/

LuaJIT is Copyright (C) 2005-2016 Mike Pall.
LuaJIT is free software, released under the MIT license.
See full Copyright Notice in the COPYRIGHT file or in luajit.h.

Documentation for LuaJIT is available in HTML format.
Please point your favorite browser to:

 doc/luajit.html
