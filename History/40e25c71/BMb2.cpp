#include "../data/deps/Freetype/include/ft2build.h"
#include FT_FREETYPE_H

// INTRINSICS
#include "Intrinsics.h"

// UTILS
#include "util/Math.h"
#include "util/Array.h"
#include "util/FileIO.h"
#include "util/MemoryArena.h"
#include "util/CustomStrings.h"

#include "Clover_Renderer.h"

internal void APIENTRY
OpenGLDebugMessageCallback(GLenum Source, GLenum Type, GLuint ID, GLenum Severity,
                           GLsizei Length, const GLchar *Message, const void *UserParam)
{
    if(Severity == GL_DEBUG_SEVERITY_LOW||
       Severity == GL_DEBUG_SEVERITY_MEDIUM||
       Severity == GL_DEBUG_SEVERITY_HIGH)
    {
        printlm("Error: %s\n", Message);
        Assert(false);
    }
    else
    {
        // NOTE(Sleepster): We don't have Tracing here because ImGui will BLAST my output window otherwise 
    }
}

internal void
glVerifyIVStatus(GLuint TestID, GLuint Type)
{
    bool32 Success = {};
    char ShaderLog[2048] = {};
    
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
        glGetShaderInfoLog(TestID, 2048, 0, ShaderLog);
        printm("ERROR ON SHADER COMPILATION: %s\n", ShaderLog);
        Assert(false);
    }
}

internal void
CloverLoadFont(memory_arena *Memory, gl_render_data *RenderData, string Filepath, uint32 FontSize, font_index FontIndex)
{
    freetype_font_data Font = {};
    Font.FontSize = FontSize;
    FT_Error Error;

    Error = FT_Init_FreeType(&Font.FontFile);
    Check(Error == 0, "Failed to initialize Freetype\n");
    
    Error = FT_New_Face(Font.FontFile, (const char *)Filepath.Data, 0, &Font.FontFace);
    Check(Error == 0, "Failed to initialize the Font Face\n");
    Check(Error != FT_Err_Unknown_File_Format, "Failed to load the font file, it is found but not supported\n");
    
    // NOTE(Sleepster): Test, we would normally use FT_Set_Pixel_Sizes(); 
    Error = FT_Set_Pixel_Sizes(Font.FontFace, 0, Font.FontSize);
    Check(Error == 0, "Issue setting the pixel size of the font\n");
    
    Font.AtlasPadding = 4;
    int32 Row = {};
    int32 Column = Font.AtlasPadding;
    
    char *TextureData = ArenaAlloc(Memory, (uint64)(sizeof(char) * (BITMAP_ATLAS_SIZE * BITMAP_ATLAS_SIZE)));
    if(TextureData)
    {
        for(uint32 GlyphIndex = 32;
            GlyphIndex < 127;
            ++GlyphIndex)
        {
            if(Column + Font.FontFace->glyph->bitmap.width + Font.AtlasPadding >= BITMAP_ATLAS_SIZE)
            {
                Column = Font.AtlasPadding;
                // NOTE(Sleepster): This is a product of our "test". Freetype uses >> 6 on sizes, so we divide by 3 to offset it 
                //                  ONLY WHEN USING FT_Set_Char_Size
                Row += Font.FontSize;
            }
            // NOTE(Sleepster): Leaving this here would make it only work for one font, perhaps make it so we pass in a font index into an array of 
            //                  fonts? Simply have a cap for the amount of fonts that can be loaded at any one point.
            RenderData->LoadedFonts[FontIndex].FontHeight = MAX((Font.FontFace->size->metrics.ascender - Font.FontFace->size->metrics.descender) >> 6,
                                                                (int32)RenderData->LoadedFonts[FontIndex].FontHeight);
            for(uint32 YIndex = 0;
                YIndex < Font.FontFace->glyph->bitmap.rows;
                ++YIndex)
            {
                for(uint32 XIndex = 0;
                    XIndex < Font.FontFace->glyph->bitmap.width;
                    ++XIndex)
                {
                    TextureData[(Row + YIndex) * BITMAP_ATLAS_SIZE + (Column + XIndex)] = 
                        Font.FontFace->glyph->bitmap.buffer[YIndex * Font.FontFace->glyph->bitmap.width + XIndex];
                }
            }
            font_glyph *CurrentGlyph = &RenderData->LoadedFonts[FontIndex].Glyphs[GlyphIndex];
            CurrentGlyph->GlyphUVs  = {Column, Row};
            CurrentGlyph->GlyphSize = 
            {
                (int32)Font.FontFace->glyph->bitmap.width, 
                (int32)Font.FontFace->glyph->bitmap.rows
            };
            CurrentGlyph->GlyphAdvance = 
            {
                real32(Font.FontFace->glyph->advance.x >> 6), 
                real32(Font.FontFace->glyph->advance.y >> 6)
            };
            CurrentGlyph->GlyphOffset = 
            {
                real32(Font.FontFace->glyph->bitmap_left),
                real32(Font.FontFace->glyph->bitmap_top)
            };
            
            Column += Font.FontFace->glyph->bitmap.width + Font.AtlasPadding;
        }
        
        FT_Done_Face(Font.FontFace);
        FT_Done_FreeType(Font.FontFile);
        
        // OPENGL TEXTURE
        {
            glActiveTexture(GL_TEXTURE0 + RenderData->TextureCount);
            ++RenderData->TextureCount;
            
            glGenTextures(1, &RenderData->LoadedFonts[FontIndex].FontAtlas.TextureID);
            glBindTexture(GL_TEXTURE_2D, RenderData->LoadedFonts[FontIndex].FontAtlas.TextureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, BITMAP_ATLAS_SIZE, BITMAP_ATLAS_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, TextureData);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    else
    {
        Check(0, "Failed to Allocate Storage\n");
    }
}

// TODO(Sleepster): Revisit this to fix the alignment issues with letters like "p" "g" "l" "y" and such
internal void
CloverLoadSDFFont(memory_arena *Memory, gl_render_data *RenderData, string Filepath, uint32 FontSize, font_index FontName)
{
    freetype_font_data Font = {};
    Font.FontSize = FontSize;
    FT_Error Error;
    
    Error = FT_Init_FreeType(&Font.FontFile);
    Check(Error == 0, "Failed to initialize Freetype\n");
    
    Error = FT_New_Face(Font.FontFile, (const char *)Filepath.Data, 0, &Font.FontFace);
    Check(Error == 0, "Failed to initialize the Font Face\n");
    Check(Error != FT_Err_Unknown_File_Format, "Failed to load the font file, it is found but not supported\n");
    
    // NOTE(Sleepster): Test, we would normally use FT_Set_Pixel_Sizes(); 
    Error = FT_Set_Pixel_Sizes(Font.FontFace, 0, Font.FontSize);
    Check(Error == 0, "Issue setting the pixel size of the font\n");
    
    Font.AtlasPadding = 8;
    int32 Row = {};
    int32 Column = Font.AtlasPadding;
    
    FT_GlyphSlot CurrentSlot = Font.FontFace->glyph;
    
    char *TextureData = ArenaAlloc(Memory, (uint64)(sizeof(char) * (BITMAP_ATLAS_SIZE * BITMAP_ATLAS_SIZE)));
    if(TextureData)
    {
        for(uint32 GlyphIndex = 32;
            GlyphIndex < 127;
            ++GlyphIndex)
        {
            FT_Load_Char(Font.FontFace, GlyphIndex, FT_LOAD_DEFAULT);
            if(Column + Font.FontFace->glyph->bitmap.width + Font.AtlasPadding >= BITMAP_ATLAS_SIZE)
            {
                Column = Font.AtlasPadding;
                Row += int32(Font.FontSize * 1.20);
            }
            
            Error = FT_Render_Glyph(CurrentSlot, FT_RENDER_MODE_SDF);
            Check(Error == 0, "Issues here\n");
            
            RenderData->LoadedFonts[FontName].FontHeight   = MAX((Font.FontFace->size->metrics.ascender - Font.FontFace->size->metrics.descender) >> 6,
                                                                 (int32)RenderData->LoadedFonts[GlyphIndex].FontHeight);
            for(uint32 YIndex = 0;
                YIndex < Font.FontFace->glyph->bitmap.rows;
                ++YIndex)
            {
                for(uint32 XIndex = 0;
                    XIndex < Font.FontFace->glyph->bitmap.width;
                    ++XIndex)
                {
                    TextureData[(Row + YIndex) * BITMAP_ATLAS_SIZE + (Column + XIndex)] = 
                        Font.FontFace->glyph->bitmap.buffer[YIndex * Font.FontFace->glyph->bitmap.width + XIndex];
                }
            }
            
            font_glyph *CurrentGlyph = &RenderData->LoadedFonts[0].Glyphs[GlyphIndex];
            CurrentGlyph->GlyphUVs  = {Column, Row};
            CurrentGlyph->GlyphSize = 
            {
                (int32)Font.FontFace->glyph->bitmap.width, 
                (int32)Font.FontFace->glyph->bitmap.rows
            };
            CurrentGlyph->GlyphAdvance = 
            {
                real32(Font.FontFace->glyph->advance.x >> 6), 
                real32(Font.FontFace->glyph->advance.y >> 6)
            };
            CurrentGlyph->GlyphOffset = 
            {
                real32(Font.FontFace->glyph->bitmap_left),
                real32(Font.FontFace->glyph->bitmap_top)
            };
            
            Column += Font.FontFace->glyph->bitmap.width + Font.AtlasPadding;
        }
    }
    
    FT_Done_Face(Font.FontFace);
    FT_Done_FreeType(Font.FontFile);
    
    // SDF TEXTURE DATA
    {
        glActiveTexture(GL_TEXTURE0 + RenderData->TextureCount);
        glGenTextures(1, &RenderData->LoadedFonts[0].FontAtlas.TextureID);
        glBindTexture(GL_TEXTURE_2D, RenderData->LoadedFonts[0].FontAtlas.TextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, BITMAP_ATLAS_SIZE, BITMAP_ATLAS_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, TextureData);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
        
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

internal gl_shader_source
CloverLoadShaderSource(memory_arena *Scratch, uint32 ShaderType, string Filepath)
{
    uint32 FileSize = 0;
    gl_shader_source ReturnShader = {};
    
    string ShaderSource = ReadEntireFileMA(Scratch, Filepath, &FileSize);
    ReturnShader.Filepath = Filepath;
    
    if(ShaderSource.Data)
    {
        const char *ShaderSourceChar = (const char *)ShaderSource.Data;

        ReturnShader.SourceID = glCreateShader(ShaderType);
        glShaderSource(ReturnShader.SourceID, 1, &ShaderSourceChar, 0);
        glCompileShader(ReturnShader.SourceID);
        glVerifyIVStatus(ReturnShader.SourceID, GL_VERTEX_SHADER);
    }
    else
    {
        Trace("File is either not found or does not contain strings!\n");
        Assert(false);
    }
    return(ReturnShader);
}

internal shader
CloverCreateShader(memory_arena *Memory, string VertexShader, string FragmentShader)
{
    glUseProgram(0);
    shader ReturnShader = {};
    
    ReturnShader.VertexShader   = CloverLoadShaderSource(Memory, GL_VERTEX_SHADER, VertexShader);
    ReturnShader.FragmentShader = CloverLoadShaderSource(Memory, GL_FRAGMENT_SHADER, FragmentShader);
    
    ReturnShader.VertexShader.LastWriteTime   = Win32GetLastWriteTime(VertexShader);
    ReturnShader.FragmentShader.LastWriteTime = Win32GetLastWriteTime(FragmentShader);
    
    ReturnShader.ShaderID = glCreateProgram();
    glAttachShader(ReturnShader.ShaderID, ReturnShader.VertexShader.SourceID);
    glAttachShader(ReturnShader.ShaderID, ReturnShader.FragmentShader.SourceID);
    glLinkProgram(ReturnShader.ShaderID);
    
    glVerifyIVStatus(ReturnShader.ShaderID, GL_PROGRAM);
    
    glDetachShader(ReturnShader.ShaderID, ReturnShader.VertexShader.SourceID);
    glDetachShader(ReturnShader.ShaderID, ReturnShader.FragmentShader.SourceID);
    glDeleteShader(ReturnShader.VertexShader.SourceID);
    glDeleteShader(ReturnShader.FragmentShader.SourceID);
    
    return(ReturnShader);
}

internal void
CloverLoadTexture(gl_render_data *RenderData, texture2d *TextureInfo, string Filepath)
{
    glActiveTexture(GL_TEXTURE0 + RenderData->TextureCount);
    RenderData->TextureCount++;
    
    glGenTextures(1, &TextureInfo->TextureID);
    glBindTexture(GL_TEXTURE_2D, TextureInfo->TextureID);
    
    TextureInfo->Filepath = Filepath;
    TextureInfo->LastWriteTime = Win32GetLastWriteTime(Filepath);
    TextureInfo->RawData = (char *)stbi_load((const char *)Filepath.Data, 
                                             &TextureInfo->TextureData.Width, 
                                             &TextureInfo->TextureData.Height, 
                                             &TextureInfo->TextureData.Channels, 
                                             4);
    if(TextureInfo->RawData)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, 
                     TextureInfo->TextureData.Width, TextureInfo->TextureData.Height, 0, 
                     GL_RGBA, GL_UNSIGNED_BYTE, TextureInfo->RawData);
    }
    stbi_image_free(TextureInfo->RawData);
    glBindTexture(GL_TEXTURE_2D, 0);
}

internal void
CloverReloadTexture(gl_render_data *RenderData, texture2d *TextureInfo, uint32 TextureIndex)
{
    glBindTexture(GL_TEXTURE_2D, TextureInfo->TextureID);
    
    TextureInfo->LastWriteTime = Win32GetLastWriteTime(TextureInfo->Filepath);
    TextureInfo->RawData = (char *)stbi_load((const char *)TextureInfo->Filepath.Data, 
                                             &TextureInfo->TextureData.Width, 
                                             &TextureInfo->TextureData.Height, 
                                             &TextureInfo->TextureData.Channels, 
                                             4);
    if(TextureInfo->RawData)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, 
                     TextureInfo->TextureData.Width, TextureInfo->TextureData.Height, 0, 
                     GL_RGBA, GL_UNSIGNED_BYTE, TextureInfo->RawData);
    }
    stbi_image_free(TextureInfo->RawData);
    glBindTexture(GL_TEXTURE_2D, 0);
}

internal void
CloverResetRendererState(gl_render_data *RenderData)
{
    RenderData->DrawFrame.VertexBufferptr            = &RenderData->DrawFrame.Vertices[0];
    RenderData->DrawFrame.TransparentVertexBufferptr = &RenderData->DrawFrame.Vertices[int32(MAX_VERTICES * 0.5f)];
    RenderData->DrawFrame.OpaqueQuadCount = 0;
    RenderData->DrawFrame.TransparentQuadCount = 0;
    RenderData->DrawFrame.TotalQuadCount = 0;
    
    RenderData->DrawFrame.UIVertexBufferptr            = &RenderData->DrawFrame.UIVertices[0];
    RenderData->DrawFrame.TransparentUIVertexBufferptr = &RenderData->DrawFrame.UIVertices[int32(MAX_VERTICES * 0.5f)];
    RenderData->DrawFrame.OpaqueUIElementCount = 0;
    RenderData->DrawFrame.TransparentUIElementCount = 0;
    RenderData->DrawFrame.TotalUIElementCount = 0;

    RenderData->DrawFrame.PointLightCount = 0;
    RenderData->DrawFrame.SpotLightCount = 0;
}

internal int32
CompareVertexYAxis(const void *A, const void *B)
{
    const vertex *VertexA = (vertex *)A;
    const vertex *VertexB = (vertex *)B;
    
    return((VertexA->Position.Y > VertexB->Position.Y) ? -1 :
           (VertexA->Position.Y < VertexB->Position.Y) ?  1 : 0);
}

internal void
CloverSetupRenderer(memory_arena *Memory, gl_render_data *RenderData)
{
    // STATE INITIALIZATION
    {
        glDebugMessageCallback(&OpenGLDebugMessageCallback, nullptr);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glEnable(GL_DEBUG_OUTPUT);
        
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GREATER);
        
        glEnable(GL_BLEND);        
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);
        
        glEnable(GL_FRAMEBUFFER_SRGB);
        glDisable(0x809D); // Disabling multisampling
        
        RenderData->ClearColor = DARK_GRAY;
        RenderData->TextureCount = 1;
    }
    
    
    uint32 Indices[MAX_INDICES] = {};
    uint32 Offset               = 0;
    for(uint32 Index = 0;
        Index < MAX_INDICES;
        Index += 6)
    {
        Indices[Index + 0] = Offset + 0;
        Indices[Index + 1] = Offset + 1;
        Indices[Index + 2] = Offset + 2;
        Indices[Index + 3] = Offset + 2;
        Indices[Index + 4] = Offset + 3;
        Indices[Index + 5] = Offset + 0;
        Offset += 4;
    }
    
    
    // GAME ASSETS BUFFER SETUP
    {
        glGenVertexArrays(1, &RenderData->GameVAOID);
        glBindVertexArray(RenderData->GameVAOID);
        
        glGenBuffers(1, &RenderData->GameVBOID);
        glBindBuffer(GL_ARRAY_BUFFER, RenderData->GameVBOID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * MAX_VERTICES, 0, GL_DYNAMIC_DRAW);
        
        glGenBuffers(1, &RenderData->GameEBOID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RenderData->GameEBOID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, Position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, TextureCoords));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, VertexNormals));
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, DrawColor));
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, TextureIndex));
        
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(4);
    }
    
    // GAME UI BUFFER SETUP
    {
        glGenVertexArrays(1, &RenderData->GameUIVAOID);
        glBindVertexArray(RenderData->GameUIVAOID);
        
        glGenBuffers(1, &RenderData->GameUIVBOID);
        glBindBuffer(GL_ARRAY_BUFFER, RenderData->GameUIVBOID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * MAX_VERTICES, 0, GL_DYNAMIC_DRAW);
        
        glGenBuffers(1, &RenderData->GameUIEBOID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RenderData->GameUIEBOID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, Position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, TextureCoords));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, VertexNormals));
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, DrawColor));
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, TextureIndex));
        
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(4);
    }

    // TEXTURE/FONT LOADING
    {
        // NOTE(Sleepster): The order is important, whatever you gen first will end up in the GL_TEXTUREX slot 
        CloverLoadTexture(RenderData, &RenderData->GameAtlas, STR("../data/res/textures/TextureAtlas.png"));
        CloverLoadSDFFont(Memory, RenderData, STR("../data/res/fonts/UbuntuMono-B.ttf"), 48, UBUNTU_MONO);
    }

    // FRAMEBUFFER SETUP
    {
        // GBUFFER
        {
            glGenFramebuffers(1, &RenderData->gBufferFBOID);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, RenderData->gBufferFBOID);
            glGenTextures(3, RenderData->gBuffer);

            // POSITION
            glBindTexture(GL_TEXTURE_2D, RenderData->gBuffer[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SizeData.Width, SizeData.Height, 0, GL_RGBA, GL_FLOAT, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, RenderData->gBuffer[0], 0);

            // NORMAL
            glBindTexture(GL_TEXTURE_2D, RenderData->gBuffer[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SizeData.Width, SizeData.Height, 0, GL_RGBA, GL_FLOAT, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, RenderData->gBuffer[1], 0);

            // ALBEDO + SPECULAR MAP
            glBindTexture(GL_TEXTURE_2D, RenderData->gBuffer[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SizeData.Width, SizeData.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, RenderData->gBuffer[2], 0);

            // DEPTH
            glGenRenderbuffers(1, &RenderData->gBufferDepthRBID);
            glBindRenderbuffer(GL_RENDERBUFFER, RenderData->gBufferDepthRBID);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SizeData.Width, SizeData.Height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RenderData->gBufferDepthRBID);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            
            uint32 Attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
            glDrawBuffers(3, Attachments);
            
            GLenum Success = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            Check(Success == GL_FRAMEBUFFER_COMPLETE, "[ ERROR ] Framebuffer Status Failure: 0x%x\n", Success);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // LIGHTING
        {
            /* glGenFramebuffers(1, &RenderData->LightingFBOID); */
            /* glBindFramebuffer(GL_DRAW_FRAMEBUFFER, RenderData->LightingFBOID); */
            /* glBindFramebuffer(GL_FRAMEBUFFER, 0); */
        }
    }

    // LIGHTING DATA STUFF
    {
        // NOTE(Sleepster): Point Light Shader Buffer
        uint64 MaxBufferSize = sizeof(struct point_light) * MAX_POINT_LIGHTS;
        glGenBuffers(1, &RenderData->PointLightSBOID);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, RenderData->PointLightSBOID);
        glBufferData(GL_SHADER_STORAGE_BUFFER, MaxBufferSize, 0, GL_DYNAMIC_DRAW);

        // NOTE(Sleepster): Spot Light Shader Buffer
        MaxBufferSize = sizeof(struct spot_light) * MAX_SPOT_LIGHTS;
        glGenBuffers(1, &RenderData->SpotLightSBOID);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, RenderData->SpotLightSBOID);
        glBufferData(GL_SHADER_STORAGE_BUFFER, MaxBufferSize, 0, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
    
    // SHADER SETUP
    {
        // NOTE(Sleepster): This is for the common shader includes 
        uint32 Size = {};
        string CommonGLHeader = ReadEntireFileMA(Memory, STR("../code/shader/CommonShader.glh"), &Size);
        glNamedStringARB(GL_SHADER_INCLUDE_ARB, -1, "/../code/shader/CommonShader.glh", int32(CommonGLHeader.Length), CSTR(CommonGLHeader));

        RenderData->BasicShader = 
            CloverCreateShader(Memory, STR("../code/shader/Basic.vert"), STR("../code/shader/Basic.frag"));
        RenderData->gBufferShader = 
            CloverCreateShader(Memory, STR("../code/shader/gBuffer_Geo.vert"), STR("../code/shader/gBuffer_Geo.frag"));
    }

    // SHADER UNIFORM / STORAGE BUFFER SETUP
    {
        RenderData->GameCamera.ViewMatrix   = mat4Identity(1.0f);
        RenderData->GameUICamera.ViewMatrix = mat4Identity(1.0f);

        RenderData->ProjectionMatrixUID        = glGetUniformLocation(RenderData->BasicShader.ShaderID,    "ProjectionMatrix");
        RenderData->ViewMatrixUID              = glGetUniformLocation(RenderData->BasicShader.ShaderID,    "ViewMatrix");

        RenderData->gBufferProjectionMatrixUID = glGetUniformLocation(RenderData->gBufferShader.ShaderID,  "ProjectionMatrix");
        RenderData->gBufferViewMatrixUID       = glGetUniformLocation(RenderData->gBufferShader.ShaderID,  "ViewMatrix");
    }
}

internal void
CloverRender(gl_render_data *RenderData)
{
    // NOTE(Sleepster): Figure out this offset  
    // OPAQUE GAME OBJECT RENDERING PASS
    glUseProgram(RenderData->BasicShader.ShaderID);
    if(RenderData->DrawFrame.OpaqueQuadCount > 0)
    {
        {
            glEnable(GL_DEPTH_TEST);
            glBindBuffer(GL_ARRAY_BUFFER, RenderData->GameVBOID);
            glBufferSubData(GL_ARRAY_BUFFER, 
                            0, 
                            (RenderData->DrawFrame.OpaqueQuadCount * 4) * sizeof(vertex), 
                            RenderData->DrawFrame.Vertices);

            glUniformMatrix4fv(RenderData->ProjectionMatrixUID, 1, GL_FALSE, &RenderData->GameCamera.ProjectionMatrix.Elements[0][0]);
            glUniformMatrix4fv(RenderData->ViewMatrixUID, 1, GL_FALSE, &RenderData->GameCamera.ViewMatrix.Elements[0][0]);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, RenderData->GameAtlas.TextureID);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, RenderData->LoadedFonts[UBUNTU_MONO].FontAtlas.TextureID);

            glBindVertexArray(RenderData->GameVAOID);
            glDrawElements(GL_TRIANGLES, 
                        RenderData->DrawFrame.OpaqueQuadCount * 6, 
                        GL_UNSIGNED_INT, 
                        0); 
        }
    }
    
    if(RenderData->DrawFrame.TransparentQuadCount > 0)
    {
        GLintptr BufferOffset   =  (RenderData->DrawFrame.OpaqueQuadCount * 4) * sizeof(vertex);
        GLintptr ElementOffset  =  (RenderData->DrawFrame.OpaqueQuadCount * 6) * sizeof(uint32); 
        // TRANSPARENT GAME OBJECT RENDERERING PASS
        {
            glDisable(GL_DEPTH_TEST);

            glEnable(GL_BLEND);        
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            glBufferSubData(GL_ARRAY_BUFFER, 
                            BufferOffset, 
                            (RenderData->DrawFrame.TransparentQuadCount * 4) * sizeof(vertex), 
                            &RenderData->DrawFrame.Vertices[int32(MAX_VERTICES * 0.5f)]);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, RenderData->GameAtlas.TextureID);

            glUniformMatrix4fv(RenderData->ProjectionMatrixUID, 1, GL_FALSE, &RenderData->GameCamera.ProjectionMatrix.Elements[0][0]);
            glUniformMatrix4fv(RenderData->ViewMatrixUID, 1, GL_FALSE, &RenderData->GameCamera.ViewMatrix.Elements[0][0]);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, RenderData->LoadedFonts[UBUNTU_MONO].FontAtlas.TextureID);

            glBindVertexArray(RenderData->GameVAOID);
            glDrawElements(GL_TRIANGLES, 
                        RenderData->DrawFrame.TransparentQuadCount * 6, 
                        GL_UNSIGNED_INT, 
                        (void*)ElementOffset);
        }
    }
    
    if(RenderData->DrawFrame.TransparentUIElementCount > 0)
    {
        GLintptr UIBufferOffset   = int32((RenderData->DrawFrame.OpaqueUIElementCount * 4) * sizeof(vertex));
        GLintptr UIElementOffset  = (RenderData->DrawFrame.OpaqueUIElementCount * 6) * sizeof(uint32); 
        // TRANSPARENT UI RENDERING PASS
        {
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);        
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            glBindBuffer(GL_ARRAY_BUFFER, RenderData->GameUIVBOID);
            glBufferSubData(GL_ARRAY_BUFFER, 
                            UIBufferOffset, 
                            (RenderData->DrawFrame.TransparentUIElementCount * 4) * sizeof(vertex), 
                            &RenderData->DrawFrame.UIVertices[int32(MAX_VERTICES * 0.5f)]);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, RenderData->GameAtlas.TextureID);

            glUniformMatrix4fv(RenderData->ProjectionMatrixUID, 1, GL_FALSE, &RenderData->GameUICamera.ProjectionMatrix.Elements[0][0]);
            glUniformMatrix4fv(RenderData->ViewMatrixUID, 1, GL_FALSE, &RenderData->GameUICamera.ViewMatrix.Elements[0][0]);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, RenderData->LoadedFonts[UBUNTU_MONO].FontAtlas.TextureID);

            glBindVertexArray(RenderData->GameUIVAOID);
            glDrawElements(GL_TRIANGLES, 
                        RenderData->DrawFrame.TransparentUIElementCount * 6, 
                        GL_UNSIGNED_INT, 
                        (void *)UIElementOffset); 
        }
    }
    
    if(RenderData->DrawFrame.OpaqueUIElementCount > 0)
    {
        // OPAQUE UI RENDERING PASS
        {
            glDisable(GL_DEPTH_TEST);

            glBindBuffer(GL_ARRAY_BUFFER, RenderData->GameUIVBOID);
            glBufferSubData(GL_ARRAY_BUFFER, 
                            0, 
                            (RenderData->DrawFrame.OpaqueUIElementCount * 4) * sizeof(vertex), 
                            RenderData->DrawFrame.UIVertices);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, RenderData->GameAtlas.TextureID);

            glUniformMatrix4fv(RenderData->ProjectionMatrixUID, 1, GL_FALSE, &RenderData->GameUICamera.ProjectionMatrix.Elements[0][0]);
            glUniformMatrix4fv(RenderData->ViewMatrixUID, 1, GL_FALSE, &RenderData->GameUICamera.ViewMatrix.Elements[0][0]);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, RenderData->LoadedFonts[UBUNTU_MONO].FontAtlas.TextureID);

            glBindVertexArray(RenderData->GameUIVAOID);
            glDrawElements(GL_TRIANGLES, 
                        RenderData->DrawFrame.OpaqueUIElementCount * 6, 
                        GL_UNSIGNED_INT, 
                        0); 
        }
    }

    CloverResetRendererState(RenderData);
}

internal void
CloverRenderTestGBuffer(gl_render_data *RenderData)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    // GEOMETRY PASS
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, RenderData->gBufferFBOID);
        glViewport(0, 0, SizeData.Width, SizeData.Height);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        // OPAQUE GAME OBJECT RENDERING PASS
        glUseProgram(RenderData->gBufferShader.ShaderID);
        if(RenderData->DrawFrame.OpaqueQuadCount > 0)
        {
            {
                glEnable(GL_DEPTH_TEST);
                glBindBuffer(GL_ARRAY_BUFFER, RenderData->GameVBOID);
                glBufferSubData(GL_ARRAY_BUFFER, 
                                0, 
                                (RenderData->DrawFrame.OpaqueQuadCount * 4) * sizeof(vertex), 
                                RenderData->DrawFrame.Vertices);

                glUniformMatrix4fv(RenderData->gBufferProjectionMatrixUID, 1, GL_FALSE, &RenderData->GameCamera.ProjectionMatrix.Elements[0][0]);
                glUniformMatrix4fv(RenderData->gBufferViewMatrixUID, 1, GL_FALSE, &RenderData->GameCamera.ViewMatrix.Elements[0][0]);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, RenderData->GameAtlas.TextureID);

                glBindVertexArray(RenderData->GameVAOID);
                glDrawElements(GL_TRIANGLES, 
                            RenderData->DrawFrame.OpaqueQuadCount * 6, 
                            GL_UNSIGNED_INT, 
                            0); 
            }
        }
    }

    GLenum Error = glGetError();
    if(Error != GL_NO_ERROR)
    {
        printm("[OPENGL ERROR]: %d", Error);
    }

    // LIGHTING PASS
    {
        /* glUseProgram(RenderData->gBufferShader.ShaderID); */

        /* glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, RenderData->PointLightSBOID); */
        /* glBufferSubData(GL_SHADER_STORAGE_BUFFER,  0, sizeof(struct point_light) * RenderData->DrawFrame.PointLightCount, RenderData->DrawFrame.PointLights); */
    }
}
