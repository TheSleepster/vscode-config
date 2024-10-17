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
    WNDCLASS Window;
    HWND WindowHandle;
    
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

    tagWNDCLASSA Win32Window.Window = {};
    Win32Window.Window.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    Win32Window.Window.lpfnWndProc = WNDPROC(Win32MainWindowCallback);
    Win32Window.Window.hInstance = hInstance;
    Win32Window.Window.lpszClassName = "Sugar_win32";
    if(RegisterClass(&Win32Window.Window)) 
    {
        HWND WindowHandle = 
            CreateWindowEx(
                    0, 
                    Win32Window.Window.lpszClassName, 
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
        if(WindowHandle) 
        {
        }
    }

    return(0);
}
