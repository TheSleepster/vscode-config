#include "Clone_Intrinsics.h"
#include "Clone_Math.h"
#define DEBUG
internal Animation
CreateAnimation(int Init, int End, int Current, int Time, int Delay)
{
    Animation Animation;

    Animation.StartingFrame = Init;
    Animation.EndingFrame = End;
    Animation.CurrentFrame = Current;
    Animation.FrameTime = Time;
    Animation.FrameDelay = Delay;

    return (Animation);
}

internal void
HandlePlayerInput(State *State)
{
    // TODO : Replace this with the State->Input struct's functionality

    if (IsKeyDown(KEY_W))
    {
        State->Player.PhysicsBody.Vel.y = -State->Player.MovementSpeed;
    }
    else if (IsKeyDown(KEY_S))
    {
        State->Player.PhysicsBody.Vel.y = State->Player.MovementSpeed;
    }

    if (IsKeyDown(KEY_A))
    {
        State->Player.PhysicsBody.Vel.x = -State->Player.MovementSpeed;
        State->Player.Flags.Flipped = true;
    }
    else if (IsKeyDown(KEY_D))
    {
        State->Player.PhysicsBody.Vel.x = State->Player.MovementSpeed;
        State->Player.Flags.Flipped = false;
    }

    if (IsKeyUp(KEY_W) && IsKeyUp(KEY_S))
    {
        State->Player.PhysicsBody.Vel.y = 0;
    }

    if (IsKeyUp(KEY_A) && IsKeyUp(KEY_D))
    {
        State->Player.PhysicsBody.Vel.x = 0;
    }

    if (IsKeyUp(KEY_W) & IsKeyUp(KEY_S) & IsKeyUp(KEY_A) & IsKeyUp(KEY_D))
    {
        State->Player.AnimationState = IDLE;
    }

    Vector2ClampValue(State->Player.PhysicsBody.Vel, -State->Player.MovementSpeed, State->Player.MovementSpeed);
    State->Player.PhysicsBody.Pos.x += State->Player.PhysicsBody.Vel.x * GetFrameTime();
    State->Player.PhysicsBody.Pos.y += State->Player.PhysicsBody.Vel.y * GetFrameTime();

    // TODO : Find a better place for this
    State->Player.PhysicsBody.Hitbox = {State->Player.PhysicsBody.Pos.x + 2, State->Player.PhysicsBody.Pos.y - real32(State->Player.TextureData.Sprite.height / 2), 10, 21};

    State->Player.TextureData.DstRect =
        {
            State->Player.PhysicsBody.Pos.x,
            State->Player.PhysicsBody.Pos.y,
            State->Player.TextureData.SrcRect.width,
            State->Player.TextureData.SrcRect.height};
}

// TODO : Animation is currently not Framerate independant
void PlayAnimation(Entity *Entity)
{
    switch (Entity->EntityType)
    {
    case PLAYER:
    {
        switch (Entity->AnimationState)
        {

        case IDLE:
        {
            ++Entity->Animations[IDLE].FrameTime;
            if (Entity->Animations[IDLE].FrameTime >= Entity->Animations[IDLE].FrameDelay)
            {
                ++Entity->Animations[IDLE].CurrentFrame;
                Entity->Animations[IDLE].FrameTime = 0;
            }

            if (Entity->Animations[IDLE].CurrentFrame > Entity->Animations[IDLE].EndingFrame)
            {
                Entity->Animations[IDLE].CurrentFrame = Entity->Animations[IDLE].StartingFrame;
            }

            Entity->TextureData.SrcRect =
                {
                    real32((Entity->TextureData.Sprite.width / Entity->TextureData.SpriteLengthInFrames) * Entity->Animations[IDLE].CurrentFrame),
                    0,
                    real32(Entity->TextureData.Sprite.width / Entity->TextureData.SpriteLengthInFrames),
                    real32(Entity->TextureData.Sprite.height),
                };
            // TODO : See if we can put this anywhere neater.
            Entity->Animations[WALKING].CurrentFrame = Entity->Animations[WALKING].StartingFrame;
        }
        break;
        case WALKING:
        {
            ++Entity->Animations[WALKING].FrameTime;
            if (Entity->Animations[WALKING].FrameTime > Entity->Animations[WALKING].FrameDelay)
            {
                ++Entity->Animations[WALKING].CurrentFrame;
                Entity->Animations[WALKING].FrameTime = 0;
            }

            if (Entity->Animations[WALKING].CurrentFrame >= Entity->Animations[WALKING].EndingFrame)
            {
                Entity->Animations[WALKING].CurrentFrame = Entity->Animations[WALKING].StartingFrame;
            }

            Entity->TextureData.SrcRect =
                {
                    real32((Entity->TextureData.Sprite.width / Entity->TextureData.SpriteLengthInFrames) * Entity->Animations[WALKING].CurrentFrame),
                    0,
                    real32(Entity->TextureData.Sprite.width / Entity->TextureData.SpriteLengthInFrames),
                    real32(Entity->TextureData.Sprite.height),
                };
            Entity->Animations[IDLE].CurrentFrame = Entity->Animations[IDLE].StartingFrame;
        }
        break;
        case JUMPING:
        {
        }
        break;
        }
    }
    break;
    case ENEMY:
    {
        switch (Entity->AnimationState)
        {
        case IDLE:
        {
        }
        break;
        case WALKING:
        {
        }
        break;
        default:
        {
        }
        break;
        }
    }
    case MAPOBJECT:
    {
        switch (Entity->AnimationState)
        {
        case IDLE:
        {
        }
        break;
        default:
        {
        }
        break;
        }
    }
    break;
    }

    if (Entity->Flags.Flipped)
    {
        Entity->TextureData.SrcRect.width = -Entity->TextureData.SrcRect.width;
        Entity->TextureData.DstRect.width = -Entity->TextureData.SrcRect.width;
    }
    else
    {
        Entity->TextureData.SrcRect.width = Entity->TextureData.SrcRect.width;
        Entity->TextureData.DstRect.width = Entity->TextureData.SrcRect.width;
    }
}

internal void
HandleCamera(State *State)
{
#ifdef DEBUG
    const int PixelHeightPerScreen = 720;
#else
    const int PixelHeightPerScreen = 360;
#endif

    State->Camera.zoom = real32(GetScreenWidth() / PixelHeightPerScreen);
    State->Camera.offset = {real32(GetScreenWidth() / 2), real32(GetScreenHeight() / 2)};
    State->Camera.target = {State->Player.PhysicsBody.Pos.x, State->Player.PhysicsBody.Pos.y};
    State->Camera.rotation = 0;
}

internal void
HandleAnimationStateMachine(Entity *Entity)
{
    if (Vec2EqualsS(Entity->PhysicsBody.Vel, 0))
    {
        Entity->Flags.IsMoving = false;
        Entity->AnimationState = IDLE;
    }
    else
    {
        Entity->Flags.IsMoving = true;
        Entity->AnimationState = WALKING;
    }

    PlayAnimation(Entity);
    PlayAnimation();
}
// FIXME : Weird stutter sometimes? Look into performance profiling
const int MAPSCALE = 2;
const int TILESIZE = 16;

internal CollisionMap
LoadLevelData(const char *Filepath)
{

    CollisionMap Grid = {};

    FILE *File = fopen(Filepath, "rt");
    if (File == nullptr)
    {
        printf("Failed to load the file: %s\n", Filepath);
        return (Grid);
    }

    int RowCount = 0;
    int ColumnCount = 0;
    char Buffer[1024] = {};
    if (fgets(Buffer, 1024, File) != NULL)
    {
        for (int i = 0; Buffer[i] != '\0'; ++i)
        {
            if (Buffer[i] != ',')
            {
                ++ColumnCount;
            }
            ++RowCount;
        }
    }
    fseek(File, 0, SEEK_SET);

    Grid.CollisionData = (char **)malloc(RowCount * sizeof(char *));
    for (int i = 0; i < RowCount; ++i)
    {
        Grid.CollisionData[i] = (char *)malloc(ColumnCount * sizeof(char));
        memset(Grid.CollisionData[i], '\0', ColumnCount);

        if (fgets(Buffer, 1024, File) != NULL)
        {
            int k = 0;
            for (int j = 0; Buffer[j] != '\0'; ++j)
            {
                if (Buffer[j] != ',' && Buffer[j] != '\n')
                {
                    Grid.CollisionData[i][k++] = Buffer[j];
                }
            }
        }
    }
    fclose(File);

    Grid.MapSize = {RowCount, ColumnCount};
    return Grid;
}

internal Level
CreateLevel(State *State, int TextureIndex, const char *Filepath)
{
    Level Level;

    Level.LevelImage = State->Textures[TextureIndex];
    Level.CollisionMap =
        LoadLevelData(Filepath);
    Level.SrcRect =
        {0, 0, real32(Level.LevelImage.width), real32(Level.LevelImage.height)};
    Level.DstRect =
        {0, 0, real32(Level.SrcRect.width * MAPSCALE), real32(Level.SrcRect.height * MAPSCALE)};

    Level.IsLoaded = true;
    Level.Index = 1;
    Level.EntityCount = 0;
    Level.Entities = (Entity *)malloc(int(sizeof(Entity)) * Level.EntityCount);

    return (Level);
}

global_variable Color TestColor = {255, 0, 0, 50};

// TODO : Move the map related functions and data into a "MapManager.cpp"
// TODO : Look into a solution to only update this when needed, not every frame
internal void
DrawLevel(Level *Level)
{
#ifdef DEBUG
    for (int Row = 0; Row < Level->CollisionMap.MapSize.x; ++Row)
    {
        for (int Column = 0; Column < Level->CollisionMap.MapSize.y; ++Column)
        {
            int TileX = (Column)*TILESIZE;
            int TileY = (Row)*TILESIZE;
            switch (Level->CollisionMap.CollisionData[Row][Column])
            {
            case '0':
            {
                DrawRectangle(
                    TileX,
                    TileY,
                    TILESIZE,
                    TILESIZE,
                    RED);
            }
            break;
            case '1':
            {
                DrawRectangle(
                    TileX,
                    TileY,
                    TILESIZE,
                    TILESIZE,
                    ORANGE);
            }
            break;
            case '2':
            {
                DrawRectangle(
                    TileX,
                    TileY,
                    TILESIZE,
                    TILESIZE,
                    GREEN);
            }
            break;
            case '3':
            {
                DrawRectangle(
                    TileX,
                    TileY,
                    TILESIZE,
                    TILESIZE,
                    PINK);
            }
            break;
            case '4':
            {
                DrawRectangle(
                    TileX,
                    TileY,
                    TILESIZE,
                    TILESIZE,
                    PURPLE);
            }
            break;
            }
        }
    }
#endif

    Color MapTest = {255, 255, 255, 100};
    DrawTexturePro(
        Level->LevelImage,
        Level->SrcRect,
        Level->DstRect,
        {0, 0},
        0,
        MapTest);
}

// TODO : Terrible. Perhaps find a way to split these into more managable functions. Specifically for the Player.
//        That way the State can be modified without modifying the player

internal State
InitGameState(State *State)
{
    State->Textures = {};
    State->TextureCount = 2;
    State->Textures = (Texture2D *)malloc(int(sizeof(Texture2D)) * State->TextureCount);
    State->Textures[0] =
        LoadTexture("../data/res/textures/PlayerCharacter.png");
    State->Textures[1] =
        LoadTexture("../data/res/mapdata/Test/simplified/AutoLayers_advanced_demo/_composite.png");
    State->CurrentLevel =
        CreateLevel(State, 1, "../data/res/mapdata/Test/simplified/AutoLayers_advanced_demo/IntGrid_layer.csv");

    State->Player.TextureData.Sprite = State->Textures[0];
    State->Player.TextureData.SpriteLengthInFrames = 9;
    State->Player.Flags.Flipped = false;

    State->Player.EntityType = PLAYER;
    State->Player.AnimationState = IDLE;
    State->Player.AnimationCount = 2;
    State->Player.Animations = (Animation *)malloc(sizeof(Animation) * State->Player.AnimationCount);
    State->Player.Animations[IDLE] = CreateAnimation(0, 1, 0, 100, 96);
    State->Player.Animations[WALKING] = CreateAnimation(4, 8, 4, 20, 24);
    State->Player.Flags.IsMoving = false;

    State->Player.PhysicsBody.Pos = {0, 0};
    State->Player.PhysicsBody.Vel = {0, 0};
    State->Player.PhysicsBody.Rotation = 0;
    State->Player.MovementSpeed = 100.0f;
    State->Gravity = 2000.0f;
    State->Player.Scale = 0;

    return (*State);
}

internal void
CollisionManager(State *State)
{
    Rectangle Tile = {};
    for (int Row = 0; Row < State->CurrentLevel.CollisionMap.MapSize.x; ++Row)
    {
        for (int Column = 0; Column < State->CurrentLevel.CollisionMap.MapSize.y; ++Column)
        {
        }
    }
}

int main()
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello, Window");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(160);

    State State;
    InitGameState(&State);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        BeginMode2D(State.Camera);
        ClearBackground(BLACK);

        CollisionManager(&State);
        HandleCamera(&State);
        HandlePlayerInput(&State);
        HandleAnimationStateMachine((Entity *)(&State.Player));
        DrawFPS(0, 0);

        // TODO : Move this somewhere else, I don't want it drawing every frame, my poor gpu is going to die.
        DrawLevel(&State.CurrentLevel);

        DrawTexturePro(
            State.Player.TextureData.Sprite,
            State.Player.TextureData.SrcRect,
            State.Player.TextureData.DstRect,
            {0, State.Player.TextureData.DstRect.height},
            State.Player.PhysicsBody.Rotation,
            TestColor);

#ifdef DEBUG
        Color TestRed = {255, 0, 0, 100};
        DrawRectanglePro(State.Player.PhysicsBody.Hitbox, {0, 8}, 0, TestRed);
#endif
        EndMode2D();
        EndDrawing();
    }
    CloseWindow();
    return (0);
}
