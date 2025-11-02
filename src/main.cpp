#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <cstring>

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
    
    BodyComponent(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
    
    void update(float dt) override {
        x += velocityX * dt;
        y += velocityY * dt;
    }
    
    void draw(SDL_Renderer* renderer, const Camera& camera) override {}
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

// SpriteComponent
class SpriteComponent : public Component {
public:
    SpriteComponent(SDL_Color color = {255, 255, 255, 255}) : m_color(color) {}
    
    void update(float dt) override {}
    
    void draw(SDL_Renderer* renderer, const Camera& camera) override {
        auto body = parent().get<BodyComponent>();
        if(!body) return;
        
        SDL_Rect rect = {
            static_cast<int>(camera.worldToScreenX(body->x)),
            static_cast<int>(camera.worldToScreenY(body->y)),
            static_cast<int>(body->width),
            static_cast<int>(body->height)
        };
        
        SDL_SetRenderDrawColor(renderer, m_color.r, m_color.g, m_color.b, 255);
        SDL_RenderFillRect(renderer, &rect);
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
    
private:
    SDL_Color m_color;
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

    static bool resolvePlatformCollision(BodyComponent* player, BodyComponent* platform) {
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
        } else {
            // Vertical collision
            if(fromTop) {
                player->y = platform->y - player->height;
                player->velocityY = 0;
                // Player is on top of platform
                return true;
            } else {
                player->y = platform->y + platform->height;
                player->velocityY = 0;
            }
        }
        return false;
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
        }
        
        // Apply gravity
        body->velocityY += gravity * dt;
        
        // Update position
        body->y += body->velocityY * dt;
        body->x += body->velocityX * dt;
        
        // Reset grounded states - ground collision will be handled by the collision system
        m_grounded = false;
        m_onPlatform = false;
        
        // Check for death by falling
        if(body->y > deathHeight) {
            respawn();
        }
    }
    
    void draw(SDL_Renderer* renderer, const Camera& camera) override {}
    
    // Public methods to be called by Game class
    void setOnPlatform(bool onPlatform) { m_onPlatform = onPlatform; }
    bool isGrounded() const { return m_grounded || m_onPlatform; }
    bool isDead() const { return m_isDead; }
    void die() { 
        m_isDead = true;
        respawn();
    }
    void respawn() { 
        m_isDead = false; 
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
    float jumpForce = 500.0f;
    float gravity = 800.0f;
    float deathHeight = 800.0f; // Player respawns if they fall below this Y position
    bool m_grounded = false;
    bool m_onPlatform = false;
    bool m_isDead = false;
};

// Behavior Components (simplified - no physics)
class PatrolBehaviorComponent : public Component {
public:
    PatrolBehaviorComponent(float left, float right, float spd) : leftBound(left), rightBound(right), speed(spd) {}
    
    void update(float dt) override {
        auto body = parent().get<BodyComponent>();
        if(!body) return;
        
        // Only handle horizontal movement
        body->velocityX = movingRight ? speed : -speed;
        
        if(body->x >= rightBound - body->width) movingRight = false;
        if(body->x <= leftBound) movingRight = true;
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
        
        // Only modify Y position for bouncing
        body->y = baseY + amplitude * std::sin(frequency * time);
    }
    
    void draw(SDL_Renderer* renderer, const Camera& camera) override {}
    
    float amplitude, frequency;
    float baseY = 0;
    float time = 0;
};

// ========================
// Simple Component Factory
// ========================
class ComponentFactory {
public:
    static void createDemoObjects(std::vector<std::unique_ptr<GameObject>>& gameObjects) {
        // Player (has ControllerComponent which handles physics)
        auto player = std::make_unique<GameObject>();
        player->add<BodyComponent>(100, 400, 50, 50);
        player->add<SpriteComponent>(SDL_Color{100, 150, 255, 255});
        player->add<ControllerComponent>();
        gameObjects.push_back(std::move(player));
        
        // Patrol Enemy (has physics component + enemy component)
        auto enemy1 = std::make_unique<GameObject>();
        enemy1->add<BodyComponent>(300, 450, 40, 40);
        enemy1->add<SpriteComponent>(SDL_Color{255, 100, 100, 255});
        enemy1->add<PhysicsComponent>();  // Add physics
        enemy1->add<PatrolBehaviorComponent>(250, 500, 150);
        enemy1->add<EnemyComponent>(); // Mark as enemy
        gameObjects.push_back(std::move(enemy1));
        
        // Bouncing Platform (no physics - it's a platform, but solid)
        auto platform = std::make_unique<GameObject>();
        platform->add<BodyComponent>(500, 300, 60, 30);
        platform->add<SpriteComponent>(SDL_Color{200, 200, 100, 255});
        platform->add<BounceBehaviorComponent>(50, 3);
        platform->add<SolidComponent>(); // Mark as solid for collision
        gameObjects.push_back(std::move(platform));
        
        // Bouncing Enemy (no physics - just bounces + enemy component)
        auto enemy2 = std::make_unique<GameObject>();
        enemy2->add<BodyComponent>(600, 200, 35, 35);
        enemy2->add<SpriteComponent>(SDL_Color{255, 150, 150, 255});
        enemy2->add<BounceBehaviorComponent>(75, 2);
        enemy2->add<EnemyComponent>(); // Mark as enemy
        gameObjects.push_back(std::move(enemy2));
        
        // Create ground as separate platforms with gaps
        createGroundPlatforms(gameObjects);
        
        std::cout << "Created " << gameObjects.size() << " demo GameObjects" << std::endl;
    }
    
private:
    static void createGroundPlatforms(std::vector<std::unique_ptr<GameObject>>& gameObjects) {
        // Create multiple ground platforms with gaps between them
        float groundY = 500.0f;
        float platformHeight = 30.0f;
        
        // Platform 1: Starting area
        auto ground1 = std::make_unique<GameObject>();
        ground1->add<BodyComponent>(0, groundY, 400, platformHeight);
        ground1->add<SpriteComponent>(SDL_Color{50, 180, 50, 255});
        ground1->add<SolidComponent>();
        gameObjects.push_back(std::move(ground1));
        
        // Gap 1: Player can fall here
        
        // Platform 2: After first gap
        auto ground2 = std::make_unique<GameObject>();
        ground2->add<BodyComponent>(500, groundY, 300, platformHeight);
        ground2->add<SpriteComponent>(SDL_Color{50, 180, 50, 255});
        ground2->add<SolidComponent>();
        gameObjects.push_back(std::move(ground2));
        
        // Gap 2: Another falling hazard
        
        // Platform 3: Further area
        auto ground3 = std::make_unique<GameObject>();
        ground3->add<BodyComponent>(900, groundY, 400, platformHeight);
        ground3->add<SpriteComponent>(SDL_Color{50, 180, 50, 255});
        ground3->add<SolidComponent>();
        gameObjects.push_back(std::move(ground3));
        
        // Gap 3: Large gap
        
        // Platform 4: Far area
        auto ground4 = std::make_unique<GameObject>();
        ground4->add<BodyComponent>(1400, groundY, 300, platformHeight);
        ground4->add<SpriteComponent>(SDL_Color{50, 180, 50, 255});
        ground4->add<SolidComponent>();
        gameObjects.push_back(std::move(ground4));
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
        
        m_window = SDL_CreateWindow("Component-Based Game Demo - Assignment", 
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
        
        // Create demo objects
        ComponentFactory::createDemoObjects(m_gameObjects);
        
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
        playerController->setOnPlatform(false);
        
        // Check collisions with all other objects
        for(size_t i = 1; i < m_gameObjects.size(); ++i) {
            auto otherObj = m_gameObjects[i].get();
            auto otherBody = otherObj->get<BodyComponent>();
            auto otherSolid = otherObj->get<SolidComponent>();
            auto otherEnemy = otherObj->get<EnemyComponent>();
            
            if(!otherBody) continue;
            
            if(CollisionSystem::checkCollision(playerBody, otherBody)) {
                // Check if it's an enemy - if so, player dies and respawns
                if(otherEnemy) {
                    playerController->die();
                    return; // Stop checking other collisions
                }
                
                // Check if it's a solid object for platform collision
                if(otherSolid) {
                    bool landedOnPlatform = CollisionSystem::resolvePlatformCollision(playerBody, otherBody);
                    if(landedOnPlatform) {
                        playerController->setOnPlatform(true);
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