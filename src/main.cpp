#include <SDL2/SDL.h>
#include <box2d/box2d.h>
#include <iostream>
#include <cstdio>
#include <cmath>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const float PIXELS_PER_METER = 20.0f;

// Convert Box2D coordinates to screen coordinates
int worldToScreenX(float x) {
    return static_cast<int>(x * PIXELS_PER_METER);
}

int worldToScreenY(float y) {
    return SCREEN_HEIGHT - static_cast<int>(y * PIXELS_PER_METER);
}

// Draw a colored rectangle with rotation
// x, y: center position
// width, height: rectangle dimensions
// angle: rotation angle in radians
// r, g, b, a: color components (0-255)
void drawRotatedRect(SDL_Renderer* renderer, 
                     float x, float y, float width, float height, 
                     float angleDegrees, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    // Create white texture once (static so it persists between calls)
    static SDL_Texture* whiteTexture = nullptr;
    if (!whiteTexture) {
        whiteTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, 
                                        SDL_TEXTUREACCESS_TARGET, 1, 1);
        SDL_SetTextureBlendMode(whiteTexture, SDL_BLENDMODE_BLEND);
        SDL_SetRenderTarget(renderer, whiteTexture);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, nullptr);
    }
    
    SDL_Rect dstRect = {
        static_cast<int>(x - width / 2.0f),
        static_cast<int>(y - height / 2.0f),
        static_cast<int>(width),
        static_cast<int>(height)
    };
    
    // Set texture color modulation
    SDL_SetTextureColorMod(whiteTexture, r, g, b);
    SDL_SetTextureAlphaMod(whiteTexture, a);
    
    // Render with rotation around center
    SDL_Point center = {static_cast<int>(width / 2.0f), static_cast<int>(height / 2.0f)};
    SDL_RenderCopyEx(renderer, whiteTexture, nullptr, &dstRect, angleDegrees, &center, SDL_FLIP_NONE);
}

// Simple Box2D Demo with one falling block
int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Alien Game Demo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create Box2D world with gravity
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, -9.8f};
    b2WorldId worldId = b2CreateWorld(&worldDef);

    // Create ground body (static)
    b2BodyDef groundBodyDef = b2DefaultBodyDef();
    groundBodyDef.position = {SCREEN_WIDTH / PIXELS_PER_METER / 2.0f, 1.0f};
    groundBodyDef.type = b2_staticBody;
    // types of bodies: static, dynamic, kinematic
    // static bodies are not affected by gravity and are used for static objects
    // dynamic bodies are affected by gravity and are used for moving objects
    // kinematic bodies are not affected by gravity and are used for objects that are moved by the player
    b2BodyId groundBodyId = b2CreateBody(worldId, &groundBodyDef);

    // Create ground box shape
    b2Polygon groundBox = b2MakeBox(SCREEN_WIDTH / PIXELS_PER_METER / 2.0f, 1.0f);
    b2ShapeDef groundShapeDef = b2DefaultShapeDef();
    groundShapeDef.enableContactEvents = true;  // Enable collision detection
    //what are thing that can be done to groundShapeDef?
    // - set the density of the shape
    // - set the friction of the shape
    // - set the restitution of the shape
    // - set the sensor of the shape
    // - set the filter data of the shape
    // - set the user data of the shape
    // - set the group index of the shape
    // - set the category bits of the shape
    // - set the mask bits of the shape
    b2CreatePolygonShape(groundBodyId, &groundShapeDef, &groundBox);

    // Create the player character
    b2BodyDef playerDef = b2DefaultBodyDef();
    playerDef.type = b2_dynamicBody;
    playerDef.position = {5.0f, 10.0f}; // starting position in meters
    playerDef.fixedRotation = true;      // prevent rotation
    b2BodyId playerBodyId = b2CreateBody(worldId, &playerDef);

    b2Polygon playerBox = b2MakeBox(0.5f, 1.0f); // 1x2 meters player
    b2ShapeDef playerShapeDef = b2DefaultShapeDef();
    playerShapeDef.density = 1.0f;
    b2ShapeId playerShapeId = b2CreatePolygonShape(playerBodyId, &playerShapeDef, &playerBox);

    b2Shape_SetRestitution(playerShapeId, 0.0f); // no bounce
    b2Shape_SetFriction(playerShapeId, 0.8f);    // some friction
    
    
    // Main loop
    bool quit = false;
    SDL_Event e;
    Uint32 lastTime = SDL_GetTicks();
    const float timeStep = 1.0f / 60.0f;
    const int subStepCount = 100;

    printf("Alien Game Demo\n");
    printf("Controls:\n");
    printf("  LEFT/RIGHT Arrow Keys or A/D - Move\n");
    printf("  UP Arrow or SPACE - Jump\n");
    printf("  ESC - Quit\n");

    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                quit = true;
            }
        }

        // Update physics
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Step the world
        b2World_Step(worldId, timeStep, subStepCount);
        
        const float moveSpeed = 5.0f;   // meters per second
        const float jumpImpulse = 8.0f;

        const Uint8* keyState = SDL_GetKeyboardState(NULL);
        b2Vec2 velocity = b2Body_GetLinearVelocity(playerBodyId);

        // Check if player is touching anything using contact data
        static bool canJump = false;
        b2ContactData contactData[16]; // Buffer for contacts
        int contactCount = b2Body_GetContactData(playerBodyId, contactData, 16);
        canJump = (contactCount > 0);

        // Move left/right with Arrow keys
        if (keyState[SDL_SCANCODE_LEFT])  velocity.x = -moveSpeed;
        if (keyState[SDL_SCANCODE_RIGHT]) velocity.x =  moveSpeed;
        
        // Move left/right with WASD
        if (keyState[SDL_SCANCODE_A])  velocity.x = -moveSpeed;
        if (keyState[SDL_SCANCODE_D])  velocity.x =  moveSpeed;
        
        // Jump with Space or Up Arrow
        if ((keyState[SDL_SCANCODE_SPACE] || keyState[SDL_SCANCODE_UP]) && canJump) {
            velocity.y = jumpImpulse;
            canJump = false;
        }

        b2Body_SetLinearVelocity(playerBodyId, velocity);



        // Clear screen
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);

        // Draw ground
        b2Vec2 groundPos = b2Body_GetPosition(groundBodyId);
        b2Rot groundRot = b2Body_GetRotation(groundBodyId);
        
        //is groundRot.angle in radians or degrees
        // it is in radians
        float groundAngle = b2Rot_GetAngle(groundRot);
        //is b2Rot_GetAngle in radians or degrees
        // it is in radians
        SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
        SDL_Rect groundRect = {
            worldToScreenX(groundPos.x - SCREEN_WIDTH / PIXELS_PER_METER / 2.0f),
            worldToScreenY(groundPos.y + 1.0f),
            static_cast<int>(SCREEN_WIDTH / PIXELS_PER_METER * PIXELS_PER_METER),
            static_cast<int>(2.0f * PIXELS_PER_METER)
        };
        SDL_RenderFillRect(renderer, &groundRect);

        

        b2Vec2 playerPos = b2Body_GetPosition(playerBodyId);
        drawRotatedRect(renderer,
                        worldToScreenX(playerPos.x),
                        worldToScreenY(playerPos.y),
                        1.0f * PIXELS_PER_METER,   // width in pixels
                        2.0f * PIXELS_PER_METER,   // height in pixels
                        0.0f,                      // no rotation
                        100, 100, 250, 255);       // color: blue
        // ===============================


        // Present
        SDL_RenderPresent(renderer);

        // Cap frame rate
        SDL_Delay(16);  // ~60 FPS
    }

    // Cleanup Box2D
    b2DestroyWorld(worldId);

    // Cleanup SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
