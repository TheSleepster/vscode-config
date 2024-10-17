/*
    This is not a final layer. Additional Requirements: 

    COMPLETE
    - GameInput (Keyboard)
    - Keymapping?
    - DeltaTime (Kinda?)



    TODO 
    - Audio (DirectSound?, SokolAudio?, Or XAudio?)
    - Audio Formats (.WAV exclusively?)
    - Asset Loading
    - Fullscreen
    - Multithreading
    - File Saving
    - Game Saving
    - Sleep/Inactivity Period
    - GetKeyboardLayout() (For non-standard QWERTY keyboards)
    - GameInput(XInput)
    - Raw Input (For multiple inputs)
    - WM_ACTIVATEAPP (For being the inactive window)
    - ClipCursor(Multi Monitor)
    - Upload with WebAssembly
*/



#include "Sugar_Intrinsics.h"
#include "win32_Sugar.h"
#include "Sugar.h"
#include "SugarAPI.h"
#include "Sugar_Input.h"

#define WIN32_LEAN_AND_MEAN
#define EXTRA_LEAN
#define NOMINMAX
#include <windows.h>
#include <wingdi.h>
#include <xinput.h>
#include "../data/deps/OpenGL/GLL.h"

#include "Sugar_OpenGLRenderer.cpp"
#include "Sugar_Input.cpp"
#include "Sugar_Physics.cpp"

global_variable bool GlobalRunning = {};
global_variable int ClientWidth = {};
global_variable int ClientHeight = {}; 

internal Win32GameCode
Win32LoadGamecode(char *SourceDLLName) 
{ 
    Win32GameCode Result = {};

    char *TempDLLName = "GamecodeTemp.dll";
    Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

    Result.IsLoaded = false;
    while(!Result.IsLoaded) 
    {
        CopyFile(SourceDLLName, TempDLLName, FALSE);
        Result.IsLoaded = true;
    }
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
            GlobalRunning = false;
            DestroyWindow(hWnd);
        }break;
        case WM_DESTROY: 
        {
            GlobalRunning = false;
            DestroyWindow(hWnd);
        }break;
        case WM_SIZE: 
        {
            RECT Rect = {};
            GetClientRect(hWnd, &Rect);
            ClientWidth = Rect.right - Rect.left;
            ClientHeight = Rect.bottom - Rect.top;
        }break;
        default: 
        { 
            Result = DefWindowProc(hWnd, Message, wParam, lParam);
        }break;
    }
    return(Result);
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

    if(!Win32OpenGL->wglCreateContextAttribsARB||!Win32OpenGL->wglChoosePixelFormatARB) 
    {
        Assert(false, "Failed to manually load WGL functions!\n");
    }
    wglMakeCurrent(WindowDC, 0);
    wglDeleteContext(TempRC);
    DestroyWindow(WindowHandle);
}

internal void 
Win32ProcessInputMessages(MSG Message, HWND WindowHandle, Input *GameInput) 
{
    while(PeekMessageA(&Message, WindowHandle, 0, 0, PM_REMOVE)) 
    {
        if(Message.message == WM_QUIT) 
        {
            GlobalRunning = false;
        }

        switch(Message.message) 
        {
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN: 
            case WM_KEYUP:
            {
                uint32 VKCode = (uint32)Message.wParam;
                bool WasDown = ((Message.lParam & (1 << 30)) != 0);
                bool IsDown = ((Message.lParam & (1 << 31)) == 0);
                
                KeyCodeID KeyCode = KeyCodeLookup[Message.wParam];
                Key *Key = &GameInput->Keyboard.Keys[KeyCode];
                Key->JustPressed = !Key->JustPressed && !Key->IsDown && IsDown;
                Key->JustReleased = !Key->JustReleased && Key->IsDown && !IsDown;
                Key->IsDown = IsDown;


                bool AltKeyIsDown = ((Message.lParam & (1 << 29)) !=0);
                if(VKCode == VK_F4 && AltKeyIsDown) 
                {
                    GlobalRunning = false;
                }
            }break;
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_XBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP: 
            case WM_XBUTTONUP:
            {
                uint32 VKCode = (uint32)Message.wParam;
                bool IsDown = (GetKeyState(VKCode) & (1 << 15));

                KeyCodeID KeyCode = KeyCodeLookup[Message.wParam];
                Key *Key = &GameInput->Keyboard.Keys[KeyCode];
                Key->JustPressed = !Key->JustPressed && !Key->IsDown && IsDown;
                Key->JustReleased = !Key->JustReleased && Key->IsDown && !IsDown;
                Key->IsDown = IsDown;
            }break;
            default: 
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }break;
        }
    }
}

int APIENTRY
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR     lpCmdLine,
        int       nShowCmd) 
{
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    WNDCLASS Window = {};
    Win32OpenGLFunctions Win32OpenGL = {};
    Win32_WindowData WindowData = {};

    Window.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    Window.lpfnWndProc = Win32MainWindowCallback;
    Window.hInstance = hInstance;
    Window.lpszClassName = "Sugar";
    
    if(RegisterClass(&Window)) 
    {
        WindowData.X = 100;
        WindowData.Y = 100;
        WindowData.WindowWidth = 1280;
        WindowData.WindowHeight = 640;

        // NOTE : This Initializes OpenGL so we can make the next context
        Win32InitializeOpenGL(hInstance, Window, &Win32OpenGL);
        // NOTE : Then we create the real OpenGL Context
        HWND WindowHandle = 
            CreateWindow(Window.lpszClassName,
                    "Sugar Framework", 
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

        // NOTE : VSYNC
        Win32OpenGL.wglSwapIntervalEXT(1);
        // NOTE : VSYNC

        if(WindowHandle) 
        {
            GameMemory GameMemory = {};
            GameMemory.PermanentStorage = MakeBumpAllocator(Megabytes(100));
            GameMemory.TransientStorage = MakeBumpAllocator(Gigabytes(1));
            
            GameRenderData = (RenderData*)BumpAllocate(&GameMemory.PermanentStorage, sizeof(RenderData));
            Assert(GameRenderData, "Failed to allocate Permanent Memory for the GameRenderData Variable!\n");
            
            InitializeOpenGLRenderer(&GameMemory);
            Win32LoadKeyData(); 

            char *SourceDLLName = "GameCode.dll";
            Win32GameCode Game = Win32LoadGamecode(SourceDLLName);

            WindowData.WindowDC = GetDC(WindowHandle);
            GlobalRunning = true;

            LARGE_INTEGER LastCounter;            
            QueryPerformanceCounter(&LastCounter);
            // TODO : Two functions are needed. Update(), and FixedUpdate(). 
            //        Update will operate under the Display Framerate, 
            //        whilst FixedUpdate has a fixed Update Time
            //        Update will be for things that are a part of the same update frequency as the renderer.
            //        FixedUpdate will be mainly for Physics.
            Input GameInput = {};
            // KEYBOARD BINDINGS
            GameInput.Keyboard.Bindings[MOVE_UP].Key = KEY_W;
            GameInput.Keyboard.Bindings[MOVE_DOWN].Key = KEY_S;
            GameInput.Keyboard.Bindings[MOVE_LEFT].Key = KEY_A;
            GameInput.Keyboard.Bindings[MOVE_RIGHT].Key = KEY_D;
            GameInput.Keyboard.Bindings[ATTACK].Key = KEY_MOUSE_LEFT;

            while(GlobalRunning) 
            {
                MSG Message = {};
                WindowData.WindowWidth = ClientWidth;
                WindowData.WindowHeight = ClientHeight;

                // MOUSE 
                POINT MousePos;
                GameInput.Keyboard.LastMouse.x = GameInput.Keyboard.CurrentMouse.x;
                GameInput.Keyboard.LastMouse.y = GameInput.Keyboard.CurrentMouse.y;
                GetCursorPos(&MousePos);
                ScreenToClient(WindowHandle, &MousePos);
                GameInput.Keyboard.CurrentMouse.x = MousePos.x;
                GameInput.Keyboard.CurrentMouse.y = MousePos.y;
                GameInput.Keyboard.RelMouse = GameInput.Keyboard.CurrentMouse - GameInput.Keyboard.LastMouse;
/*
                for(DWORD ControllerIndex = 0; 
                    ControllerIndex < 1; 
                    ++ControllerIndex) 
                {
                    XINPUT_STATE ControllerState;
                    if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) 
                    {   
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                        bool DPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool DPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool DPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool DPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool StartButton = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool BackButton = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool LeftThumbDown = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
                        bool RightThumbDown = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
                        bool LeftShoulderDown = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RightShoulderDown = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        uint8 LeftTrigger = Pad->bLeftTrigger;
                        uint8 RightTrigger = Pad->bRightTrigger;

                        int16 LeftStickX = Pad->sThumbLX;
                        int16 LeftStickY = Pad->sThumbLY;
                        int16 RightStickX = Pad->sThumbRX;
                        int16 RightStickY = Pad->sThumbRY;

                        XINPUT_VIBRATION Vibration = {};    
                        Vibration.wLeftMotorSpeed = 6500;
                        Vibration.wRightMotorSpeed = 6500;
                        XInputSetState(ControllerIndex, &Vibration);
                    }
                    else 
                    {
                        break; // NOTE : No controller avaliable
                    }
                }
*/

                // HOTRELOADING
                FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceDLLName);
                if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0) 
                {
                    Win32UnloadGamecode(&Game);
                    Game = Win32LoadGamecode(SourceDLLName);
                }

                // MESSAGES
                Win32ProcessInputMessages(Message, WindowHandle, &GameInput);

                // TODO : This will eventually go inside of out "Update()" loop
                Game.UpdateAndRender(&GameMemory, GameRenderData, &GameInput);
                OpenGLRender(&GameMemory, &WindowData);
                GameMemory.TransientStorage.Used = 0;

                // DELTA TIME
                LARGE_INTEGER EndCounter;
                QueryPerformanceCounter(&EndCounter);
                int64 DeltaCounter = EndCounter.QuadPart - LastCounter.QuadPart;    
                real32 MSPerFrame = (1000 * (real32)DeltaCounter) / (real32)PerfCountFrequency;
                int32 FPS = (int32)PerfCountFrequency / (int32)DeltaCounter;

                // PERFORMANCE PROFILING
                char Buffer[256];
                sprintf(Buffer, "%.02fms, FPS: %d\n", MSPerFrame, FPS);
                OutputDebugStringA(Buffer);
                LastCounter = EndCounter;
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
