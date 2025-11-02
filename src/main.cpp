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
class PatrolBehaviorComponent;
class BounceBehaviorComponent;

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
    virtual void draw(SDL_Renderer* r) = 0;
    
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

    void draw(SDL_Renderer* renderer) {
        for(auto& c : components) {
            c->draw(renderer);
        }
    }

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
    
    void draw(SDL_Renderer* renderer) override {}
};

// SpriteComponent
class SpriteComponent : public Component {
public:
    SpriteComponent(SDL_Color color = {255, 255, 255, 255}) : m_color(color) {}
    
    void update(float dt) override {}
    
    void draw(SDL_Renderer* renderer) override {
        auto body = parent().get<BodyComponent>();
        if(!body) return;
        
        SDL_Rect rect = {
            static_cast<int>(body->x),
            static_cast<int>(body->y),
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

// ControllerComponent (FIXED VERSION - no floating)
class ControllerComponent : public Component {
public:
    void update(float dt) override {
        auto body = parent().get<BodyComponent>();
        if(!body) return;
        
        auto& input = InputSystem::getInstance();
        
        body->velocityX = 0;
        
        if(input.isKeyPressed(SDL_SCANCODE_A) || input.isKeyPressed(SDL_SCANCODE_LEFT)) {
            body->velocityX = -speed;
        }
        if(input.isKeyPressed(SDL_SCANCODE_D) || input.isKeyPressed(SDL_SCANCODE_RIGHT)) {
            body->velocityX = speed;
        }
        if((input.isKeyJustPressed(SDL_SCANCODE_SPACE) || input.isKeyJustPressed(SDL_SCANCODE_UP)) && m_grounded) {
            body->velocityY = -jumpForce;
            m_grounded = false;
        }
        
        // Apply gravity
        body->velocityY += gravity * dt;
        
        // Update position
        body->y += body->velocityY * dt;
        body->x += body->velocityX * dt;
        
        // FIXED: Proper ground collision check
        if(body->y >= 500 - body->height) {
            body->y = 500 - body->height; // Snap to ground
            body->velocityY = 0;
            m_grounded = true;
        } else {
            m_grounded = false;
        }
        
        // Keep in bounds
        if(body->x < 0) body->x = 0;
        if(body->x > 800 - body->width) body->x = 800 - body->width;
    }
    
    void draw(SDL_Renderer* renderer) override {}
    
private:
    float speed = 300.0f;
    float jumpForce = 500.0f;
    float gravity = 800.0f;
    bool m_grounded = false;
};

// Behavior Components
class PatrolBehaviorComponent : public Component {
public:
    PatrolBehaviorComponent(float left, float right, float spd) : leftBound(left), rightBound(right), speed(spd) {}
    
    void update(float dt) override {
        auto body = parent().get<BodyComponent>();
        if(!body) return;
        
        body->velocityX = movingRight ? speed : -speed;
        
        if(body->x >= rightBound - body->width) movingRight = false;
        if(body->x <= leftBound) movingRight = true;
    }
    
    void draw(SDL_Renderer* renderer) override {}
    
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
        body->y = baseY + amplitude * std::sin(frequency * time);
    }
    
    void draw(SDL_Renderer* renderer) override {}
    
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
        // Player
        auto player = std::make_unique<GameObject>();
        player->add<BodyComponent>(100, 400, 50, 50);
        player->add<SpriteComponent>(SDL_Color{100, 150, 255, 255});
        player->add<ControllerComponent>();
        gameObjects.push_back(std::move(player));
        
        // Patrol Enemy
        auto enemy1 = std::make_unique<GameObject>();
        enemy1->add<BodyComponent>(300, 450, 40, 40);
        enemy1->add<SpriteComponent>(SDL_Color{255, 100, 100, 255});
        enemy1->add<PatrolBehaviorComponent>(250, 500, 150);
        gameObjects.push_back(std::move(enemy1));
        
        // Bouncing Platform
        auto platform = std::make_unique<GameObject>();
        platform->add<BodyComponent>(500, 300, 60, 30);
        platform->add<SpriteComponent>(SDL_Color{200, 200, 100, 255});
        platform->add<BounceBehaviorComponent>(50, 3);
        gameObjects.push_back(std::move(platform));
        
        // Bouncing Enemy
        auto enemy2 = std::make_unique<GameObject>();
        enemy2->add<BodyComponent>(600, 200, 35, 35);
        enemy2->add<SpriteComponent>(SDL_Color{255, 150, 150, 255});
        enemy2->add<BounceBehaviorComponent>(75, 2);
        gameObjects.push_back(std::move(enemy2));
        
        std::cout << "Created " << gameObjects.size() << " demo GameObjects" << std::endl;
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
                obj->update(deltaTime);
            }
            
            // Render
            SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
            SDL_RenderClear(m_renderer);
            
            // Draw ground
            SDL_SetRenderDrawColor(m_renderer, 50, 180, 50, 255);
            SDL_Rect ground = {0, 500, 800, 100};
            SDL_RenderFillRect(m_renderer, &ground);
            
            // Draw objects
            for(auto& obj : m_gameObjects) {
                obj->draw(m_renderer);
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
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    std::vector<std::unique_ptr<GameObject>> m_gameObjects;
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