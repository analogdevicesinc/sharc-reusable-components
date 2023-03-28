# oss-services/pa-pinknoise

## Overview

This component generates pink noise

## Origin

https://github.com/PortAudio/portaudio/blob/master/examples/paex_pink.c

## Required components

- None

## Recommended components

- None

## Integrate the source

- Copy the contents of the `src` directory to a project source directory.
- Copy the contents of the `inc` directory to a project include directory
  and edit `pa-pinknoise_cfg.h` as needed for your application.

## Example usage

```
#include <stdbool.h>
#include "pa-pinknoise.h"

PinkNoise pn;
float sample;
bool ok;

ok = InitializePinkNoise(&pn, 16);
if (ok) {
    sample = GeneratePinkNoise(&pn);
}
```

