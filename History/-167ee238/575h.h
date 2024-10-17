#pragma once

#include "../Sugar_Intrinsics.h"
#include <stdint.h>

typedef float real32;
typedef double real64;
typedef int32_t int32;
typedef int64_t int64;

struct vec2 
{
    real32 x;
    real32 y;
};

struct ivec2 
{
    int x;
    int y;
};

inline int64
maxi64(int64 a, int64 b) 
{
    if(a > b) 
    {
        return(a);
    }
    return(b);
}