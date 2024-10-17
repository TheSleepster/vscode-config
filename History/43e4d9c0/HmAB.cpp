#include "Sugar_Intrinsics.h"
#include "Sugar.h"
#include "SugarAPI.h"
#include "win32_Sugar.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../data/deps/stbimage/stb_image.h"
#include "../data/deps/OpenGL/GLL.h"

enum ShaderType 
{
    VERTEX_SHADER,
    FRAGMENT_SHADER,

    SHADER_COUNT
};

struct glContext 
{
    GLuint ProgramID; 
    GLuint TextureID;
    GLuint TransformSBOID;
    GLuint ScreenSizeID;
    
    const char *VertexShaderFilepath;
    const char *FragmentShaderFilepath;
    const char *TextureDataFilepath;

    FILETIME TextureTimestamp;
    FILETIME ShaderTimestamp;
    
    ShaderType ShaderType;
};

global_variable glContext glContext = {};
// TODO : Make these "GetLastWriteTime" functions platform agnostic

internal void
LoadTexture(const char *Filepath) 
{
    glActiveTexture(GL_TEXTURE0);
    int Width, Height, Channels;
    char *Result = (char *)stbi_load(Filepath, &Width, &Height, &Channels, 4); 
    if(Result) 
    {
        glContext.TextureTimestamp = Win32GetLastWriteTime(Filepath);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, Width, Height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, Result);
        stbi_image_free(Result);
    }
}

internal GLuint 
LoadShader(int ShaderType, const char *Filepath, GameMemory *GameMemory) 
{
    int FileSize = 0;
    GLuint ShaderID = {}; 

    char *Shader = ReadEntireFileBA(Filepath, &FileSize, &GameMemory->TransientStorage); 
    if(Shader) 
    {
        ShaderID = glCreateShader(ShaderType);
        glShaderSource(ShaderID, 1, &Shader, 0);
        glCompileShader(ShaderID);
        {
            int Success;
            char ShaderLog[2048] = {};

            glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &Success);
            if(!Success) 
            {
                glGetShaderInfoLog(ShaderID, 2048, 0, ShaderLog);
                Assert(false, ShaderLog);
            }
        }
    }
    return(ShaderID);
}

internal void 
ReloadShaders(GameMemory *GameMemory) 
{
    GLuint VertexShaderID = 
        LoadShader(GL_VERTEX_SHADER, glContext.VertexShaderFilepath, GameMemory);
    GLuint FragmentShaderID = 
        LoadShader(GL_FRAGMENT_SHADER, glContext.FragmentShaderFilepath, GameMemory);

    glAttachShader(glContext.ProgramID, VertexShaderID);
    glAttachShader(glContext.ProgramID, FragmentShaderID);
    glLinkProgram(glContext.ProgramID);

    glDetachShader(glContext.ProgramID, VertexShaderID);
    glDetachShader(glContext.ProgramID, FragmentShaderID);
    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    FILETIME TimeStampVertex = Win32GetLastWriteTime(glContext.VertexShaderFilepath); 
    FILETIME TimeStampFragment = Win32GetLastWriteTime(glContext.FragmentShaderFilepath); 
    glContext.ShaderTimestamp = maxFiletime(TimeStampVertex, TimeStampFragment);
}

internal void
OpenGLRender(GameMemory *GameMemory, Win32_WindowData WindowData) 
{
    FILETIME NewTextureWriteTime = Win32GetLastWriteTime(glContext.TextureDataFilepath);
    if(CompareFileTime(&NewTextureWriteTime, &glContext.TextureTimestamp) != 0) 
    {
        LoadTexture(glContext.TextureDataFilepath);
    }
    
    FILETIME TimeStampVertex = Win32GetLastWriteTime(glContext.VertexShaderFilepath); 
    FILETIME TimeStampFragment = Win32GetLastWriteTime(glContext.FragmentShaderFilepath); 
    if(CompareFileTime(&TimeStampVertex, &glContext.ShaderTimestamp) != 0||
       CompareFileTime(&TimeStampFragment, &glContext.ShaderTimestamp) != 0) 
    { 
        ReloadShaders(GameMemory);
    }

    glClearColor(1.0f, 0.1f, 1.0f, 1.0f);
    glClearDepth(0.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, WindowData.WindowWidth, WindowData.WindowHeight);

    vec2 ScreenSize = {(real32)WindowData.WindowWidth, (real32)WindowData.WindowHeight};
    glUniform2fv(glContext.ScreenSizeID, 1, &ScreenSize.x);

    glBufferSubData
        (GL_SHADER_STORAGE_BUFFER, 0, sizeof(Transform) * GameRenderData->TransformCount, GameRenderData->Transforms);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, GameRenderData->TransformCount);
    GameRenderData->TransformCount = 0;
    SwapBuffers(WindowData.WindowDC);
}

internal void APIENTRY
OpenGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, 
                    GLsizei length, const GLchar *message, const void *userparam) 
{
    if(severity == GL_DEBUG_SEVERITY_LOW||
       severity == GL_DEBUG_SEVERITY_MEDIUM||
       severity == GL_DEBUG_SEVERITY_HIGH) 
    {
        Assert(false, message);
    }
    else 
    {
        Trace(message);
    }
}

// TODO : Clean this up. Idrk how you can though.

internal void
InitializeOpenGLRenderer(GameMemory *GameMemory)
{
    LoadGLFunctions();

    glContext.VertexShaderFilepath = "../code/shader/BasicVertexShader.glsl";
    glContext.FragmentShaderFilepath = "../code/shader/BasicFragmentShader.glsl";
    glContext.TextureDataFilepath = "../data/res/textures/TextureAtlas.png";

    glDebugMessageCallback(&OpenGLDebugCallback, nullptr);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glEnable(GL_DEBUG_OUTPUT);

    // LOAD A SHADER
    GLuint VertexShaderID = 
        LoadShader(GL_VERTEX_SHADER, glContext.VertexShaderFilepath, GameMemory);
    GLuint FragmentShaderID = 
        LoadShader(GL_FRAGMENT_SHADER, glContext.FragmentShaderFilepath, GameMemory);

    FILETIME TimeStampVertex = Win32GetLastWriteTime(glContext.VertexShaderFilepath); 
    FILETIME TimeStampFragment = Win32GetLastWriteTime(glContext.FragmentShaderFilepath); 
    glContext.ShaderTimestamp = maxFiletime(TimeStampVertex, TimeStampFragment);
    // END OF LOADING

    glContext.ProgramID = glCreateProgram();
    glAttachShader(glContext.ProgramID, VertexShaderID);
    glAttachShader(glContext.ProgramID, FragmentShaderID);
    glLinkProgram(glContext.ProgramID);

    // NOTE : Local scope to test for the successful compilation of the Program
    {
        int Success;
        char ProgramLog[2048] = {};

        glGetProgramiv(glContext.ProgramID, GL_LINK_STATUS, &Success);
        if(!Success) 
        {
            glGetProgramInfoLog(glContext.ProgramID, 2048, 0, ProgramLog);
            Assert(false, ProgramLog); 
        }
    }

    glDetachShader(glContext.ProgramID, VertexShaderID);
    glDetachShader(glContext.ProgramID, FragmentShaderID);
    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    int Width, Height, Channels;
    char *Data = (char *)stbi_load(glContext.TextureDataFilepath, &Width, &Height, &Channels, 4); 
    Assert(Data, "Failed to get the TextureAtlas' data!\n");

    // NOTE : This is similar to creating a WGL context.
    // - You create a texture
    // - You Bind to it
    // - You Assign some attributes to it
    // - Use it
    {
        glGenTextures(1, &glContext.TextureID);
        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_2D, glContext.TextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, Width, Height,
                0, GL_RGBA, GL_UNSIGNED_BYTE, Data);
        stbi_image_free(Data);
    }

    glGenBuffers(1, &glContext.TransformSBOID);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, glContext.TransformSBOID);

    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Transform) * MAX_TRANSFORMS, 
            GameRenderData->Transforms, GL_DYNAMIC_DRAW);
    glContext.ScreenSizeID = glGetUniformLocation(glContext.ProgramID, "ScreenSize");

    glEnable(GL_FRAMEBUFFER_SRGB);
    glDisable(0x809D); // Disable multisampling
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);

    glUseProgram(glContext.ProgramID);
}
