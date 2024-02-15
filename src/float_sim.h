#ifndef HIRSCHBERG_FLOAT_SIM_H
#define HIRSCHBERG_FLOAT_SIM_H

#define VALUE_NAME float_sim
#define VALUE_TYPE float
#define HIRSCHBERG_VALUE_EQUALS(a, b) (fabs((a) - (b)) < FLT_EPSILON)
#define HIRSCHBERG_SIMILARITY
#include "hirschberg.h"
#undef VALUE_NAME
#undef VALUE_TYPE
#undef HIRSCHBERG_SIMILARITY

#endif