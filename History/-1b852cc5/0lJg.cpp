#include "Sugar_Intrinsics.h"
#include "Sugar.h"
#include "SugarAPI.h"
#include "win32_Sugar.h"

// TODO: Should the GameUpdateAndRender function ACTUALLY do the rendering? Should we be treating the OpenGL
// Renderer in the same way we are treating the platform? Where instead of the GAME Rendering the items. We call
// out to the renderer with information about what needs to be rendered.

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) 
{
    GameRenderData = GameRenderDataIn;
    for(int i = 0; i < 10; ++i) 
    {
        for(int j = 0; j < 10; ++j) 
        {
            DrawSprite(SPRITE_DICE, {i * 100.0f, j * 100.0f}, {100.0f, 100.0f});
        }
    }
}