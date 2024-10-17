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

internal bool AABBvsAABB(AABB A, AABB, B) 
{
    if(A.Max.x < B.Min.x || A.Min.x > B.Max.x) 
    {
        return(false);
    }    
}