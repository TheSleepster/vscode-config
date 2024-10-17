#define SDL_MAIN_HANDLED
#include <stdio.h>
#include <cstdlib>
#include <cstdint>

#define global_variable static
#define local_persist static
#define internal static

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uin64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

int main() 
{
    printf("Entry\n");
    return 0;
}
