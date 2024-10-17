#include "Sugar_Intrinsics.h"
#include "util/Sugar_Math.h"
#include "win32_Sugar.h"
#include "Sugar.h"
#include "SugarAPI.h"

struct AABB 
{
    vec2 Min;
    vec2 Max;
};

struct Circle 
{
    real32 Radius;
    vec2 Position;
};

internal bool AABBvsAABB(AABB A, AABB, B) 
{
    if(A.Max.x < B.Min.x || A.Min.x > B.Max.x) {return(false);}    
    if(A.Max.y < B.Min.y || A.Min.y > B.Max.y) {return(false);}    

    return(true);
}