#pragma once 

#include <cstdlib>
#include <cstdint>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#define global_variable static
#define local_persist static
#define internal static

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uin64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

typedef Vector2 v2;
typedef Vector3 v3;
typedef Vector4 v4;

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#include "Clone_Math.h"

// NOTE : yes I know Block Comments exist. Fuck you.
struct Input 
{
    struct Up 
    {
        bool Pressed;
        bool Down;
    }Up;

    struct Down 
    {
        bool Pressed;
        bool Down;
    }Down;

    struct Left
    {
        bool Pressed;
        bool Down;
    }Left;

    struct Right
    {
        bool Pressed;
        bool Down;
    }Right;

    struct Jump
    {
        bool Pressed;
        bool Down;
    }Jump;
    
    struct Attack 
    {
        bool Pressed;
        bool Down;
    }Attack;
};

// NOTE : Perhaps we could decide which entity type something is through a simple enum?
// For example, Set the enum to PLAYER to create a PLAYER Entity?
// Perhaps set it to GOBLIN to create a GOBLIN entity?

enum AnimationState 
{
    IDLE,
    WALKING,
    JUMPING,
};

struct Animation 
{
    int StartingFrame;
    int EndingFrame;
    int CurrentFrame;
    int FrameTime;
    int FrameDelay;
};    

// NOTE : Fuck you, everything is a box for now.
enum PhysicsBodyShape 
{
    BOX,
};

// TODO : Collision and Mask Layers.

struct PhysicsBody2D 
{
    int BodyID;
    v2 Pos;
    v2 Vel;
    real32 Rotation;
    bool Grounded;
    bool Gravitic;
    bool Enabled;

    // NOTE : Temp
    Rectangle Hitbox;
    PhysicsBodyShape PhysicsBodyShape;
};

// TODO : Potentially create subtypes that indicate what kind of ENEMY or MAPOPJECT something is
enum EntityType 
{
    PLAYER,
    ENEMY,
    MAPOBJECT,
};

struct EntityFlags 
{
    bool IsMoving;
    bool Flipped;
    bool IsOnGround;
};

struct TextureData 
{
    int SpriteLengthInFrames;
    Texture2D Sprite;
    Rectangle SrcRect;
    Rectangle DstRect;
    Rectangle Hitbox;
};

struct Entity 
{
    EntityFlags Flags;
    AnimationState AnimationState;
    EntityType EntityType;
    TextureData TextureData;
    PhysicsBody2D PhysicsBody;
    Animation *Animations;

    // TODO : Create an array of Animations, the size of this array will differ between entity archetypes
    //        but that's why we have substructs of entities. We can itterate through entities of the same type efficiently
    int AnimationCount;
    real32 MovementSpeed;
    real32 Scale;
};

struct Player : public Entity 
{
    Animation IdleAnimation;
    Animation WalkingAnimation;
};

// NOTE : This way currently doesn't let me place a destructable wall as part of the tileset since the IMAGE
// is what will be drawn as the level. BUT we can just make a hole in the map and fill it with an Entity that
// would be a destructible wall instead. Same for items like Torches, Chests, or Pots.

struct CollisionMap 
{
    v2i MapSize;
    char **CollisionData;
};

struct Level 
{
    Texture2D LevelImage;
    Rectangle SrcRect;
    Rectangle DstRect;
    Entity *Entities;

    CollisionMap CollisionMap;

    bool IsLoaded;
    int Index;
    int EntityCount;
};

// NOTE : Global State

struct State 
{
    int TextureCount;
    int LevelCount;
    real32 Gravity;
    Texture2D *Textures;
    Level CurrentLevel;
    
    Camera2D Camera;    
    Input Input;

    Player Player;
};
