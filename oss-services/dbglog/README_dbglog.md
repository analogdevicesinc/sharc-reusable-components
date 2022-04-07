# oss-services/dbglog

## Overview

This component is comprised of a single header file that can be included in the source files to provide a consistent logging experience.  It is a slightly modified version of the dbglog sub-component from the umm_malloc project.  The original source and license can be found here: [dbglog](https://github.com/rhempel/c-helper-macros/tree/develop)

## Required components

- None

## Recommended components

- None

## Integrate the source

- Copy the contents of the `inc` directory to a project include directory.
- By default dbglog logs to `stdout` using `printf()`.  This can be overridden by defining the  `DBGLOG_FUNCTION` macro to a `printf()` compatible function prior to including the `dbglog` header.

## Run

- The `dbglog`source contains instructions for usage.  Some additional examples are shown below: 

```C
/* Alternative logging functions must be defined before including the dbglog header */
#define DBGLOG_FUNCTION myPrintf

/* The log level must be defined before including the dbglog header */
#define DBGLOG_LEVEL DBGLOG_INFO
#include "dbglog.h"
```

