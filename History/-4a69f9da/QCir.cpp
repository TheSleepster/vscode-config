

/*
    This is not a final layer. Additional Requirements: 

    - GameInput
    - Audio
    - DeltaTime
    - Asset Loading
    - Fullscreen
    - Multithreading
    - File Saving
    - Game Saving
    - Sleep/Inactivity Period
    - GetKeyboardLayout (For non-standard QWERTY keyboards)
    - Raw Input (For multiple inputs)
    - WM_ACTIVATEAPP (For being the inactive window)
    - ClipCursor(Multi Monitor)
*/



#include "Sugar_Intrinsics.h"
#include "win32_Sugar.h"
#include "Sugar.h"
#include "SugarAPI.h"

#define WIN32_LEAN_AND_MEAN
#define EXTRA_LEAN
#define NOMINMAX
#include <windows.h>
#include <wingdi.h>
#include "../data/deps/OpenGL/GLL.h"

// TODO : Remove this from the global space
#include "Sugar_OpenGLRenderer.cpp"

global_variable ivec2 ClientSize = {};

LRESULT CALLBACK
Win32MainWindowCallback(HWND    hWnd,
                        UINT    Message,
                        WPARAM  wParam,
                        LPARAM  lParam) 
{
    LRESULT Result = 0;
    switch(Message) 
    {
        case WM_CLOSE: 
        {
            DestroyWindow(hWnd);
        }break;
        case WM_DESTROY: 
        {
            DestroyWindow(hWnd);
        }break;
        case WM_SIZE: 
        {
            RECT Rect = {};
            GetClientRect(hWnd, &Rect);
            ClientSize.x = Rect.right - Rect.left;
            ClientSize.y = Rect.bottom - Rect.top;
        }break;
        default: 
        { 
            Result = DefWindowProc(hWnd, Message, wParam, lParam);
        }break;
    }
    return(Result);
}

internal Win32GameCode
Win32LoadGamecode(char *SourceDLLName) 
{ 
    Win32GameCode Result = {};

    char *TempDLLName = "GamecodeTemp.dll";
    Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

    CopyFile(SourceDLLName, TempDLLName, FALSE);
    Result.GameCodeDLL = LoadLibraryA(TempDLLName);

    if(Result.GameCodeDLL) 
    {
        Result.UpdateAndRender = 
            (game_update_and_render *)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender"); 
        Result.IsValid = (Result.UpdateAndRender);
    }
    if(!Result.IsValid) 
    {
        Result.UpdateAndRender= GameUpdateAndRenderStub;
    }
    return(Result);
}

internal void 
Win32UnloadGamecode(Win32GameCode *Gamecode) 
{
    if(Gamecode->GameCodeDLL) 
    {
        FreeLibrary(Gamecode->GameCodeDLL);
        Gamecode->GameCodeDLL = 0;
    }
    Gamecode->IsValid = false;
    Gamecode->UpdateAndRender = GameUpdateAndRenderStub;
}

internal void
Win32InitializeOpenGL(HINSTANCE hInstance, WNDCLASS Window, Win32OpenGLFunctions *Win32OpenGL) 
{
    HWND WindowHandle = 
        CreateWindow(Window.lpszClassName,
                "Sugar",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                hInstance,
                0);
    HDC WindowDC = GetDC(WindowHandle);

    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
    DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
    DesiredPixelFormat.nVersion = 1;
    DesiredPixelFormat.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
    DesiredPixelFormat.cColorBits = 32;
    DesiredPixelFormat.cAlphaBits = 8;
    DesiredPixelFormat.cDepthBits = 24;
    
    int PixelFormat = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(WindowDC, PixelFormat, sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, PixelFormat, &SuggestedPixelFormat);

    HGLRC TempRC = wglCreateContext(WindowDC);
    wglMakeCurrent(WindowDC, TempRC);

// NOTE : Loading wgl specific OpenGL functions
    Win32OpenGL->wglChoosePixelFormatARB = 
    (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    Win32OpenGL->wglCreateContextAttribsARB = 
    (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    Win32OpenGL->wglSwapIntervalEXT =
    (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

    if(!Win32OpenGL->wglCreateContextAttribsARB||
       !Win32OpenGL->wglChoosePixelFormatARB||
       !Win32OpenGL->wglSwapIntervalEXT) 
    {
        Assert(false, "Failed to manually load WGL functions!\n");
    }
    wglMakeCurrent(WindowDC, 0);
    wglDeleteContext(TempRC);
    DestroyWindow(WindowHandle);
}

int APIENTRY
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR     lpCmdLine,
        int       nShowCmd) 
{
    Win32_WindowData WindowData = {};
    Win32OpenGLFunctions Win32OpenGL = {};
    WNDCLASS Window = {};

    Window.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    Window.lpfnWndProc = Win32MainWindowCallback;
    Window.hInstance = hInstance;
    Window.lpszClassName = "Sugar";
    
    if(RegisterClass(&Window)) 
    {
        WindowData.X = 100;
        WindowData.Y = 100;
        WindowData.WindowWidth = 1280;
        WindowData.WindowHeight = 720;

        // NOTE : This Initializes OpenGL so we can make the next context
        Win32InitializeOpenGL(hInstance, Window, &Win32OpenGL);
        // NOTE : Then we create the real OpenGL Context
        HWND WindowHandle = 
            CreateWindow(Window.lpszClassName,
                    "Sugar",
                    WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                    WindowData.X,
                    WindowData.Y,
                    WindowData.WindowWidth,
                    WindowData.WindowHeight,
                    0,
                    0,
                    hInstance,
                    0);
        HDC WindowDC = GetDC(WindowHandle);
        const int PixelAttributes[] = 
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
            WGL_SWAP_METHOD_ARB,    WGL_SWAP_COPY_ARB,
            WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
            WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
            WGL_COLOR_BITS_ARB,     32,
            WGL_ALPHA_BITS_ARB,     8,
            WGL_DEPTH_BITS_ARB,     24,
            0
        };
        UINT NumPixelFormats;
        int PixelFormat = 0;
        if(!Win32OpenGL.wglChoosePixelFormatARB(WindowDC, PixelAttributes, 0, 1, &PixelFormat, &NumPixelFormats)) 
        {
            Assert(false, "Failed to choose the pixel format the second time\n");
        }
        PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
        DescribePixelFormat(WindowDC, PixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &DesiredPixelFormat);
        if(!SetPixelFormat(WindowDC, PixelFormat, &DesiredPixelFormat)) 
        {
            Assert(false, "Failed to set the main PixelFormat\n");
        }

        const int ContextAttributes[] = 
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
            0
        };

        HGLRC RenderingContext = Win32OpenGL.wglCreateContextAttribsARB(WindowDC, 0, ContextAttributes);

        if(!RenderingContext) 
        {
            Assert(false, "Failed to create the RenderingContext\n");
        }

        if(!wglMakeCurrent(WindowDC, RenderingContext)) 
        {
            Assert(false, "Failed to make current wglMakeCurrent(WindowDc, RenderingContext)\n");
        }

        if(WindowHandle) 
        {
            GameMemory GameMemory = {};
            GameMemory.PermanentStorage = MakeBumpAllocator(Megabytes(50));
            GameMemory.TransientStorage = MakeBumpAllocator(Megabytes(50));
            
            GameRenderData = (RenderData*)BumpAllocate(&GameMemory.PermanentStorage, sizeof(RenderData));
            Assert(GameRenderData, "Failed to allocate Permanent Memory for the GameRenderData Variable!\n");
            
            InitializeOpenGLRenderer(&GameMemory);
            Win32OpenGL.wglSwapIntervalEXT(1);

            MSG Message;
            WindowData.GlobalRunning = true;
            WindowData.WindowDC = GetDC(WindowHandle);
            
            char *SourceDLLName = "GameCode.dll";
            Win32GameCode Game = Win32LoadGamecode(SourceDLLName);

            // TODO : Delta time implimentation. I have no fucking clue how to do this properly
            while(WindowData.GlobalRunning) 
            {
                // TODO : See if this has much of a performance impact
                WindowData.WindowWidth = ClientSize.x;
                WindowData.WindowHeight = ClientSize.y;

                FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceDLLName);
                if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0) 
                {
                    Win32UnloadGamecode(&Game);
                    Game = Win32LoadGamecode(SourceDLLName);
                }
                while(PeekMessageA(&Message, WindowHandle, 0, 0, PM_REMOVE)) 
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                Game.UpdateAndRender(&GameMemory, GameRenderData);
                OpenGLRender(&GameMemory, WindowData);
                GameMemory.TransientStorage.Used = 0;
            }
        }
        else 
        {
            Assert(false, "Failed to create the WindowHandle!\n");
        }
    }
    else 
    {
        Assert(false, "Failed to create the WindowClass!\n");
    }
    return(0);
}
