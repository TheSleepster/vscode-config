#pragma once
#include "../Sugar_Intrinsics.h"

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

int64 max(int64 a, int64 b) 
{
    if(a > b) 
    {
        return(a);
    }   
    return(b);
}
