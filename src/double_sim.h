#ifndef HIRSCHBERG_DOUBLE_SIM_H
#define HIRSCHBERG_DOUBLE_SIM_H

#include <float.h>

#define VALUE_NAME double_sim
#define VALUE_TYPE double
#define HIRSCHBERG_VALUE_EQUALS(a, b) (fabs((a) - (b)) < DBL_EPSILON)
#define HIRSCHBERG_SIMILARITY
#include "hirschberg.h"
#undef VALUE_NAME
#undef VALUE_TYPE
#undef HIRSCHBERG_SIMILARITY

#endif