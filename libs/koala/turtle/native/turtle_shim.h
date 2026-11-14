/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Shared helpers for the koala/turtle native module.
 *
 * Logo-style turtle graphics over SDL2, modelled on CPython's `turtle`
 * (which itself sits on Tk). The turtle keeps a position/heading/pen state
 * and a list of drawn line segments; rendering to an SDL window is deferred
 * until `done()`/`update()`, so the state machine works headless (and is
 * unit-testable without a display).
 *
 * SDL2 is a system library, linked dynamically (like koala/tls links ssl);
 * no source is vendored.
 */

#ifndef KOALA_TURTLE_SHIM_H
#define KOALA_TURTLE_SHIM_H

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>
#include "object.h"
#include "excobj.h"

#define T_PI 3.14159265358979323846

void turtle_reg_turtle(NativeLib *lib);

#endif /* KOALA_TURTLE_SHIM_H */