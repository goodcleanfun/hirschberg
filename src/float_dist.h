#ifndef HIRSCHBERG_FLOAT_DIST_H
#define HIRSCHBERG_FLOAT_DIST_H

#include <float.h>

#define VALUE_NAME float_dist
#define VALUE_TYPE float
#define HIRSCHBERG_VALUE_EQUALS(a, b) (fabs((a) - (b)) < FLT_EPSILON)
#define MAX_VALUE FLT_MAX
#include "hirschberg.h"
#undef VALUE_NAME
#undef VALUE_TYPE
#undef MAX_VALUE

#endif