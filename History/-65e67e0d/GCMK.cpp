#include "quasar_Standards.h"


struct gameState 
{
    enum state 
    {
        menu,
        game,
        restart,
        end,
    };
};

struct Flags 
{
    bool isAlive;
    bool splittable;
};

struct ColliderData 
{
    Rectangle boxCollider;
    bool isAlive;
};

struct RenderInfo 
{
    Rectangle srcRect;
    Rectangle dstRect;
};

struct Projectile 
{
    Flags flags;
    vec2 pos;
    vec2 vel;
    real32 rotation;
    int16 lifeTime;
};

struct PlayerData 
{
    Texture2D sprite;
    Flags flags;
    ColliderData collider;
    
    vec2 pos;
    vec2 vel;
    real32 accel;
    real32 friction;
    real32 rotation;
    real32 deltaR;
    int maxSpeed;
    
    int maxHealth;
    int currentHealth;
};

struct AsteroidData 
{
    Texture2D sprite;
    Flags flags;
    ColliderData collider;
    
    vec2 pos;
    vec2 vel;
    real32 rotation;
    real32 deltaR;
    
    int scale;
    int maxHealth;
    int currentHealth;
};

global_variable Texture2D playerTexture1;
global_variable Texture2D asteroidTexture1;
global_variable Texture2D asteroidTexture2;
global_variable Texture2D asteroidTexture3;
global_variable Texture2D asteroidTexture4;

global_variable gameState gameState;

// NOTE : PLAYER HANDLING
global_variable PlayerData *player = nullptr;

internal PlayerData *initPlayerData() 
{
    PlayerData *dummy = (PlayerData*)malloc(sizeof(struct PlayerData));
    
    dummy->flags.isAlive = true;
    dummy->sprite = playerTexture1;
    dummy->pos = {200, 300};
    dummy->vel = {0, 0};
    dummy->accel = 0.2f;
    dummy->friction = 0.02f;
    dummy->rotation = 0;
    dummy->deltaR = 0;
    dummy->maxSpeed = 8;
    dummy->maxHealth = 5;
    dummy->currentHealth = 5;
    
    return(dummy);
}

vec2 handlePlayerMovement() 
{
    // NOTE : Vertical movement checks
    
    if(IsKeyDown(KEY_W)) 
    {
        if(player->vel.y >= -player->maxSpeed) 
        {
            player->vel.y += -player->accel;
        }
        else if(player->vel.y <= -player->maxSpeed) 
        {
            player->vel.y = (real32)-player->maxSpeed;
        }
    }
    
    else if(IsKeyDown(KEY_S)) 
    {
        if(player->vel.y <= player->maxSpeed) 
        {
            player->vel.y += player->accel;
        }
        else if(player->vel.y >= player->maxSpeed) 
        {
            player->vel.y = (real32)player->maxSpeed;
        }
    }
    
    else 
    {
        if(player->vel.y > 0) 
        {
            player->vel.y += -player->accel;
            
            if(player->vel.y < 0) 
            {
                player->vel.y = 0;
            }
        }
        
        if(player->vel.y < 0) 
        {
            player->vel.y += player->accel;
            
            if(player->vel.y > 0) 
            {
                player->vel.y = 0;
            }
        }
    }
    
    // NOTE : Horizontal movement checks
    
    if(IsKeyDown(KEY_A)) 
    {
        if(player->vel.x >= -player->maxSpeed) 
        {
            player->vel.x += -player->accel;
        }
        else if(player->vel.x <= -player->maxSpeed) 
        {
            player->vel.x = (real32)-player->maxSpeed;
        }
    }
    
    else if(IsKeyDown(KEY_D)) 
    {
        if(player->vel.x <= player->maxSpeed) 
        {
            player->vel.x += player->accel;
        }
        else if(player->vel.x >= player->maxSpeed) 
        {
            player->vel.x = (real32)player->maxSpeed;
        }
    }
    
    else 
    {
        if(player->vel.x > 0) 
        {
            player->vel.x -= player->accel;
            
            if(player->vel.x < 0) 
            {
                player->vel.x = 0;
            }
        }
        
        if(player->vel.x < 0) 
        {
            player->vel.x += player->accel;
            
            if(player->vel.x > 0) 
            {
                player->vel.x = 0;
            }
        }
    }
    
    Vector2Normalize(player->vel);
    
    player->pos.x += player->vel.x;
    player->pos.y += player->vel.y;
    
    return(player->pos);
}

void handlePlayerData() 
{
    if(player == nullptr) 
    {
        player = initPlayerData();
    } 
    
    if(player->currentHealth <= 0) 
    {
        player->flags.isAlive = false;
        player = nullptr;
    } 
    else 
    {
        RenderInfo renderInfo;
        vec2 oldPlayerPos = player->pos;
        vec2 newPlayerPos = handlePlayerMovement();
        
        renderInfo.srcRect = {0, 0, (real32)player->sprite.width, (real32)player->sprite.height};
        renderInfo.dstRect=  
        {newPlayerPos.x, newPlayerPos.y, (real32)player->sprite.width * 5, (real32)player->sprite.height * 5};
        
        // NOTE : Edge Collision Handling
        if(player->pos.x > WINDOW_WIDTH - 32)
        {
            player->pos.x = 0;
        }
        
        else if(player->pos.x < 0) 
        {
            player->pos.x = WINDOW_WIDTH - 32;
        }
        
        if(player->pos.y > WINDOW_HEIGHT) 
        {
            player->pos.y = 0;
        }
        
        else if(player->pos.y < 0) 
        {
            player->pos.y = WINDOW_HEIGHT - 32;
        }
        
        // NOTE : Rotation stuffs
        player->deltaR = 2;
        
        EntityType entityType;
        entityType = playerType;
        
        player->rotation += handleRotation(playerType, player->deltaR);
        
        // NOTE : Collider Data
        player->collider.boxCollider = 
        {
            renderInfo.dstRect.x - (renderInfo.dstRect.width / 4), 
            renderInfo.dstRect.y - (renderInfo.dstRect.height / 4), 
            renderInfo.dstRect.width / 2, 
            renderInfo.dstRect.height / 2
        };
        player->collider.isAlive = true;
#ifdef DEBUG
        DrawRectangleRec(player->collider.boxCollider, RED);
#endif
        DrawTexturePro
        (
         player->sprite, 
         renderInfo.srcRect, 
         renderInfo.dstRect, 
         {42.5, 42.5},
         player->rotation, 
         WHITE
         );
    }
}

// NOTE : Collision checking between the player and the Asteroids

bool collisionChecking(Rectangle PlayerCollider, Rectangle AsteroidCollider)
{
    bool areColliding;
    areColliding = CheckCollisionRecs(PlayerCollider, AsteroidCollider);
    
    return(areColliding);
}

// NOTE : ASTEROID HANDING

const int32 MaxEntities = 100;

global_variable AsteroidData *Asteroids[MaxEntities] = {};
global_variable int AsteroidCount = 4;

AsteroidData *createAsteroid() 
{
    AsteroidData *dummyAsteroid = (AsteroidData*)malloc(sizeof( struct AsteroidData));
    int textureSeed = GetRandomValue(0, 3);
    
    switch(textureSeed) 
    {
        case 0:
        dummyAsteroid->sprite = asteroidTexture1;
        break;
        case 1:
        dummyAsteroid->sprite = asteroidTexture2; 
        break;
        case 2:
        dummyAsteroid->sprite = asteroidTexture3; 
        break;
        default:
        dummyAsteroid->sprite = asteroidTexture4; 
        break;
    };
    
    dummyAsteroid->flags.isAlive = true; 
    dummyAsteroid->flags.splittable = true;
    dummyAsteroid->pos = {(real32)(GetRandomValue(1, 1280)), (real32)GetRandomValue(1, 720)};
    dummyAsteroid->vel = {(real32)GetRandomValue(1, 4), (real32)GetRandomValue(1, 4)};
    dummyAsteroid->rotation = 0;
    dummyAsteroid->deltaR = 0;
    dummyAsteroid->scale = 5;
    dummyAsteroid->maxHealth = 2;
    dummyAsteroid->currentHealth = 2;
    
    return(dummyAsteroid);
}

void handleAsteroidData() 
{
    for(int i = 0; i <= AsteroidCount; i++) 
    {
        if(Asteroids[i] == nullptr) 
        {
            Asteroids[i] = createAsteroid();
            printf("Asteroid Created!: %i\n", i);
        }
        else 
        {
            // NOTE : Position Data
            
            if(i < AsteroidCount/2) 
            {
                Asteroids[i]->pos.x += Asteroids[i]->vel.x; 
                Asteroids[i]->pos.y += Asteroids[i]->vel.y;
            }
            else 
            {
                Asteroids[i]->pos.x += -Asteroids[i]->vel.x;
                Asteroids[i]->pos.y += -Asteroids[i]->vel.y;
            }
            
            // NOTE : Rotation Data
            
            if(Asteroids[i]->deltaR == 0) 
            {
                Asteroids[i]->deltaR = (real32)GetRandomValue(-2, 2);
            }
            
            if(i < AsteroidCount/2) 
            {
                Asteroids[i]->rotation += handleRotation(objectType, Asteroids[i]->deltaR);
            }
            else 
            {
                Asteroids[i]->rotation -= handleRotation(objectType, Asteroids[i]->deltaR);
            }
            
            // NOTE : Rendering Data
            
            RenderInfo renderInfo;
            
            renderInfo.srcRect = 
            {0, 0, (real32)Asteroids[i]->sprite.width, (real32)Asteroids[i]->sprite.height};
            
            renderInfo.dstRect = 
            {
                Asteroids[i]->pos.x, 
                Asteroids[i]->pos.y, 
                renderInfo.srcRect.width * Asteroids[i]->scale,
                renderInfo.srcRect.height * Asteroids[i]->scale
            };
            
            // NOTE : Collider Data
            
            Asteroids[i]->collider.boxCollider = 
            {
                renderInfo.dstRect.x - (renderInfo.dstRect.width / 2), 
                renderInfo.dstRect.y - (renderInfo.dstRect.height / 2), 
                renderInfo.dstRect.width, 
                renderInfo.dstRect.height
            };
#ifdef DEBUG
            DrawRectangleRec(Asteroids[i]->collider.boxCollider, RED);
#endif
            
            Asteroids[i]->collider.isAlive = true;
            
            // NOTE : Edge collision checking
            
            if(Asteroids[i]->collider.boxCollider.x  + (renderInfo.dstRect.width) > 1280) 
            {
                Asteroids[i]->vel.x *= -1;
            }
            else if(Asteroids[i]->collider.boxCollider.x < 0) 
            {
                Asteroids[i]->vel.x *= -1;
            }
            
            if(Asteroids[i]->collider.boxCollider.y > 720) 
            {
                Asteroids[i]->vel.y *= -1;
            } 
            else if(Asteroids[i]->collider.boxCollider.y < 0) 
            {
                Asteroids[i]->vel.y *= -1;
            }  
            
            // NOTE : Health check (deleted if it's dead);
            
            if(Asteroids[i]->currentHealth <= 0) 
            {
                Asteroids[i]->flags.isAlive = false;
                Asteroids[i]->collider.isAlive = false;
            }
            
            // NOTE : Drawing & collision with player 
            
            // TODO: Maybe take a closer look at improving this collision. Seems a little wonky with just
            // this method.
            // WARNING : This might cause a crash later. Deallocation is scary
            const real32 iFrameInterval = 75;
            local_persist real32 collisionTimer = 0;
            
            if(Asteroids[i]->flags.isAlive) 
            {
                if(collisionChecking(player->collider.boxCollider, Asteroids[i]->collider.boxCollider)) 
                {
                    if(collisionTimer >= iFrameInterval) 
                    {
                        Asteroids[i]->vel.x *= -1;
                        Asteroids[i]->vel.y *= -1;
                        
                        Asteroids[i]->currentHealth --;
                        player->currentHealth--;
                        
                        collisionTimer = 0;
                    }
                    collisionTimer ++;
                }
                
                DrawTexturePro
                (
                 Asteroids[i]->sprite, 
                 renderInfo.srcRect, 
                 renderInfo.dstRect, 
                 {42.0f, 42.0f},
                 Asteroids[i]->rotation,
                 WHITE
                 );
            }
            else 
            {
                free((void*)Asteroids[i]);
                Asteroids[i] = nullptr;
            }
        }
    } 
}

const int maxBullets = 100;
const real32 bulletSpeed = 25.0f;
global_variable int bulletCount = 0;
Projectile *bullets[maxBullets] = {};

global_variable int fireRate = 0;
global_variable int maxFireRate = 30;

Projectile *CreateBullet() 
{
    Projectile *dummy = (Projectile*)malloc(sizeof(Projectile));
    
    dummy->pos = player->pos;
    dummy->rotation = player->rotation;
    dummy->vel.x = (real32)(sin(toRadian(player->rotation)) * bulletSpeed);
    dummy->vel.y = (real32)(-cos(toRadian(player->rotation)) * bulletSpeed);
    dummy->flags.isAlive = true;
    dummy->lifeTime = 1000;
    
    bulletCount++;
    return(dummy);
}

void handleProjectileData() 
{
    fireRate++;
    
    if(IsKeyPressed(KEY_SPACE) && fireRate >= maxFireRate) 
    {
        if(bulletCount >= maxBullets) 
        {
            bulletCount = 0;
        }
        
        bullets[bulletCount] = CreateBullet();
        fireRate = 0;
        bulletCount++;
    }
    
    for(int i = 0; i < maxBullets; i++) 
    {
        if(bullets[i] != nullptr) 
        {
            bullets[i]->pos.x += bullets[i]->vel.x;
            bullets[i]->pos.y += bullets[i]->vel.y;
            
            DrawCircleV(bullets[i]->pos, 5, BLUE);
            bullets[i]->lifeTime -= 1;
            
            if(bullets[i]->lifeTime <= 0) 
            {
                bullets[i]->flags.isAlive = false;
                free((void*)bullets[i]);
                bullets[i] = nullptr;
                bulletCount--;
            }
            
            for(int j = 0; j <= AsteroidCount; j++) 
            {
                if(Asteroids[j] != nullptr && bullets[i] != nullptr) 
                {
                    if(CheckCollisionCircleRec(bullets[i]->pos, 5, Asteroids[j]->collider.boxCollider)) 
                    {
                        Asteroids[j]->currentHealth--;
                        
                        bullets[i]->flags.isAlive = false;
                        free((void*)bullets[i]);
                        bullets[i] = nullptr;
                        bulletCount--;
                    } 
                }
            }
        }
    }
}

int main() 
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello, Window!");  
    SetTargetFPS(60);
    
    playerTexture1 = 
        createTextureFromImageFile("../data/res/textures/playerSprite.png"); 
    
    asteroidTexture1 = 
        createTextureFromImageFile("../data/res/textures/asteroidtexture1.png");
    
    asteroidTexture2 = 
        createTextureFromImageFile("../data/res/textures/asteroidtexture2.png");
    
    asteroidTexture3 = 
        createTextureFromImageFile("../data/res/textures/asteroidtexture3.png");
    
    asteroidTexture4 = 
        createTextureFromImageFile("../data/res/textures/asteroidtexture4.png");
    
    while(!WindowShouldClose()) 
    {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawFPS(0, 0);
        
        handlePlayerData();
        handleAsteroidData();
        handleProjectileData();
        
        EndDrawing();
    }
    
    CloseWindow();
    return(0);
}
