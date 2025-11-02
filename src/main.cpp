#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <cstring>
#include <fstream>
#include <string>

// ========================
// Forward Declarations
// ========================
class GameObject;
class Component;
class BodyComponent;
class SpriteComponent;
class ControllerComponent;
class PhysicsComponent;
class PatrolBehaviorComponent;
class BounceBehaviorComponent;
class HorizontalMoveBehaviorComponent;
class SolidComponent;
class EnemyComponent;

// ========================
// Camera
// ========================
class Camera {
public:
    float x = 0, y = 0;
    
    void follow(float targetX, float targetY, float screenWidth, float screenHeight) {
        // Center the camera on the target
        x = targetX - screenWidth / 2;
        y = targetY - screenHeight / 2;
    }
    
    float worldToScreenX(float worldX) const { return worldX - x; }
    float worldToScreenY(float worldY) const { return worldY - y; }
};

// ========================
// Input System
// ========================
class InputSystem {
public:
    static InputSystem& getInstance() {
        static InputSystem instance;
        return instance;
    }
    
    void update() {
        std::memcpy(m_previousKeys, m_currentKeys, SDL_NUM_SCANCODES);
        const Uint8* currentKeyState = SDL_GetKeyboardState(nullptr);
        std::memcpy(m_currentKeys, currentKeyState, SDL_NUM_SCANCODES);
    }
    
    bool isKeyPressed(SDL_Scancode key) const { 
        return m_currentKeys[key]; 
    }
    
    bool isKeyJustPressed(SDL_Scancode key) const { 
        return m_currentKeys[key] && !m_previousKeys[key]; 
    }
    
private:
    InputSystem() {
        std::memset(m_currentKeys, 0, SDL_NUM_SCANCODES);
        std::memset(m_previousKeys, 0, SDL_NUM_SCANCODES);
    }
    
    Uint8 m_currentKeys[SDL_NUM_SCANCODES];
    Uint8 m_previousKeys[SDL_NUM_SCANCODES];
};

// ========================
// Texture Manager
// ========================
class TextureManager {
public:
    static TextureManager& getInstance() {
        static TextureManager instance;
        return instance;
    }
    
    SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& filePath, const std::string& textureKey) {
        // Check if texture already loaded
        auto it = m_textures.find(textureKey);
        if(it != m_textures.end()) {
            return it->second;
        }
        
        // Load new texture (using BMP for simplicity - you can use SDL_image for PNG)
        SDL_Surface* surface = SDL_LoadBMP(filePath.c_str());
        if(!surface) {
            std::cerr << "Failed to load image: " << filePath << " - " << SDL_GetError() << std::endl;
            // Create a placeholder surface if file not found
            surface = SDL_CreateRGBSurface(0, 64, 64, 32, 0, 0, 0, 0);
            SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 0, 255)); // Magenta placeholder
        }
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        
        if(!texture) {
            std::cerr << "Failed to create texture: " << textureKey << " - " << SDL_GetError() << std::endl;
            return nullptr;
        }
        
        m_textures[textureKey] = texture;
        std::cout << "Loaded texture: " << textureKey << std::endl;
        return texture;
    }
    
    SDL_Texture* getTexture(const std::string& textureKey) {
        auto it = m_textures.find(textureKey);
        return (it != m_textures.end()) ? it->second : nullptr;
    }
    
    void cleanup() {
        for(auto& pair : m_textures) {
            SDL_DestroyTexture(pair.second);
        }
        m_textures.clear();
    }
    
private:
    TextureManager() = default;
    std::unordered_map<std::string, SDL_Texture*> m_textures;
};

// ========================
// Base Component
// ========================
class Component {
public:
    virtual ~Component() = default;
    virtual void update(float dt) = 0;
    virtual void draw(SDL_Renderer* r, const Camera& camera) = 0;
    
    GameObject& parent() { return *m_parent; }
    void setParent(GameObject* p) { m_parent = p; }
    
protected:
    GameObject* m_parent = nullptr;
};

// ========================
// GameObject
// ========================
class GameObject {
public:
    template<typename T, typename... Args>
    T* add(Args&&... args) {
        auto comp = std::make_unique<T>(std::forward<Args>(args)...);
        comp->setParent(this);
        T* ptr = comp.get();
        components.emplace_back(std::move(comp));
        return ptr;
    }

    template<typename T>
    T* get() {
        for(auto& c : components) {
            if(auto ptr = dynamic_cast<T*>(c.get())) {
                return ptr;
            }
        }
        return nullptr;
    }

    void update(float dt) {
        for(auto& c : components) {
            c->update(dt);
        }
    }

    void draw(SDL_Renderer* renderer, const Camera& camera) {
        for(auto& c : components) {
            c->draw(renderer, camera);
        }
    }

    bool isActive = true;

private:
    std::vector<std::unique_ptr<Component>> components;
};

// ========================
// Required Components
// ========================

// BodyComponent
class BodyComponent : public Component {
public:
    float x, y, width, height;
    float velocityX = 0, velocityY = 0;
    float angle = 0;
    float prevX = 0, prevY = 0;
    
    BodyComponent(float x, float y, float w, float h) : x(x), y(y), width(w), height(h), prevX(x), prevY(y) {}
    
    void update(float dt) override {
        prevX = x;
        prevY = y;
        
        x += velocityX * dt;
        y += velocityY * dt;
    }
    
    void draw(SDL_Renderer* renderer, const Camera& camera) override {}
    
    float getVelocityX() const { return x - prevX; }
    float getVelocityY() const { return y - prevY; }
};

// PhysicsComponent - Handles gravity and ground collision for any object
class PhysicsComponent : public Component {
public:
    void update(float dt) override {
        auto body = parent().get<BodyComponent>();
        if(!body) return;
        
        // Apply gravity
        body->velocityY += gravity * dt;
        body->y += body->velocityY * dt;
    }
    
    void draw(SDL_Renderer* renderer, const Camera& camera) override {}
    
private:
    float gravity = 800.0f;
};

// SolidComponent - Marks an object as solid for collision
class SolidComponent : public Component {
public:
    void update(float dt) override {}
    void draw(SDL_Renderer* renderer, const Camera& camera) override {}
    
    bool isSolid = true;
};

// EnemyComponent - Marks an object as an enemy that can kill the player
class EnemyComponent : public Component {
public:
    void update(float dt) override {}
    void draw(SDL_Renderer* renderer, const Camera& camera) override {}
    
    bool isEnemy = true;
};

// SpriteComponent with texture and sprite sheet support
class SpriteComponent : public Component {
public:
    SpriteComponent(const std::string& textureKey = "", SDL_Color color = {255, 255, 255, 255}) 
        : m_textureKey(textureKey), m_color(color), m_texture(nullptr) {}
    
    void update(float dt) override {
        // Update animation frame if needed
        if(m_animated) {
            m_animationTimer += dt;
            if(m_animationTimer >= m_frameDuration) {
                m_animationTimer = 0;
                m_currentFrame = (m_currentFrame + 1) % m_totalFrames;
            }
        }
    }
    
    void draw(SDL_Renderer* renderer, const Camera& camera) override {
        auto body = parent().get<BodyComponent>();
        if(!body) return;
        
        SDL_Rect destRect = {
            static_cast<int>(camera.worldToScreenX(body->x)),
            static_cast<int>(camera.worldToScreenY(body->y)),
            static_cast<int>(body->width),
            static_cast<int>(body->height)
        };
        
        // If we have a texture, use it (stretched to fit the body)
        if(m_texture) {
            // For platforms: stretch the texture to fit the platform width
            // For sprites with animation: use sprite sheet logic
            if(m_usingSpriteSheet) {
                // Calculate row and column for multi-row sprite sheets
                int row = m_currentFrame / m_framesPerRow;
                int col = m_currentFrame % m_framesPerRow;
                
                SDL_Rect srcRect = {
                    m_spriteWidth * col,
                    m_spriteHeight * row,
                    m_spriteWidth,
                    m_spriteHeight
                };
                
                SDL_RenderCopy(renderer, m_texture, &srcRect, &destRect);
            } else {
                // For static textures (like platforms): use entire texture stretched
                SDL_RenderCopy(renderer, m_texture, NULL, &destRect);
            }
        } 
        // Otherwise fall back to colored rectangles
        else {
            SDL_SetRenderDrawColor(renderer, m_color.r, m_color.g, m_color.b, 255);
            SDL_RenderFillRect(renderer, &destRect);
            
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &destRect);
        }
    }
    
    // Texture management methods
    void setTexture(SDL_Texture* texture) { 
        m_texture = texture; 
    }
    
    // For multi-row sprite sheets
    void setSpriteSheet(int frameWidth, int frameHeight, int totalFrames, int framesPerRow, float frameRate = 10.0f) {
        m_usingSpriteSheet = true;
        m_spriteWidth = frameWidth;
        m_spriteHeight = frameHeight;
        m_totalFrames = totalFrames;
        m_framesPerRow = framesPerRow;
        m_frameDuration = 1.0f / frameRate;
        m_animated = true;
        
        std::cout << "Sprite sheet configured: " << frameWidth << "x" << frameHeight 
                  << ", " << totalFrames << " frames, " << framesPerRow << " per row" << std::endl;
    }
    
    // For single-row sprite sheets (backward compatibility)
    void setSpriteSheet(int frameWidth, int frameHeight, int totalFrames, float frameRate = 10.0f) {
        setSpriteSheet(frameWidth, frameHeight, totalFrames, totalFrames, frameRate);
    }
    
    void setStaticFrame(int frame) {
        m_currentFrame = frame;
        m_animated = false;
    }
    
private:
    std::string m_textureKey;
    SDL_Color m_color;
    SDL_Texture* m_texture;
    
    // Sprite sheet animation properties
    bool m_usingSpriteSheet = false;
    bool m_animated = false;
    int m_spriteWidth = 0;
    int m_spriteHeight = 0;
    int m_totalFrames = 1;
    int m_framesPerRow = 1;  // frames per row for multi-row sheets
    int m_currentFrame = 0;
    float m_animationTimer = 0.0f;
    float m_frameDuration = 0.1f;
};

// ========================
// Collision System
// ========================
class CollisionSystem {
public:
    static bool checkCollision(BodyComponent* a, BodyComponent* b) {
        return (a->x < b->x + b->width &&
                a->x + a->width > b->x &&
                a->y < b->y + b->height &&
                a->y + a->height > b->y);
    }

    static bool resolvePlatformCollision(BodyComponent* player, BodyComponent* platform, float platformVelocityX) {
        // Calculate overlap in all directions
        float overlapLeft = (player->x + player->width) - platform->x;
        float overlapRight = (platform->x + platform->width) - player->x;
        float overlapTop = (player->y + player->height) - platform->y;
        float overlapBottom = (platform->y + platform->height) - player->y;

        // Find the smallest overlap
        bool fromLeft = std::abs(overlapLeft) < std::abs(overlapRight);
        bool fromTop = std::abs(overlapTop) < std::abs(overlapBottom);

        float minOverlapX = fromLeft ? overlapLeft : overlapRight;
        float minOverlapY = fromTop ? overlapTop : overlapBottom;

        // Resolve in the direction of least overlap
        if(std::abs(minOverlapX) < std::abs(minOverlapY)) {
            // Horizontal collision
            if(fromLeft) {
                player->x = platform->x - player->width;
            } else {
                player->x = platform->x + platform->width;
            }
            player->velocityX = 0;
            return false;
        } else {
            // Vertical collision
            if(fromTop) {
                player->y = platform->y - player->height;
                player->velocityY = 0;
                
                // NEW: Carry player with moving platform
                if(std::abs(platformVelocityX) > 0.1f) {
                    player->x += platformVelocityX;
                }
                
                // Player is on top of platform
                return true;
            } else {
                player->y = platform->y + platform->height;
                player->velocityY = 0;
                return false;
            }
        }
    }
};

// ControllerComponent (handles input + physics for player)
class ControllerComponent : public Component {
public:
    void update(float dt) override {
        auto body = parent().get<BodyComponent>();
        if(!body) return;
        
        // Don't process input if player is dead
        if(m_isDead) return;
        
        auto& input = InputSystem::getInstance();
        
        // Store position before applying movement
        float prevX = body->x;
        
        body->velocityX = 0;
        
        if(input.isKeyPressed(SDL_SCANCODE_A) || input.isKeyPressed(SDL_SCANCODE_LEFT)) {
            body->velocityX = -speed;
        }
        if(input.isKeyPressed(SDL_SCANCODE_D) || input.isKeyPressed(SDL_SCANCODE_RIGHT)) {
            body->velocityX = speed;
        }
        if((input.isKeyJustPressed(SDL_SCANCODE_SPACE) || input.isKeyJustPressed(SDL_SCANCODE_UP)) && (m_grounded || m_onPlatform)) {
            body->velocityY = -jumpForce;
            m_grounded = false;
            m_onPlatform = false;
            m_attachedPlatform = nullptr;
        }
        
        // Apply gravity
        body->velocityY += gravity * dt;
        
        // Update position
        body->y += body->velocityY * dt;
        
        // If attached to a platform, move with it
        if(m_attachedPlatform) {
            auto platformBody = m_attachedPlatform->get<BodyComponent>();
            if(platformBody) {
                float platformDeltaX = platformBody->x - m_lastPlatformX;
                body->x += platformDeltaX;
            }
        } else {
            // Normal horizontal movement
            body->x += body->velocityX * dt;
        }
        
        // Store current platform position for next frame
        if(m_attachedPlatform) {
            auto platformBody = m_attachedPlatform->get<BodyComponent>();
            if(platformBody) {
                m_lastPlatformX = platformBody->x;
            }
        }
        
        // Reset grounded states
        m_grounded = false;
        m_onPlatform = false;
        
        // Check for death by falling
        if(body->y > deathHeight) {
            respawn();
        }
    }
    
    void draw(SDL_Renderer* renderer, const Camera& camera) override {}
    
    // Public methods to be called by Game class
    void setOnPlatform(bool onPlatform, GameObject* platform = nullptr) { 
        m_onPlatform = onPlatform; 
        if(onPlatform && platform) {
            m_attachedPlatform = platform;
            auto platformBody = platform->get<BodyComponent>();
            if(platformBody) {
                m_lastPlatformX = platformBody->x;
            }
        } else if (!onPlatform) {
            m_attachedPlatform = nullptr;
        }
    }
    
    bool isGrounded() const { return m_grounded || m_onPlatform; }
    bool isDead() const { return m_isDead; }
    void die() { 
        m_isDead = true;
        m_attachedPlatform = nullptr;
        respawn();
    }
    void respawn() { 
        m_isDead = false; 
        m_attachedPlatform = nullptr;
        auto body = parent().get<BodyComponent>();
        if(body) {
            body->x = 100;
            body->y = 400;
            body->velocityX = 0;
            body->velocityY = 0;
        }
    }
    
private:
    float speed = 300.0f;
    float jumpForce = 275.0f;
    float gravity = 900.0f;
    float deathHeight = 800.0f;
    bool m_grounded = false;
    bool m_onPlatform = false;
    bool m_isDead = false;
    GameObject* m_attachedPlatform = nullptr;
    float m_lastPlatformX = 0.0f;
};

// Behavior Components
class PatrolBehaviorComponent : public Component {
public:
    PatrolBehaviorComponent(float left, float right, float spd) : leftBound(left), rightBound(right), speed(spd) {}
    
    void update(float dt) override {
        auto body = parent().get<BodyComponent>();
        if(!body) return;
        
        // Store previous position for velocity calculation
        body->prevX = body->x;
        
        // Only handle horizontal movement
        body->velocityX = movingRight ? speed : -speed;
        body->x += body->velocityX * dt;
        
        if(body->x >= rightBound - body->width) movingRight = false;
        if(body->x <= leftBound) movingRight = true;
        
        // Calculate actual velocity based on movement
        body->velocityX = (body->x - body->prevX) / dt;
    }
    
    void draw(SDL_Renderer* renderer, const Camera& camera) override {}
    
    float leftBound, rightBound, speed;
    bool movingRight = true;
};

class BounceBehaviorComponent : public Component {
public:
    BounceBehaviorComponent(float amp, float freq) : amplitude(amp), frequency(freq) {}
    
    void update(float dt) override {
        auto body = parent().get<BodyComponent>();
        if(!body) return;
        
        if(baseY == 0) baseY = body->y;
        time += dt;
        
        // Store previous position
        body->prevY = body->y;
        
        // Only modify Y position for bouncing
        body->y = baseY + amplitude * std::sin(frequency * time);
        
        // Calculate velocity
        body->velocityY = (body->y - body->prevY) / dt;
    }
    
    void draw(SDL_Renderer* renderer, const Camera& camera) override {}
    
    float amplitude, frequency;
    float baseY = 0;
    float time = 0;
};

// HorizontalMoveBehaviorComponent - Moves platform left and right
class HorizontalMoveBehaviorComponent : public Component {
public:
    HorizontalMoveBehaviorComponent(float left, float right, float spd) 
        : leftBound(left), rightBound(right), speed(spd) {}
    
    void update(float dt) override {
        auto body = parent().get<BodyComponent>();
        if(!body) return;
        
        // Store previous position for velocity calculation
        body->prevX = body->x;
        
        // Move horizontally
        float moveAmount = movingRight ? speed * dt : -speed * dt;
        body->x += moveAmount;
        
        // Change direction at boundaries
        if(body->x >= rightBound - body->width) movingRight = false;
        if(body->x <= leftBound) movingRight = true;
        
        // Calculate and set velocity (distance moved this frame)
        body->velocityX = (body->x - body->prevX) / dt;
    }
    
    void draw(SDL_Renderer* renderer, const Camera& camera) override {}
    
    float leftBound, rightBound, speed;
    bool movingRight = true;
};

// ========================
// XML Component Factory
// ========================
class XMLComponentFactory {
public:
    static std::vector<std::unique_ptr<GameObject>> createFromXML(SDL_Renderer* renderer, const std::string& filename) {
        std::vector<std::unique_ptr<GameObject>> gameObjects;
        
        std::cout << "Loading game objects from: " << filename << std::endl;
        
        // Load textures first
        loadTextures(renderer);
        
        // Create game objects
        createSimulatedXMLObjects(gameObjects, renderer);
        
        return gameObjects;
    }
    
private:
    static void loadTextures(SDL_Renderer* renderer) {
        auto& textureManager = TextureManager::getInstance();
        
        // Load your sprite sheets with correct dimensions
        textureManager.loadTexture(renderer, "assets/character.bmp", "player_texture");
        textureManager.loadTexture(renderer, "assets/enemy.bmp", "enemy_texture");
        textureManager.loadTexture(renderer, "assets/tileset.bmp", "tile_texture");
        
        // Debug: Check if texture loaded
        SDL_Texture* tileTex = textureManager.getTexture("tile_texture");
        if(tileTex) {
            int w, h;
            SDL_QueryTexture(tileTex, NULL, NULL, &w, &h);
            std::cout << "Tileset dimensions: " << w << "x" << h << std::endl;
        } else {
            std::cout << "Tileset failed to load!" << std::endl;
        }
    }
        
    static void createSimulatedXMLObjects(std::vector<std::unique_ptr<GameObject>>& gameObjects, SDL_Renderer* renderer) {
        auto& textureManager = TextureManager::getInstance();
        
        // Player with 12-frame sprite sheet (single row, 128x96 per frame)
        auto player = std::make_unique<GameObject>();
        player->add<BodyComponent>(100, 400, 128, 96); // Match sprite size
        auto playerSprite = player->add<SpriteComponent>("player_texture");
        playerSprite->setTexture(textureManager.getTexture("player_texture"));
        playerSprite->setSpriteSheet(128, 96, 12, 12.0f); // 128x96 per frame, 12 total frames
        player->add<ControllerComponent>();
        gameObjects.push_back(std::move(player));
        
        // Platforms with tileset textures
        createPlatforms(gameObjects, renderer);
        
        // Enemies with sprite sheets
        createEnemies(gameObjects, renderer);
        
        std::cout << "Created " << gameObjects.size() << " GameObjects from XML configuration" << std::endl;
    }
    
    static void createPlatforms(std::vector<std::unique_ptr<GameObject>>& gameObjects, SDL_Renderer* renderer) {
        auto& textureManager = TextureManager::getInstance();
        
        // Platform positions and sizes - all use the same single tile
        std::vector<std::tuple<float, float, float>> platforms = {
            // x, y, width
            {0, 500, 400}, 
            {600, 500, 400},
            {1500, 500, 500},
            {2200, 500, 200},
            {2600, 500, 1000}
        };
        
        for(const auto& [x, y, width] : platforms) {
            auto platform = std::make_unique<GameObject>();
            platform->add<BodyComponent>(x, y, width, 58); // Match tile height (58px)
            auto sprite = platform->add<SpriteComponent>("tile_texture");
            sprite->setTexture(textureManager.getTexture("tile_texture"));
            // DON'T set up as sprite sheet - just use the texture as-is
            // This will stretch the 96x58 texture to fit the platform width
            platform->add<SolidComponent>();
            gameObjects.push_back(std::move(platform));
        }
        
        // Moving platform - use colored rectangle (no texture)
        auto movingPlatform = std::make_unique<GameObject>();
        movingPlatform->add<BodyComponent>(1250, 450, 150, 20); // Smaller platform
        auto movingSprite = movingPlatform->add<SpriteComponent>("", SDL_Color{150, 150, 255, 255}); // Blue color
        // No texture set - will use colored rectangle fallback
        movingPlatform->add<HorizontalMoveBehaviorComponent>(1050, 1450, 75);
        movingPlatform->add<SolidComponent>();
        gameObjects.push_back(std::move(movingPlatform));
    }
    
    static void createEnemies(std::vector<std::unique_ptr<GameObject>>& gameObjects, SDL_Renderer* renderer) {
        auto& textureManager = TextureManager::getInstance();
        
        // Patrolling enemy - 83x64 per frame (664 ÷ 8 = 83)
        auto enemy1 = std::make_unique<GameObject>();
        enemy1->add<BodyComponent>(1600, 442, 83, 64); // Adjusted Y position for 58px platform
        auto enemy1Sprite = enemy1->add<SpriteComponent>("enemy_texture");
        enemy1Sprite->setTexture(textureManager.getTexture("enemy_texture"));
        enemy1Sprite->setSpriteSheet(83, 64, 8, 8.0f); // 83x64 per frame, 8 total frames
        enemy1->add<PatrolBehaviorComponent>(1575, 1950, 100);
        enemy1->add<EnemyComponent>();
        gameObjects.push_back(std::move(enemy1));
        
        // Flying enemies
        std::vector<std::tuple<float, float, float, float>> flyingEnemies = {
            {2750, 330, 80, 2.2f}, {2850, 340, 100, 1.5f}, {2950, 330, 90, 2.2f},
            {3150, 375, 80, 2.0f}, {3200, 340, 80, 2.5f}
        };
        
        for(const auto& [x, y, amp, freq] : flyingEnemies) {
            auto enemy = std::make_unique<GameObject>();
            enemy->add<BodyComponent>(x, y, 83, 64);
            auto enemySprite = enemy->add<SpriteComponent>("enemy_texture");
            enemySprite->setTexture(textureManager.getTexture("enemy_texture"));
            enemySprite->setSpriteSheet(83, 64, 8, 10.0f);
            enemy->add<BounceBehaviorComponent>(amp, freq);
            enemy->add<EnemyComponent>();
            gameObjects.push_back(std::move(enemy));
        }
    }
};

// ========================
// Game Class
// ========================
class Game {
public:
    bool initialize() {
        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        m_window = SDL_CreateWindow("Component-Based Platformer with Sprite Sheets", 
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                   800, 600, 0);
        if(!m_window) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
        if(!m_renderer) {
            std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Load game objects from XML with texture support
        m_gameObjects = XMLComponentFactory::createFromXML(m_renderer, "scene.xml");
        
        return true;
    }
    
    void run() {
        Uint32 lastTime = SDL_GetTicks();
        bool running = true;
        SDL_Event event;
        
        while(running) {
            while(SDL_PollEvent(&event)) {
                if(event.type == SDL_QUIT) {
                    running = false;
                }
            }
            
            Uint32 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;
            
            InputSystem::getInstance().update();
            
            for(auto& obj : m_gameObjects) {
                if(obj->isActive) {
                    obj->update(deltaTime);
                }
            }
            
            // Check collisions after all objects have updated
            checkCollisions();
            
            // Update camera to follow player
            updateCamera();
            
            // Render
            SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
            SDL_RenderClear(m_renderer);
            
            // Draw objects (including ground platforms)
            for(auto& obj : m_gameObjects) {
                if(obj->isActive) {
                    obj->draw(m_renderer, m_camera);
                }
            }
            
            SDL_RenderPresent(m_renderer);
            SDL_Delay(16);
        }
    }
    
    void shutdown() {
        // Clean up textures
        TextureManager::getInstance().cleanup();
        
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
    }
    
private:
    void updateCamera() {
        auto playerObj = m_gameObjects[0].get(); // Player is first object
        auto playerBody = playerObj->get<BodyComponent>();
        
        if(playerBody) {
            // Follow player's center
            float playerCenterX = playerBody->x + playerBody->width / 2;
            float playerCenterY = playerBody->y + playerBody->height / 2;
            m_camera.follow(playerCenterX, playerCenterY, 800, 600);
        }
    }
    
    void checkCollisions() {
        auto playerObj = m_gameObjects[0].get(); // Player is first object
        auto playerBody = playerObj->get<BodyComponent>();
        auto playerController = playerObj->get<ControllerComponent>();
        
        if(!playerBody || !playerController || playerController->isDead()) return;
        
        // Reset platform status
        playerController->setOnPlatform(false, nullptr);
        
        // Check collisions with all other objects
        for(size_t i = 1; i < m_gameObjects.size(); ++i) {
            auto otherObj = m_gameObjects[i].get();
            auto otherBody = otherObj->get<BodyComponent>();
            auto otherSolid = otherObj->get<SolidComponent>();
            auto otherEnemy = otherObj->get<EnemyComponent>();
            
            if(!otherBody) continue;
            
            // Check player collisions
            if(CollisionSystem::checkCollision(playerBody, otherBody)) {
                // Check if it's an enemy - if so, player dies and respawns
                if(otherEnemy) {
                    playerController->die();
                    return; // Stop checking other collisions
                }
                
                // Check if it's a solid object for platform collision
                if(otherSolid) {
                    float platformVelocityX = otherBody->getVelocityX();
                    bool landedOnPlatform = CollisionSystem::resolvePlatformCollision(playerBody, otherBody, platformVelocityX);
                    if(landedOnPlatform) {
                        playerController->setOnPlatform(true, otherObj);
                    }
                }
            }
            
            if(otherEnemy) {
                auto enemyBody = otherBody;
                auto enemyPhysics = otherObj->get<PhysicsComponent>();
                
                // Only check collisions for enemies that have physics (gravity)
                if(enemyPhysics) {
                    for(size_t j = 1; j < m_gameObjects.size(); ++j) {
                        // Don't check collision with self
                        if(i == j) continue;
                        
                        auto groundObj = m_gameObjects[j].get();
                        auto groundBody = groundObj->get<BodyComponent>();
                        auto groundSolid = groundObj->get<SolidComponent>();
                        
                        if(!groundBody || !groundSolid) continue;
                        
                        // Check if enemy is colliding with solid ground
                        if(CollisionSystem::checkCollision(enemyBody, groundBody)) {
                            // Simple ground collision resolution for enemies
                            float overlapTop = (enemyBody->y + enemyBody->height) - groundBody->y;
                            float overlapBottom = (groundBody->y + groundBody->height) - enemyBody->y;
                            
                            // If enemy is above the ground (landing on it)
                            if(std::abs(overlapTop) < std::abs(overlapBottom)) {
                                enemyBody->y = groundBody->y - enemyBody->height;
                                enemyBody->velocityY = 0;
                            }
                        }
                    }
                }
            }
        }
    }
    
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    std::vector<std::unique_ptr<GameObject>> m_gameObjects;
    Camera m_camera;
};

// ========================
// Main
// ========================
int main() {
    Game game;
    
    if(!game.initialize()) {
        return 1;
    }
    
    game.run();
    game.shutdown();
    
    return 0;
}