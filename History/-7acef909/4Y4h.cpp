#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <stdint.h>
#include <winuser.h>

#include "util/glad/glad.c"

#define global_variable static
#define local_persist static
#define internal static

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

struct Win32Window 
{
    WNDCLASS WindowClass;
    HWND Window;
    
    bool Running;
};

LRESULT WINAPI
Win32MainWindowCallback(WNDPROC lpPrevWndFunc,
                  HWND    hWnd,
                  UINT    Msg,
                  WPARAM  wParam,
                  LPARAM  lParam) 
{
    LRESULT Result = 0;
    return(Result);
}

int WINAPI
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR     lpCmdLine,
        int       nShowCmd) 
{
    Win32Window Win32Window;

    Win32Window.Window = {};
    Win32Window.WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    Win32Window.WindowClass.lpfnWndProc = WNDPROC(Win32MainWindowCallback);
    Win32Window.WindowClass.hInstance = hInstance;
    Win32Window.WindowClass.lpszClassName = "Sugar_win32";
    if(RegisterClass(&Win32Window.WindowClass)) 
    {
        printf("Failed to register the Win32WindowClass\n");
        return(1);
    }
    Win32Window.Window = 
        CreateWindowEx(
                0, 
                Win32Window.WindowClass.lpszClassName, 
                "Sugar", 
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                hInstance,
                0);
    if(Win32Window.Window == NULL) 
    {
        printf("Failed to create the Window Handle!\n");
        return(1);
    }

    ShowWindow(Win32Window.Window, SW_SHOW);

    return(0);
}
