#pragma once

#include <math.h>
#include "vecmath.h"

// static float const animation_time = 800;

extern int ___force_rebuild___jklmnop;

// static float anim_get(float t)
// {
//   // ended up just approximating this
//   // https://www.cssportal.com/css-cubic-bezier-generator/?x1=0.5&y1=-0.4&x2=0.5&y2=1.4
//   // because I don't wanna deal with solving a cubic here.
//
//   // return 0.558625*em(sin)(4.065545*x-2.01651032)-0.558625*em(sin)(-2.01651032);
// #define it(x) (em(sin, (x) * M_PI - M_PI * .5)/2 + .5)
//   float y = it(t);
//   return em(sqrt, it(y));
// }
