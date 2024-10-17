#include "Intrinsics.h"

// UTILS
#include "util/Math.h"
#include "util/Array.h"
#include "util/FileIO.h"
#include "util/CustomStrings.h"
#include "util/Pairs.h"
#include "util/Sorting.h"

// CLOVER HEADERS
#include "Clover.h"
#include "Clover_Globals.h"
#include "Clover_Audio.h"
#include "Clover_Input.h" 
#include "Clover_Renderer.h"
#include "shader/CommonShader.glh"

// IMGUI IMPl
#include "../data/deps/ImGui/imgui.h"
#include "../data/deps/ImGui/imgui_impl_win32.h"
#include "../data/deps/ImGUI/imgui_impl_opengl3.h"

// NOTE(Sleepster): If MiniAudio starts acting weird, I compiled it in a non debug mode.
#include "Clover_Audio.cpp"
#include "Clover_Draw.cpp"
#include "Clover_UI.cpp"


global_variable entity *Player = {};


// NOTE(Sleepster): Random Number Generation stuff
#define RAND_MAX_64 0xFFFFFFFFFFFFFFFFull
#define MULTIPLIER 6364136223846793005ull
#define INCREMENT 1442695040888963407ull

global_variable uint64 RDTSCRandomSeed = __rdtsc();

internal inline uint64
PeekRandom()
{
    RDTSCRandomSeed = RDTSCRandomSeed * MULTIPLIER + INCREMENT;
    return(RDTSCRandomSeed);
}

internal inline uint64 
GetRandom(void)
{
    uint64 RandomSeed = PeekRandom();
    return(RandomSeed);
}

internal inline real32
GetRandomReal32(void)
{
    return((real32)GetRandom() / (real32)UINT64_MAX);
}

internal inline real32
GetRandomReal32_Range(real32 Minimum, real32 Maximum)
{
    return((Maximum - Minimum)*GetRandomReal32() + Minimum);
}

internal inline void
LoadSpriteData(game_state *State)
{
    State->GameData.Sprites[SPRITE_Nil]                   = {.AtlasOffset = {  0,  0}, .SpriteSize = {16, 16}};
    State->GameData.Sprites[SPRITE_Player]                = {.AtlasOffset = { 17,  0}, .SpriteSize = {12, 11}};
    State->GameData.Sprites[SPRITE_UIItemBox]             = {.AtlasOffset = {112,  0}, .SpriteSize = {16, 16}};
    State->GameData.Sprites[SPRITE_ToolPickaxe]           = {.AtlasOffset = {  0, 48}, .SpriteSize = {11, 13}};
    State->GameData.Sprites[SPRITE_ToolWoodAxe]           = {.AtlasOffset = { 16, 48}, .SpriteSize = {11, 13}};
    State->GameData.Sprites[SPRITE_FullHeartContainer]    = {.AtlasOffset = {  0, 64}, .SpriteSize = {11,  9}};
    State->GameData.Sprites[SPRITE_HalfHeartContainer]    = {.AtlasOffset = { 16, 64}, .SpriteSize = {11,  9}};
    State->GameData.Sprites[SPRITE_EmptyHeartContainer]   = {.AtlasOffset = { 32, 64}, .SpriteSize = {11,  9}};
    State->GameData.Sprites[SPRITE_TestEnemyUnit]         = {.AtlasOffset = { 16, 16}, .SpriteSize = {13, 11}};
    State->GameData.Sprites[SPRITE_SelectionBox]          = {.AtlasOffset = {112, 16}, .SpriteSize = {16, 16}};
    State->GameData.Sprites[SPRITE_Workbench]             = {.AtlasOffset = { 16, 80}, .SpriteSize = {16, 16}};
    State->GameData.Sprites[SPRITE_Furnace]               = {.AtlasOffset = {  0, 80}, .SpriteSize = {16, 16}};
    State->GameData.Sprites[SPRITE_Outline]               = {.AtlasOffset = {128,  0}, .SpriteSize = {16, 16}};

    State->GameData.Sprites[SPRITE_Rock]                  = {.AtlasOffset = { 48, 35}, .SpriteSize = {16, 13}};
    State->GameData.Sprites[SPRITE_Pebbles]               = {.AtlasOffset = { 48, 48}, .SpriteSize = {13, 12}};
    State->GameData.Sprites[SPRITE_Tree00]                = {.AtlasOffset = { 96, 31}, .SpriteSize = {16, 17}};
    State->GameData.Sprites[SPRITE_Branches]              = {.AtlasOffset = { 96, 48}, .SpriteSize = {12, 12}};
    State->GameData.Sprites[SPRITE_Tree01]                = {.AtlasOffset = {130, 27}, .SpriteSize = {18, 21}};
    State->GameData.Sprites[SPRITE_Trunk]                 = {.AtlasOffset = {128, 48}, .SpriteSize = {11, 11}};
    State->GameData.Sprites[SPRITE_RubyOre]               = {.AtlasOffset = { 64, 35}, .SpriteSize = {16, 13}};
    State->GameData.Sprites[SPRITE_RubyChunk]             = {.AtlasOffset = { 64, 48}, .SpriteSize = {13, 12}};
    State->GameData.Sprites[SPRITE_SapphireOre]           = {.AtlasOffset = { 80, 35}, .SpriteSize = {16, 13}};
    State->GameData.Sprites[SPRITE_SapphireChunk]         = {.AtlasOffset = { 80, 48}, .SpriteSize = {16, 13}};
}

internal inline void
LoadItemData(game_state *State)
{
    State->GameData.GameItems[ITEM_Nil] = {};
    State->GameData.GameItems[ITEM_Pebbles] = 
    {
        .Archetype = ARCH_Pebbles,      
        .Flags = IS_VALID|IS_ITEM|IS_IN_INVENTORY, 
        .Sprite = SPRITE_Pebbles,       
        .ItemID = ITEM_Pebbles,          
        .MaxStackCount = 64, 
        .ItemName = STR("Pebbles"),         
        .ItemDesc = STR("These are some pebbles!"),
        .CraftingFormula = 
        {
            {ITEM_SapphireOreChunk, 1}, 
        },
        .UniqueMaterialCount = 1,
        .FormulaResultCount  = 3,
        .Craftable = true,
    };
    
    State->GameData.GameItems[ITEM_Branches] = 
    {
        .Archetype = ARCH_Branches,     
        .Flags = IS_VALID|IS_ITEM|IS_IN_INVENTORY, 
        .Sprite = SPRITE_Branches,      
        .ItemID = ITEM_Branches,         
        .MaxStackCount = 64,
        .ItemName = STR("Branches"),        
        .ItemDesc = STR("These are some branches!"),
        .CraftingFormula = 
        {
            {ITEM_Trunk, 1},
        },
        .UniqueMaterialCount = 1,
        .FormulaResultCount  = 3,
        .Craftable = true,
    };
    
    State->GameData.GameItems[ITEM_Trunk] = 
    {
        .Archetype = ARCH_Trunk,      
        .Flags = IS_VALID|IS_ITEM|IS_IN_INVENTORY, 
        .Sprite = SPRITE_Trunk,         
        .ItemID = ITEM_Trunk,            
        .MaxStackCount = 64, 
        .ItemName = STR("Pine Logs"),       
        .ItemDesc = STR("This is a bundle of logs!"),
        .CraftingFormula = 
        {
            {ITEM_Branches, 3}
        },
        .UniqueMaterialCount = 1,
        .FormulaResultCount  = 1,
        .Craftable = true,
    };
    
    State->GameData.GameItems[ITEM_RubyOreChunk] = 
    {
        .Archetype = ARCH_RubyOreChunk,
        .Flags = IS_VALID|IS_ITEM|IS_IN_INVENTORY, 
        .Sprite = SPRITE_RubyChunk,     
        .ItemID = ITEM_RubyOreChunk,     
        .MaxStackCount = 64, 
        .ItemName = STR("Ruby Chunks"),     
        .ItemDesc = STR("These are chunks of Ruby Rock!"),
        .Craftable = false,
    };
    
    State->GameData.GameItems[ITEM_SapphireOreChunk] = 
    {
        .Archetype = ARCH_SapphireOreChunk,
        .Flags = IS_VALID|IS_ITEM|IS_IN_INVENTORY, 
        .Sprite = SPRITE_SapphireChunk, 
        .ItemID = ITEM_SapphireOreChunk, 
        .MaxStackCount = 64, 
        .ItemName = STR("Sapphire Chunks"), 
        .ItemDesc = STR("These are some chunks of Sapphire Rock!"),
        .Craftable = false,
    };
    
    State->GameData.GameItems[ITEM_ToolPickaxe] = 
    {
        .Archetype = ARCH_SimplePickaxe,      
        .Flags = IS_VALID|IS_ITEM|IS_IN_INVENTORY, 
        .Sprite = SPRITE_ToolPickaxe,   
        .ItemID = ITEM_ToolPickaxe,      
        .MaxStackCount = 1,  
        .ItemName = STR("Simple Pickaxe"),  
        .ItemDesc = STR("This a pickaxe, It can be used to mine ores!"),
        .CraftingFormula = 
        {
            {ITEM_Branches, 1},
            {ITEM_Pebbles, 2},
        },
        .UniqueMaterialCount = 2,
        .FormulaResultCount  = 1,
        .Craftable = true,
    };
    
    State->GameData.GameItems[ITEM_ToolWoodAxe] = 
    {
        .Archetype = ARCH_SimpleWoodAxe,
        .Flags = IS_VALID|IS_ITEM|IS_IN_INVENTORY, 
        .Sprite = SPRITE_ToolWoodAxe,   
        .ItemID = ITEM_ToolWoodAxe,      
        .MaxStackCount = 1,  
        .ItemName = STR("Simple Wood Axe"), 
        .ItemDesc = STR("This is a wood axe, It can be used to cut trees!"),
        .CraftingFormula = 
        {
            {ITEM_Branches, 1},
            {ITEM_Pebbles, 2},
        },
        .UniqueMaterialCount = 2,
        .FormulaResultCount  = 1,
        .Craftable = true,
    };
    
    State->GameData.GameItems[ITEM_Workbench] = 
    {
        .Archetype = ARCH_Workbench,  
        .Flags = IS_VALID|IS_BUILDABLE,            
        .Sprite = SPRITE_Workbench,   
        .ItemID = ITEM_Workbench,      
        .MaxStackCount = 1,  
        .ItemName = STR("Workbench"), 
        .ItemDesc = STR("This is a Workbench, it is used for crafting!"),
        .CraftingFormula = 
        {
            {ITEM_Trunk, 2},
            {ITEM_Branches, 3},
            {ITEM_Pebbles, 2},
        },
        .UniqueMaterialCount = 3,
        .FormulaResultCount  = 1,
        .Craftable = true,
    };
    
    State->GameData.GameItems[ITEM_Furnace] = 
    {
        .Archetype = ARCH_Furnace,
        .Flags = IS_VALID|IS_BUILDABLE,            
        .Sprite = SPRITE_Furnace,   
        .ItemID = ITEM_Furnace,      
        .MaxStackCount = 1,  
        .ItemName = STR("Furnace"), 
        .ItemDesc = STR("This is a Furnace, It can be used to smelt ores!"),
        .CraftingFormula = 
        {
            {ITEM_SapphireOreChunk, 3},
            {ITEM_Pebbles, 6},
        },
        .UniqueMaterialCount = 2,
        .FormulaResultCount  = 1,
        .Craftable = true,
    };
    
    State->GameData.ItemSprites[ITEM_Pebbles]          = MakePair(ITEM_Pebbles,          SPRITE_Pebbles);
    State->GameData.ItemSprites[ITEM_Branches]         = MakePair(ITEM_Branches,         SPRITE_Branches);
    State->GameData.ItemSprites[ITEM_Trunk]            = MakePair(ITEM_Trunk,            SPRITE_Trunk);
    State->GameData.ItemSprites[ITEM_SapphireOreChunk] = MakePair(ITEM_SapphireOreChunk, SPRITE_SapphireChunk);
    State->GameData.ItemSprites[ITEM_RubyOreChunk]     = MakePair(ITEM_RubyOreChunk,     SPRITE_RubyChunk);
    State->GameData.ItemSprites[ITEM_ToolPickaxe]      = MakePair(ITEM_ToolPickaxe,      SPRITE_ToolPickaxe);
    State->GameData.ItemSprites[ITEM_ToolWoodAxe]      = MakePair(ITEM_ToolWoodAxe,      SPRITE_ToolWoodAxe);
    State->GameData.ItemSprites[ITEM_Workbench]        = MakePair(ITEM_Workbench,        SPRITE_Workbench);
    State->GameData.ItemSprites[ITEM_Furnace]          = MakePair(ITEM_Furnace,          SPRITE_Furnace);
}

internal entity *
CreateEntity(game_state *State)
{
    entity *Result = {};
    
    for(uint32 EntityIndex = 1;
        EntityIndex < MAX_ENTITIES;
        ++EntityIndex)
    {
        entity *Found = &State->World.Entities[EntityIndex]; 
        if(!(Found->Flags & IS_VALID))
        {
            Result = Found;
            Result->EntityID = EntityIndex;
            break;
        }
    }
    Assert(Result);
    
    ++State->World.EntityCounter;
    Result->Flags = IS_VALID;
    return(Result);
}

internal inline void
DeleteEntity(entity *Entity)
{
    memset(Entity, 0, sizeof(struct entity));
}

internal void
HandleInput(game_state *State, entity *PlayerIn, time Time)
{
    // NOTE(Sleepster): Player Position 
    vec2 InputAxis = {};
    if(IsGameKeyDown(MOVE_UP, &State->GameInput))
    {
        InputAxis.Y += 1.0f;
    }
    else if(IsGameKeyDown(MOVE_DOWN, &State->GameInput))
    {
        InputAxis.Y -= 1.0f;
    }
    
    if(IsGameKeyDown(MOVE_LEFT, &State->GameInput))
    {
        InputAxis.X -= 1.0f;
    }
    else if(IsGameKeyDown(MOVE_RIGHT, &State->GameInput))
    {
        InputAxis.X += 1.0f;
    }
    
    vec2 OldPlayerP = PlayerIn->Position;
    
    vec2 NextPos = {PlayerIn->Position.X + (PlayerIn->Position.X - OldPlayerP.X) + (PlayerIn->Speed * InputAxis.X) * (Time.Delta),
        PlayerIn->Position.Y + (PlayerIn->Position.Y - OldPlayerP.Y) + (PlayerIn->Speed * InputAxis.Y) * (Time.Delta)};
    PlayerIn->Position = v2Lerp(NextPos, Time.Delta, OldPlayerP);
    
    // NOTE(Sleepster): Game Update Stuff 

    if(IsKeyPressed(KEY_ESCAPE, &State->GameInput))
    {
        Player->Inventory.SelectedInventoryItem = {};
    }
    if(IsGameKeyPressed(INVENTORY, &State->GameInput))
    {
        State->DisplayPlayerInventory = !State->DisplayPlayerInventory;
    }
    if(IsGameKeyPressed(SHOW_HOTBAR, &State->GameInput))
    {
        State->DisplayPlayerHotbar = !State->DisplayPlayerHotbar;
    }
    if(IsGameKeyPressed(BUILD_MENU, &State->GameInput))
    {
        if(State->GameUIState == UI_State_Building)
        {
            State->GameUIState = UI_State_Nil;
            return;
        }
        State->GameUIState = UI_State_Building;
    }
    if(IsGameKeyPressed(HOTBAR_01, &State->GameInput))
    {
        if(Player->Inventory.CurrentInventorySlot == 0) 
        {
            Player->Inventory.CurrentInventorySlot = NULLSLOT;
            return;
        }
        Player->Inventory.CurrentInventorySlot = 0;
    }
    if(IsGameKeyPressed(HOTBAR_02, &State->GameInput))
    {
        if(Player->Inventory.CurrentInventorySlot == 1) 
        {
            Player->Inventory.CurrentInventorySlot = NULLSLOT;
            return;
        }
        Player->Inventory.CurrentInventorySlot = 1;
    }
    if(IsGameKeyPressed(HOTBAR_03, &State->GameInput))
    {
        if(Player->Inventory.CurrentInventorySlot == 2) 
        {
            Player->Inventory.CurrentInventorySlot = NULLSLOT;
            return;
        }
        Player->Inventory.CurrentInventorySlot = 2;
    }
    if(IsGameKeyPressed(HOTBAR_04, &State->GameInput))
    {
        if(Player->Inventory.CurrentInventorySlot == 3) 
        {
            Player->Inventory.CurrentInventorySlot = NULLSLOT;
            return;
        }
        Player->Inventory.CurrentInventorySlot = 3;
    }
    if(IsGameKeyPressed(HOTBAR_05, &State->GameInput))
    {
        if(Player->Inventory.CurrentInventorySlot == 4) 
        {
            Player->Inventory.CurrentInventorySlot = NULLSLOT;
            return;
        }
        Player->Inventory.CurrentInventorySlot = 4;
    }
    if(IsGameKeyPressed(HOTBAR_06, &State->GameInput))
    {
        if(Player->Inventory.CurrentInventorySlot == 5) 
        {
            Player->Inventory.CurrentInventorySlot = NULLSLOT;
            return;
        }
        Player->Inventory.CurrentInventorySlot = 5;
    }
    if(IsGameKeyPressed(HOTBAR_07, &State->GameInput))
    {
        if(Player->Inventory.CurrentInventorySlot == 6) 
        {
            Player->Inventory.CurrentInventorySlot = NULLSLOT;
            return;
        }
        Player->Inventory.CurrentInventorySlot = 6;
    }
    
    if(IsKeyPressed(KEY_HOME, &State->GameInput))
    {
        State->DrawDebug = !State->DrawDebug;
    }
}

internal void
SetupPlayer(game_state *State, entity *Entity)
{
    Entity->Archetype   = ARCH_Player;
    Entity->Sprite      = SPRITE_Player; 
    Entity->Flags      += IS_ACTIVE|IS_ACTOR;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_Player].SpriteSize); 
    Entity->Health      = PlayerHealth;
    Entity->Position    = {};
    Entity->Rotation    = 0;
    Entity->Speed       = 100.0f;              // PIXELS PER SECOND
    Entity->BoxCollider = {};
    Entity->DroppedFromInventoryItemID  = ITEM_Nil;            // DROPS
}

internal void
SetupRock(game_state *State, entity *Entity)
{
    Entity->Archetype   = ARCH_Rock;
    Entity->Sprite      = SPRITE_Rock; 
    Entity->Flags      += IS_ACTIVE|IS_SOLID|IS_DESTRUCTABLE;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_Rock].SpriteSize);
    Entity->Health      = RockHealth;
    Entity->Position    = {};
    Entity->Rotation    = 0;
    Entity->Speed       = 1.0f;
    Entity->BoxCollider = {};

    Entity->UniqueDropCount = 1;
    Entity->EntityDrops[0] = 
    {
        entity_item_drop{ITEM_Pebbles, 1},
    };
}

internal void
SetupTree00(game_state *State, entity *Entity)
{
    Entity->Archetype   = ARCH_Tree00;
    Entity->Sprite      = SPRITE_Tree00; 
    Entity->Flags      += IS_ACTIVE|IS_SOLID|IS_DESTRUCTABLE;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_Tree00].SpriteSize);
    Entity->Health      = TreeHealth;
    Entity->Position    = {};
    Entity->Rotation    = 0;
    Entity->Speed       = 1.0f;
    Entity->BoxCollider = {};

    Entity->UniqueDropCount = 1;
    Entity->EntityDrops[0] = 
    {
        entity_item_drop{ITEM_Branches, 1},
    };
}

internal void
SetupTree01(game_state *State, entity *Entity)
{
    Entity->Archetype   = ARCH_Tree01;
    Entity->Sprite      = SPRITE_Tree01; 
    Entity->Flags      += IS_ACTIVE|IS_SOLID|IS_DESTRUCTABLE;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_Tree01].SpriteSize);
    Entity->Health      = TreeHealth;
    Entity->Position    = {};
    Entity->Rotation    = 0;
    Entity->Speed       = 1.0f;
    Entity->BoxCollider = {};

    Entity->UniqueDropCount = 1;
    Entity->EntityDrops[0] = 
    {
        entity_item_drop{ITEM_Trunk, 1},
    };
}

internal void
SetupRubyNode(game_state *State, entity *Entity)
{
    Entity->Archetype   = ARCH_RubyNode;
    Entity->Sprite      = SPRITE_RubyOre; 
    Entity->Flags      += IS_ACTIVE|IS_SOLID|IS_DESTRUCTABLE;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_RubyOre].SpriteSize);
    Entity->Health      = NodeHealth;
    Entity->Position    = {};
    Entity->Rotation    = 0;
    Entity->Speed       = 1.0f;
    Entity->BoxCollider = {};

    Entity->UniqueDropCount = 1;
    Entity->EntityDrops[0] = 
    {
        entity_item_drop{ITEM_RubyOreChunk, 1},
    };
}

internal void
SetupSapphireNode(game_state *State, entity *Entity)
{
    Entity->Archetype   = ARCH_SapphireNode;
    Entity->Sprite      = SPRITE_SapphireOre; 
    Entity->Flags      += IS_ACTIVE|IS_SOLID|IS_DESTRUCTABLE;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_SapphireOre].SpriteSize);
    Entity->Health      = NodeHealth;
    Entity->Position    = {};
    Entity->Rotation    = 0;
    Entity->Speed       = 1.0f;
    Entity->BoxCollider = {};
    
    Entity->UniqueDropCount = 1;
    Entity->EntityDrops[0] = 
    {
        entity_item_drop{ITEM_SapphireOreChunk, 1},
    };
}

internal void
SetupItemPebbles(game_state *State, entity *Entity)
{
    Entity->Archetype = ARCH_Pebbles;
    Entity->Sprite    = SPRITE_Pebbles;
    Entity->Flags    += IS_ACTIVE|IS_ITEM|CAN_BE_PICKED_UP;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_Pebbles].SpriteSize) * 0.8f;
    Entity->DroppedFromInventoryItemID    = ITEM_Pebbles;
}

internal void
SetupItemBranches(game_state *State, entity *Entity)
{
    Entity->Archetype = ARCH_Branches;
    Entity->Sprite    = SPRITE_Branches;
    Entity->Flags    += IS_ACTIVE|IS_ITEM|CAN_BE_PICKED_UP;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_Branches].SpriteSize) * 0.8f;
    Entity->DroppedFromInventoryItemID    = ITEM_Branches;
}

internal void
SetupItemTrunk(game_state *State, entity *Entity)
{
    Entity->Archetype = ARCH_Trunk;
    Entity->Sprite    = SPRITE_Trunk;
    Entity->Flags    += IS_ACTIVE|IS_ITEM|CAN_BE_PICKED_UP;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_Trunk].SpriteSize) * 0.8f;
    Entity->DroppedFromInventoryItemID    = ITEM_Trunk;
}

internal void
SetupItemRubyChunk(game_state *State, entity *Entity)
{
    Entity->Archetype = ARCH_RubyOreChunk;
    Entity->Sprite    = SPRITE_RubyChunk;
    Entity->Flags    += IS_ACTIVE|IS_ITEM|CAN_BE_PICKED_UP;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_RubyChunk].SpriteSize) * 0.8f;
    Entity->DroppedFromInventoryItemID    = ITEM_RubyOreChunk;
}

internal void
SetupItemSapphireChunk(game_state *State, entity *Entity)
{
    Entity->Archetype = ARCH_SapphireOreChunk;
    Entity->Sprite    = SPRITE_SapphireChunk;
    Entity->Flags    += IS_ACTIVE|IS_ITEM|CAN_BE_PICKED_UP;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_SapphireChunk].SpriteSize) * 0.8f;
    Entity->DroppedFromInventoryItemID    = ITEM_SapphireOreChunk;
}

internal void
SetupItemToolPickaxe(game_state *State, entity *Entity)
{
    Entity->Archetype = ARCH_SimplePickaxe;
    Entity->Sprite    = SPRITE_ToolPickaxe;
    Entity->Flags    += IS_ACTIVE|IS_ITEM|CAN_BE_PICKED_UP;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_ToolPickaxe].SpriteSize);
    Entity->DroppedFromInventoryItemID    = ITEM_ToolPickaxe;
}

internal void
SetupItemToolWoodAxe(game_state *State, entity *Entity)
{
    Entity->Archetype = ARCH_SimpleWoodAxe;
    Entity->Sprite    = SPRITE_ToolWoodAxe;
    Entity->Flags    += IS_ACTIVE|IS_ITEM|CAN_BE_PICKED_UP;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_ToolWoodAxe].SpriteSize);
    Entity->DroppedFromInventoryItemID    = ITEM_ToolWoodAxe;
}

internal void
SetupItemWorkbench(game_state *State, entity *Entity)
{
    Entity->Archetype = ARCH_Workbench;
    Entity->Sprite    = SPRITE_Workbench;
    Entity->Flags    += IS_ACTIVE|IS_ITEM|CAN_BE_PICKED_UP|IS_BUILDABLE;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_Workbench].SpriteSize) * 0.5f;
    Entity->DroppedFromInventoryItemID    = ITEM_Workbench;
}

internal void
SetupItemFurnace(game_state *State, entity *Entity)
{
    Entity->Archetype = ARCH_Furnace,
    Entity->Sprite    = SPRITE_Furnace;
    Entity->Flags    += IS_ACTIVE|IS_ITEM|CAN_BE_PICKED_UP|IS_BUILDABLE;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_Furnace].SpriteSize) * 0.5f;
    Entity->DroppedFromInventoryItemID    = ITEM_Furnace;
}

internal void
SetupBuildingWorkbench(game_state *State, entity *Entity)
{
    Entity->Archetype = ARCH_Workbench;
    Entity->Sprite    = SPRITE_Workbench;
    Entity->Flags    += IS_ACTIVE|IS_BUILDABLE|IS_PLACED|IS_DESTRUCTABLE;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_Workbench].SpriteSize);
    Entity->Health      = NodeHealth;
    Entity->Rotation    = 0;
    Entity->Speed       = 1.0f;
    Entity->BoxCollider = {};
    
    Entity->UniqueDropCount = 1;
    Entity->EntityDrops[0] = 
    {
        entity_item_drop{ITEM_Workbench, 1},
    };
}

internal void
SetupBuildingFurnace(game_state *State, entity *Entity)
{
    Entity->Archetype = ARCH_Workbench;
    Entity->Sprite    = SPRITE_Furnace;
    Entity->Flags    += IS_ACTIVE|IS_BUILDABLE|IS_PLACED|IS_DESTRUCTABLE;
    Entity->Size        = v2Cast(State->GameData.Sprites[SPRITE_Furnace].SpriteSize);
    
    Entity->Health      = NodeHealth;
    Entity->Rotation    = 0;
    Entity->Speed       = 1.0f;
    Entity->BoxCollider = {};
    
    Entity->UniqueDropCount = 1;
    Entity->EntityDrops[0] = 
    {
        entity_item_drop{ITEM_Furnace, 1},
    };
}

internal inline void
ResetGame(gl_render_data *RenderData, game_state *State, game_memory *Memory)
{
    for(uint32 i = 0; i < MAX_ENTITIES; i++)
    {
        entity *Temp = &State->World.Entities[i];
        DeleteEntity(Temp);
    }
    
    for(uint32 i = 0; i < SPRITE_Count; i++)
    {
        State->GameData.Sprites[i] = {};
    }
    
    State->DisplayPlayerHotbar = true;
    State->DisplayPlayerInventory = false;
    State->DisplayCraftingMenu = false;
    State->ActiveCraftingStation = {};
    State->ActiveRecipe = {};
}

internal int32
WorldToTilei32(real32 WorldPosition)
{
    return(int32(floorf(WorldPosition / (real32)TILE_SIZE)));
}

internal real32
TileToWorldr32(int32 WorldPosition)
{
    return((real32)WorldPosition * (real32)TILE_SIZE);
}

internal ivec2 
WorldToTilePos(vec2 WorldPosition)
{
    ivec2 Result = {};
    
    Result.X = WorldToTilei32(WorldPosition.X);
    Result.Y = WorldToTilei32(WorldPosition.Y);
    
    return(Result);
}

internal vec2
TileToWorldPos(ivec2 TilePosition)
{
    vec2 Result = {};
    
    Result.X = TileToWorldr32(TilePosition.X);
    Result.Y = TileToWorldr32(TilePosition.Y);
    
    return(Result);
}

internal vec2
RoundToTile(vec2 WorldPosition)
{
    vec2 NewWorldPosition = TileToWorldPos(WorldToTilePos(WorldPosition));
    return(NewWorldPosition);
}

internal inline real32
SinBreatheNormalized(real32 Time, real32 Modifier, real32 Min, real32 Max)
{
    real32 SineValue = (sinf(Modifier * 2 * PI32 * Time) + 1.0f) / 2.0f;
    return(Min + (Max - Min) * SineValue);
}

internal inline real32
SinBreathe(real32 Time, real32 Modifier)
{
    return(sinf(Time * Modifier));
}

inline int
CompareEntityYAxis(const void *A, const void *B)
{
    const entity *EntityA = (const entity*)A;
    const entity *EntityB = (const entity*)B;
    
    return((EntityA->Position.Y > EntityB->Position.Y) ?  1 :
           (EntityA->Position.Y < EntityB->Position.Y) ? -1 : 0);
}

internal bool
SwapInventoryItems(entity_item_inventory *Inventory, item *ItemA, item *ItemB)
{   
    item TempItem = *ItemA;
    Inventory->Items[ItemA->OccupiedInventorySlot] = *ItemB;
    Inventory->Items[ItemB->OccupiedInventorySlot] = TempItem;
    
    return(true);
}

internal void
SetupDroppedEntity(gl_render_data *RenderData, game_state *State, item *SelectionItem, entity *SpawnedItem)
{
    switch(SelectionItem->Archetype)
    {
        case ARCH_Pebbles:
        {
            SetupItemPebbles(State, SpawnedItem);
        }break;
        case ARCH_Trunk:
        {
            SetupItemTrunk(State, SpawnedItem);
        }break;
        case ARCH_Branches:
        {
            SetupItemBranches(State, SpawnedItem);
        }break;
        case ARCH_RubyOreChunk:
        {
            SetupItemRubyChunk(State, SpawnedItem);
        }break;
        case ARCH_SapphireOreChunk:
        {
            SetupItemSapphireChunk(State, SpawnedItem);
        }break;
        case ARCH_SimplePickaxe:
        {
            SetupItemToolPickaxe(State, SpawnedItem);
        }break;
        case ARCH_SimpleWoodAxe:
        {
            SetupItemToolWoodAxe(State, SpawnedItem);
        }break;
        case ARCH_Workbench:
        {
            SetupItemWorkbench(State, SpawnedItem);
        }break;
        case ARCH_Furnace:
        {
            SetupItemFurnace(State, SpawnedItem);
        }break;
    }
    
    SpawnedItem->Flags -= CAN_BE_PICKED_UP;
    SpawnedItem->DroppedFromInventoryItemCount = SelectionItem->CurrentStack;
    
    vec2 WorldMouseCoords = TransformMouseCoords(RenderData->GameCamera.ViewMatrix, RenderData->GameCamera.ProjectionMatrix, State->GameInput.Keyboard.CurrentMouse, SizeData);
    real32 Distance = fabsf(v2Distance(Player->Position, WorldMouseCoords));
    vec2 Direction = v2Normalize(WorldMouseCoords - Player->Position);
    
    SpawnedItem->Position = Player->Position;
    if(Distance <= MaxDropDistance)
    {
        vec2 DropPosition = Player->Position + Direction * Distance;
        SpawnedItem->Target = DropPosition;
    }
    else
    {
        vec2 MaxDropPosition = Player->Position + Direction * MaxDropDistance;
        SpawnedItem->Target = MaxDropPosition;
    }
}

internal real32
r32Clamp(real32 Value, real32 Min, real32 Max)
{
    if (Value <= Min) return Min;
    if (Value >= Max) return Max;
    return Value;
}

internal vec2 
v2Clamp(vec2 Value, vec2 Min, vec2 Max)
{
    Value.X = r32Clamp(Value.X, Min.X, Max.X);
    Value.Y = r32Clamp(Value.Y, Min.Y, Max.Y);
    return(Value);
}

internal void
ResetItemSlotState(item *Item)
{
    Item->Sprite = {};
    Item->CurrentStack = 0;
    Item->ItemName = {};
    Item->ItemDesc = {};
    Item->ItemID   = {};
    Item->MaxStackCount = {};
    Item->Flags = {};
}

internal inline sprite_type
GetSpriteFromPair(game_state *State, item_id ID)
{
    for(const auto &ItemData : State->GameData.ItemSprites)
    {
        if(ItemData.First == ID)
        {
            return(ItemData.Second);
        }
    }
    return(SPRITE_Nil);
}

internal inline item_id
GetItemIDFromPair(game_state *State, sprite_type Sprite)
{
    for(const auto &ItemData : State->GameData.ItemSprites)
    {
        if(ItemData.Second == Sprite)
        {
            return(ItemData.First);
        }
    }
}

internal bool
IsItemCraftable(int *ItemCounts, item *Craft)
{
    if(Craft->CraftingFormula)
    {
        for(uint32 FormulaIndex = 0;
            FormulaIndex < Craft->UniqueMaterialCount;
            FormulaIndex++)
        {
            int ItemCount = ItemCounts[FormulaIndex];
            if(ItemCount >= Craft->CraftingFormula[FormulaIndex].RequiredCount)
            {
                continue;
            }
            else
            {
                return(false);
            }
        }
        return(true);
    }
    return(false);
}

internal void
AddItemToPlayerInventory(game_state *State, entity *Player, entity *Temp)
{
    if((Temp->Flags & IS_ITEM) && (Temp->Flags & CAN_BE_PICKED_UP))
    {
        real32 ItemDistance = fabsf(v2Distance(Temp->Position, Player->Position));
        if(ItemDistance <= ItemPickupDist)
        {
            for(int32 InventoryIndex = 0;
                InventoryIndex < TOTAL_INVENTORY_SIZE;
                ++InventoryIndex)
            {
                item NewItem = State->GameData.GameItems[Temp->DroppedFromInventoryItemID];
                if(Player->Inventory.Items[InventoryIndex].ItemID == NewItem.ItemID && 
                   Player->Inventory.Items[InventoryIndex].CurrentStack < 
                   Player->Inventory.Items[InventoryIndex].MaxStackCount)
                {
                    Player->Inventory.Items[InventoryIndex].CurrentStack++;
                    // NOTE(Sleepster): If two matching IDs are found, skip to the deletion 
                    DeleteEntity(Temp);
                    return;
                }
            }
            
            for(int32 InventoryIndex = 0;
                InventoryIndex < TOTAL_INVENTORY_SIZE;
                ++InventoryIndex)
            {
                item NewItem = State->GameData.GameItems[Temp->DroppedFromInventoryItemID];
                if(Player->Inventory.Items[InventoryIndex].ItemID == 0)
                {
                    NewItem.CurrentStack = Temp->DroppedFromInventoryItemCount;
                    if(NewItem.CurrentStack == 0)
                    {
                        NewItem.CurrentStack = 1;
                    }
                    Player->Inventory.Items[InventoryIndex] = NewItem;
                    Player->Inventory.Items[InventoryIndex].OccupiedInventorySlot = InventoryIndex;
                    DeleteEntity(Temp);
                    break;
                }
            }
        }
    }
}

extern
GAME_ON_AWAKE(GameOnAwake)
{
    ResetGame(RenderData, State, Memory);
    LoadSpriteData(State);
    LoadItemData(State);

    // TODO(Sleepster): Write a proper implementation of Mini Audio's low level API so that 
    //                  hotreloading the engine doesn't just crash the program
    
    //PlaySound(&Memory->TemporaryStorage, State, STR("boop.wav"), 1);
    //PlayTrackFromDisk(&Memory->TemporaryStorage, State, STR("Test.mp3"), 0.5f);
    
    real32 SizeScaler = WORLD_SIZE * 10;
    for(uint32 EntityIndex = 0;
        EntityIndex < 50;
        ++EntityIndex)
    {
        entity *En = CreateEntity(State);
        SetupRock(State, En);
        En->Position = vec2{GetRandomReal32_Range(-SizeScaler, SizeScaler), GetRandomReal32_Range(-SizeScaler, SizeScaler)};
        En->Position = TileToWorldPos(WorldToTilePos(En->Position));
        
        
        entity *En2 = CreateEntity(State);
        SetupTree00(State, En2);
        En2->Position = vec2{GetRandomReal32_Range(-SizeScaler, SizeScaler), GetRandomReal32_Range(-SizeScaler, SizeScaler)};
        En2->Position = TileToWorldPos(WorldToTilePos(En2->Position));
        
        
        entity *En3 = CreateEntity(State);
        SetupTree01(State, En3);
        En3->Position = vec2{GetRandomReal32_Range(-SizeScaler, SizeScaler), GetRandomReal32_Range(-SizeScaler, SizeScaler)};
        En3->Position = TileToWorldPos(WorldToTilePos(En3->Position));
        
        
        entity *En4 = CreateEntity(State);
        SetupRubyNode(State, En4);
        En4->Position = vec2{GetRandomReal32_Range(-SizeScaler, SizeScaler), GetRandomReal32_Range(-SizeScaler, SizeScaler)};
        En4->Position = TileToWorldPos(WorldToTilePos(En4->Position));
        
        
        entity *En5 = CreateEntity(State);
        SetupSapphireNode(State, En5);
        En5->Position = vec2{GetRandomReal32_Range(-SizeScaler, SizeScaler), GetRandomReal32_Range(-SizeScaler, SizeScaler)};
        En5->Position = TileToWorldPos(WorldToTilePos(En5->Position));
    }
    
    entity *WorkbenchTest = CreateEntity(State);
    SetupBuildingWorkbench(State, WorkbenchTest);
    WorkbenchTest->Position = {0, -80};
    WorkbenchTest->Position = TileToWorldPos(WorldToTilePos(WorkbenchTest->Position));
    WorkbenchTest->BoxCollider = CreateRange(vec2{WorkbenchTest->Position.X - (TILE_SIZE * 0.5f), WorkbenchTest->Position.Y}, 
                                             vec2{WorkbenchTest->Position.X - (TILE_SIZE * 0.5f), WorkbenchTest->Position.Y} + WorkbenchTest->Size);
    
    entity *FurnaceTest = CreateEntity(State);
    SetupBuildingFurnace(State, FurnaceTest);
    FurnaceTest->Position = {20, -80};
    FurnaceTest->Position = TileToWorldPos(WorldToTilePos(FurnaceTest->Position));
    FurnaceTest->BoxCollider = CreateRange(vec2{FurnaceTest->Position.X - (TILE_SIZE * 0.5f), FurnaceTest->Position.Y}, 
                                           vec2{FurnaceTest->Position.X - (TILE_SIZE * 0.5f), FurnaceTest->Position.Y} + FurnaceTest->Size);
    
    entity *GroundWorkbench = CreateEntity(State);
    SetupItemWorkbench(State, GroundWorkbench);
    GroundWorkbench->Position = {0, -100};
    GroundWorkbench->Target   = {0, -100};
    GroundWorkbench->BoxCollider = CreateRange(vec2{GroundWorkbench->Position.X - (TILE_SIZE * 0.5f), GroundWorkbench->Position.Y}, 
                                               vec2{GroundWorkbench->Position.X - (TILE_SIZE * 0.5f), GroundWorkbench->Position.Y} + GroundWorkbench->Size);

    entity *GroundFurnace = CreateEntity(State);
    SetupItemFurnace(State, GroundFurnace);
    GroundFurnace->Position = {20, -100};
    GroundFurnace->Target   = {20, -100};
    
    entity *Pickaxe = CreateEntity(State);
    SetupItemToolPickaxe(State, Pickaxe);
    Pickaxe->Position = {0, 150};
    Pickaxe->Target  = {0, 150};
    
    
    entity *Pickaxe2 = CreateEntity(State);
    SetupItemToolPickaxe(State, Pickaxe2);
    Pickaxe2->Position = {32, 150};
    Pickaxe2->Target = {32, 150};
    
    
    Player = CreateEntity(State);
    SetupPlayer(State, Player);
    
    State->DisplayPlayerHotbar = true;
}

extern
GAME_UPDATE_AND_DRAW(GameUpdateAndDraw)
{
    DrawImGui(State, RenderData, Time);
    
    State->World.WorldFrame = {};
    
    HandleLoadedSounds(State);
    HandleLoadedTracks(State);
    
    // MATRICES
    {
        // NOTE(Sleepster): GAME 
        RenderData->GameCamera.Zoom = 5.3f;
        
        mat4 ScaleMatrix                              = mat4MakeScale(vec3{1.0f * RenderData->GameCamera.Zoom, 1.0f * RenderData->GameCamera.Zoom, 1.0f});
        mat4 TranslationMatrix                        = mat4Translate(vec3{-RenderData->GameCamera.Position.X, -RenderData->GameCamera.Position.Y, 0.0f});
        
        RenderData->GameCamera.ViewMatrix             = mat4Identity(1.0f);
        RenderData->GameCamera.ViewMatrix             = mat4Multiply(TranslationMatrix, RenderData->GameCamera.ViewMatrix);
        RenderData->GameCamera.ViewMatrix             = mat4Multiply(ScaleMatrix, RenderData->GameCamera.ViewMatrix);
        RenderData->GameCamera.ProjectionMatrix       = mat4RHGLOrtho((real32)SizeData.Width * -0.5f, (real32)SizeData.Width * 0.5f, (real32)SizeData.Height * -0.5f, (real32)SizeData.Height * 0.5f, -1.0f, 1.0f); 
        RenderData->GameCamera.ProjectionViewMatrix   = mat4Multiply(RenderData->GameCamera.ProjectionMatrix, RenderData->GameCamera.ViewMatrix);        
        
        // NOTE(Sleepster): UI
        RenderData->GameUICamera.ProjectionMatrix     = mat4RHGLOrtho((real32)SizeData.Width * -0.5f, (real32)SizeData.Width * 0.5f, (real32)SizeData.Height * -0.5f, (real32)SizeData.Height * 0.5f, -1.0f, 1.0f); 
        RenderData->GameUICamera.ViewMatrix           = mat4Multiply(mat4Identity(1.0f), ScaleMatrix);
        RenderData->GameUICamera.ProjectionViewMatrix = mat4Multiply(RenderData->GameUICamera.ProjectionMatrix, RenderData->GameUICamera.ViewMatrix);
    }
    
    vec2 MouseToWorld  = TransformMouseCoords(RenderData->GameCamera.ViewMatrix, 
                                              RenderData->GameCamera.ProjectionMatrix, 
                                              State->GameInput.Keyboard.CurrentMouse, 
                                              SizeData);
    
    vec2 MouseToScreen = TransformMouseCoords(RenderData->GameUICamera.ViewMatrix,
                                              RenderData->GameUICamera.ProjectionMatrix, 
                                              State->GameInput.Keyboard.CurrentMouse, 
                                              SizeData);
    
    // NOTE(Sleepster): SELECTED ENTITY
    real32 SelectionDistance = 32.0f;
    real32 MinimumDistance = 0;
    for(uint32 EntityIndex = 0;
        EntityIndex <= State->World.EntityCounter;
        ++EntityIndex)
    {
        entity *Temp = &State->World.Entities[EntityIndex];
        if((Temp->Flags & IS_VALID))
        {
            real32 Distance = fabsf(v2Distance(Temp->Position, MouseToWorld));
            real32 PlayerToObjectDistance = fabsf(v2Distance(Temp->Position, Player->Position));
            if(Distance <= SelectionDistance && PlayerToObjectDistance <= MaxHitRange)
            {
                if(!State->World.WorldFrame.SelectedEntity || (Distance < MinimumDistance) || (Temp->EntityID != State->World.WorldFrame.SelectedEntity->EntityID))
                {
                    State->World.WorldFrame.SelectedEntity = Temp;
                    MinimumDistance = Distance;
                }

                // NOTE(Sleepster): Handle Entity Destruction 
                if(IsGameKeyPressed(ATTACK, &State->GameInput) && 
                   (Temp->Flags & IS_DESTRUCTABLE) && 
                   !(Temp->Flags & IS_UI) && 
                   PlayerToObjectDistance <= MaxHitRange &&
                   State->GameUIState == UI_State_Nil)
                {
                    --Temp->Health;
                    if(Temp->Health <= 0)
                    {
                        for(uint32 DropIndex = 0;
                            DropIndex < Temp->UniqueDropCount;
                            DropIndex++)
                        {
                            for(uint32 DropCount = 0;
                                DropCount < Temp->EntityDrops[DropIndex].DropAmount;
                                DropCount++)
                            {
                                entity *CreatedEntity = CreateEntity(State);
                                item DroppedItem = State->GameData.GameItems[Temp->EntityDrops[DropCount].DroppedItem];

                                SetupDroppedEntity(RenderData, State, &DroppedItem, CreatedEntity);
                                CreatedEntity->Position = Temp->Position;
                                CreatedEntity->Target   = Temp->Position;
                            }
                        }

                        //PlaySound(&Memory->TemporaryStorage, State, STR("boop.wav"), 1);
                        State->World.WorldFrame.SelectedEntity = {};
                        DeleteEntity(Temp);
                    }
                }
            }
            // NOTE(Sleepster): Add Item to Inventory
            AddItemToPlayerInventory(State, Player, Temp);
        }
    }
    
    // NOTE(Sleepster): New Hotbar UI 
    if(State->DisplayPlayerHotbar)
    {
        State->UIContext.UICameraViewMatrix       = RenderData->GameUICamera.ViewMatrix;
        State->UIContext.UICameraProjectionMatrix = RenderData->GameUICamera.ProjectionMatrix;
        State->UIContext.GameInput                = &State->GameInput;
        State->UIContext.ActiveFont               = &RenderData->LoadedFonts[UBUNTU_MONO];
        State->UIContext.ActiveFontIndex          = UBUNTU_MONO;
        
        const real32 Width = SizeData.X * 0.25f;
        const real32 Padding = 4.0f;
        const real32 IconSize = 16.0f;
        const real32 TotalWidth = (PLAYER_HOTBAR_COUNT * IconSize) + ((PLAYER_HOTBAR_COUNT - 1) * Padding);
        const real32 StartingX = (Width / 2.0f) - (TotalWidth / 2.0f);
        
        real32 YOffset = -90.0f;
        
        for(uint32 InventorySlot = 0;
            InventorySlot < PLAYER_HOTBAR_COUNT;
            ++InventorySlot)
        {
            real32 SlotOffset = (IconSize + Padding) * InventorySlot;
            
            mat4 XForm = mat4Identity(1.0f);
            XForm = mat4Multiply(XForm, mat4Translate(vec3{StartingX + SlotOffset, YOffset, 0.0}));
            XForm = mat4Multiply(XForm, mat4Translate(vec3{IconSize * -0.55f, 0.0f, 0.0f}));
            XForm = mat4Multiply(XForm, mat4MakeScale(vec3{IconSize, IconSize, 1.0}));
            
            vec4 MatrixPosition = XForm.Columns[3];
            vec2 SlotPosition   = {MatrixPosition.X, MatrixPosition.Y};
            
            static_sprite_data Sprite = GetSprite(State, SPRITE_UIItemBox);
            ui_element_state HotbarSlotState = CloverUIButton(&State->UIContext, STR("HotbarSlot"), SlotPosition, {IconSize, IconSize}, Sprite, WHITE);
            ui_element *HotbarSlot = &State->UIContext.UIElements[HotbarSlotState.UIID.ID];
            
            Player->Inventory.InventorySlotButtons[InventorySlot] = HotbarSlot;
            
            HotbarSlot->XForm = XForm;
            HotbarSlot->Sprite = Sprite;
            HotbarSlot->DrawColor = WHITE;
            
            item *Item = &Player->Inventory.Items[InventorySlot];
            Item->OccupiedInventorySlot = InventorySlot;
            
            Sprite = GetSprite(State, Item->Sprite);
            if(Sprite != State->GameData.Sprites[SPRITE_Nil])
            {
                if(!HotbarSlotState.IsHot)
                {
                    XForm = mat4Multiply(XForm, mat4MakeScale(vec3{0.65, 0.65, 1.0}));
                }
                
                if(InventorySlot == Player->Inventory.CurrentInventorySlot)
                {
                    XForm = mat4Multiply(XForm, mat4MakeScale(vec3{1.2, 1.2, 1.0}));
                }
                DrawUISpriteXForm(RenderData, XForm, Sprite, 0, WHITE);
            }
            
            if(HotbarSlotState.IsHot)
            {
                HotbarSlot->DrawColor = RED;
                if(Item->Sprite != SPRITE_Nil)
                {
                    XForm = mat4Multiply(XForm, mat4MakeScale(vec3{0.8, 0.8, 1.0}));
                    
                    real32 NewUIYOffset;
                    real32 NewUIYDescOffset;
                    real32 SpriteYOffset;
                    if(State->DisplayPlayerInventory)
                    {
                        NewUIYOffset = 24.0f;
                        NewUIYDescOffset = 8.0f;
                        SpriteYOffset = 25.0f;
                    }
                    else
                    {
                        NewUIYOffset = 16.0f;
                        NewUIYDescOffset = 3.0f;
                        SpriteYOffset = 20.0f;
                    }
                    
                    vec2 UIBoxSize = {10, 25};
                    
                    mat4 UIXForm = mat4Identity(1.0f);
                    UIXForm = mat4Multiply(UIXForm, mat4Translate(vec3{StartingX + SlotOffset, YOffset, 0.0}));
                    UIXForm = mat4Multiply(UIXForm, mat4Translate(vec3{IconSize * -0.55f, NewUIYOffset, 0.0f}));
                    
                    // NOTE(Sleepster): String Size to influence the BosSize
                    vec4 UIMatrixPosition = UIXForm.Columns[3];
                    vec2 Position = vec2{UIMatrixPosition.X, UIMatrixPosition.Y};
                    
                    static_sprite_data SpriteData = GetSprite(State, Item->Sprite);
                    vec2 SpriteSize = v2Cast(SpriteData.SpriteSize);
                    
                    ui_element *ItemDescData = CloverUIMakeTextElement(&State->UIContext, Item->ItemDesc, {Position.X, Position.Y + NewUIYDescOffset}, 10, TEXT_ALIGNMENT_Center, GREEN);
                    CloverUIMakeTextElement(&State->UIContext, Item->ItemName, {Position.X + 4, Position.Y + NewUIYOffset}, 10, TEXT_ALIGNMENT_Center, GREEN);
                    
                    mat4 SpriteXForm = UIXForm;
                    UIXForm = mat4Multiply(UIXForm, mat4Translate(vec3{0, NewUIYOffset, 0}));
                    UIXForm = mat4Multiply(UIXForm, mat4MakeScale(v2Expand(UIBoxSize + ItemDescData->Size, 1.0f)));
                    DrawUISpriteXForm(RenderData, UIXForm, GetSprite(State, SPRITE_Nil), 0, vec4{0.2f, 0.2f, 0.2f, 0.2f});
                    
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4Translate(vec3{UIBoxSize.X + ItemDescData->Size.X * -0.5f, SpriteYOffset, 0}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4MakeScale(vec3{IconSize, IconSize, 1.0f}));
                    DrawUISpriteXForm(RenderData, SpriteXForm, SpriteData, 0, WHITE);
                    
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4MakeScale(vec3{1 / IconSize, 1 / IconSize, 1.0f}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4Translate(vec3{-(UIBoxSize.X + ItemDescData->Size.X * -0.5f), 0, 0}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4Translate(vec3{(UIBoxSize.X + ItemDescData->Size.X * -0.5f), 19, 0}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4Translate(vec3{-7, 0, 0}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4MakeScale(vec3{16, 16, 1.0f}));
                    DrawUISpriteXForm(RenderData, SpriteXForm, GetSprite(State, SPRITE_Nil), 0, vec4{0.1f, 0.1f, 0.1f, 0.4f});
                    
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4MakeScale(vec3{1 / IconSize, 1 / IconSize, 1.0f}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4Translate(vec3{2, -6, 0}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4MakeScale(vec3{16, 16, 1.0f}));
                    
                    vec4 ItemCountTextPosition = SpriteXForm.Columns[3];
                    Position = vec2{ItemCountTextPosition.X, ItemCountTextPosition.Y};
                    CloverUIMakeTextElement(&State->UIContext, sprints(&Memory->TemporaryStorage, STR("x%d"), Item->CurrentStack), {Position.X + 3, Position.Y}, 15, TEXT_ALIGNMENT_Center, GREEN);
                }
            }
            
            // SELECTED INVENTORY ITEM
            if(HotbarSlot->IsActive)
            {
                if(Item->Sprite != SPRITE_Nil && !Player->Inventory.SelectedInventoryItem)
                {
                    Player->Inventory.SelectedInventoryItem = Item;
                }
            }
            
            if(Player->Inventory.CurrentInventorySlot == InventorySlot)
            {
                Player->Inventory.SelectedHotbarItem = &Player->Inventory.Items[InventorySlot];
            }
        }
    }
    
    // NOTE(Sleepster): Large Inventory UI 
    const real32 InventoryYOffset = -70.0f;
    if(State->DisplayPlayerInventory)
    {
        // TODO(Sleepster): This is all copy-pasted, clean it up and move common code into a seperate function 
        State->UIContext.UICameraViewMatrix       = RenderData->GameUICamera.ViewMatrix;
        State->UIContext.UICameraProjectionMatrix = RenderData->GameUICamera.ProjectionMatrix;
        State->UIContext.GameInput                = &State->GameInput;
        State->UIContext.ActiveFont               = &RenderData->LoadedFonts[UBUNTU_MONO];
        State->UIContext.ActiveFontIndex          = UBUNTU_MONO;
        
        const real32 Width = SizeData.X * 0.25f;
        const real32 Padding = 1.00f;
        const real32 IconSize = 16.0f;
        const real32 TotalWidth = ((PLAYER_INVENTORY_SIZE * IconSize) + (IconSize * 2) - 7) + ((PLAYER_INVENTORY_SIZE - 1) * Padding);
        const real32 StartingX = (Width / 2.0f) - (TotalWidth / 2.0f);
        
        for(uint32 InventorySlot = PLAYER_HOTBAR_COUNT;
            InventorySlot < TOTAL_INVENTORY_SIZE;
            ++InventorySlot)
        {
            real32 SlotOffset = (IconSize + Padding) * (InventorySlot - 6);
            
            mat4 XForm = mat4Identity(1.0f);
            XForm = mat4Multiply(XForm, mat4Translate(vec3{StartingX + SlotOffset, InventoryYOffset, 0.0}));
            XForm = mat4Multiply(XForm, mat4Translate(vec3{IconSize * -0.55f, 0.0f, 0.0f}));
            XForm = mat4Multiply(XForm, mat4MakeScale(vec3{IconSize, IconSize, 1.0}));
            
            vec4 MatrixPosition = XForm.Columns[3];
            vec2 SlotPosition   = {MatrixPosition.X, MatrixPosition.Y};
            
            static_sprite_data Sprite = GetSprite(State, SPRITE_UIItemBox);
            ui_element_state HotbarSlotState = CloverUIButton(&State->UIContext, STR("InventorySlot"), SlotPosition, {IconSize, IconSize}, Sprite, WHITE);
            ui_element *HotbarSlot = &State->UIContext.UIElements[HotbarSlotState.UIID.ID];
            
            Player->Inventory.InventorySlotButtons[InventorySlot] = HotbarSlot;
            
            HotbarSlot->XForm = XForm;
            HotbarSlot->Sprite = Sprite;
            HotbarSlot->DrawColor = WHITE;
            
            item *Item = &Player->Inventory.Items[InventorySlot];
            Item->OccupiedInventorySlot = InventorySlot;
            
            Sprite = GetSprite(State, Item->Sprite);
            if(Sprite != State->GameData.Sprites[SPRITE_Nil] && !HotbarSlotState.IsHot)
            {
                XForm = mat4Multiply(XForm, mat4MakeScale(vec3{0.65, 0.65, 1.0}));
                DrawUISpriteXForm(RenderData, XForm, Sprite, 0, WHITE);
            }
            
            if(HotbarSlotState.IsHot)
            {
                HotbarSlot->DrawColor = RED;
                if(Item->Sprite != SPRITE_Nil)
                {
                    XForm = mat4Multiply(XForm, mat4MakeScale(vec3{1.0, 1.0, 1.0}));
                    DrawUISpriteXForm(RenderData, XForm, Sprite, 0, WHITE);
                    
                    real32 NewUIYOffset;
                    real32 NewUIYDescOffset;
                    real32 SpriteYOffset;
                    if(State->DisplayPlayerInventory)
                    {
                        NewUIYOffset = 24.0f;
                        NewUIYDescOffset = 8.0f;
                        SpriteYOffset = 25.0f;
                    }
                    else
                    {
                        NewUIYOffset = 16.0f;
                        NewUIYDescOffset = 3.0f;
                        SpriteYOffset = 20.0f;
                    }
                    
                    vec2 UIBoxSize = {10, 25};
                    
                    mat4 UIXForm = mat4Identity(1.0f);
                    UIXForm = mat4Multiply(UIXForm, mat4Translate(vec3{StartingX + SlotOffset, -90, 0.0}));
                    UIXForm = mat4Multiply(UIXForm, mat4Translate(vec3{IconSize * -0.55f, NewUIYOffset, 0.0f}));
                    
                    vec4 UIMatrixPosition = UIXForm.Columns[3];
                    vec2 Position = vec2{UIMatrixPosition.X, UIMatrixPosition.Y};
                    
                    static_sprite_data SpriteData = GetSprite(State, Item->Sprite);
                    vec2 SpriteSize = v2Cast(SpriteData.SpriteSize);
                    
                    ui_element *ItemDescData = CloverUIMakeTextElement(&State->UIContext, Item->ItemDesc, {Position.X, Position.Y + NewUIYDescOffset}, 10, TEXT_ALIGNMENT_Center, GREEN);
                    CloverUIMakeTextElement(&State->UIContext, Item->ItemName, {Position.X + 4, Position.Y + NewUIYOffset}, 10, TEXT_ALIGNMENT_Center, GREEN);
                    
                    mat4 SpriteXForm = UIXForm;
                    UIXForm = mat4Multiply(UIXForm, mat4Translate(vec3{0, NewUIYOffset, 0}));
                    UIXForm = mat4Multiply(UIXForm, mat4MakeScale(v2Expand(UIBoxSize + ItemDescData->Size, 1.0f)));
                    DrawUISpriteXForm(RenderData, UIXForm, GetSprite(State, SPRITE_Nil), 0, vec4{0.2f, 0.2f, 0.2f, 0.2f});
                    
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4Translate(vec3{UIBoxSize.X + ItemDescData->Size.X * -0.5f, SpriteYOffset, 0}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4MakeScale(vec3{IconSize, IconSize, 1.0f}));
                    DrawUISpriteXForm(RenderData, SpriteXForm, SpriteData, 0, WHITE);
                    
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4MakeScale(vec3{1 / IconSize, 1 / IconSize, 1.0f}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4Translate(vec3{-(UIBoxSize.X + ItemDescData->Size.X * -0.5f), 0, 0}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4Translate(vec3{(UIBoxSize.X + ItemDescData->Size.X * -0.5f), 19, 0}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4Translate(vec3{-7, 0, 0}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4MakeScale(vec3{16, 16, 1.0f}));
                    DrawUISpriteXForm(RenderData, SpriteXForm, GetSprite(State, SPRITE_Nil), 0, vec4{0.1f, 0.1f, 0.1f, 0.4f});
                    
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4MakeScale(vec3{1 / IconSize, 1 / IconSize, 1.0f}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4Translate(vec3{2, -6, 0}));
                    SpriteXForm = mat4Multiply(SpriteXForm, mat4MakeScale(vec3{16, 16, 1.0f}));
                    
                    vec4 ItemCountTextPosition = SpriteXForm.Columns[3];
                    Position = vec2{ItemCountTextPosition.X, ItemCountTextPosition.Y};
                    CloverUIMakeTextElement(&State->UIContext, sprints(&Memory->TemporaryStorage, STR("x%d"), Item->CurrentStack), {Position.X + 3, Position.Y}, 15, TEXT_ALIGNMENT_Center, GREEN);
                }
            }
        }
    }
    
    // NOTE(Sleepster): Swapping Inventory Positions 
    for(uint32 InventoryIndexSlot = 0;
        InventoryIndexSlot < TOTAL_INVENTORY_SIZE;
        InventoryIndexSlot++)
    {
        item *Item = &Player->Inventory.Items[InventoryIndexSlot];
        ui_element *InventoryElement = Player->Inventory.InventorySlotButtons[InventoryIndexSlot]; 
        if(Item->CurrentStack == 0 && InventoryIndexSlot == Player->Inventory.CurrentInventorySlot)
        {
            ResetItemSlotState(Item);
        }
        
        if(InventoryElement)
        {
            // NOTE(Sleepster): Inventory Item Selection first item 
            if(InventoryElement->IsActive)
            {
                if(Item->Sprite != SPRITE_Nil && !Player->Inventory.SelectedInventoryItem)
                {
                    Player->Inventory.SelectedInventoryItem = Item;
                }
            }
            
            // TODO(Sleepster): Render the Item count if it's needed 
            /* if(Item->CurrentStack > 1 && Item->MaxStackCount > 1) */
            /* { */
            /*     mat4 TextXForm = InventoryElement->XForm; */
            /*     vec2 TextPosition = TextXForm.Columns[3].XY; */
            /*     DrawUIText(RenderData, sprints(&Memory->TemporaryStorage, STR("x%d"), Item->CurrentStack), {TextPosition.X, TextPosition.Y}, 10, UBUNTU_MONO, GREEN); */
            /* } */ 
            
            // NOTE(Sleepster): Scale sprite if selected 
            if(InventoryIndexSlot == Player->Inventory.CurrentInventorySlot)
            {
                InventoryElement->XForm = mat4Multiply(InventoryElement->XForm, mat4MakeScale(vec3{1.20f, 1.20f, 1.0f}));
            }
            
            // NOTE(Sleepster): Choose selection 
            if((Player->Inventory.SelectedInventoryItem && Player->Inventory.SelectedInventoryItem->Sprite != SPRITE_Nil))
            {
                item *Selection = Player->Inventory.SelectedInventoryItem;
                vec2 MousePos = TransformMouseCoords(RenderData->GameUICamera.ViewMatrix, 
                                                     RenderData->GameUICamera.ProjectionMatrix, 
                                                     State->GameInput.Keyboard.CurrentMouse, 
                                                     SizeData);
                
                static_sprite_data SelectionSprite = GetSprite(State, Selection->Sprite);
                
                CloverUIPushLayer(&State->UIContext, 1);
                CloverUISpriteElement(&State->UIContext, MousePos + vec2{1, 0}, v2Cast(SelectionSprite.SpriteSize), NULLMATRIX, SelectionSprite, WHITE);
                CloverUIPushLayer(&State->UIContext, 0);
                
                // NOTE(Sleepster): Dropping Selected inventory Items 
                if(IsGameKeyPressed(DROP_ITEM, &State->GameInput))
                {
                    entity *SpawnedItem = CreateEntity(State);
                    SetupDroppedEntity(RenderData, State, Selection, SpawnedItem);
                    
                    Player->Inventory.Items[Player->Inventory.SelectedInventoryItem->OccupiedInventorySlot] = {};
                    Player->Inventory.SelectedInventoryItem = {};
                }
            }
            
            // NOTE(Sleepster): Swap item stuff 
            if(!Player->Inventory.SwapItem)
            {
                item *Check = &Player->Inventory.Items[InventoryIndexSlot];
                if(InventoryElement->IsActive && Player->Inventory.SelectedInventoryItem && Check->OccupiedInventorySlot != Player->Inventory.SelectedInventoryItem->OccupiedInventorySlot)
                {
                    Player->Inventory.SwapItem = Check;
                }
            }
            
            // NOTE(Sleepster): Actually swapping the two 
            if(Player->Inventory.SwapItem && Player->Inventory.SelectedInventoryItem)
            {
                SwapInventoryItems(&Player->Inventory, Player->Inventory.SelectedInventoryItem, Player->Inventory.SwapItem); 
                Player->Inventory.SwapItem = {};
                Player->Inventory.SelectedInventoryItem = {};
            }
        }
    }
    
    // NOTE(Sleepster): Quickdropping HotbarItem
    if(IsGameKeyPressed(DROP_HELD, &State->GameInput))
    {
        item *HotbarItem = Player->Inventory.SelectedHotbarItem;
        if(HotbarItem && Player->Inventory.CurrentInventorySlot != NULLSLOT)
        {
            if(HotbarItem->CurrentStack != 0)
            {
                if(IsKeyDown(KEY_CONTROL, &State->GameInput))
                {
                    for(uint32 ItemCount = 0;
                        ItemCount < HotbarItem->CurrentStack;
                        ItemCount++)
                    {
                        entity *DroppedEntity = CreateEntity(State);
                        SetupDroppedEntity(RenderData, State, HotbarItem, DroppedEntity);
                        HotbarItem->CurrentStack = 0;
                    }
                }
                else
                {
                    --HotbarItem->CurrentStack;
                    entity *DroppedEntity = CreateEntity(State);
                    SetupDroppedEntity(RenderData, State, HotbarItem, DroppedEntity);
                    DroppedEntity->DroppedFromInventoryItemCount = 1;
                }
            }
        }
    }
    
    // NOTE(Sleepster): UI Rendering 
    CloverUIDrawWidgets(RenderData, &State->UIContext);
    CloverUIResetState(&State->UIContext); 

    // NOTE(Sleepster): Building
    {
        if(State->GameUIState == UI_State_Building)
        {
            const real32 IconSize = 12;
            const real32 Padding = 2;
            const real32 BoxWidth  = (IconSize * 5) + ((5 - 1) * Padding);
            
            real32 BoxHeight = 50;
            
            const vec4 BoxColor = {0.0, 0.0, 0.0, 0.6};
            const real32 StartingXOffset = -IconSize * 2;
            
            int IconCount = 0;
            mat4 XForm = mat4Identity(1.0f);
            for(uint32 Element = 0;
                Element < ITEM_IDCount;
                Element++)
            {
                item *Item = &State->GameData.GameItems[Element];
                if(Item && Item->Flags & IS_BUILDABLE)
                {
                    real32 NewXOffset = StartingXOffset + ((IconSize + Padding) * IconCount);
                    XForm = mat4Identity(1.0f);
                    XForm = mat4Translation(XForm, vec3{NewXOffset, 10, 0});
                    
                    vec2 SpriteSize = {IconSize, IconSize};
                    if(State->ActiveBlueprint && Item->ItemID == State->ActiveBlueprint->ItemID)
                    {
                        SpriteSize = {14, 14};
                    }
                    XForm = mat4Scale(XForm, v2Expand(SpriteSize, 1));
                    CloverUISpriteElement(&State->UIContext, {0, 0}, {0, 0}, XForm, GetSprite(State, Item->Sprite), WHITE);
                    vec2 Position = XForm.Columns[3].XY;
                    
                    CloverUIPushLayer(&State->UIContext, 1);
                    ui_element_state Button = CloverUIButton(&State->UIContext, STR("Element"), Position, SpriteSize, GetSprite(State, SPRITE_Outline), WHITE);
                    CloverUIPushLayer(&State->UIContext, 0);
                    
                    IconCount++;
                    if((Button.IsPressed && !State->ActiveBlueprint) || (Button.IsPressed && Item->ItemID != State->ActiveBlueprint->ItemID))
                    {
                        State->ActiveBlueprint = Item;
                    }
                    else if(Button.IsPressed && State->ActiveBlueprint)
                    {
                        State->ActiveBlueprint = {};
                    }
                }
            }
            
            vec2 BoxPosition = {0, 0};
            if(State->ActiveBlueprint)
            {
                item *Item = State->ActiveBlueprint;
                BoxHeight = 115;
                BoxPosition = {0, -20};
                
                XForm = mat4Identity(1.0f);
                XForm = mat4Multiply(XForm, mat4Translate(v2Expand({BoxPosition.X, BoxPosition.Y + 20}, 0)));
                XForm = mat4Multiply(XForm, mat4MakeScale(vec3{BoxWidth, 2, 1}));
                
                CloverUIPushLayer(&State->UIContext, 1);
                DrawUISpriteXForm(RenderData, XForm, GetSprite(State, SPRITE_Nil), 0, vec4{0.0, 0.0, 0.0, 0.8f});
                CloverUIPushLayer(&State->UIContext, 0);
                
                int InventoryCount[MAX_CRAFTING_ELEMENTS] = {};
                for(uint32 InventorySlotIndex = 0;
                    InventorySlotIndex < TOTAL_INVENTORY_SIZE;
                    InventorySlotIndex++)
                {
                    item *InventoryItem = &Player->Inventory.Items[InventorySlotIndex];
                    for(uint32 FormulaIndex = 0;
                        FormulaIndex < State->ActiveBlueprint->UniqueMaterialCount;
                        ++FormulaIndex)
                    {
                        crafting_material *FormulaItem = &State->ActiveBlueprint->CraftingFormula[FormulaIndex];
                        if(InventoryItem->ItemID == FormulaItem->CraftingMaterial)
                        {
                            InventoryCount[FormulaIndex] = InventoryItem->CurrentStack;
                        }
                    }
                }
                const real32 InitialYOffset = -20;
                for(uint32 MaterialIndex = 0;
                    MaterialIndex < Item->UniqueMaterialCount;
                    MaterialIndex++)
                {
                    crafting_material *Material = &Item->CraftingFormula[MaterialIndex];
                    real32 NewYOffset = InitialYOffset - ((14 + Padding) * MaterialIndex);
                    
                    XForm = mat4Identity(1.0f);
                    XForm = mat4Translation(XForm, vec3{0, NewYOffset + 10, 0});
                    XForm = mat4Scale(XForm, vec3{30, 12, 1});
                    CloverUISpriteElement(&State->UIContext, {0, 0}, {0, 0}, XForm, GetSprite(State, SPRITE_Nil), {0.4, 0.4, 0.4, 0.3});
                    
                    XForm = mat4Identity(1.0f);
                    XForm = mat4Translation(XForm, vec3{-10, NewYOffset + 10, 0});
                    XForm = mat4Scale(XForm, vec3{8, 8, 1});
                    
                    static_sprite_data Sprite = GetSprite(State, GetSpriteFromPair(State, Material->CraftingMaterial));
                    
                    CloverUIPushLayer(&State->UIContext, 2);
                    CloverUISpriteElement(&State->UIContext, {0, 0}, {0, 0}, XForm, Sprite, WHITE);
                    CloverUIMakeTextElement(&State->UIContext, sprints(&Memory->TemporaryStorage, STR("%d/%d"), InventoryCount[MaterialIndex], Material->RequiredCount), {10, NewYOffset + 5}, 10, TEXT_ALIGNMENT_Center, BLACK);
                    CloverUIPushLayer(&State->UIContext, 0);
                }
                
                vec4 ButtonColor = {0.2, 0.2, 0.2, 0.4};
                CloverUIPushLayer(&State->UIContext, 4);
                ui_element_state Button = CloverUIButton(&State->UIContext, STR("Element"), {0, -65}, {40, 12}, GetSprite(State, SPRITE_Nil), ButtonColor);
                CloverUIMakeTextElement(&State->UIContext, STR("BUILD!"), {5, -70}, 10, TEXT_ALIGNMENT_Center, WHITE);
                CloverUIPushLayer(&State->UIContext, 0);
                
                ui_element *ButtonId = &State->UIContext.UIElements[Button.UIID.ID];
                if(Button.IsHot)
                {
                    ButtonId->DrawColor = {0.6, 0.2, 0.2, 0.4};
                    if(IsGameKeyDown(ATTACK, &State->GameInput))
                    {
                        ButtonId->Size = ButtonId->Size * 1.1;
                    }
                }
                
                if(Button.IsPressed)
                {
                    if(IsItemCraftable(InventoryCount, State->ActiveBlueprint))
                    {
                        for(uint32 InventorySlotIndex = 0;
                            InventorySlotIndex < TOTAL_INVENTORY_SIZE;
                            InventorySlotIndex++)
                        {
                            item *InventoryItem = &Player->Inventory.Items[InventorySlotIndex];
                            for(uint32 FormulaIndex = 0;
                                FormulaIndex < State->ActiveBlueprint->UniqueMaterialCount;
                                ++FormulaIndex)
                            {
                                crafting_material *FormulaItem = &State->ActiveBlueprint->CraftingFormula[FormulaIndex];
                                if(InventoryItem->ItemID == FormulaItem->CraftingMaterial)
                                {
                                    InventoryItem->CurrentStack -= FormulaItem->RequiredCount;
                                    if(InventoryItem->CurrentStack <= 0)
                                    {
                                        ResetItemSlotState(InventoryItem);
                                    }
                                }
                            }
                        }
                        
                        // NOTE(Sleepster): Simply Drop the item in the player's inventory
                        switch(State->ActiveBlueprint->Archetype)
                        {
                            case ARCH_Workbench:
                            {
                                entity *Workbench = CreateEntity(State);
                                SetupItemWorkbench(State, Workbench);
                                Workbench->Position = Player->Position;
                                Workbench->Target   = Player->Position;
                            }break;
                            case ARCH_Furnace:
                            {
                                entity *Furnace = CreateEntity(State);
                                SetupItemFurnace(State, Furnace);
                                Furnace->Position = Player->Position;
                                Furnace->Target   = Player->Position;
                            }break;
                        }
                    }
                }
            }
            else
            {
                BoxPosition = {0, 12.5};
            }
            XForm = mat4Identity(1.0f);
            XForm = mat4Multiply(XForm, mat4Translate(v2Expand(BoxPosition, 0)));
            XForm = mat4Multiply(XForm, mat4MakeScale(vec3{BoxWidth, BoxHeight, 1}));
            
            DrawUISpriteXForm(RenderData, XForm, GetSprite(State, SPRITE_Nil), 0, vec4{0.0, 0.0, 0.0, 0.8f});
            DrawUIText(RenderData, STR("Building..."), {-IconSize, 20}, 15, UBUNTU_MONO, WHITE);
        }
        
        // NOTE(Sleepster): Building From Inventory/Hotbar
        if((Player->Inventory.SelectedHotbarItem || Player->Inventory.SelectedInventoryItem) && (Player->Inventory.CurrentInventorySlot != NULLSLOT))
        {
            item *InventoryItem = {};
            item *HotbarItem = {};
            item *Item = {};
            for(uint32 InventoryIndex = 0;
                InventoryIndex < TOTAL_INVENTORY_SIZE;
                InventoryIndex++)
            {
                Item = &Player->Inventory.Items[InventoryIndex];
                if(Item == Player->Inventory.SelectedHotbarItem) 
                {
                    HotbarItem = Item;
                    break;
                }
                if(Item == Player->Inventory.SelectedInventoryItem) 
                {
                    InventoryItem = Item;
                    break;
                };
            }
            
            if(HotbarItem && (HotbarItem->Flags & IS_BUILDABLE) || (InventoryItem && (InventoryItem->Flags & IS_BUILDABLE)))
            {
                vec2 MousePosition = RoundToTile(vec2{MouseToWorld.X + (TILE_SIZE * 0.5f), MouseToWorld.Y});
                static_sprite_data HotbarSprite = GetSprite(State, HotbarItem->Sprite);
                
                mat4 XForm = mat4Identity(1.0f);
                XForm = mat4Multiply(XForm, mat4Translate(vec3{MousePosition.X, MousePosition.Y  + (TILE_SIZE * 0.5f), 0}));
                XForm = mat4Multiply(XForm, mat4MakeScale(v2Expand(v2Cast(HotbarSprite.SpriteSize), 1)));
                DrawSpriteXForm(RenderData, XForm, HotbarSprite, 0, vec4{0.2, 0.2, 0.2, 0.3f});
                
                if(IsGameKeyPressed(INTERACT, &State->GameInput))
                {
                    bool Overlap = {};
                    for(uint32 EntityCounter = 0;
                        EntityCounter <= State->World.EntityCounter;
                        EntityCounter++)
                    {
                        entity *TestBuildingBounds = &State->World.Entities[EntityCounter];
                        Overlap = IsRangeWithinBounds(MouseToWorld, TestBuildingBounds->BoxCollider);
                        if(Overlap)
                        {
                            break;
                        }
                    }
                    if(!Overlap)
                    {
                        entity *Building = CreateEntity(State);
                        switch(Item->ItemID)
                        {
                            case ITEM_Workbench:
                            {
                                SetupBuildingWorkbench(State, Building);
                            }break;
                            case ITEM_Furnace:
                            {
                                SetupBuildingFurnace(State, Building);
                            }break;
                        }
                        Building->Position = MousePosition;
                        Building->BoxCollider = CreateRange(vec2{Building->Position.X - (TILE_SIZE * 0.5f), Building->Position.Y}, 
                                                            vec2{Building->Position.X - (TILE_SIZE * 0.5f), Building->Position.Y} + Building->Size);
                        
                        if(InventoryItem) ResetItemSlotState(InventoryItem);
                        if(HotbarItem) ResetItemSlotState(HotbarItem);
                    }
                }
            }
        }
    }
    
    // NOTE(Sleepster): Crafting
    {
        // NOTE(Sleepster): Can the crafting dialogue be displayed?
        if(IsGameKeyPressed(CRAFTING, &State->GameInput))
        {
            if(!State->ActiveCraftingStation && State->GameUIState != UI_State_Crafting)
            {
                for(uint32 EntityIndex = 0;
                    EntityIndex <= State->World.EntityCounter;
                    EntityIndex++)
                {
                    entity *Temp = &State->World.Entities[EntityIndex];
                    if(Temp->Flags & IS_PLACED)
                    {
                        real32 Distance = v2Distance(Player->Position, Temp->Position);
                        if(Distance <= ItemPickupDist && State->World.WorldFrame.SelectedEntity) 
                        {
                            State->GameUIState = UI_State_Crafting;
                            State->ActiveCraftingStation = Temp;
                            break;
                        }
                    }
                }
            }
            else
            {
                State->GameUIState = UI_State_Nil;
                State->ActiveCraftingStation = {};
            }
        }
        
        // NOTE(Sleepster): If it can, display it
        if(State->GameUIState == UI_State_Crafting)
        {
            const real32 IconSize = 12;
            const real32 Padding = 2;
            const real32 BoxWidth  = (IconSize * 5) + ((5 - 1) * Padding);
            const real32 BoxHeight = 120;
            
            const vec2 BoxPosition = {-45, 0};
            const vec4 BoxColor = {0.0, 0.0, 0.0, 0.6};
            
            const real32 StartingXOffset = -BoxWidth;
            const real32 StartingYOffset = 32;
            
            // NOTE(Sleepster): This can be up to 4;
            int IconCount = 0;
            int RowCount = 0;
            
            mat4 XForm = mat4Identity(1.0f);
            XForm = mat4Translation(XForm, v2Expand(BoxPosition, 0));
            XForm = mat4Scale(XForm, vec3{BoxWidth, BoxHeight, 1});
            CloverUISpriteElement(&State->UIContext, {0, 0}, {0, 0}, XForm, GetSprite(State, SPRITE_Nil), BoxColor);
            
            XForm = mat4Identity(1.0f);
            XForm = mat4Translation(XForm, v2Expand(BoxPosition * -1, 0));
            XForm = mat4Scale(XForm, vec3{BoxWidth, BoxHeight, 1});
            CloverUISpriteElement(&State->UIContext, {0, 0}, {0, 0}, XForm, GetSprite(State, SPRITE_Nil), BoxColor);
            
            DrawUIText(RenderData, STR("Crafting"), {-55, 40}, 15, UBUNTU_MONO, WHITE);
            for(uint32 Element = 0;
                Element < ITEM_IDCount;
                Element++)
            {
                item *Item = &State->GameData.GameItems[Element];
                if((Item && Item->Craftable) && Item && !(Item->Flags & IS_BUILDABLE))
                {
                    real32 NewXOffset = StartingXOffset + ((IconSize + Padding) * IconCount);
                    real32 NewYOffset = StartingYOffset - ((IconSize + Padding) * RowCount);
                    
                    XForm = mat4Identity(1.0f);
                    XForm = mat4Translation(XForm, vec3{NewXOffset, NewYOffset, 0});
                    
                    vec2 SpriteSize = {IconSize, IconSize};
                    if(Item == State->ActiveRecipe)
                    {
                        SpriteSize = {14, 14};
                    }
                    XForm = mat4Scale(XForm, v2Expand(SpriteSize, 1));
                    CloverUISpriteElement(&State->UIContext, {0, 0}, {0, 0}, XForm, GetSprite(State, Item->Sprite), WHITE);
                    vec2 Position = XForm.Columns[3].XY;
                    CloverUIPushLayer(&State->UIContext, 1);
                    ui_element_state Button = CloverUIButton(&State->UIContext, STR("Element"), Position, SpriteSize, GetSprite(State, SPRITE_Outline), WHITE);
                    CloverUIPushLayer(&State->UIContext, 0);
                    
                    IconCount++;
                    if(IconCount >= 4)
                    {
                        IconCount = 0;
                        RowCount = 1;
                    }
                    
                    if((Button.IsPressed && !State->ActiveRecipe) || (Button.IsPressed && Item->ItemID != State->ActiveRecipe->ItemID))
                    {
                        State->ActiveRecipe = Item;
                    }
                    else if(Button.IsPressed && State->ActiveRecipe)
                    {
                        State->ActiveRecipe = {};
                    }
                }
            }
            
            // NOTE(Sleepster): If we have an active recipe, display the recipe
            if(State->ActiveRecipe)
            {
                item *Item = State->ActiveRecipe;
                CloverUIMakeTextElement(&State->UIContext, Item->ItemName, {50, 40}, 15, TEXT_ALIGNMENT_Center, WHITE);
                
                XForm = mat4Identity(1.0f);
                XForm = mat4Translation(XForm, vec3{45, 30, 0});
                XForm = mat4Scale(XForm, vec3{IconSize, IconSize, 1});
                CloverUISpriteElement(&State->UIContext, {0, 0}, {0, 0}, XForm, GetSprite(State, Item->Sprite), WHITE);
                
                int InventoryCount[MAX_CRAFTING_ELEMENTS] = {};
                for(uint32 InventorySlotIndex = 0;
                    InventorySlotIndex < TOTAL_INVENTORY_SIZE;
                    InventorySlotIndex++)
                {
                    item *InventoryItem = &Player->Inventory.Items[InventorySlotIndex];
                    for(uint32 FormulaIndex = 0;
                        FormulaIndex < State->ActiveRecipe->UniqueMaterialCount;
                        ++FormulaIndex)
                    {
                        crafting_material *FormulaItem = &State->ActiveRecipe->CraftingFormula[FormulaIndex];
                        if(InventoryItem->ItemID == FormulaItem->CraftingMaterial)
                        {
                            InventoryCount[FormulaIndex] = InventoryItem->CurrentStack;
                        }
                    }
                }
                
                const real32 InitialYOffset = 10;
                for(uint32 MaterialIndex = 0;
                    MaterialIndex < Item->UniqueMaterialCount;
                    MaterialIndex++)
                {
                    crafting_material *Material = &Item->CraftingFormula[MaterialIndex];
                    real32 NewYOffset = InitialYOffset - ((14 + Padding) * MaterialIndex);
                    
                    XForm = mat4Identity(1.0f);
                    XForm = mat4Translation(XForm, vec3{45, NewYOffset, 0});
                    XForm = mat4Scale(XForm, vec3{30, 12, 1});
                    CloverUISpriteElement(&State->UIContext, {0, 0}, {0, 0}, XForm, GetSprite(State, SPRITE_Nil), {0.4, 0.4, 0.4, 0.3});
                    
                    XForm = mat4Identity(1.0f);
                    XForm = mat4Translation(XForm, vec3{40, NewYOffset, 0});
                    XForm = mat4Scale(XForm, vec3{8, 8, 1});
                    
                    static_sprite_data Sprite = GetSprite(State, GetSpriteFromPair(State, Material->CraftingMaterial));
                    
                    CloverUIPushLayer(&State->UIContext, 2);
                    CloverUISpriteElement(&State->UIContext, {0, 0}, {0, 0}, XForm, Sprite, WHITE);
                    CloverUIMakeTextElement(&State->UIContext, sprints(&Memory->TemporaryStorage, STR("%d/%d"), InventoryCount[MaterialIndex], Material->RequiredCount), {55, NewYOffset - 5}, 10, TEXT_ALIGNMENT_Center, BLACK);
                    CloverUIPushLayer(&State->UIContext, 0);
                }
                
                vec4 ButtonColor = {0.2, 0.2, 0.2, 0.4};
                ui_element_state Button = CloverUIButton(&State->UIContext, STR("Element"), {45, -50}, {40, 12}, GetSprite(State, SPRITE_Nil), ButtonColor);
                ui_element *ButtonId = &State->UIContext.UIElements[Button.UIID.ID];
                if(Button.IsHot)
                {
                    ButtonId->DrawColor = {0.6, 0.2, 0.2, 0.4};
                    if(IsGameKeyDown(ATTACK, &State->GameInput))
                    {
                        ButtonId->Size = ButtonId->Size * 1.1;
                    }
                }
                
                if(Button.IsPressed)
                {
                    if(IsItemCraftable(&InventoryCount[0], State->ActiveRecipe))
                    {
                        for(uint32 Index = 0;
                            Index < State->ActiveRecipe->FormulaResultCount;
                            Index++)
                        {
                            entity *CraftedItem = CreateEntity(State);
                            SetupDroppedEntity(RenderData, State, State->ActiveRecipe, CraftedItem);
                            CraftedItem->Position = State->ActiveCraftingStation->Position;
                            CraftedItem->Target = State->ActiveCraftingStation->Position;
                        }
                        
                        for(uint32 InventorySlotIndex = 0;
                            InventorySlotIndex < TOTAL_INVENTORY_SIZE;
                            InventorySlotIndex++)
                        {
                            item *InventoryItem = &Player->Inventory.Items[InventorySlotIndex];
                            for(uint32 FormulaIndex = 0;
                                FormulaIndex < State->ActiveRecipe->UniqueMaterialCount;
                                ++FormulaIndex)
                            {
                                crafting_material *FormulaItem = &State->ActiveRecipe->CraftingFormula[FormulaIndex];
                                if(InventoryItem->ItemID == FormulaItem->CraftingMaterial)
                                {
                                    InventoryItem->CurrentStack -= FormulaItem->RequiredCount;
                                    if(InventoryItem->CurrentStack <= 0)
                                    {
                                        ResetItemSlotState(InventoryItem);
                                    }
                                }
                            }
                        }
                    }
                }
                CloverUIMakeTextElement(&State->UIContext, STR("Craft"), {48, -55}, 10, TEXT_ALIGNMENT_Center, BLACK);
            }
            else
            {
                CloverUIMakeTextElement(&State->UIContext, STR("Please Select an Item"), {50, 0}, 10, TEXT_ALIGNMENT_Center, WHITE);
            }
            
            // NOTE(Sleepster): If the player gets to far, stop displaying it
            real32 Distance = v2Distance(Player->Position, State->ActiveCraftingStation->Position);
            if(Distance > ItemPickupDist)
            {
                State->GameUIState = UI_State_Nil;
                State->ActiveCraftingStation = {};
            }
        }
    }
    
    
    // TRANSPARENCY TEST
    {
        mat4 Identity  = mat4Identity(1.0f);
        mat4 Translate = mat4Multiply(Identity, mat4Translate(vec3{10.0f, 10.0f, 0.0f}));
        mat4 Scale     = mat4Multiply(Identity, mat4MakeScale(vec3{100.0f, 100.0f, 1.0f}));
        
        mat4 Total = Translate * Scale;
        DrawRectXForm(RenderData, Total, {16, 16}, 0, vec4{1.0f, 0.0f, 1.0f, 0.3f});
    }
    
    
    // NOTE(Sleepster): Sorting, off for now. Breaks too much 
    {
        //qsort(State->World.Entities, State->World.EntityCounter, sizeof(struct entity), CompareEntityYAxis); 
    }
    
    for(uint32 EntityIndex = 0;
        EntityIndex <= State->World.EntityCounter;
        ++EntityIndex)
    {
        entity *Temp = &State->World.Entities[EntityIndex];
        if((Temp->Flags & IS_VALID))
        {
            real32 Distance = fabsf(v2Distance(Temp->Position, MouseToWorld));
            real32 PlayerToObjectDistance = fabsf(v2Distance(Temp->Position, Player->Position));
            if(Distance <= SelectionDistance && PlayerToObjectDistance <= MaxHitRange)
            {
                if(!State->World.WorldFrame.SelectedEntity || (Distance < MinimumDistance))
                {
                    State->World.WorldFrame.SelectedEntity = Temp;
                    MinimumDistance = Distance;
                }
            }
        }
    }

    vec2 SelectionBoxDrawSize = {16, 16};
    // NOTE(Sleepster): DRAW ENTITIES
    for(uint32 EntityIndex = 0;
        EntityIndex <= State->World.EntityCounter;
        ++EntityIndex)
    {
        entity *Temp = &State->World.Entities[EntityIndex];
        SelectionBoxDrawSize.X = SinBreatheNormalized(Time.CurrentTimeInSeconds, 0.5f, 13.0f, 15.0f);
        SelectionBoxDrawSize.Y = SinBreatheNormalized(Time.CurrentTimeInSeconds, 0.5f, 13.0f, 15.0f);
        
        if(Temp->Flags & IS_VALID)
        {
            switch(Temp->Archetype)
            {
                case ARCH_Player:
                {
                    Player = Temp;
                    HandleInput(State, Temp, Time);
                    RenderData->GameCamera.Target = Temp->Position;
                    
                    v2Approach(&RenderData->GameCamera.Position, RenderData->GameCamera.Target, 5.0f, Time.Delta);
                    DrawEntity(RenderData, State, Temp, Temp->Position, WHITE);
                }break;
                default:
                {
                    if(Temp->Flags & IS_ITEM)
                    {
                        v2Approach(&Temp->Position, Temp->Target, 5.0f, Time.Delta);
                        Temp->Position.Y += 0.01f * SinBreathe(Time.CurrentTimeInSeconds, 1.25f);
                        if(v2Distance(Temp->Target, Temp->Position) <= PickupEpsilon)
                        {
                            Temp->Flags += CAN_BE_PICKED_UP;
                        }
                    }

                    if(State->World.WorldFrame.SelectedEntity == Temp && !(Temp->Flags & IS_ITEM))
                    {
                        State->World.WorldFrame.SelectedEntity = Temp;
                        static_sprite_data SelectionBoxSprite = GetSprite(State, SPRITE_SelectionBox);
                        static_sprite_data EntitySprite = GetSprite(State, Temp->Sprite);
                        
                        DrawSprite(RenderData, 
                                   SelectionBoxSprite, 
                                   Temp->Position 
                                   - vec2{0, real32(EntitySprite.SpriteSize.Y * 0.25f)},
                                   SelectionBoxDrawSize, 
                                   WHITE, 
                                   0, 
                                   1);
                    }
                    DrawEntity(RenderData, State, Temp, Temp->Position, WHITE);
                }break;
            }
        }
    }

    // NOTE(Sleepster): Draw the Tiles
    ivec2  PlayerOffset = WorldToTilePos(Player->Position);
    ivec2  TileRadius   = {14, 14};
    
    for(int32 TileX = PlayerOffset.X - TileRadius.X;
        TileX < PlayerOffset.X + TileRadius.Y;
        ++TileX)
    {
        for(int32 TileY = PlayerOffset.Y - TileRadius.Y;
            TileY < PlayerOffset.Y + TileRadius.Y;
            ++TileY)
        {
            if((TileX + (TileY % 2 == 0)) % 2 == 0)
            {
                real32 X = TileX * TILE_SIZE;
                real32 Y = TileY * TILE_SIZE;
                DrawQuad(RenderData, {X, Y - (TILE_SIZE)}, {TILE_SIZE, TILE_SIZE}, 0, DARK_GRAY, 0);
            }
        }
    }
    
    CreatePointLight(RenderData, vec2{0, 0}, 1, 10, RED);
}

extern
GAME_FIXED_UPDATE(GameFixedUpdate)
{
}
