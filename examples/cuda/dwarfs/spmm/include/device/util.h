#pragma once

/**
 * Number of elements in static array
 */
#define ARRAY_SIZE(x)                              \
    (sizeof(x)/sizeof(x[0]))
