#ifndef HIRSCHBERG_DOUBLE_DIST_H
#define HIRSCHBERG_DOUBLE_DIST_H

#include <float.h>

#define VALUE_NAME double_dist
#define VALUE_TYPE double
#define HIRSCHBERG_VALUE_EQUALS(a, b) (fabs((a) - (b)) < DBL_EPSILON)
#define MAX_VALUE DBL_MAX
#include "hirschberg.h"
#undef VALUE_NAME
#undef VALUE_TYPE
#undef MAX_VALUE

#endif