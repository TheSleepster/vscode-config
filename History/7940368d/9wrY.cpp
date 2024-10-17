#include "../data/shader/Shiver_SharedShaderHeader.h"
#include "util/MemoryArena.h"
#include "util/Math.h"
#include "Shiver.h"
// OPENGL

#if 0
#include "../data/deps/OpenGL/GLL.h"
#include "../data/deps/OpenGL/glcorearb.h"
#include "../data/deps/OpenGL/glext.h"
#include "../data/deps/OpenGL/wglext.h"
#endif

// STB IMAGE TEXTURE LOADING
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../data/deps/stbimage/stb_image.h"
#include "../data/deps/stbimage/stb_image_write.h"

// GAME HEADERS
#include "Win32_Shiver.h"
#include "Shiver_Renderer.h"

internal void
Win32InitializeOpenGLFunctionPointers(WNDCLASS Window, HINSTANCE hInstance, 
                                      win32openglfunctions *WGLFunctions)
{
    HWND DummyWindow = 
        CreateWindow(Window.lpszClassName,
                     "OpenGLFunctionGetter",
                     WS_OVERLAPPEDWINDOW,
                     CW_USEDEFAULT,
                     CW_USEDEFAULT,
                     CW_USEDEFAULT,
                     CW_USEDEFAULT,
                     0,
                     0,
                     hInstance,
                     0);
    HDC DummyContext = GetDC(DummyWindow);
    
    PIXELFORMATDESCRIPTOR DFormat = {};
    DFormat.nSize = sizeof(DFormat);
    DFormat.nVersion = 1;
    DFormat.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    DFormat.iPixelType = PFD_TYPE_RGBA;
    DFormat.cColorBits = 32;
    DFormat.cAlphaBits = 8;
    DFormat.cDepthBits = 24;
    
    int32 dPixelFormat = ChoosePixelFormat(DummyContext, &DFormat);
    PIXELFORMATDESCRIPTOR DSuggestedFormat;
    DescribePixelFormat(DummyContext, dPixelFormat, sizeof(DSuggestedFormat), &DSuggestedFormat);
    SetPixelFormat(DummyContext, dPixelFormat,  &DSuggestedFormat);
    
    HGLRC TempRC = wglCreateContext(DummyContext);
    wglMakeCurrent(DummyContext, TempRC);
    
    WGLFunctions->wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)
        wglGetProcAddress("wglChoosePixelFormatARB");
    WGLFunctions->wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)
        wglGetProcAddress("wglCreateContextAttribsARB");
    WGLFunctions->wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)
        wglGetProcAddress("wglSwapIntervalEXT");
    if(!WGLFunctions->wglChoosePixelFormatARB||
       !WGLFunctions->wglCreateContextAttribsARB||
       !WGLFunctions->wglSwapIntervalEXT)
    {
        Assert(false, "Failed to extract OpenGL function Pointers!\n");
    }
    
    wglMakeCurrent(DummyContext, 0);
    wglDeleteContext(TempRC);
    ReleaseDC(DummyWindow, DummyContext);
    DestroyWindow(DummyWindow);
}

internal void APIENTRY
OpenGLDebugMessageCallback(GLenum Source, GLenum Type, GLuint ID, GLenum Severity,
                           GLsizei Length, const GLchar *Message, const void *UserParam)
{
    if(Severity == GL_DEBUG_SEVERITY_LOW||
       Severity == GL_DEBUG_SEVERITY_MEDIUM||
       Severity == GL_DEBUG_SEVERITY_HIGH)
    {
        print_m("Error: %s\n", Message);
        Assert(false, "STOPPING\n");
    }
    else
    {
        print_m("Warning: %s\n", Message);
    }
}

internal void
sh_glVerifyivSuccess(GLuint TestID, GLuint Type)
{
    bool32 Success = {};
    char ShaderLog[512] = {};
    
    switch(Type)
    {
        case GL_VERTEX_SHADER:
        case GL_FRAGMENT_SHADER:
        {
            glGetShaderiv(TestID, GL_COMPILE_STATUS, &Success);
        }break;
        case GL_PROGRAM:
        {
            glGetProgramiv(TestID, GL_LINK_STATUS, &Success);
        }break;
    }
    if(!Success)
    {
        glGetShaderInfoLog(TestID, 512, 0, ShaderLog);
        print_m("ERROR ON SHADER: %s\n", ShaderLog);
        Assert(false, "STOPPING\n");
    }
}

internal void
sh_glCreateAndLoadTexture(const char *Filepath, texture2d TextureInfo)
{
    glActiveTexture(TextureInfo.ActiveTexture);
    TextureInfo.RawData = 
    (char *)stbi_load(Filepath, &TextureInfo.TextureData.Width, &TextureInfo.TextureData.Height, &TextureInfo.TextureData.Channels, 4);
    
    if(TextureInfo.RawData)
    {
        glGenTextures(0, &TextureInfo.TextureID);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, 
                     TextureInfo.TextureData.Width, TextureInfo.TextureData.Height, 0, 
                     GL_RGBA, GL_UNSIGNED_BYTE, TextureInfo.RawData);
    }
    stbi_image_free(TextureInfo.RawData);
}

internal void
sh_glLoadExistingTexture(const char *Filepath, texture2d TextureInfo)
{
    glActiveTexture(TextureInfo.ActiveTexture);
    TextureInfo.RawData = 
    (char *)stbi_load(Filepath, &TextureInfo.TextureData.Width, &TextureInfo.TextureData.Height, &TextureInfo.TextureData.Channels, 4);
    
    if(TextureInfo.RawData)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, 
                     TextureInfo.TextureData.Width, TextureInfo.TextureData.Height, 0, 
                     GL_RGBA, GL_UNSIGNED_BYTE, TextureInfo.RawData);
    }
    stbi_image_free(TextureInfo.RawData);
}



internal glshaderprogram
sh_glCreateShader(int32 ShaderType, char *Filepath)
{
    uint32 FileSize = 0;
    glshaderprogram ReturnShader = {};
    MemoryArena Scratch = MakeMemoryArena(Kilobytes(200));
    
    char *ShaderHeader = ReadEntireFileMA("shader/Shiver_SharedShaderHeader.h", &FileSize, &Scratch);
    char *ShaderSource = ReadEntireFileMA(Filepath, &FileSize, &Scratch);
    
    ReturnShader.Filepath = Filepath;
    ReturnShader.LastWriteTime = Win32GetLastWriteTime(Filepath); 
    
    if(ShaderSource && ShaderHeader)
    {
        char *ShaderSources[] = 
        {
            "#version 430 core\n",
            ShaderHeader,
            ShaderSource
        };
        
        ReturnShader.ShaderID = glCreateShader(ShaderType);
        glShaderSource(ReturnShader.ShaderID, ArrayCount(ShaderSources), ShaderSources, 0);
        glCompileShader(ReturnShader.ShaderID);
        sh_glVerifyivSuccess(ReturnShader.ShaderID, GL_VERTEX_SHADER);
    }
    else
    {
        Trace("File is either not found or does not contain strings!\n");
        Assert(false, "STOPPING\n");
    }
    return(ReturnShader);
}

internal shader
sh_glCreateProgram(glshaderprogram VertexShaderID, glshaderprogram FragmentShaderID)
{
    shader ReturnProgram;
    ReturnProgram.Shader = glCreateProgram();
    
    glAttachShader(ReturnProgram.Shader, VertexShaderID.ShaderID);
    glAttachShader(ReturnProgram.Shader, FragmentShaderID.ShaderID);
    glLinkProgram(ReturnProgram.Shader);
    
    sh_glVerifyivSuccess(ReturnProgram.Shader, GL_PROGRAM);
    
    glDetachShader(ReturnProgram.Shader, VertexShaderID.ShaderID);
    glDetachShader(ReturnProgram.Shader, FragmentShaderID.ShaderID);
    glDeleteShader(VertexShaderID.ShaderID);
    glDeleteShader(FragmentShaderID.ShaderID);
    
    ReturnProgram.VertexShader = VertexShaderID;
    ReturnProgram.FragmentShader = FragmentShaderID;
    
    return(ReturnProgram);
}

internal void
InitializeOpenGLRendererData(glrenderdata *RenderData, MemoryArena *TransientStorage)
{
    LoadOpenGLFunctions();
    
    glDebugMessageCallback(&OpenGLDebugMessageCallback, nullptr);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glEnable(GL_DEBUG_OUTPUT);
    
    /////////////////////////////
    // NOTE(Sleepster): Shader setup
    {
        RenderData->Shaders[BASIC].VertexShader.Filepath = "shader/Basic.vert";
        RenderData->Shaders[BASIC].FragmentShader.Filepath = "shader/Basic.frag";
        
        RenderData->Shaders[BASIC].VertexShader = 
            sh_glCreateShader(GL_VERTEX_SHADER, RenderData->Shaders[BASIC].VertexShader.Filepath);
        
        RenderData->Shaders[BASIC].FragmentShader = 
            sh_glCreateShader(GL_FRAGMENT_SHADER, RenderData->Shaders[BASIC].FragmentShader.Filepath);
        
        RenderData->Shaders[BASIC] = 
            sh_glCreateProgram(RenderData->Shaders[BASIC].VertexShader, RenderData->Shaders[BASIC].FragmentShader);
    }
    
    RenderData->ScreenSizeID = 
        glGetUniformLocation(RenderData->Shaders[BASIC].Shader, "ScreenSize");
    RenderData->OrthographicMatrixID = 
        glGetUniformLocation(RenderData->Shaders[BASIC].Shader, "CameraMatrix");
    
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    glGenBuffers(1, &RenderData->RendererTransformsSBOID);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, RenderData->RendererTransformsSBOID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(renderertransform) * MAX_TRANSFORMS, 
                 RenderData->RendererTransforms, GL_DYNAMIC_DRAW);
    
    RenderData->Textures[TEXTURE_GAME_ATLAS].Filepath = "res/textures/TextureAtlas.png";
    RenderData->Textures[TEXTURE_GAME_ATLAS].ActiveTexture = GL_TEXTURE0;
    sh_glCreateAndLoadTexture(RenderData->Textures[TEXTURE_GAME_ATLAS].Filepath, RenderData->Textures[TEXTURE_GAME_ATLAS]);
    
    RenderData->Textures[TEXTURE_GAME_ATLAS].LastWriteTime = Win32GetLastWriteTime(RenderData->Textures[TEXTURE_GAME_ATLAS].Filepath);
    
    RenderData->Shaders[BASIC].VertexShader.LastWriteTime = 
        Win32GetLastWriteTime(RenderData->Shaders[BASIC].VertexShader.Filepath);
    
    RenderData->Shaders[BASIC].FragmentShader.LastWriteTime = 
        Win32GetLastWriteTime(RenderData->Shaders[BASIC].FragmentShader.Filepath);
    
    RenderData->Cameras[CAMERA_GAME].Position = {160, -90};
    RenderData->Cameras[CAMERA_GAME].Viewport = {WORLD_WIDTH, WORLD_HEIGHT};
    
    glEnable(GL_FRAMEBUFFER_SRGB);
    glDisable(0x809D); // Disabling multisampling
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    
    glUseProgram(RenderData->Shaders[BASIC].Shader);
}

internal void
sh_glRender(win32windowdata *WindowData, HWND WindowHandle, glrenderdata *RenderData, MemoryArena *Memory)
{
    //////////////////////
    // NOTE(Sleepster): Shader and texture reloading
#if SHIVER_SLOW
    FILETIME NewTextureWriteTime = Win32GetLastWriteTime(RenderData->Textures[TEXTURE_GAME_ATLAS].Filepath);
    FILETIME NewVertexShaderWriteTime = Win32GetLastWriteTime(RenderData->Shaders[BASIC].VertexShader.Filepath);
    FILETIME NewFragmentShaderWriteTime = Win32GetLastWriteTime(RenderData->Shaders[BASIC].FragmentShader.Filepath);
    
    if(CompareFileTime(&NewTextureWriteTime, &RenderData->Textures[TEXTURE_GAME_ATLAS].LastWriteTime) != 0)
    {
        sh_glLoadExistingTexture(RenderData->Textures[TEXTURE_GAME_ATLAS].Filepath, RenderData->Textures[TEXTURE_GAME_ATLAS]);
        RenderData->Textures[TEXTURE_GAME_ATLAS].LastWriteTime = NewTextureWriteTime;
        Sleep(100);
    }
    
    if(CompareFileTime(&NewVertexShaderWriteTime, &RenderData->Shaders[BASIC].VertexShader.LastWriteTime) != 0||
       CompareFileTime(&NewFragmentShaderWriteTime, &RenderData->Shaders[BASIC].FragmentShader.LastWriteTime) != 0)
    {
        RenderData->Shaders[BASIC].VertexShader = 
            sh_glCreateShader(GL_VERTEX_SHADER, RenderData->Shaders[BASIC].VertexShader.Filepath);
        
        RenderData->Shaders[BASIC].FragmentShader = 
            sh_glCreateShader(GL_FRAGMENT_SHADER, RenderData->Shaders[BASIC].FragmentShader.Filepath);
        
        RenderData->Shaders[BASIC] = 
            sh_glCreateProgram(RenderData->Shaders[BASIC].VertexShader, RenderData->Shaders[BASIC].FragmentShader);
        
        glUseProgram(RenderData->Shaders[BASIC].Shader);
        Sleep(100);
    }
#endif
    ///////////////////////
    // NOTE(Sleepster): Actual renderering
    HDC WindowDC = GetDC(WindowHandle);
    glClearColor(0.1f, 0.6f, 1.0f, 1.0f);
    glClearDepth(0.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, WindowData->SizeData.Width, WindowData->SizeData.Height);
    
    vec2 WindowSize = {(real32)WindowData->SizeData.Width, (real32)WindowData->SizeData.Height};
    glUniform2fv(RenderData->ScreenSizeID, 1, &WindowSize.x);
    
    vec4 CameraInfo = 
    {
        RenderData->Cameras[CAMERA_GAME].Position.x - RenderData->Cameras[CAMERA_GAME].Viewport.x, 
        RenderData->Cameras[CAMERA_GAME].Position.x + RenderData->Cameras[CAMERA_GAME].Viewport.x, 
        RenderData->Cameras[CAMERA_GAME].Position.y - RenderData->Cameras[CAMERA_GAME].Viewport.y, 
        RenderData->Cameras[CAMERA_GAME].Position.y + RenderData->Cameras[CAMERA_GAME].Viewport.y
    };
    
    
    RenderData->Cameras[CAMERA_GAME].Matrix = CreateOrthographicMatrix(CameraInfo);
    glUniformMatrix4fv(RenderData->OrthographicMatrixID, 1, GL_FALSE, (const GLfloat *)&RenderData->Cameras[CAMERA_GAME].Matrix.Elements[0][0]);
    
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(renderertransform) * RenderData->TransformCounter, RenderData->RendererTransforms);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, RenderData->TransformCounter);
    SwapBuffers(WindowDC);
}
