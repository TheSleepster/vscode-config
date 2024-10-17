#pragma once

#include "../Sugar_Intrinsics.h"
#include <stdint.h>
#include <math.h>

typedef float real32;
typedef double real64;
typedef int32_t int32;
typedef int64_t int64;

struct vec2 
{
    real32 x;
    real32 y;
};

vec2 operator-(vec2 A , vec2 B)
{
    vec2 Result = {};
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return(Result);
}

vec2 operator+(vec2 A, vec2 B) 
{
    vec2 Result = {};
    Result.x = A.x * B.x;
    Result.y = A.y * B.y;

    return(Result);
}

vec2 operator*(vec2 A, vec2 B) 
{
    vec2 Result = {};
    Result.x = A.x * B.x;
    Result.y = A.y * B.y;

    return(Result);
}

vec2 operator/(vec2 A, vec2 B) 
{
    vec2 Result = {};
    Result.x = A.x / B.x;
    Result.y = A.y / B.y;

    return(Result);
}

struct ivec2 
{
    int x;
    int y;
};

ivec2 operator-(ivec2 A , ivec2 B)
{
    ivec2 Result = {};
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return(Result);
}

ivec2 operator+(ivec2 A, ivec2 B) 
{
    ivec2 Result = {};
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return(Result);
}

ivec2 operator*(ivec2 A, ivec2 B) 
{
    ivec2 Result = {};
    Result.x = A.x * B.x;
    Result.y = A.y * B.y;

    return(Result);
}

ivec2 operator/(ivec2 A, ivec2 B) 
{
    ivec2 Result = {};
    Result.x = A.x / B.x;
    Result.y = A.y / B.y;

    return(Result);
}

inline int64
maxi64(int64 a, int64 b) 
{
    if(a > b) 
    {
        return(a);
    }
    return(b);
}

inline real32 
fsqrt(real32 Float) 
{    
    real32 Result;
    Result = sqrtf(Float);
    return(Result);
}

inline int32
isqrt(int32 Number) 
{
}
