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
#include <sstream>

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
// View Class
// ========================
class View {
    public:
        View(float centerX = 0, float centerY = 0, float scale = 1.0f, float angle = 0.0f)
            : m_centerX(centerX), m_centerY(centerY), m_scale(scale), m_angle(angle) {}
        
        void setCenter(float x, float y) { m_centerX = x; m_centerY = y; }
        void setScale(float scale) { m_scale = scale; }
        void setAngle(float angle) { m_angle = angle; }
        
        // Transform world coordinates to screen coordinates
        float worldToScreenX(float worldX) const { 
            return (worldX - m_centerX) * m_scale + m_screenWidth / 2; 
        }
        float worldToScreenY(float worldY) const { 
            return (worldY - m_centerY) * m_scale + m_screenHeight / 2; 
        }
        
        // Transform screen coordinates to world coordinates  
        float screenToWorldX(float screenX) const { 
            return (screenX - m_screenWidth / 2) / m_scale + m_centerX; 
        }
        float screenToWorldY(float screenY) const { 
            return (screenY - m_screenHeight / 2) / m_scale + m_centerY; 
        }
        
        // Set screen dimensions for proper transformation
        void setScreenDimensions(int width, int height) { 
            m_screenWidth = width; 
            m_screenHeight = height; 
        }
        
        // Get transformed rectangle for rendering
        SDL_Rect getTransformedRect(float worldX, float worldY, float width, float height) const {
            return {
                static_cast<int>(worldToScreenX(worldX)),
                static_cast<int>(worldToScreenY(worldY)),
                static_cast<int>(width * m_scale),
                static_cast<int>(height * m_scale)
            };
        }
        
    private:
        float m_centerX, m_centerY;
        float m_scale = 1.0f;
        float m_angle = 0.0f;
        int m_screenWidth = 800;
        int m_screenHeight = 600;
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
        // DEBUG
        std::cout << "=== LOADING TEXTURE ===" << std::endl;
        std::cout << "File: " << filePath << std::endl;
        std::cout << "Key: " << textureKey << std::endl;
        
        // Always remove existing texture first
        auto it = m_textures.find(textureKey);
        if(it != m_textures.end()) {
            SDL_DestroyTexture(it->second);
            m_textures.erase(it);
            std::cout << "Removed old cached texture: " << textureKey << std::endl;
        }
        
        // Load new texture
        SDL_Surface* surface = SDL_LoadBMP(filePath.c_str());
        if(!surface) {
            std::cerr << "FAILED to load BMP: " << filePath << " - " << SDL_GetError() << std::endl;
            surface = SDL_CreateRGBSurface(0, 64, 64, 32, 0, 0, 0, 0);
            SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 0, 255));
        } else {
            std::cout << "Successfully loaded BMP: " << surface->w << "x" << surface->h << std::endl;
        }
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        
        if(!texture) {
            std::cerr << "FAILED to create texture from surface: " << SDL_GetError() << std::endl;
            return nullptr;
        }
        
        m_textures[textureKey] = texture;
        std::cout << "Texture created and cached successfully" << std::endl;
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
class Engine {
    public:
        static Engine& getInstance() {
            static Engine instance;
            return instance;
        }
        
        bool initialize(const std::string& title, int width, int height) {
            // SDL initialization
            if(SDL_Init(SDL_INIT_VIDEO) < 0) {
                std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
                return false;
            }
            
            m_window = SDL_CreateWindow(title.c_str(), 
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                       width, height, 0);
            if(!m_window) {
                std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
                return false;
            }
            
            m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
            if(!m_renderer) {
                std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
                return false;
            }
            
            m_mainView.setScreenDimensions(width, height);
            std::cout << "Engine initialized: " << width << "x" << height << std::endl;
            return true;
        }
        
        void setTargetFPS(int fps) { 
            m_targetFPS = fps; 
            m_frameDelay = 1000.0f / fps;
        }
        
        void beginFrame() {
            m_frameStart = SDL_GetTicks();
        }
        
        void endFrame() {
            m_frameTime = SDL_GetTicks() - m_frameStart;
            
            // Frame rate limiting
            if(m_frameDelay > m_frameTime) {
                SDL_Delay(m_frameDelay - m_frameTime);
            }
            
            // Update deltaTime (in seconds)
            m_deltaTime = (SDL_GetTicks() - m_frameStart) / 1000.0f;
        }
        
        void shutdown() {
            if(m_renderer) SDL_DestroyRenderer(m_renderer);
            if(m_window) SDL_DestroyWindow(m_window);
            SDL_Quit();
        }
        
        // Getters
        static float deltaTime() { return getInstance().m_deltaTime; }
        static View& getMainView() { return getInstance().m_mainView; }
        static SDL_Renderer* getRenderer() { return getInstance().m_renderer; }
        static SDL_Window* getWindow() { return getInstance().m_window; }
        
    private:
        Engine() = default;
        SDL_Window* m_window = nullptr;
        SDL_Renderer* m_renderer = nullptr;
        View m_mainView;
        int m_targetFPS = 60;
        float m_frameDelay = 16.67f;
        Uint32 m_frameStart = 0;
        float m_frameTime = 0;
        float m_deltaTime = 0.016f;
    };
    
// ========================
// Base Component
// ========================
class Component {
public:
    virtual ~Component() = default;
    virtual void update(float dt) = 0;
    virtual void draw(SDL_Renderer* r, const View& view) = 0;    
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
    
        void draw(SDL_Renderer* renderer, const View& view) {
            for(auto& c : components) {
                c->draw(renderer, view);
            }
        }
    
        bool isActive = true;
    
    private:
        std::vector<std::unique_ptr<Component>> components;
    };

// ========================
// Required Components
// ========================
// ========================
// Tiling Background Component
// ========================
class TilingBackgroundComponent : public Component {
    public:
        TilingBackgroundComponent(const std::string& textureKey, float scrollSpeedX = 0.0f, float scrollSpeedY = 0.0f) 
            : m_textureKey(textureKey), m_scrollSpeedX(scrollSpeedX), m_scrollSpeedY(scrollSpeedY), m_texture(nullptr) {}
        
        void update(float dt) override {
            // Update scroll offsets for animated backgrounds
            m_scrollOffsetX += m_scrollSpeedX * dt;
            m_scrollOffsetY += m_scrollSpeedY * dt;
            
            // Wrap offsets to prevent overflow
            if (m_scrollOffsetX >= m_textureWidth) m_scrollOffsetX -= m_textureWidth;
            if (m_scrollOffsetX <= -m_textureWidth) m_scrollOffsetX += m_textureWidth;
            if (m_scrollOffsetY >= m_textureHeight) m_scrollOffsetY -= m_textureHeight;
            if (m_scrollOffsetY <= -m_textureHeight) m_scrollOffsetY += m_textureHeight;
        }
        
        void draw(SDL_Renderer* renderer, const View& view) override {  // ADD 'override' here
            if (!m_texture) {
                m_texture = TextureManager::getInstance().getTexture(m_textureKey);
                if (!m_texture) {
                    SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
                    SDL_RenderClear(renderer);
                    return;
                }
                
                // Automatically get the actual texture dimensions
                SDL_QueryTexture(m_texture, NULL, NULL, &m_textureWidth, &m_textureHeight);
                
                std::cout << "Tiling background loaded: " << m_textureKey 
                          << " (" << m_textureWidth << "x" << m_textureHeight << ")" << std::endl;
            }
            
            // Get screen dimensions
            int screenWidth, screenHeight;
            SDL_GetRendererOutputSize(renderer, &screenWidth, &screenHeight);
            
            // Calculate starting positions for tiling
            int startX = static_cast<int>(-m_scrollOffsetX);
            int startY = static_cast<int>(-m_scrollOffsetY);
            
            // Ensure we cover the entire screen with some margin
            int tilesX = (screenWidth / m_textureWidth) + 2;
            int tilesY = (screenHeight / m_textureHeight) + 2;
            
            // Draw tiled background
            for (int y = 0; y < tilesY; y++) {
                for (int x = 0; x < tilesX; x++) {
                    SDL_Rect destRect = {
                        startX + (x * m_textureWidth),
                        startY + (y * m_textureHeight),
                        m_textureWidth,
                        m_textureHeight
                    };
                    SDL_RenderCopy(renderer, m_texture, NULL, &destRect);
                }
            }
            
            // Debug info (optional)
            static int debugCounter = 0;
            if (debugCounter++ % 300 == 0) {
                std::cout << "Drawing tiling background: " << tilesX << "x" << tilesY << " tiles" 
                          << " at offset (" << m_scrollOffsetX << "," << m_scrollOffsetY << ")" << std::endl;
            }
        }
        
        // Method to change scroll speed dynamically
        void setScrollSpeed(float speedX, float speedY) {
            m_scrollSpeedX = speedX;
            m_scrollSpeedY = speedY;
        }
        
    private:
        std::string m_textureKey;
        SDL_Texture* m_texture;
        float m_scrollSpeedX;
        float m_scrollSpeedY;
        float m_scrollOffsetX = 0.0f;
        float m_scrollOffsetY = 0.0f;
        int m_textureWidth = 0;
        int m_textureHeight = 0;
    };
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
    
    void draw(SDL_Renderer* renderer, const View& view) override {}

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
    
void draw(SDL_Renderer* renderer, const View& view) override {}    
private:
    float gravity = 800.0f;
};

// SolidComponent - Marks an object as solid for collision
class SolidComponent : public Component {
public:
    void update(float dt) override {}
    void draw(SDL_Renderer* renderer, const View& view) override {}    
    bool isSolid = true;
};

// EnemyComponent - Marks an object as an enemy that can kill the player
class EnemyComponent : public Component {
public:
    void update(float dt) override {}
    void draw(SDL_Renderer* renderer, const View& view) override {}    
    bool isEnemy = true;
};

// SpriteComponent with texture and sprite sheet support
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
        
        void draw(SDL_Renderer* renderer, const View& view) override {
            auto body = parent().get<BodyComponent>();
            if(!body) return;
            
            SDL_Rect destRect = view.getTransformedRect(body->x, body->y, body->width, body->height);
            
            // If we have a texture, use it
            if(m_texture) {
                // For custom source rectangle (specific tile coordinates)
                if(m_usingCustomSource) {
                    SDL_RenderCopy(renderer, m_texture, &m_customSrcRect, &destRect);
                }
                // For sprite sheets with static frames (platforms)
                else if(m_usingSpriteSheet && !m_animated) {
                    // Calculate row and column for the static frame
                    int row = m_currentFrame / m_framesPerRow;
                    int col = m_currentFrame % m_framesPerRow;
                    
                    SDL_Rect srcRect = {
                        m_spriteWidth * col,
                        m_spriteHeight * row,
                        m_spriteWidth,
                        m_spriteHeight
                    };
                    SDL_RenderCopy(renderer, m_texture, &srcRect, &destRect);
                }
                // For animated sprite sheets (characters, enemies)
                else if(m_usingSpriteSheet && m_animated) {
                    int row = m_currentFrame / m_framesPerRow;
                    int col = m_currentFrame % m_framesPerRow;
                    
                    SDL_Rect srcRect = {
                        m_spriteWidth * col,
                        m_spriteHeight * row,
                        m_spriteWidth,
                        m_spriteHeight
                    };
                    SDL_RenderCopy(renderer, m_texture, &srcRect, &destRect);
                }
                // For static textures (stretched to fit)
                else {
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
            m_animated = false; // Don't animate platforms
        }
    
        // Set specific tile from the sprite sheet
        void setTile(int tileX, int tileY, int tileWidth, int tileHeight) {
            m_usingSpriteSheet = true;
            m_spriteWidth = tileWidth;
            m_spriteHeight = tileHeight;
            
            // For platforms, we don't use frame calculation - we use direct source rectangle
            m_usingCustomSource = true;
            m_customSrcRect = {
                tileX * tileWidth,
                tileY * tileHeight,
                tileWidth,
                tileHeight
            };
            
            m_animated = false; // Static for platforms
            m_totalFrames = 1; // Only using one frame
            
            std::cout << "Tile set to: (" << tileX << "," << tileY << ") at position (" 
                      << m_customSrcRect.x << "," << m_customSrcRect.y << ")" << std::endl;
        }
    
        // Set specific source rectangle directly
        void setSourceRect(int x, int y, int width, int height) {
            m_usingCustomSource = true;
            m_customSrcRect = {x, y, width, height};
            m_animated = false;
        }
        
    private:
        std::string m_textureKey;
        SDL_Color m_color;
        SDL_Texture* m_texture;
        SDL_Rect m_customSrcRect = {0, 0, 0, 0};
    
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
        bool m_usingCustomSource = false;
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
            body->velocityX = -speed;  // Multiply by deltaTime
        }
        if(input.isKeyPressed(SDL_SCANCODE_D) || input.isKeyPressed(SDL_SCANCODE_RIGHT)) {
            body->velocityX = speed;   // Multiply by deltaTime
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
    
    void draw(SDL_Renderer* renderer, const View& view) override {}    
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
        float movement = movingRight ? speed * dt : -speed * dt;
        body->x += movement;
        
        if(body->x >= rightBound - body->width) movingRight = false;
        if(body->x <= leftBound) movingRight = true;
        
        // Calculate actual velocity based on movement
        body->velocityX = (body->x - body->prevX) / dt;
    }
    
    void draw(SDL_Renderer* renderer, const View& view) override {}

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
    
    void draw(SDL_Renderer* renderer, const View& view) override {}    
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
    
    void draw(SDL_Renderer* renderer, const View& view) override {}

    float leftBound, rightBound, speed;
    bool movingRight = true;
};

// ========================
// XML Parser
// ========================
class XMLParser {
    public:
        static std::vector<std::unique_ptr<GameObject>> parseXML(SDL_Renderer* renderer, const std::string& filename);
        
        // Make this method public
        static std::string extractAttribute(const std::string& line, const std::string& attrName);
    
    private:
        static SDL_Color parseColor(const std::string& colorStr);
        static std::unique_ptr<GameObject> createGameObject(SDL_Renderer* renderer, const std::string& type, 
                                                           const std::unordered_map<std::string, std::string>& attrs);
        static std::string readCompleteTag(std::ifstream& file, std::string firstLine);
    };
    
    // Implementation of extractAttribute (outside the class)
    std::string XMLParser::extractAttribute(const std::string& line, const std::string& attrName) {
        size_t pos = line.find(attrName + "=\"");
        if (pos == std::string::npos) return "";
        
        pos += attrName.length() + 2; // Move past attrName="
        size_t endPos = line.find("\"", pos);
        if (endPos == std::string::npos) return "";
        
        return line.substr(pos, endPos - pos);
    }
    
    // Helper function to read a complete XML tag that might span multiple lines
    std::string XMLParser::readCompleteTag(std::ifstream& file, std::string firstLine) {
        std::string completeTag = firstLine;
        
        // Check if the tag is already complete (ends with /> or >)
        if (firstLine.find("/>") != std::string::npos || firstLine.find(">") != std::string::npos) {
            return completeTag;
        }
        
        // Continue reading lines until we find the end of the tag
        std::string line;
        while (std::getline(file, line)) {
            // Remove whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            
            if (line.empty()) continue;

            if (line.find("<Level>") != std::string::npos || line.find("</Level>") != std::string::npos) {
                continue;
            }
            completeTag += " " + line;
            
            // Check if the tag is now complete
            if (line.find("/>") != std::string::npos || line.find(">") != std::string::npos) {
                break;
            }
        }
        
        return completeTag;
    }
    
    // Implementation of parseXML
    std::vector<std::unique_ptr<GameObject>> XMLParser::parseXML(SDL_Renderer* renderer, const std::string& filename) {
        std::vector<std::unique_ptr<GameObject>> gameObjects;
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open XML file: " << filename << std::endl;
            return gameObjects;
        }
        
        std::string line;
        std::string currentObjectType;
        std::unordered_map<std::string, std::string> currentAttributes;
        
        while (std::getline(file, line)) {
            // Remove whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            
            if (line.empty()) continue;
            
            // Check for component tags and read complete tags
            if (line.find("<GameObject") != std::string::npos) {
                std::string completeTag = readCompleteTag(file, line);
                currentObjectType = extractAttribute(completeTag, "type");
                currentAttributes.clear();
                std::cout << "GameObject: " << currentObjectType << std::endl;
            }
            else if (line.find("<BodyComponent") != std::string::npos) {
                std::string completeTag = readCompleteTag(file, line);
                currentAttributes["x"] = extractAttribute(completeTag, "x");
                currentAttributes["y"] = extractAttribute(completeTag, "y");
                currentAttributes["width"] = extractAttribute(completeTag, "width");
                currentAttributes["height"] = extractAttribute(completeTag, "height");
                std::cout << "BodyComponent: " << currentAttributes["x"] << "," << currentAttributes["y"] 
                          << " " << currentAttributes["width"] << "x" << currentAttributes["height"] << std::endl;
            }
            else if (line.find("<PatrolBehaviorComponent") != std::string::npos) {
                std::string completeTag = readCompleteTag(file, line);
                currentAttributes["left"] = extractAttribute(completeTag, "left");
                currentAttributes["right"] = extractAttribute(completeTag, "right");
                currentAttributes["speed"] = extractAttribute(completeTag, "speed");
            }
            else if (line.find("<BounceBehaviorComponent") != std::string::npos) {
                std::string completeTag = readCompleteTag(file, line);
                currentAttributes["amplitude"] = extractAttribute(completeTag, "amplitude");
                currentAttributes["frequency"] = extractAttribute(completeTag, "frequency");
            }
            else if (line.find("<HorizontalMoveBehaviorComponent") != std::string::npos) {
                std::string completeTag = readCompleteTag(file, line);
                currentAttributes["left"] = extractAttribute(completeTag, "left");
                currentAttributes["right"] = extractAttribute(completeTag, "right");
                currentAttributes["speed"] = extractAttribute(completeTag, "speed");
            }
            else if (line.find("</GameObject>") != std::string::npos) {
                // Create the GameObject with all collected attributes
                auto obj = createGameObject(renderer, currentObjectType, currentAttributes);
                if (obj) {
                    gameObjects.push_back(std::move(obj));
                }
                currentAttributes.clear();
                std::cout << "--- Finished GameObject ---" << std::endl;
            }
            else if (line.find("<TilingBackgroundComponent") != std::string::npos) {
                std::string completeTag = readCompleteTag(file, line);
                currentAttributes["textureKey"] = extractAttribute(completeTag, "textureKey");
                currentAttributes["scrollSpeedX"] = extractAttribute(completeTag, "scrollSpeedX");
                currentAttributes["scrollSpeedY"] = extractAttribute(completeTag, "scrollSpeedY");
                
                std::cout << "TilingBackgroundComponent: " << currentAttributes["textureKey"] 
                          << " scroll: (" << currentAttributes["scrollSpeedX"] << "," << currentAttributes["scrollSpeedY"] << ")" << std::endl;
            }
            else if (line.find("<SpriteComponent") != std::string::npos) {
                std::string completeTag = readCompleteTag(file, line);
                currentAttributes["textureKey"] = extractAttribute(completeTag, "textureKey");
                currentAttributes["spriteSheet"] = extractAttribute(completeTag, "spriteSheet");
                currentAttributes["frameWidth"] = extractAttribute(completeTag, "frameWidth");
                currentAttributes["frameHeight"] = extractAttribute(completeTag, "frameHeight");
                currentAttributes["totalFrames"] = extractAttribute(completeTag, "totalFrames");
                currentAttributes["frameRate"] = extractAttribute(completeTag, "frameRate");
                currentAttributes["color"] = extractAttribute(completeTag, "color");
                // Add tile attributes
                currentAttributes["tileX"] = extractAttribute(completeTag, "tileX");
                currentAttributes["tileY"] = extractAttribute(completeTag, "tileY");
                currentAttributes["tileWidth"] = extractAttribute(completeTag, "tileWidth");
                currentAttributes["tileHeight"] = extractAttribute(completeTag, "tileHeight");
                
                std::cout << "SpriteComponent - textureKey: " << currentAttributes["textureKey"] 
                          << ", tileX: " << currentAttributes["tileX"]
                          << ", tileY: " << currentAttributes["tileY"] << std::endl;
            }
        }
        
        file.close();
        std::cout << "Parsed " << gameObjects.size() << " GameObjects from XML" << std::endl;
        return gameObjects;
    }
    
    // Implementation of parseColor
    SDL_Color XMLParser::parseColor(const std::string& colorStr) {
        SDL_Color color = {255, 255, 255, 255}; // Default white
        if (colorStr.empty()) return color;
        
        std::stringstream ss(colorStr);
        std::string token;
        std::vector<int> rgb;
        
        while (std::getline(ss, token, ',')) {
            rgb.push_back(std::stoi(token));
        }
        
        if (rgb.size() >= 3) {
            color.r = rgb[0];
            color.g = rgb[1];
            color.b = rgb[2];
        }
        
        return color;
    }
    
    // Implementation of createGameObject
    std::unique_ptr<GameObject> XMLParser::createGameObject(SDL_Renderer* renderer, const std::string& type, 
                                                           const std::unordered_map<std::string, std::string>& attrs) {
        auto& textureManager = TextureManager::getInstance();
        auto obj = std::make_unique<GameObject>();
        
        if (type == "player") {
            // Player
            float x = std::stof(attrs.at("x"));
            float y = std::stof(attrs.at("y"));
            float width = std::stof(attrs.at("width"));
            float height = std::stof(attrs.at("height"));
            
            obj->add<BodyComponent>(x, y, width, height);
            
            auto sprite = obj->add<SpriteComponent>(attrs.at("textureKey"));
            SDL_Texture* texture = textureManager.getTexture(attrs.at("textureKey"));
            if (texture) {
                sprite->setTexture(texture);
            }
            
            // Check if sprite sheet should be configured
            if (attrs.find("spriteSheet") != attrs.end() && attrs.at("spriteSheet") == "true") {
                int frameWidth = std::stoi(attrs.at("frameWidth"));
                int frameHeight = std::stoi(attrs.at("frameHeight"));
                int totalFrames = std::stoi(attrs.at("totalFrames"));
                float frameRate = std::stof(attrs.at("frameRate"));
                
                std::cout << "=== CONFIGURING PLAYER SPRITE SHEET ===" << std::endl;
                std::cout << "Frame: " << frameWidth << "x" << frameHeight << std::endl;
                std::cout << "Frames: " << totalFrames << " at " << frameRate << " fps" << std::endl;
                
                sprite->setSpriteSheet(frameWidth, frameHeight, totalFrames, frameRate);
            }
            
            obj->add<ControllerComponent>();
        }
        else if (type == "platform" || type == "moving_platform") {
            // Platform
            float x = std::stof(attrs.at("x"));
            float y = std::stof(attrs.at("y"));
            float width = std::stof(attrs.at("width"));
            float height = std::stof(attrs.at("height"));
            
            obj->add<BodyComponent>(x, y, width, height);
            obj->add<SolidComponent>();
            
            // Handle sprite with texture or color
            if (attrs.find("textureKey") != attrs.end() && !attrs.at("textureKey").empty()) {
                auto sprite = obj->add<SpriteComponent>(attrs.at("textureKey"));
                SDL_Texture* texture = textureManager.getTexture(attrs.at("textureKey"));
                if (texture) {
                    sprite->setTexture(texture);
                    
                    // === ADD TILE SUPPORT RIGHT HERE ===
                    // Check if we should use a specific tile from the tilesheet
                    if (attrs.find("tileX") != attrs.end() && attrs.find("tileY") != attrs.end()) {
                        int tileX = std::stoi(attrs.at("tileX"));
                        int tileY = std::stoi(attrs.at("tileY"));
                        int tileWidth = std::stoi(attrs.at("tileWidth"));
                        int tileHeight = std::stoi(attrs.at("tileHeight"));
                        
                        sprite->setTile(tileX, tileY, tileWidth, tileHeight);
                        std::cout << "Platform using tile: " << tileX << "," << tileY 
                                  << " (" << tileWidth << "x" << tileHeight << ")" << std::endl;
                    }
                    // === END OF TILE SUPPORT ===
                }
            } else if (attrs.find("color") != attrs.end()) {
                SDL_Color color = parseColor(attrs.at("color"));
                obj->add<SpriteComponent>("", color);
            }
            
            // Moving platform behavior
            if (type == "moving_platform") {
                float left = std::stof(attrs.at("left"));
                float right = std::stof(attrs.at("right"));
                float speed = std::stof(attrs.at("speed"));
                obj->add<HorizontalMoveBehaviorComponent>(left, right, speed);
            }
        }
        else if (type == "enemy" || type == "flying_enemy") {
            // Enemy
            float x = std::stof(attrs.at("x"));
            float y = std::stof(attrs.at("y"));
            float width = std::stof(attrs.at("width"));
            float height = std::stof(attrs.at("height"));
            
            obj->add<BodyComponent>(x, y, width, height);
            obj->add<EnemyComponent>();
            
            auto sprite = obj->add<SpriteComponent>(attrs.at("textureKey"));
            SDL_Texture* texture = textureManager.getTexture(attrs.at("textureKey"));
            if (texture) {
                sprite->setTexture(texture);
            }
            
            // Check if sprite sheet should be configured
            if (attrs.find("spriteSheet") != attrs.end() && attrs.at("spriteSheet") == "true") {
                int frameWidth = std::stoi(attrs.at("frameWidth"));
                int frameHeight = std::stoi(attrs.at("frameHeight"));
                int totalFrames = std::stoi(attrs.at("totalFrames"));
                float frameRate = std::stof(attrs.at("frameRate"));
                
                std::cout << "=== CONFIGURING ENEMY SPRITE SHEET ===" << std::endl;
                std::cout << "Frame: " << frameWidth << "x" << frameHeight << std::endl;
                std::cout << "Frames: " << totalFrames << " at " << frameRate << " fps" << std::endl;
                
                sprite->setSpriteSheet(frameWidth, frameHeight, totalFrames, frameRate);
            }
            
            // Enemy behavior
            if (type == "enemy") {
                float left = std::stof(attrs.at("left"));
                float right = std::stof(attrs.at("right"));
                float speed = std::stof(attrs.at("speed"));
                obj->add<PatrolBehaviorComponent>(left, right, speed);
            } else if (type == "flying_enemy") {
                float amplitude = std::stof(attrs.at("amplitude"));
                float frequency = std::stof(attrs.at("frequency"));
                obj->add<BounceBehaviorComponent>(amplitude, frequency);
            }
        }
        else if (type == "tiling_background") {
            // Tiling background object - SAFELY get attributes
            std::string textureKey = "";
            if (attrs.find("textureKey") != attrs.end()) {
                textureKey = attrs.at("textureKey");
            } else {
                std::cerr << "ERROR: tiling_background missing required textureKey attribute" << std::endl;
                return nullptr;
            }
            
            float scrollSpeedX = 0.0f;
            float scrollSpeedY = 0.0f;
            
            if (attrs.find("scrollSpeedX") != attrs.end()) {
                scrollSpeedX = std::stof(attrs.at("scrollSpeedX"));
            }
            if (attrs.find("scrollSpeedY") != attrs.end()) {
                scrollSpeedY = std::stof(attrs.at("scrollSpeedY"));
            }
            
            std::cout << "Creating tiling background with texture: " << textureKey 
                      << " scroll: (" << scrollSpeedX << "," << scrollSpeedY << ")" << std::endl;
            
            obj->add<TilingBackgroundComponent>(textureKey, scrollSpeedX, scrollSpeedY);
        }
        return obj;
    }
// ========================
// XML Component Factory
// ========================
class XMLComponentFactory {
public:
    static std::vector<std::unique_ptr<GameObject>> createFromXML(SDL_Renderer* renderer, const std::string& filename) {
        std::vector<std::unique_ptr<GameObject>> gameObjects;
        
        std::cout << "Loading game objects from: " << filename << std::endl;
        
        // Load textures first from the XML
        loadTexturesFromXML(renderer, filename);
        
        // Parse the XML file to create game objects
        gameObjects = XMLParser::parseXML(renderer, filename);
        
        return gameObjects;
    }
    
private:
    static void loadTexturesFromXML(SDL_Renderer* renderer, const std::string& filename) {
        auto& textureManager = TextureManager::getInstance();
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "ERROR: Failed to open XML file: " << filename << std::endl;
            return;
        }
        
        std::cout << "=== Loading Textures from XML ===" << std::endl;
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("<Texture") != std::string::npos) {
                std::string filePath = XMLParser::extractAttribute(line, "file");
                std::string textureKey = XMLParser::extractAttribute(line, "key");
                
                if (!filePath.empty() && !textureKey.empty()) {
                    std::cout << "Found texture definition: " << textureKey << " -> " << filePath << std::endl;
                    textureManager.loadTexture(renderer, filePath, textureKey);
                }
            }
        }
        
        file.close();
        std::cout << "=== Finished Loading Textures ===" << std::endl;
    }
};

// ========================
// Game Class
// ========================
class Game {
    public:
        bool initialize() {
            // Use Engine for initialization
            if(!Engine::getInstance().initialize("Component-Based Platformer with Sprite Sheets", 800, 600)) {
                return false;
            }
            
            Engine::getInstance().setTargetFPS(60);
            
            // FORCE COMPLETE CLEANUP - Add these lines
            m_gameObjects.clear();
            TextureManager::getInstance().cleanup();
            
            std::cout << "=== LOADING NEW LEVEL ===" << std::endl;
            
            // Test if XML file exists
            std::ifstream testFile("scene.xml");
            if (!testFile.is_open()) {
                std::cerr << "ERROR: Cannot open scene.xml! Make sure it's in the same directory as the executable." << std::endl;
                return false;
            }
            testFile.close();
            // Load game objects from XML
            m_gameObjects = XMLComponentFactory::createFromXML(Engine::getRenderer(), "scene.xml");
            
            if (m_gameObjects.empty()) {
                std::cerr << "ERROR: No game objects loaded from XML!" << std::endl;
                return false;
            }
            
            debugLoadedObjects();
            std::cout << "=== GAME INITIALIZATION COMPLETE ===" << std::endl;
            return true;
        }
        
        void run() {
            Uint32 lastTime = SDL_GetTicks();
            bool running = true;
            SDL_Event event;
            
            std::cout << "=== GAME LOOP STARTED ===" << std::endl;
            
            while(running) {
                // Engine frame management
                Engine::getInstance().beginFrame();
                
                // Input handling
                while(SDL_PollEvent(&event)) {
                    if(event.type == SDL_QUIT) {
                        running = false;
                    }
                    // You can add more event handling here if needed
                }
                
                InputSystem::getInstance().update();
                
                // Update using engine's deltaTime
                update(Engine::deltaTime());
                
                // Render
                render();
                
                Engine::getInstance().endFrame();
            }
            
            std::cout << "=== GAME LOOP ENDED ===" << std::endl;
        }
        
        void shutdown() {
            std::cout << "=== SHUTTING DOWN GAME ===" << std::endl;
            m_gameObjects.clear();
            TextureManager::getInstance().cleanup();
            Engine::getInstance().shutdown();
        }
        
    private:
        void update(float deltaTime) {
            // Update all game objects using proper deltaTime
            for(auto& obj : m_gameObjects) {
                if(obj->isActive) {
                    obj->update(deltaTime);
                }
            }
            
            updateCamera();
            checkCollisions();
            
            // Optional: Debug FPS display
            static int frameCount = 0;
            static float timeAccumulator = 0.0f;
            timeAccumulator += deltaTime;
            frameCount++;
            
            if(timeAccumulator >= 1.0f) {
                std::cout << "FPS: " << frameCount << ", DeltaTime: " << deltaTime << std::endl;
                frameCount = 0;
                timeAccumulator = 0.0f;
            }
        }
        
        void render() {
            SDL_Renderer* renderer = Engine::getRenderer();
            
            // Clear screen
            SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255); // Sky blue background
            SDL_RenderClear(renderer);
            
            // Get the main view from Engine
            View& mainView = Engine::getMainView();
            
            // Render backgrounds first
            for(auto& obj : m_gameObjects) {
                if(obj->isActive && obj->get<TilingBackgroundComponent>()) {
                    obj->draw(renderer, mainView);
                }
            }
            
            // Then render all other game objects
            for(auto& obj : m_gameObjects) {
                if(obj->isActive && !obj->get<TilingBackgroundComponent>()) {
                    obj->draw(renderer, mainView);
                }
            }
            
            // Optional: Render debug information
            renderDebugInfo(renderer);
            
            SDL_RenderPresent(renderer);
        }
        
        void updateCamera() {
            auto playerObj = findPlayer();
            if (!playerObj) {
                // Debug: No player found
                static bool warned = false;
                if (!warned) {
                    std::cout << "WARNING: No player object found for camera tracking!" << std::endl;
                    warned = true;
                }
                return;
            }
            
            auto playerBody = playerObj->get<BodyComponent>();
            if(playerBody) {
                // Update engine's main view to follow player
                View& mainView = Engine::getMainView();
                mainView.setCenter(
                    playerBody->x + playerBody->width / 2,
                    playerBody->y + playerBody->height / 2
                );
                
                // Optional: Add camera smoothing or bounds checking here
            }
        }
        
        GameObject* findPlayer() {
            for(auto& obj : m_gameObjects) {
                if(obj->get<ControllerComponent>()) {
                    return obj.get();
                }
            }
            return nullptr;
        }
        
        void checkCollisions() {
            auto playerObj = findPlayer();
            if (!playerObj) return;
            
            auto playerBody = playerObj->get<BodyComponent>();
            auto playerController = playerObj->get<ControllerComponent>();
            
            if(!playerBody || !playerController || playerController->isDead()) return;
            
            // Reset platform status
            playerController->setOnPlatform(false, nullptr);
            
            // Check collisions with all other objects
            for(size_t i = 0; i < m_gameObjects.size(); ++i) {
                auto otherObj = m_gameObjects[i].get();
                
                // Skip the player object itself and background objects
                if(otherObj == playerObj || otherObj->get<TilingBackgroundComponent>()) {
                    continue;
                }
                
                auto otherBody = otherObj->get<BodyComponent>();
                auto otherSolid = otherObj->get<SolidComponent>();
                auto otherEnemy = otherObj->get<EnemyComponent>();
                
                if(!otherBody) continue;
                
                // Check player collisions - USE FULL BODY SIZE (no scaling)
                if(CollisionSystem::checkCollision(playerBody, otherBody)) {
                    // Check if it's an enemy - if so, player dies and respawns
                    if(otherEnemy) {
                        std::cout << "Player died by enemy collision!" << std::endl;
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
                
                // Handle enemy physics with platforms
                if(otherEnemy) {
                    auto enemyBody = otherBody;
                    auto enemyPhysics = otherObj->get<PhysicsComponent>();
                    
                    // Only check collisions for enemies that have physics (gravity)
                    if(enemyPhysics) {
                        for(size_t j = 0; j < m_gameObjects.size(); ++j) {
                            // Don't check collision with self or background
                            if(i == j || m_gameObjects[j].get()->get<TilingBackgroundComponent>()) {
                                continue;
                            }
                            
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
        
        void debugLoadedObjects() {
            std::cout << "=== LOADED OBJECTS DEBUG ===" << std::endl;
            int platformCount = 0;
            int movingPlatformCount = 0;
            int enemyCount = 0;
            int playerCount = 0;
            int backgroundCount = 0;
            int totalObjects = 0;
            
            for(auto& obj : m_gameObjects) {
                totalObjects++;
                
                if(obj->get<SolidComponent>()) {
                    platformCount++;
                    if(obj->get<HorizontalMoveBehaviorComponent>()) {
                        movingPlatformCount++;
                    }
                }
                if(obj->get<EnemyComponent>()) {
                    enemyCount++;
                }
                if(obj->get<ControllerComponent>()) {
                    playerCount++;
                }
                if(obj->get<TilingBackgroundComponent>()) {
                    backgroundCount++;
                }
            }
            
            std::cout << "Total GameObjects: " << totalObjects << std::endl;
            std::cout << "Players: " << playerCount << std::endl;
            std::cout << "Platforms: " << platformCount << " (moving: " << movingPlatformCount << ")" << std::endl;
            std::cout << "Enemies: " << enemyCount << std::endl;
            std::cout << "Backgrounds: " << backgroundCount << std::endl;
            
            // Log positions of first few platforms for verification
            int loggedPlatforms = 0;
            for(auto& obj : m_gameObjects) {
                if(obj->get<SolidComponent>() && loggedPlatforms < 5) {
                    auto body = obj->get<BodyComponent>();
                    if(body) {
                        std::cout << "Platform " << (loggedPlatforms + 1) << " at: " 
                                  << body->x << "," << body->y << " size: " 
                                  << body->width << "x" << body->height << std::endl;
                        loggedPlatforms++;
                    }
                }
            }
            std::cout << "=============================" << std::endl;
        }
        
        void renderDebugInfo(SDL_Renderer* renderer) {
            View& mainView = Engine::getMainView(); // Add this line if missing
            
            auto playerObj = findPlayer();
            if (playerObj) {
                auto playerBody = playerObj->get<BodyComponent>();
                if (playerBody) {
                    // Draw the ACTUAL collision box (full size - no scaling)
                    SDL_Rect debugRect = mainView.getTransformedRect(
                        playerBody->x,  // No offset
                        playerBody->y,  // No offset
                        playerBody->width,  // Full width
                        playerBody->height  // Full height
                    );
                    
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 128); // Semi-transparent red
                    SDL_RenderDrawRect(renderer, &debugRect);
                    
                    // The visual bounds are the same as collision bounds now
                    // So we don't need the green box, or keep it to show they're identical
                    SDL_Rect visualRect = mainView.getTransformedRect(
                        playerBody->x, playerBody->y, 
                        playerBody->width, playerBody->height
                    );
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 64); // Semi-transparent green
                    SDL_RenderDrawRect(renderer, &visualRect);
                }
            }
        }
        
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