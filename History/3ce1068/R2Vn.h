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

// FLOAT OPERATIONS

inline real32 
fSquare(real32 A) 
{
    real32 Result = A * A;
    return(Result);
}

inline real32
Inner(vec2 A, vec2 B) 
{
    real32 Result = A.x*B.x + A.y*B.y;
    return(Result);
}

inline real32 
fsqrt(real32 Float) 
{    
    real32 Result;
    Result = sqrtf(Float);
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

// INTEGER OPERATIONS 

inline int64
maxi64(int64 a, int64 b) 
{
    if(a > b) 
    {
        return(a);
    }
    return(b);
}

inline int32 
iSquare (int32 A) 
{
    int32 Result = A * A;
    return(Result);
}

inline int32
isqrt(int32 Number) 
{
    int32 Result;
    Result = sqrt(Number);
    return(Result);
}
