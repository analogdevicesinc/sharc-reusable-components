# oss-services/sharc-lua

## Overview

This package is a recent version of the popular [Lua](https://www.lua.org/) scripting language with a makefile configured to build a library for use on the SHARC+ core.

## Required components

- Shell (specifically linenoise if an interactive Lua commandline is required)

## Recommended components

- Simple UART driver
- UART stdio driver

## Build the library

- Copy the 'lua-5.3.5' directory into a convenient place to build the library
- In the 'lua-5.3.5' directory, type 'make sharc-lua'
- The lua library can be found in 'src/liblua.a'

## Integrate the library

- Copy 'liblua.a' into your project and add it to the linker command line
- Copy 'lauxlib.h', 'lprefix.h', 'lua.h', 'luaconf.h', and 'lualib.h' into a convenient project include directory.
- If an interactive Lua command-line is desired, copy 'lua.c' from the 'src' directory into the project.

## Info

- The function `lua_readline()` in 'lua.c' has been specially modified to work with the non-blocking, locally modified, version of linenoise included in the Shell reusable module.
- Lua takes advantage of the "default" linenoise context by passing zero (or NULL) as the first argument to linenoise_getline().  If the default context is utilized for something else, make sure to allocate and initialize a separate context for the Lua command line.
