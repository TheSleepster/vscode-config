#include "Intrinsics.h"
#include "Shiver.h"
#include "Shiver_AudioEngine.h"
#include "util/ShiverArray.h"

// Solving order
// - Do Caveman
// - Wait until performance :(
// - Fix with less Caveman
// - Unga bunga

// NOTE(Sleepster): The Simplex is no longer just 3 points since we are now using EPA for the distance and normal calculations
//                  Won't be a dynamic buffer, if you have more than 128 verts what the shit are you doing? Change this then.

// TODO(Sleepster): Make it so that instead of checking all 8 directions, we only check 4 (North, South, East, West)
internal void
ResolveSolidCollision(entity *A, gjk_epa_data Data)
{
    vec2 PenVector = Data.CollisionNormal * (real32)Data.Depth;
    A->Velocity = v2Reflect(A->Velocity, Data.CollisionNormal)*0.5f;
    
    if(Data.CollisionNormal.y == 1)
    {
        A->Position.y += -real32(EPSILON + Data.Depth);
    }
    else if(Data.CollisionNormal.y == -1)
    {
        A->Position.y += real32(EPSILON + Data.Depth);
    }
    
    if(Data.CollisionNormal.x == 1)
    {
        A->Position.x += -real32(EPSILON + Data.Depth);
    }
    else if(Data.CollisionNormal.x == -1)
    {
        A->Position.x += real32(EPSILON + Data.Depth);
    }
    
    A->Velocity = {0.0f, 0.0f};
}

// NOTE(Sleepster): This function updates both the simplex and the direction vector
internal bool
GJK_ComputeSimplexData(simplex *Simplex, vec2 *Direction)
{
    bool Result = 0;
    switch(Simplex->VertexCount)
    {
        // NOTE(Sleepster): One Vertex, Invalid
        case 1:
        {
            Assert(false, "Invalid code path, Simplex has one Point");
        }break;
        
        // NOTE(Sleepster): Two Vertices, find a third to create the triangle Simplex
        case 2:
        {
            vec2 A = Simplex->Vertex[1];
            vec2 B = Simplex->Vertex[0];
            vec2 AO = v2Invert(A);
            vec2 AB = B - A;
            if(v2Dot(AB, AO)) // Is Within bounds of Minkowski sum
            {
                vec2 NewDirection = v2Perp(AB);
                if(v2Dot(NewDirection, AO) < 0) // Wrong direction
                {
                    NewDirection = v2Invert(NewDirection);
                    Check(v2Dot(NewDirection, AO) >= 0, "Invalid code path\n");
                    // NOTE(Sleepster): Wind clockwise
                    Simplex->Vertex[0] = A;
                    Simplex->Vertex[1] = B;
                }
                *Direction = NewDirection;
            }
            else
            {
                // NOTE(Sleepster): Readjust the direction and try again.
                Simplex->Vertex[0] = A;
                Simplex->VertexCount = 1;
                *Direction = AO;
            }
        }break;
        
        // NOTE(Sleepster): The Triangle Simplex Created, Tested below.
        case 3:
        {
            vec2 A = Simplex->Vertex[2];
            vec2 B = Simplex->Vertex[1];
            vec2 C = Simplex->Vertex[0];
            vec2 AO = v2Invert(A);
            vec2 AB = B - A;
            vec2 AC = C - A;
            // NOTE(Sleepster): Check for Clockwise Winding
            if(v2Dot(v2Perp(AC), AB) > 0)
            {
                Trace("Invalid code path, if(v2Dot(v2Perp(AC), AB) > 0) on LN 118\n");
                B = Simplex->Vertex[0];
                C = Simplex->Vertex[1];
                
                vec2 Temp = AB;
                AB = AC;
                AC = Temp;
                Assert(v2Dot(v2Perp(AB), AC) > 0, "What?\n");
            }
            // NOTE(Sleepster): Checks
            vec2 ACPerp = v2Perp(AC);
            vec2 ABPerp = v2Invert(v2Perp(AB));
            
            bool OnLeft = (v2Dot(ACPerp, AO) > 0);
            bool OnRight = (v2Dot(ABPerp, AO) > 0);
            bool SimpleCase = 0;
            
            // NOTE(Sleepster): Determine which direction to check relative to the origin
            if(!OnRight && !OnLeft)
            {
                Result = 1;
                break;
            }
            else if(OnLeft)
            {
                Assert(!OnRight, "On right as well?\n");
                // NOTE(Sleepster): If it is on the left, than we have gone to far from the origin. Readjust and try again.
                if(v2Dot(AC, AO) > 0)
                {
                    *Direction = ACPerp;
                    // NOTE(Sleepster): Swap the first and third vertices and try again
                    Simplex->Vertex[0] = C;
                    Simplex->Vertex[1] = A;
                    Simplex->VertexCount = 2;
                }
                else
                {
                    SimpleCase = 1;
                }
            }
            else if(OnRight)
            {
                Assert(!OnLeft, "On left as well?\n");
                if(v2Dot(AB, AO) > 0)
                {
                    *Direction = ABPerp;
                    // NOTE(Sleepster): Swap the first and second vertices and try again.
                    Simplex->Vertex[1] = B;
                    Simplex->Vertex[0] = A;
                    Simplex->VertexCount = 2;
                }
                else
                {
                    SimpleCase = 1;
                }
            }
            if(SimpleCase)
            {
                // NOTE(Sleepster): Failure. Restart completely.
                Simplex->Vertex[0] = A;
                Simplex->VertexCount = 1;
                *Direction = AO;
            }
        }break;
        default:
        {
            // NOTE(Sleepster): Something went SERIOUSLY Wrong
            Assert(false, "How?\n");
        }break;
    }
    return(Result);
}

internal bool32
GJK_FurthestPoint(entity *Entity, vec2 Direction)
{
    int BestIndex = 0;
    real32 MaxProduct = v2Dot(Entity->Vertex[0], Direction);
    vec2 Result = {};
    for(int Point = 1;
        Point < Entity->VertexCount;
        ++Point)
    {
        real32 Product = v2Dot(Entity->Vertex[Point], Direction);
        if(Product > MaxProduct)
        {
            MaxProduct = Product;
            BestIndex = Point;
        }
    }
    return(BestIndex);
}

internal vec2
GJK_Support(entity *A, entity *B, vec2 Direction)
{
    // NOTE(Sleepster): Find the most extreme point on each entity
    int AMax = GJK_FurthestPoint(A, Direction);
    int BMax = GJK_FurthestPoint(B, -Direction);
    
    return(A->Vertex[AMax] - B->Vertex[BMax]);
}

internal gjk_data
HandleGJK(entity *A, entity *B)
{
    bool Result = {};
    gjk_data GJKData = {};
    // NOTE(Sleepster): Early return if there is a full shape.
    if(A->VertexCount < 3 || B->VertexCount < 3) return(GJKData);
    
    // NOTE(Sleepster): Initial Point to start with. Initial direction does not matter
    vec2 iSupportPoint = GJK_Support(A, B, {1, 0});
    
    simplex Simplex = {};
    Simplex.Vertex[0] = iSupportPoint;
    Simplex.VertexCount = 1;
    // NOTE(Sleepster): Invert Initial direction to begin the search
    vec2 Direction = v2Invert(iSupportPoint);
    for(;;)
    {
        vec2 Point = GJK_Support(A, B, Direction);
        // NOTE(Sleepster): Early exit, Collision cannot occur
        if(v2Dot(Point, Direction) < 0)
        {
            Result = 0;
            break;
        }
        
        Simplex.Vertex[Simplex.VertexCount++] = Point;
        dAssert(Simplex.VertexCount >= 2 && Simplex.VertexCount <= 3); // control our vertices here
        if(GJK_ComputeSimplexData(&Simplex, &Direction))
        {
            Result = 1;
            break;
        }
    }
    
    GJKData.Collision = Result;
    GJKData.Simplex = Simplex;
    return(GJKData);
}

internal void
EPA_AddToSimplex(simplex *Simplex, vec2 Point, int32 InsertionIndex)
{
    if(Simplex->VertexCount >= ArrayCount(Simplex->Vertex))
    {
        Assert(false, "Simplex has grown too large to handle\n");
    }
    for(int32 VertexIndex = Simplex->VertexCount - 1;
        VertexIndex >= InsertionIndex;
        --VertexIndex)
    {
        Simplex->Vertex[VertexIndex + 1] = Simplex->Vertex[VertexIndex];
    }
    
    ++Simplex->VertexCount;
    Simplex->Vertex[InsertionIndex] = Point;
}

// NOTE(Sleepster): Expanded Polytope Algorithm
internal gjk_epa_data
GJK_EPA(entity *A, entity *B)
{
    gjk_epa_data CollisionInfo = {};
    gjk_data GJKInfo = HandleGJK(A, B);
    CollisionInfo.Collision = GJKInfo.Collision;
    if(GJKInfo.Collision)
    {
        simplex *Simplex = &GJKInfo.Simplex;
        for(;;)
        {
            // NOTE(Sleepster): Get the edge closest to the origin of our Minkowski difference,
            //                  Assume Clockwise
            epa_edge Edge = {};
            for(int32 VertexIndex = 0;
                VertexIndex < Simplex->VertexCount;
                ++VertexIndex)
            {
                int32 IndexA = VertexIndex;
                int32 IndexB = (VertexIndex == (Simplex->VertexCount - 1)) ? 0 : VertexIndex + 1; // Wrap the index;
                
                vec2 PointA = Simplex->Vertex[IndexA];
                vec2 PointB = Simplex->Vertex[IndexB];
                vec2 AO = v2Invert(PointA);
                vec2 AB = {PointB.x - PointA.x, PointB.y - PointA.y};
                
                // NOTE(Sleepster): This the edge normal that points AWAY from the origin when the simplex is clockwise oriented
                vec2 EdgeNormal = v2Normalize(v2Perp(AB));
                if(v2Dot(EdgeNormal, AO) > 0)
                {
                    EdgeNormal = v2Invert(EdgeNormal);
                }
                
                // NOTE(Sleepster): Find the Distance from the edge. The closest distance is always the perpindicular distance from the edge.
                real32 Distance = v2Dot(EdgeNormal, AO);
                
                Assert(Distance <= 0, "Distance is not less than zero\n");
                Distance *= -1; // Invert Distance for new search
                Assert(Distance >= 0, "Distance is not greater than zero\n");
                
                if(Distance < Edge.Distance || VertexIndex == 0)
                {
                    // NOTE(Sleepster): Shorter edge found
                    Edge.Distance = Distance;
                    Edge.Index = IndexB;
                    Edge.Normal = EdgeNormal;
                }
            }
            vec2 EPAPoint = GJK_Support(A, B, Edge.Normal);
            real64 FloatDistance = v2Dot(EPAPoint, Edge.Normal);
            
            // NOTE(Sleepster): See if these points are the same as previously acquired ones. If they are, we have our solution
            if(FloatDistance - Edge.Distance < GJK_TOLERANCE)
            {
                // NOTE(Sleepster): Invert the edge normal so that it is pointing towards our Minkowski Difference's origin
                CollisionInfo.CollisionNormal = Edge.Normal;
                CollisionInfo.Depth = FloatDistance;
                break;
            }
            else
            {
                // NOTE(Sleepster): If we're here, we haven't reached the edge of our Minkowski Difference.
                //                  Continue by adding points to the simplex in between the two points that made the closest edge.
                EPA_AddToSimplex(&GJKInfo.Simplex, EPAPoint, Edge.Index + 1);
            }
        }
    }
    return(CollisionInfo);
}

internal bool
GJK(entity *A, entity *B)
{
    gjk_data Result = HandleGJK(A, B);
    return(Result.Collision);
}

internal void
UpdateEntityColliderData(entity *Entity)
{
    Entity->Vertex[TOP_LEFT]     = {Entity->Position};
    Entity->Vertex[TOP_RIGHT]    = {Entity->Position.x + Entity->Size.x, Entity->Position.y};
    Entity->Vertex[BOTTOM_LEFT]  = {Entity->Position.x, Entity->Position.y + Entity->Size.y};
    Entity->Vertex[BOTTOM_RIGHT] = {Entity->Position + Entity->Size};
}
// NOTE(Sleepster): END OF GJK

// TODO(Sleepster): Make it so that we are not making a new sprite every frame, only when we need to actually make it
internal entity
CreateEntity(static_sprites SpriteID, vec2 Position, vec2 Size, glrenderdata *RenderData, gamestate *State)
{
    entity Entity = {};
    switch(SpriteID)
    {
        case SPRITE_DICE:
        {
            Entity.Flags = IS_ACTOR|IS_ACTIVE;
            Entity.Restitution = 0.0f;
            Entity.Mass = 100.0f;
            Entity.InvMass = 1 / Entity.Mass;
            Entity.Speed = 0.5f;
        }break;
        case SPRITE_FLOOR:
        {
            Entity.Flags = IS_TILE|IS_ACTIVE;
            Entity.Restitution = 0.0f;
            Entity.Mass = 10.0f;
            Entity.InvMass = 1 / Entity.Mass;
        }break;
        case SPRITE_WALL:
        {
            Entity.Flags = IS_SOLID|IS_ACTIVE;
            Entity.Restitution = 0.0f;
            Entity.Mass = 10.0f;
            Entity.InvMass = 1 / Entity.Mass;
        }break;
    }
    
    Entity.SpriteID = SpriteID;
    Entity.Sprite = RenderData->StaticSprites[SpriteID];
    Entity.Position = Position;
    Entity.Size = Size;
    
    Entity.Vertex[TOP_LEFT]     = {Entity.Position};
    Entity.Vertex[TOP_RIGHT]    = {Entity.Position.x + Entity.Size.x, Entity.Position.y};
    Entity.Vertex[BOTTOM_LEFT]  = {Entity.Position.x, Entity.Position.y + Entity.Size.y};
    Entity.Vertex[BOTTOM_RIGHT] = {Entity.Position + Entity.Size};
    Entity.VertexCount = 4;
    
    State->CurrentEntityCount++;
    return(Entity);
}

internal inline void
DrawEntityStaticSprite2D(entity Entity, glrenderdata *RenderData)
{
    sh_glDrawStaticSprite2D(Entity.SpriteID, Entity.Position, iv2Cast(Entity.Size), 0.0f, RenderData);
}

internal inline entity
DeleteEntity(void)
{
    entity Entity = {};
    return(Entity);
}

internal void
DrawTilemap(const uint8 Tilemap[TILEMAP_SIZE_Y][TILEMAP_SIZE_X], glrenderdata *RenderData, gamestate *State)
{
    for(int32 Row = 0;
        Row < TILEMAP_SIZE_Y;
        ++Row)
    {
        for(int32 Column = 0;
            Column < TILEMAP_SIZE_X;
            ++Column)
        {
            switch(Tilemap[Row][Column])
            {
                case 0:
                {
                    State->Entities[State->CurrentEntityCount] =
                        CreateEntity(SPRITE_FLOOR, {real32(Column * TILESIZE), real32(Row * TILESIZE)}, {16, 16}, RenderData, State);
                }break;
                case 1:
                {
                    State->Entities[State->CurrentEntityCount] =
                        CreateEntity(SPRITE_WALL, {real32(Column * TILESIZE), real32(Row * TILESIZE)}, {16, 16}, RenderData, State);
                }break;
            }
        }
    }
}

internal void
UpdatePlayerPosition(gamestate *State, time Time)
{
    entity *Player = &State->Entities[1];
    
    if(IsGameKeyDown(MOVE_UP, &State->GameInput))
    {
        Player->Acceleration.y = -1.0f * Time.DeltaTime;
    }
    
    else if(IsGameKeyDown(MOVE_DOWN, &State->GameInput))
    {
        Player->Acceleration.y = 1.0f * Time.DeltaTime;
    }
    
    if(IsGameKeyDown(MOVE_LEFT, &State->GameInput))
    {
        Player->Acceleration.x = -1.0f * Time.DeltaTime;
    }
    
    else if(IsGameKeyDown(MOVE_RIGHT, &State->GameInput))
    {
        Player->Acceleration.x = 1.0f * Time.DeltaTime;
    }
    
    if(Player->Acceleration.x != 0 && Player->Acceleration.y != 0)
    {
        Player->Acceleration *= 0.77f;
    }
    
    vec2 OldPlayerP = Player->Position;
    vec2 OldPlayerV = Player->Velocity;
    
    // NOTE(Sleepster): ODE here for friction
    Player->Acceleration += -(0.05f * Player->Velocity);
    
    Player->Position = (0.5f * (Player->Acceleration * Square(Time.DeltaTime)) + (Player->Velocity * Time.DeltaTime) + OldPlayerP);
    Player->Velocity = (Player->Speed * (Player->Acceleration * Time.DeltaTime)) + OldPlayerV;
    Player->Position = v2Lerp(Player->Position, OldPlayerP, Time.DeltaTime);
}

extern "C"
GAME_ON_AWAKE(GameOnAwake)
{
    AudioSubsystem = AudioEngineIn;
    const uint8 Tilemap[TILEMAP_SIZE_Y][TILEMAP_SIZE_X] =
    {
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 1,1,1,0, 0,1,1,0, 0,0,0,0, 1},
        {1,0,0,0, 1,0,0,0, 0,0,1,0, 0,0,0,0, 1},
        {1,0,0,0, 1,0,0,0, 0,0,1,0, 0,0,0,0, 1},
        {1,0,0,0, 1,0,0,0, 0,0,1,0, 0,0,0,0, 1},
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1}
    };
    
    sh_glLoadSpriteSheet(RenderData);
    
    State->CurrentEntityCount = 0;
    State->Entities[0] = {};
    State->Entities[1] = CreateEntity(SPRITE_DICE, {160, 30}, {16, 16}, RenderData, State);
    DrawTilemap(Tilemap, RenderData, State);
    
    // NOTE(Sleepster): If we ware playing a background track, hot reloading is dead
    //sh_FMODPlaySoundFX(AudioSubsystem->SoundFX[TEST_MUSIC]);
    //sh_PlayBackgroundTrack(SUNKEN_SEA);
}



// NOTE(Sleepster): Make a function that adds to the players speed in the fixed time step to try and fix this weird collision jank
// The idea is that this will add to the velocity here, independant of the framerate so that the player will move the same regardless
// of framerate, but the main frame-indepenent update function will still keep the position correct, potentially fixing our collider
// problems
extern "C"
GAME_FIXED_UPDATE(GameFixedUpdate)
{
    // NOTE(Sleepster): For some reason a "for" loop being present within this fixedupdate causes the game to break
    UpdatePlayerPosition(State, Time);
}

extern "C"
GAME_UPDATE_AND_RENDER(GameUnlockedUpdate)
{
    RenderData->Cameras[CAMERA_GAME].Position = {130, -60};
    
    for(int32 EntityIndex = 2;
        EntityIndex <= State->CurrentEntityCount;
        ++EntityIndex)
    {
        if((State->Entities[EntityIndex].Flags & IS_SOLID))
        {
            gjk_epa_data CollisionData = GJK_EPA(&State->Entities[1], &State->Entities[EntityIndex]);
            if(CollisionData.Collision)
            {
                ResolveSolidCollision(&State->Entities[1], CollisionData);
                sh_PlaySound("boop");
            }
        }
    }
    
    
    for(int32 EntityIndex = 1;
        EntityIndex <= State->CurrentEntityCount;
        ++EntityIndex)
    {
        UpdateEntityColliderData(&State->Entities[EntityIndex]);
    }
    
    // NOTE(Sleepster): Move this out of here? Seems a little odd the game would be doing the rendering
    for(int32 EntityIndex = 1;
        EntityIndex <= State->CurrentEntityCount;
        ++EntityIndex)
    {
        DrawEntityStaticSprite2D(State->Entities[EntityIndex], RenderData);
    }
}
