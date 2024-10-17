#include "Sugar.h"
#include "Sugar_Math.h"
#include "SugarAPI.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../data/deps/stbimage/stb_image.h"
#include "../data/deps/OpenGL/GLL.h"

global_variable const char *TEXTURE_ATLAS_PATH = "../data/res/textures/TextureAtlas.png";

struct glContext 
{
    GLuint ProgramID; 
    GLuint TextureID;
    GLuint TransformSBOID;
    GLuint ScreenSizeID;
};

global_variable glContext glContext = {};

internal void OpenGLRender(Win32_WindowData WindowData) 
{
    glClearColor(1.0f, 0.4f, 1.0f, 1.0f);
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

internal void
InitializeOpenGLRenderer(GameMemory *GameMemory)
{
    LoadGLFunctions();
    glDebugMessageCallback(&OpenGLDebugCallback, nullptr);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glEnable(GL_DEBUG_OUTPUT);
    
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    int FileSize = 0;
    char *VertexShader = ReadEntireFileBA("../code/shader/BasicVertexShader.glsl", &FileSize, &GameMemory->TransientStorage);
    char *FragmentShader = ReadEntireFileBA("../code/shader/BasicFragmentShader.glsl", &FileSize, &GameMemory->TransientStorage);
    Assert(VertexShader, "Failed to load the Vertex Shader!\n");
    Assert(FragmentShader, "Failed to load the Fragment Shader!\n");

    glShaderSource(VertexShaderID, 1, &VertexShader, 0);
    glShaderSource(FragmentShaderID, 1, &FragmentShader, 0);

    glCompileShader(VertexShaderID);
    glCompileShader(FragmentShaderID);
    // NOTE : Local scope to test for the successful compilation of the Vertex shader
    {
        int Success;
        char ShaderLog[2048] = {};

        glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Success);
        if(!Success) 
        {
            glGetShaderInfoLog(VertexShaderID, 2048, 0, ShaderLog);
            Assert(false, ShaderLog);
        }
    }

    // NOTE : Local scope to test for the successful compilation of the Fragment shader
    {
        int Success;
        char ShaderLog[2048] = {};

        glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Success);
        if(!Success) 
        {
            glGetShaderInfoLog(FragmentShaderID, 2048, 0, ShaderLog);
            Assert(false, ShaderLog);
        }
    }

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

    // NOTE : Testing Texture Rendering
    int Width, Height, Channels;
    char *Data = (char *)stbi_load(TEXTURE_ATLAS_PATH, &Width, &Height, &Channels, 4); 
    Assert(Data, "Failed to get the TextureAtlas' data!\n");

    // NOTE : This is similar to creating a WGL context.
    // - You create a texture
    // - You Bind to it
    // - You Assign some attributes to it
    // - Use it
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