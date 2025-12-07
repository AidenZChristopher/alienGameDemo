#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
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
#include <iomanip>
#include <box2d/box2d.h>
#include <tinyxml2.h>
#include <ctime>

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
class Box2DPhysicsComponent;
class Box2DContactListener;
class BulletComponent;
class DestructibleBoxComponent;
class CheckpointComponent;

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
        
        // Load new texture - try IMG_Load first (supports PNG, BMP, etc.), then fall back to SDL_LoadBMP
        SDL_Surface* surface = IMG_Load(filePath.c_str());
        if(!surface) {
            // Fall back to BMP loader
            surface = SDL_LoadBMP(filePath.c_str());
            if(!surface) {
                std::cerr << "FAILED to load image: " << filePath << " - " << SDL_GetError() << std::endl;
                surface = SDL_CreateRGBSurface(0, 64, 64, 32, 0, 0, 0, 0);
                SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 0, 255));
            } else {
                std::cout << "Successfully loaded BMP: " << surface->w << "x" << surface->h << std::endl;
                
                // Enable color keying to make white (255, 255, 255) transparent
                // This removes the white background from sprites
                Uint32 colorKey = SDL_MapRGB(surface->format, 255, 255, 255);
                SDL_SetColorKey(surface, SDL_TRUE, colorKey);
            }
        } else {
            std::cout << "Successfully loaded image: " << surface->w << "x" << surface->h << std::endl;
            
            // For PNG files with alpha channel, we don't need color keying
            // But for BMP files loaded via IMG_Load, apply color keying
            std::string extension = filePath.substr(filePath.find_last_of(".") + 1);
            if (extension == "bmp" || extension == "BMP") {
                Uint32 colorKey = SDL_MapRGB(surface->format, 255, 255, 255);
                SDL_SetColorKey(surface, SDL_TRUE, colorKey);
            }
        }
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        
        if(!texture) {
            std::cerr << "FAILED to create texture from surface: " << SDL_GetError() << std::endl;
            return nullptr;
        }
        
        // Enable alpha blending on the texture for transparency support
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        
        m_textures[textureKey] = texture;
        std::cout << "Texture created and cached successfully with transparency enabled" << std::endl;
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
// Sound Manager
// ========================
class SoundManager {
public:
    static SoundManager& getInstance() {
        static SoundManager instance;
        return instance;
    }
    
    // Load a sound effect (WAV file)
    Mix_Chunk* loadSound(const std::string& filePath, const std::string& soundKey) {
        std::cout << "=== LOADING SOUND ===" << std::endl;
        std::cout << "File: " << filePath << std::endl;
        std::cout << "Key: " << soundKey << std::endl;
        
        // Remove existing sound if it exists
        auto it = m_sounds.find(soundKey);
        if(it != m_sounds.end()) {
            Mix_FreeChunk(it->second);
            m_sounds.erase(it);
            std::cout << "Removed old cached sound: " << soundKey << std::endl;
        }
        
        // Load sound effect
        Mix_Chunk* sound = Mix_LoadWAV(filePath.c_str());
        if(!sound) {
            std::cerr << "FAILED to load WAV: " << filePath << " - " << Mix_GetError() << std::endl;
            return nullptr;
        }
        
        m_sounds[soundKey] = sound;
        std::cout << "Sound loaded successfully: " << soundKey << std::endl;
        return sound;
    }
    
    // Play a sound effect
    void playSound(const std::string& soundKey, int loops = 0) {
        auto it = m_sounds.find(soundKey);
        if(it != m_sounds.end()) {
            Mix_PlayChannel(-1, it->second, loops); // -1 = use any available channel
        } else {
            std::cerr << "Sound not found: " << soundKey << std::endl;
        }
    }
    
    // Load background music (MP3, OGG, WAV, etc.)
    bool loadMusic(const std::string& filePath) {
        std::cout << "=== LOADING MUSIC ===" << std::endl;
        std::cout << "File: " << filePath << std::endl;
        
        // Free existing music if any
        if (m_music) {
            Mix_FreeMusic(m_music);
            m_music = nullptr;
        }
        
        // Load music
        m_music = Mix_LoadMUS(filePath.c_str());
        if(!m_music) {
            std::cerr << "FAILED to load music: " << filePath << " - " << Mix_GetError() << std::endl;
            return false;
        }
        
        std::cout << "Music loaded successfully" << std::endl;
        return true;
    }
    
    // Play background music (loops = -1 for infinite loop)
    void playMusic(int loops = -1) {
        if (m_music) {
            if (Mix_PlayMusic(m_music, loops) == -1) {
                std::cerr << "Failed to play music: " << Mix_GetError() << std::endl;
            }
        }
    }
    
    // Pause music
    void pauseMusic() {
        if (Mix_PlayingMusic()) {
            Mix_PauseMusic();
        }
    }
    
    // Resume music
    void resumeMusic() {
        if (Mix_PausedMusic()) {
            Mix_ResumeMusic();
        }
    }
    
    // Stop music
    void stopMusic() {
        Mix_HaltMusic();
    }
    
    void cleanup() {
        for(auto& pair : m_sounds) {
            Mix_FreeChunk(pair.second);
        }
        m_sounds.clear();
        
        if (m_music) {
            Mix_FreeMusic(m_music);
            m_music = nullptr;
        }
    }
    
private:
    SoundManager() = default;
    std::unordered_map<std::string, Mix_Chunk*> m_sounds;
    Mix_Music* m_music = nullptr;
};

class Engine {
    public:
        static Engine& getInstance() {
            static Engine instance;
            return instance;
        }
        
        bool initialize(const std::string& title, int width, int height) {
            // SDL initialization
            if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
                std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
                return false;
            }
            
            // Initialize SDL_image
            int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
            if(!(IMG_Init(imgFlags) & imgFlags)) {
                std::cerr << "SDL_image initialization failed: " << IMG_GetError() << std::endl;
                // Continue anyway, we can still use BMP files
            }
            
            // Initialize SDL_mixer
            if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
                std::cerr << "SDL_mixer initialization failed: " << Mix_GetError() << std::endl;
                return false;
            }
            
            // Initialize SDL_ttf
            if(TTF_Init() == -1) {
                std::cerr << "SDL_ttf initialization failed: " << TTF_GetError() << std::endl;
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
            Mix_CloseAudio();
            IMG_Quit();
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
    const GameObject& parent() const { return *m_parent; }
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

    template<typename T>
    const T* get() const {
        for(const auto& c : components) {
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
// Box2D Integration
// ========================

// Helper functions for Box2D ID validation
inline bool IsValid(b2WorldId id) {
    return id.index1 != 0;
}

inline bool IsValid(b2BodyId id) {
    return id.index1 != 0;
}

// Box2D World Manager (Singleton)
class Box2DWorld {
public:
    static Box2DWorld& getInstance() {
        static Box2DWorld instance;
        return instance;
    }
    
    void initialize(const b2Vec2& gravity = b2Vec2{0.0f, 9.8f}) {
        if (IsValid(m_worldId)) {
            b2DestroyWorld(m_worldId);
        }
        
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = gravity;
        m_worldId = b2CreateWorld(&worldDef);
        
        // Create contact listener
        m_contactListener = std::make_unique<Box2DContactListener>();
        // Note: Box2D C API contact listener setup may vary by version
        // Contact events will be handled through query callbacks if needed
        
        std::cout << "Box2D World initialized with gravity: " << gravity.x << ", " << gravity.y << std::endl;
    }
    
    void update(float dt) {
        if (IsValid(m_worldId)) {
            // Cap delta time for stability
            const float maxDt = 1.0f / 30.0f; // Cap at 30 FPS minimum
            float clampedDt = std::min(dt, maxDt);
            
            b2World_Step(m_worldId, clampedDt, m_velocityIterations);
        }
    }
    
    void shutdown() {
        if (IsValid(m_worldId)) {
            b2DestroyWorld(m_worldId);
            m_worldId = {0};
        }
        m_contactListener.reset();
    }
    
    b2WorldId getWorld() const { return m_worldId; }
    
    // Raycasting
    struct RaycastResult {
        bool hit;
        b2Vec2 point;
        b2Vec2 normal;
        float fraction;
        b2BodyId bodyId;
    };
    
    RaycastResult rayCast(const b2Vec2& point1, const b2Vec2& point2) {
        RaycastResult result;
        result.hit = false;
        result.fraction = 1.0f;
        
        if (!IsValid(m_worldId)) return result;
        
        b2QueryFilter filter = b2DefaultQueryFilter();
        b2Vec2 translation = {point2.x - point1.x, point2.y - point1.y};
        b2RayResult rayResult = b2World_CastRayClosest(m_worldId, point1, translation, filter);
        
        if (rayResult.hit) {
            result.hit = true;
            result.point = rayResult.point;
            result.normal = rayResult.normal;
            result.fraction = rayResult.fraction;
            result.bodyId = b2Shape_GetBody(rayResult.shapeId);
        }
        
        return result;
    }
    
    // AABB Query
    void queryAABB(const b2AABB& aabb, std::vector<b2BodyId>& results) {
        results.clear();
        if (!IsValid(m_worldId)) return;
        
        QueryCallback callback;
        b2QueryFilter filter = b2DefaultQueryFilter();
        b2World_OverlapAABB(m_worldId, aabb, filter, QueryWrapper, &callback);
        results = callback.foundBodies;
    }
    
    Box2DContactListener* getContactListener() const {
        return m_contactListener.get();
    }
    
private:
    Box2DWorld() = default;
    ~Box2DWorld() { shutdown(); }
    Box2DWorld(const Box2DWorld&) = delete;
    Box2DWorld& operator=(const Box2DWorld&) = delete;
    
    b2WorldId m_worldId = {0};
    int m_velocityIterations = 6;
    
    std::unique_ptr<Box2DContactListener> m_contactListener;
    
    // AABB Query callback
    class QueryCallback {
    public:
        std::vector<b2BodyId> foundBodies;
    };
    
    static bool QueryWrapper(b2ShapeId shapeId, void* context) {
        QueryCallback* callback = static_cast<QueryCallback*>(context);
        b2BodyId bodyId = b2Shape_GetBody(shapeId);
        if (IsValid(bodyId)) {
            callback->foundBodies.push_back(bodyId);
        }
        return true; // Continue query
    }
    
};

// Contact Listener for collision events
// Note: Box2D C API contact listening is handled differently than C++ API
// For demonstration, we can check contacts manually using queries
class Box2DContactListener {
public:
    struct Contact {
        b2BodyId bodyIdA;
        b2BodyId bodyIdB;
        GameObject* objA;
        GameObject* objB;
    };
    
    void handleContact(b2BodyId bodyIdA, b2BodyId bodyIdB) {
        if (!IsValid(bodyIdA) || !IsValid(bodyIdB)) return;
        
        GameObject* objA = static_cast<GameObject*>(b2Body_GetUserData(bodyIdA));
        GameObject* objB = static_cast<GameObject*>(b2Body_GetUserData(bodyIdB));
        
        if (objA && objB) {
            // Store contact for processing later
            Contact contact;
            contact.bodyIdA = bodyIdA;
            contact.bodyIdB = bodyIdB;
            contact.objA = objA;
            contact.objB = objB;
            m_contacts.push_back(contact);
        }
    }
    
    std::vector<Contact> getContactsAndClear() {
        std::vector<Contact> contacts = m_contacts;
        m_contacts.clear();
        return contacts;
    }
    
private:
    std::vector<Contact> m_contacts;
};

// ========================
// Required Components
// ========================
// ========================
// Tiling Background Component
// ========================
class KillZoneComponent : public Component {
public:
    KillZoneComponent() = default;
    void update(float dt) override {}
    void draw(SDL_Renderer* renderer, const View& view) override {} // Invisible, no drawing
};

// ========================
// Checkpoint Component
// ========================
class CheckpointComponent : public Component {
public:
    CheckpointComponent() = default;
    void update(float dt) override {}
    void draw(SDL_Renderer* renderer, const View& view) override {} // Drawing handled by SpriteComponent
};

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

// ========================
// Box2D Physics Component (moved here to access BodyComponent)
// ========================
class Box2DPhysicsComponent : public Component {
public:
    enum BodyType {
        STATIC = b2_staticBody,
        KINEMATIC = b2_kinematicBody,
        DYNAMIC = b2_dynamicBody
    };
    
    Box2DPhysicsComponent(BodyType type = DYNAMIC, 
                         float density = 1.0f,
                         float friction = 0.3f,
                         float restitution = 0.1f)
        : m_bodyType(type), m_density(density), 
          m_friction(friction), m_restitution(restitution) {}
    
    ~Box2DPhysicsComponent() {
        destroyBody();
    }
    
    void update(float dt) override {
        auto bodyComp = parent().get<BodyComponent>();
        if (!bodyComp || !IsValid(m_bodyId)) return;
        
        // Sync Box2D position to BodyComponent
        b2Vec2 position = b2Body_GetPosition(m_bodyId);
        b2Rot rotation = b2Body_GetRotation(m_bodyId);
        float angle = b2Rot_GetAngle(rotation);
        
        bodyComp->x = position.x * m_physicsToWorldScale;
        bodyComp->y = position.y * m_physicsToWorldScale;
        bodyComp->angle = angle;
        
        // Update velocity
        b2Vec2 velocity = b2Body_GetLinearVelocity(m_bodyId);
        bodyComp->velocityX = velocity.x;
        bodyComp->velocityY = velocity.y;
    }
    
    void draw(SDL_Renderer* renderer, const View& view) override {
        // Debug drawing can be added here if needed
    }
    
    // Create a box body
    void createBody(float x, float y, float width, float height) {
        auto& world = Box2DWorld::getInstance();
        b2WorldId physicsWorld = world.getWorld();
        if (!IsValid(physicsWorld)) return;
        
        // Destroy existing body if any
        if (IsValid(m_bodyId)) {
            destroyBody();
        }
        
        // Create body definition
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = static_cast<b2BodyType>(m_bodyType);
        bodyDef.position = b2Vec2{x / m_physicsToWorldScale, y / m_physicsToWorldScale};
        
        m_bodyId = b2CreateBody(physicsWorld, &bodyDef);
        
        if (!IsValid(m_bodyId)) return;
        
        // Store GameObject pointer in userData
        b2Body_SetUserData(m_bodyId, &parent());
        
        // Create box shape
        b2Polygon polygon = b2MakeBox(
            (width / 2.0f) / m_physicsToWorldScale,
            (height / 2.0f) / m_physicsToWorldScale
        );
        
        // Create fixture
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = m_density;
        // Friction and restitution are typically set via filter or body properties
        // Some Box2D C API versions may handle these differently
        
        b2CreatePolygonShape(m_bodyId, &shapeDef, &polygon);
    }
    
    // Create a circle body
    void createCircleBody(float x, float y, float radius) {
        auto& world = Box2DWorld::getInstance();
        b2WorldId physicsWorld = world.getWorld();
        if (!IsValid(physicsWorld)) return;
        
        if (IsValid(m_bodyId)) {
            destroyBody();
        }
        
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = static_cast<b2BodyType>(m_bodyType);
        bodyDef.position = b2Vec2{x / m_physicsToWorldScale, y / m_physicsToWorldScale};
        
        m_bodyId = b2CreateBody(physicsWorld, &bodyDef);
        
        if (!IsValid(m_bodyId)) return;
        
        b2Body_SetUserData(m_bodyId, &parent());
        
        b2Circle circle;
        circle.radius = radius / m_physicsToWorldScale;
        
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = m_density;
        
        b2CreateCircleShape(m_bodyId, &shapeDef, &circle);
    }
    
    // Apply force at center
    void applyForce(float forceX, float forceY) {
        if (!IsValid(m_bodyId)) return;
        b2Vec2 force = {forceX, forceY};
        b2Body_ApplyForceToCenter(m_bodyId, force, true);
    }
    
    // Apply force at point
    void applyForceAtPoint(float forceX, float forceY, float pointX, float pointY) {
        if (!IsValid(m_bodyId)) return;
        b2Vec2 force = {forceX, forceY};
        b2Vec2 point = {pointX / m_physicsToWorldScale, pointY / m_physicsToWorldScale};
        b2Body_ApplyForce(m_bodyId, force, point, true);
    }
    
    // Apply impulse at center
    void applyImpulse(float impulseX, float impulseY) {
        if (!IsValid(m_bodyId)) return;
        b2Vec2 impulse = {impulseX, impulseY};
        b2Body_ApplyLinearImpulseToCenter(m_bodyId, impulse, true);
    }
    
    // Set linear velocity
    void setLinearVelocity(float velX, float velY) {
        if (!IsValid(m_bodyId)) return;
        b2Vec2 velocity = {velX, velY};
        b2Body_SetLinearVelocity(m_bodyId, velocity);
    }
    
    // Set angular velocity
    void setAngularVelocity(float velocity) {
        if (!IsValid(m_bodyId)) return;
        b2Body_SetAngularVelocity(m_bodyId, velocity);
    }
    
    // Get body ID
    b2BodyId getBodyId() const { return m_bodyId; }
    
    // Stop the body (set velocity to zero)
    void stopBody() {
        if (!IsValid(m_bodyId)) return;
        b2Body_SetLinearVelocity(m_bodyId, b2Vec2{0, 0});
        b2Body_SetAngularVelocity(m_bodyId, 0);
    }
    
    // Check if body is awake
    bool isAwake() const {
        return IsValid(m_bodyId) ? b2Body_IsAwake(m_bodyId) : false;
    }
    
    void destroyBody() {
        if (IsValid(m_bodyId)) {
            auto& world = Box2DWorld::getInstance();
            b2WorldId physicsWorld = world.getWorld();
            if (IsValid(physicsWorld)) {
                b2DestroyBody(m_bodyId);
                m_bodyId = {0};
            }
        }
    }
    
private:
    b2BodyId m_bodyId = {0};
    BodyType m_bodyType;
    float m_density;
    float m_friction;
    float m_restitution;
    
    // Scale factor: Box2D uses meters, game uses pixels
    // 1 meter = 100 pixels
    static constexpr float m_physicsToWorldScale = 100.0f;
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

// BulletComponent - Marks an object as a bullet that can kill enemies
class BulletComponent : public Component {
public:
    BulletComponent(float directionX = 1.0f, float speed = 500.0f) 
        : m_directionX(directionX), m_speed(speed) {}
    
    void update(float dt) override {
        // Bullet movement is handled by Box2D physics
        // This component just marks the object as a bullet
    }
    
    void draw(SDL_Renderer* renderer, const View& view) override {}
    
    float getDirectionX() const { return m_directionX; }
    float getSpeed() const { return m_speed; }
    
private:
    float m_directionX;  // 1.0 = right, -1.0 = left
    float m_speed;       // Bullet speed
};

// DestructibleBoxComponent - Marks an object as a destructible box that can be destroyed by bullets
class DestructibleBoxComponent : public Component {
public:
    void update(float dt) override {}
    void draw(SDL_Renderer* renderer, const View& view) override {}
    bool isDestructible = true;
};

// SpriteComponent with texture and sprite sheet support
// SpriteComponent with texture and sprite sheet support
class SpriteComponent : public Component {
    public:
        SpriteComponent(const std::string& textureKey = "", SDL_Color color = {255, 255, 255, 255}) 
            : m_textureKey(textureKey), m_color(color), m_texture(nullptr) {}
        
        void update(float dt) override {
            // Update animation frame if needed
            if(m_animated && m_totalFrames > 0 && m_frameDuration > 0.0f) {
                m_animationTimer += dt;
                // Determine how many frames we should loop through
                int frameLoopCount = (m_framesInRow > 0) ? m_framesInRow : m_totalFrames;

                while(m_animationTimer >= m_frameDuration && frameLoopCount > 0) {
                    m_animationTimer -= m_frameDuration;
                    m_currentFrame = (m_currentFrame + 1) % frameLoopCount;
                }
            }
        }
        
        void draw(SDL_Renderer* renderer, const View& view) override {
            auto body = parent().get<BodyComponent>();
            if(!body) return;
            
            // Lazy load texture if not already loaded
            if(!m_texture && !m_textureKey.empty()) {
                m_texture = TextureManager::getInstance().getTexture(m_textureKey);
            }
            
            // Use custom render size if set, otherwise use body dimensions
            float renderWidth = (m_renderWidth > 0) ? m_renderWidth : body->width;
            float renderHeight = (m_renderHeight > 0) ? m_renderHeight : body->height;
            // Apply render offset if set (for centering hitbox within sprite)
            float renderX = body->x + m_renderOffsetX;
            float renderY = body->y + m_renderOffsetY;
            SDL_Rect destRect = view.getTransformedRect(renderX, renderY, renderWidth, renderHeight);
            
            // If we have a texture, use it
            if(m_texture) {
                // For custom source rectangle (specific tile coordinates)
                if(m_usingCustomSource) {
                    SDL_RenderCopy(renderer, m_texture, &m_customSrcRect, &destRect);
                }
                // For sprite sheets with static frames (platforms)
                else if(m_usingSpriteSheet && !m_animated) {
                    // Ensure sprite dimensions are valid
                    if (m_spriteWidth <= 0 || m_spriteHeight <= 0) {
                        SDL_RenderCopy(renderer, m_texture, NULL, &destRect);
                        return;
                    }
                    
                    // Calculate row and column for the static frame
                    int row = m_currentFrame / m_framesPerRow;
                    int col = m_currentFrame % m_framesPerRow;
                    
                    // Calculate source rectangle - EXACTLY one frame
                    int srcX = m_spriteWidth * col;
                    int srcY = m_spriteHeight * row;
                    int srcW = m_spriteWidth;   // Width of ONE frame only
                    int srcH = m_spriteHeight;  // Height of ONE frame only
                    
                    // Get texture dimensions to clamp source rectangle
                    int texWidth, texHeight;
                    SDL_QueryTexture(m_texture, NULL, NULL, &texWidth, &texHeight);
                    
                    // Clamp source rectangle to ensure it doesn't exceed texture bounds
                    if (srcX + srcW > texWidth) srcW = texWidth - srcX;
                    if (srcY + srcH > texHeight) srcH = texHeight - srcY;
                    if (srcX < 0) { srcX = 0; }
                    if (srcY < 0) { srcY = 0; }
                    
                    // Ensure we have valid dimensions
                    if (srcW <= 0) srcW = m_spriteWidth;
                    if (srcH <= 0) srcH = m_spriteHeight;
                    
                    SDL_Rect srcRect = { srcX, srcY, srcW, srcH };
                    if (m_flipHorizontal) {
                        SDL_RenderCopyEx(renderer, m_texture, &srcRect, &destRect, 0.0, NULL, SDL_FLIP_HORIZONTAL);
                    } else {
                        SDL_RenderCopy(renderer, m_texture, &srcRect, &destRect);
                    }
                }
                // For animated sprite sheets (characters, enemies)
                else if(m_usingSpriteSheet && m_animated) {
                    // Ensure sprite dimensions are valid
                    if (m_spriteWidth <= 0 || m_spriteHeight <= 0) {
                        // Fallback to rendering entire texture if dimensions not set
                        if (m_flipHorizontal) {
                            SDL_RenderCopyEx(renderer, m_texture, NULL, &destRect, 0.0, NULL, SDL_FLIP_HORIZONTAL);
                        } else {
                            SDL_RenderCopy(renderer, m_texture, NULL, &destRect);
                        }
                        return;
                    }
                    
                    int row, col;
                    if (m_framesInRow > 0 && m_animationRow >= 0) {
                        // Row-based animation (stay within specified row)
                        // Clamp currentFrame to valid range for this row
                        int safeFrame = m_currentFrame;
                        if (safeFrame < 0) safeFrame = 0;
                        if (safeFrame >= m_framesInRow) safeFrame = safeFrame % m_framesInRow;
                        col = safeFrame;
                        row = m_animationRow;
                        
                        // Ensure row is valid
                        if (row < 0) row = 0;
                        int maxRows = (m_totalFrames + m_framesPerRow - 1) / m_framesPerRow;
                        if (row >= maxRows) row = maxRows - 1;
                    } else {
                        // Default behaviour: iterate through entire sprite sheet
                        int safeFrame = (m_currentFrame >= 0 && m_currentFrame < m_totalFrames) 
                                        ? m_currentFrame 
                                        : (m_currentFrame % m_totalFrames + m_totalFrames) % m_totalFrames;
                        row = safeFrame / m_framesPerRow;
                        col = safeFrame % m_framesPerRow;
                    }
                    
                    // Clamp column to valid range
                    if (col < 0) col = 0;
                    if (col >= m_framesPerRow) col = m_framesPerRow - 1;
                    
                    // Calculate source rectangle - EXACTLY one frame (one tile from tilesheet)
                    int srcX = m_spriteWidth * col;   // X position in tilesheet (column * frameWidth)
                    int srcY = m_spriteHeight * row;  // Y position in tilesheet (row * frameHeight)
                    int srcW = m_spriteWidth;         // Width of EXACTLY one frame
                    int srcH = m_spriteHeight;        // Height of EXACTLY one frame
                    
                    // Get texture dimensions to validate
                    int texWidth, texHeight;
                    SDL_QueryTexture(m_texture, NULL, NULL, &texWidth, &texHeight);
                    
                    // Clamp source rectangle to ensure it doesn't exceed texture bounds
                    if (srcX + srcW > texWidth) srcW = texWidth - srcX;
                    if (srcY + srcH > texHeight) srcH = texHeight - srcY;
                    if (srcX < 0) { srcX = 0; }
                    if (srcY < 0) { srcY = 0; }
                    
                    // Final safety: ensure dimensions are exactly one frame, no more
                    if (srcW > m_spriteWidth) srcW = m_spriteWidth;
                    if (srcH > m_spriteHeight) srcH = m_spriteHeight;
                    if (srcW <= 0) srcW = m_spriteWidth;
                    if (srcH <= 0) srcH = m_spriteHeight;
                    
                    SDL_Rect srcRect = { srcX, srcY, srcW, srcH };
                    if (m_flipHorizontal) {
                        SDL_RenderCopyEx(renderer, m_texture, &srcRect, &destRect, 0.0, NULL, SDL_FLIP_HORIZONTAL);
                    } else {
                        SDL_RenderCopy(renderer, m_texture, &srcRect, &destRect);
                    }
                }
                // For static textures (stretched to fit)
                else {
                    if (m_flipHorizontal) {
                        SDL_RenderCopyEx(renderer, m_texture, NULL, &destRect, 0.0, NULL, SDL_FLIP_HORIZONTAL);
                    } else {
                        SDL_RenderCopy(renderer, m_texture, NULL, &destRect);
                    }
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
        
        // Force sprite to render as full texture (not sprite sheet)
        void setFullTextureMode() {
            m_usingSpriteSheet = false;
            m_usingCustomSource = false;
            m_animated = false;
        }
        
        // For multi-row sprite sheets
        void setSpriteSheet(int frameWidth, int frameHeight, int totalFrames, int framesPerRow, float frameRate = 10.0f) {
            m_usingSpriteSheet = true;
            m_spriteWidth = frameWidth;
            m_spriteHeight = frameHeight;
            m_totalFrames = totalFrames;
            m_framesPerRow = framesPerRow > 0 ? framesPerRow : 1;
            m_frameDuration = (frameRate > 0.0f) ? (1.0f / frameRate) : 0.1f;
            m_animated = true;
            m_animationTimer = 0.0f;
            m_currentFrame = 0;
            m_framesInRow = 0;
            m_animationRow = -1;
            
            std::cout << "Sprite sheet configured: " << frameWidth << "x" << frameHeight 
                      << ", " << totalFrames << " frames, " << m_framesPerRow << " per row, " 
                      << frameRate << " fps" << std::endl;
            std::cout << "  -> Expected texture size: " << (frameWidth * framesPerRow) << "x" 
                      << (frameHeight * ((totalFrames + framesPerRow - 1) / framesPerRow)) << std::endl;
        }

        // Constrain animation to a specific row (optionally specify frames in that row)
        void setAnimationRow(int row, int framesInRow = -1, bool resetFrame = true) {
            m_animationRow = row;
            if (framesInRow > 0) {
                m_framesInRow = framesInRow;
            }

            if (resetFrame) {
                m_currentFrame = 0;
                m_animationTimer = 0.0f;
            }
        }

        int getAnimationRow() const { return m_animationRow; }
        
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
        
        // Set custom render size (overrides body dimensions for rendering only)
        void setRenderSize(float width, float height) {
            m_renderWidth = width;
            m_renderHeight = height;
        }
        
        // Set render offset (adjusts sprite rendering position relative to body)
        void setRenderOffset(float offsetX, float offsetY) {
            m_renderOffsetX = offsetX;
            m_renderOffsetY = offsetY;
        }
        
        // Set horizontal flip
        void setFlipHorizontal(bool flip) {
            m_flipHorizontal = flip;
        }
        
        bool getFlipHorizontal() const {
            return m_flipHorizontal;
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
        int m_animationRow = -1;   // Which row to animate (for multi-row sprite sheets)
        int m_framesInRow = 0;     // Number of frames available in the animation row
        bool m_usingCustomSource = false;
        
        // Custom render size (0 means use body dimensions)
        float m_renderWidth = 0.0f;
        float m_renderHeight = 0.0f;
        
        // Render offset (adjusts sprite position relative to body position)
        float m_renderOffsetX = 0.0f;
        float m_renderOffsetY = 0.0f;
        
        // Flip flag for horizontal flipping
        bool m_flipHorizontal = false;
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
        
        // Check for death by falling - but don't respawn here
        // The Game class will handle death and initials entry
        // This check is kept for backwards compatibility but won't auto-respawn
        if(body->y > deathHeight) {
            // Mark as dead, but don't respawn - let Game class handle it
            m_isDead = true;
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
    bool isMoving() const {
        auto body = parent().get<BodyComponent>();
        auto physics = parent().get<Box2DPhysicsComponent>();
        
        if (physics && IsValid(physics->getBodyId())) {
            // Check Box2D velocity
            b2Vec2 vel = b2Body_GetLinearVelocity(physics->getBodyId());
            return std::abs(vel.x) > 0.1f;
        } else if (body) {
            // Check legacy velocity
            return std::abs(body->velocityX) > 0.1f;
        }
        return false;
    }
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
    float deathHeight = 3000.0f;
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
        
        // Get Box2D physics component if it exists
        auto physics = parent().get<Box2DPhysicsComponent>();
        
        // Check boundaries and update direction
        if(body->x >= rightBound - body->width) movingRight = false;
        if(body->x <= leftBound) movingRight = true;
        
        // Calculate velocity (in world units per second)
        float velocityX = movingRight ? speed : -speed;
        body->velocityX = velocityX;
        
        // If we have Box2D physics, set the body's linear velocity
        if(physics && IsValid(physics->getBodyId())) {
            b2Vec2 currentVel = b2Body_GetLinearVelocity(physics->getBodyId());
            b2Body_SetLinearVelocity(physics->getBodyId(), b2Vec2{velocityX / 100.0f, currentVel.y}); // Convert to Box2D units
        }
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
                currentAttributes["framesPerRow"] = extractAttribute(completeTag, "framesPerRow");
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
            
            // Center the hitbox horizontally and align it to the bottom vertically
            // Sprite renders at 62x50, hitbox is 34x40 (20% smaller width, 20% smaller height)
            // Offset: horizontal = (62-34)/2 = 14px right (centered), vertical = 50-40 = 10px down (bottom-aligned)
            const float spriteRenderWidth = 62.0f;
            const float spriteRenderHeight = 50.0f;
            const float hitboxOffsetX = (spriteRenderWidth - width) / 2.0f;  // Center horizontally
            const float hitboxOffsetY = spriteRenderHeight - height; // Align to bottom (sprite height - hitbox height)
            
            // Adjust body position to center the hitbox within the sprite
            obj->add<BodyComponent>(x + hitboxOffsetX, y + hitboxOffsetY, width, height);
            
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
                
                // Check if framesPerRow is specified (for multi-row sprite sheets)
                int framesPerRow = totalFrames; // Default: assume single row
                if (attrs.find("framesPerRow") != attrs.end()) {
                    framesPerRow = std::stoi(attrs.at("framesPerRow"));
                }
                
                std::cout << "=== CONFIGURING PLAYER SPRITE SHEET ===" << std::endl;
                std::cout << "Frame: " << frameWidth << "x" << frameHeight << std::endl;
                std::cout << "Frames: " << totalFrames << " at " << frameRate << " fps" << std::endl;
                std::cout << "Frames per row: " << framesPerRow << std::endl;
                
                // Use 5-parameter version to support multi-row sprite sheets
                sprite->setSpriteSheet(frameWidth, frameHeight, totalFrames, framesPerRow, frameRate);
                
                // If this is the player character with multi-row sprite sheet, set up row-based animation
                if (texture && framesPerRow < totalFrames) {
                    // Initialize with idle animation (row 0, top row)
                    sprite->setAnimationRow(0, framesPerRow);
                    std::cout << "Player sprite initialized with row-based animation (row 0)" << std::endl;
                }
                
                // Set render size to original size (62x50) while hitbox is 20% smaller in width and 10% smaller in height (34x45)
                sprite->setRenderSize(62.0f, 50.0f);
                
                // Offset sprite rendering back to original position to keep sprite visual position unchanged
                // while hitbox is centered within the sprite
                sprite->setRenderOffset(-hitboxOffsetX, -hitboxOffsetY);
            }
            
            obj->add<ControllerComponent>();
        }
        else if (type == "platform" || type == "moving_platform") {
            // Platform
            std::cout << "=== CREATING PLATFORM ===" << std::endl;
            float x = std::stof(attrs.at("x"));
            float y = std::stof(attrs.at("y"));
            float width = std::stof(attrs.at("width"));
            float height = std::stof(attrs.at("height"));
            
            std::cout << "Platform position: (" << x << "," << y << ") size: " << width << "x" << height << std::endl;
            
            obj->add<BodyComponent>(x, y, width, height);
            obj->add<SolidComponent>();
            
            // Handle sprite with texture or color
            // Always use platform_texture for platforms unless a non-empty color is provided
            if (attrs.find("color") != attrs.end() && !attrs.at("color").empty()) {
                std::cout << "Platform using COLOR instead of texture" << std::endl;
                SDL_Color color = parseColor(attrs.at("color"));
                obj->add<SpriteComponent>("", color);
            } else {
                // Force platforms to use platform_texture (ignore textureKey from XML for platforms)
                std::string textureKey = "platform_texture";
                std::cout << "Forcing platform to use textureKey: " << textureKey << " (ignoring XML textureKey: " 
                          << (attrs.find("textureKey") != attrs.end() ? attrs.at("textureKey") : "none") << ")" << std::endl;
                
                // Create sprite WITHOUT textureKey to avoid lazy loading issues
                auto sprite = obj->add<SpriteComponent>("");
                SDL_Texture* texture = textureManager.getTexture(textureKey);
                if (texture) {
                    // Verify texture dimensions
                    int texWidth, texHeight;
                    SDL_QueryTexture(texture, NULL, NULL, &texWidth, &texHeight);
                    
                    sprite->setTexture(texture);
                    // CRITICAL: Explicitly force sprite to render as full texture (not sprite sheet)
                    sprite->setFullTextureMode();
                    std::cout << "SUCCESS: Platform at (" << x << "," << y << ") using texture: " << textureKey 
                              << " (full texture " << texWidth << "x" << texHeight << ", not sprite sheet)" << std::endl;
                    std::cout << "  -> Sprite explicitly set to full texture mode (m_usingSpriteSheet=false)" << std::endl;
                } else {
                    std::cerr << "ERROR: Platform at (" << x << "," << y << ") texture not found: " << textureKey << std::endl;
                }
            }
            std::cout << "=== FINISHED CREATING PLATFORM ===" << std::endl;
            
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
            
            // Add Box2D physics component to enemies
            // Patrolling enemies (type="enemy") use KINEMATIC to prevent falling
            // Flying enemies (type="flying_enemy") use DYNAMIC to allow gravity/bouncing
            Box2DPhysicsComponent::BodyType bodyType = (type == "enemy") ? 
                Box2DPhysicsComponent::KINEMATIC : 
                Box2DPhysicsComponent::DYNAMIC;
            
            auto enemyPhysics = obj->add<Box2DPhysicsComponent>(
                bodyType,
                1.0f,  // density
                0.3f,  // friction
                0.3f   // restitution
            );
            enemyPhysics->createBody(x, y, width, height);
            
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
                // Only add PatrolBehaviorComponent if the attributes exist
                if (attrs.find("left") != attrs.end() && 
                    attrs.find("right") != attrs.end() && 
                    attrs.find("speed") != attrs.end()) {
                    float left = std::stof(attrs.at("left"));
                    float right = std::stof(attrs.at("right"));
                    float speed = std::stof(attrs.at("speed"));
                    obj->add<PatrolBehaviorComponent>(left, right, speed);
                    std::cout << "Added PatrolBehaviorComponent: left=" << left << ", right=" << right << ", speed=" << speed << std::endl;
                } else {
                    std::cerr << "Warning: Enemy at (" << x << "," << y << ") missing PatrolBehaviorComponent attributes" << std::endl;
                }
            } else if (type == "flying_enemy") {
                // Only add BounceBehaviorComponent if the attributes exist
                if (attrs.find("amplitude") != attrs.end() && 
                    attrs.find("frequency") != attrs.end()) {
                    float amplitude = std::stof(attrs.at("amplitude"));
                    float frequency = std::stof(attrs.at("frequency"));
                    obj->add<BounceBehaviorComponent>(amplitude, frequency);
                } else {
                    std::cerr << "Warning: Flying enemy at (" << x << "," << y << ") missing BounceBehaviorComponent attributes" << std::endl;
                }
            }
        }
        else if (type == "checkpoint") {
            // Checkpoint flag
            float x = std::stof(attrs.at("x"));
            float y = std::stof(attrs.at("y"));
            float width = std::stof(attrs.at("width"));
            float height = std::stof(attrs.at("height"));
            
            obj->add<BodyComponent>(x, y, width, height);
            obj->add<CheckpointComponent>();
            
            // Add sprite for the flag
            if (attrs.find("textureKey") != attrs.end() && !attrs.at("textureKey").empty()) {
                auto sprite = obj->add<SpriteComponent>(attrs.at("textureKey"));
                SDL_Texture* texture = textureManager.getTexture(attrs.at("textureKey"));
                if (texture) {
                    sprite->setTexture(texture);
                }
                
                // Flag texture is a sprite sheet with 5 frames (320px wide), but we only want to show the first frame (static)
                // Use setSourceRect to render only the first 64x64 frame from the sprite sheet
                sprite->setSourceRect(0, 0, 64, 64); // First frame: x=0, y=0, width=64, height=64
                sprite->setRenderSize(64.0f, 64.0f); // Render at 64x64
                
                // Check if sprite sheet should be configured (for animation - not used for flag)
                if (attrs.find("spriteSheet") != attrs.end() && attrs.at("spriteSheet") == "true") {
                    int frameWidth = std::stoi(attrs.at("frameWidth"));
                    int frameHeight = std::stoi(attrs.at("frameHeight"));
                    int totalFrames = std::stoi(attrs.at("totalFrames"));
                    float frameRate = std::stof(attrs.at("frameRate"));
                    
                    // Check if framesPerRow is specified (for multi-row sprite sheets)
                    int framesPerRow = totalFrames; // Default: assume single row
                    if (attrs.find("framesPerRow") != attrs.end()) {
                        framesPerRow = std::stoi(attrs.at("framesPerRow"));
                    }
                    
                    std::cout << "=== CONFIGURING CHECKPOINT SPRITE SHEET ===" << std::endl;
                    std::cout << "Frame: " << frameWidth << "x" << frameHeight << std::endl;
                    std::cout << "Frames: " << totalFrames << " at " << frameRate << " fps" << std::endl;
                    std::cout << "Frames per row: " << framesPerRow << std::endl;
                    
                    // Use 5-parameter version to support multi-row sprite sheets
                    sprite->setSpriteSheet(frameWidth, frameHeight, totalFrames, framesPerRow, frameRate);
                    
                    // Ensure sprite renders at the same size as the body (64x64)
                    // This ensures proper centering - sprite and body should align perfectly
                    sprite->setRenderSize(static_cast<float>(frameWidth), static_cast<float>(frameHeight));
                    
                    // For flag animation: anchor at bottom so only top animates
                    // Body position represents the top-left, but we want bottom to stay fixed
                    // The body is already positioned so its bottom aligns with the platform
                    // No render offset needed - the body position already accounts for bottom alignment
                }
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
        
        // Load sound effects
        auto& soundManager = SoundManager::getInstance();
        soundManager.loadSound("assets/shoot.wav", "shoot_sound");
        
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
            
            // Initialize Box2D world with gravity (keep for potential Box2D features)
            Box2DWorld::getInstance().initialize(b2Vec2{0.0f, 9.8f});
            
            // FORCE COMPLETE CLEANUP
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
            
            // Manually load platform texture BEFORE loading XML to ensure it's available when platforms are created
            auto& textureManager = TextureManager::getInstance();
            SDL_Renderer* renderer = Engine::getRenderer();
            
            // Force reload the texture to clear any old cached version
            SDL_Texture* oldTex = textureManager.getTexture("platform_texture");
            if (oldTex) {
                std::cout << "Clearing old platform_texture from cache..." << std::endl;
            }
            
            SDL_Texture* platformTex = textureManager.loadTexture(renderer, "assets/platform.bmp", "platform_texture");
            if (platformTex) {
                int texWidth, texHeight;
                SDL_QueryTexture(platformTex, NULL, NULL, &texWidth, &texHeight);
                std::cout << "Platform texture loaded successfully! Dimensions: " << texWidth << "x" << texHeight << std::endl;
            } else {
                std::cerr << "WARNING: Failed to load platform texture! Check if assets/platform.bmp exists." << std::endl;
            }
            
            // Load game objects from XML
            m_gameObjects = XMLComponentFactory::createFromXML(Engine::getRenderer(), "scene.xml");
            
            if (m_gameObjects.empty()) {
                std::cerr << "ERROR: No game objects loaded from XML!" << std::endl;
                return false;
            }
            
            debugLoadedObjects();
            
            // Create a destructible box on the first platform
            // Place it at position (200, 470) - on top of the first platform
            createDestructibleBox(200, 470, 40, 40);
            
            // Create destructible boxes on the long platform (platform at x=6200, y=500, width=800)
            // Player must shoot through these boxes to progress
            // Only create first, middle, and last box with increased spacing
            std::vector<float> boxPositions = {6300.0f, 6800.0f, 7300.0f}; // First, middle, end positions (500px spacing)
            createDestructibleBoxesAtPositions(500, 40, 40, boxPositions);
            
            // Create kill zone at Y=3000 (transparent box that kills player on contact)
            createKillZone(0, 3000, 10000, 100); // Wide box spanning the level (taller to prevent tunneling)
            
            // Load and play background music
            auto& soundManager = SoundManager::getInstance();
            // Try different music formats
            if (!soundManager.loadMusic("assets/background_music.mp3")) {
                if (!soundManager.loadMusic("assets/background_music.ogg")) {
                    soundManager.loadMusic("assets/background_music.wav");
                }
            }
            soundManager.playMusic(-1); // Play in infinite loop
            
            // Load font for score display (try to use a default system font or create a simple one)
            // On Windows, try common font paths
            m_font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 24);
            if (!m_font) {
                m_font = TTF_OpenFont("C:/Windows/Fonts/calibri.ttf", 24);
            }
            if (!m_font) {
                // If no system font found, we'll render without font (use simple text rendering)
                std::cerr << "Warning: Could not load font for score display. Score will not be visible." << std::endl;
            }
            
            // Initialize and start timer when game first loads
            m_gameTimer = 0.0f;
            m_timerRunning = true;
            
            std::cout << "=== GAME INITIALIZATION COMPLETE ===" << std::endl;
            std::cout << "Controls:" << std::endl;
            std::cout << "WASD - Move player (W=Jump, A=Left, D=Right)" << std::endl;
            std::cout << "Left Mouse Button or J - Shoot bullet" << std::endl;
            std::cout << "SPACE - Apply impulse upward" << std::endl;
            std::cout << "R - Perform raycast" << std::endl;
            std::cout << "Q - Perform AABB query" << std::endl;
            
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
                    // Safety check: skip invalid events
                    if(event.type == SDL_FIRSTEVENT || event.type >= SDL_LASTEVENT) {
                        continue;
                    }
                    
                    if(event.type == SDL_QUIT) {
                        running = false;
                    }
                    
                    // Handle high scores screen (press enter to continue)
                    if(m_showingHighScores) {
                        if(event.type == SDL_KEYDOWN) {
                            if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                                m_showingHighScores = false;
                                resetGame();
                            }
                        }
                    }
                    
                    // Handle text input for initials entry
                    if(m_enteringInitials && m_textInputActive) {
                        if(event.type == SDL_TEXTINPUT) {
                            // Only accept alphabetic characters
                            char c = event.text.text[0];
                            if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                                if(m_playerInitials.length() < 3) {
                                    // Convert to uppercase
                                    if(c >= 'a' && c <= 'z') {
                                        c = c - 'a' + 'A';
                                    }
                                    m_playerInitials += c;
                                }
                            }
                        } else if(event.type == SDL_KEYDOWN) {
                            if(event.key.keysym.sym == SDLK_BACKSPACE) {
                                if(m_playerInitials.length() > 0) {
                                    m_playerInitials.pop_back();
                                }
                            } else if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                                // Submit if at least 1 character
                                if(m_playerInitials.length() >= 1) {
                                    saveScoreToXML(m_playerInitials, m_score);
                                    m_textInputActive = false;
                                    m_enteringInitials = false;
                                    SDL_StopTextInput();
                                    
                                    // Show high scores
                                    m_showingHighScores = true;
                                }
                            }
                        }
                    }
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
            Box2DWorld::getInstance().shutdown();
            m_gameObjects.clear();
            TextureManager::getInstance().cleanup();
            SoundManager::getInstance().cleanup();
            if (m_font) {
                TTF_CloseFont(m_font);
                m_font = nullptr;
            }
            Engine::getInstance().shutdown();
        }
        
    private:
        void update(float deltaTime) {
            // Don't update game logic if entering initials or showing high scores
            if(m_enteringInitials || m_showingHighScores) {
                return;
            }
            
            // Check for 'R' key to restart (kill player)
            auto& input = InputSystem::getInstance();
            if (input.isKeyJustPressed(SDL_SCANCODE_R)) {
                auto playerObj = findPlayer();
                if (playerObj) {
                    auto playerController = playerObj->get<ControllerComponent>();
                    auto playerPhysics = playerObj->get<Box2DPhysicsComponent>();
                    
                    if (playerController && !playerController->isDead()) {
                        std::cout << "Player restarted by pressing 'R' key" << std::endl;
                        
                        // Stop the physics body immediately
                        if (playerPhysics) {
                            playerPhysics->stopBody();
                        }
                        
                        // Kill the player
                        playerController->die();
                        
                        // Stop background music
                        SoundManager::getInstance().stopMusic();
                        
                        // Show initials entry screen
                        m_enteringInitials = true;
                        m_playerInitials = "";
                        m_textInputActive = true;
                        SDL_StartTextInput();
                        resetGameObjects(); // Reset all game objects
                        return; // Don't continue with other updates
                    }
                }
            }
            
            // Update Box2D physics world first
            Box2DWorld::getInstance().update(deltaTime);
            
            // Handle shooting
            handleShooting();
            
            // Update shooting timer
            if (m_playerShootTimer > 0.0f) {
                m_playerShootTimer -= deltaTime;
                if (m_playerShootTimer < 0.0f) m_playerShootTimer = 0.0f;
            }
            
            // Update shooting cooldown timer
            if (m_shootCooldown > 0.0f) {
                m_shootCooldown -= deltaTime;
                if (m_shootCooldown < 0.0f) m_shootCooldown = 0.0f;
            }
            
            // Update music pause timer (resume music after 3 seconds)
            if (m_musicPauseTimer > 0.0f) {
                m_musicPauseTimer -= deltaTime;
                if (m_musicPauseTimer <= 0.0f) {
                    m_musicPauseTimer = 0.0f;
                    SoundManager::getInstance().resumeMusic();
                }
            }
            
            // Update player animations based on state
            updatePlayerAnimations();
            
            // Update all game objects using proper deltaTime
            for(auto& obj : m_gameObjects) {
                if(obj->isActive) {
                    obj->update(deltaTime);
                }
            }
            
            // Update game timer if it's running
            if (m_timerRunning) {
                m_gameTimer += deltaTime;
            }
            
            // Check for falling death AFTER GameObject update (to catch any falling)
            auto playerObj = findPlayer();
            if (playerObj) {
                auto playerBody = playerObj->get<BodyComponent>();
                auto playerController = playerObj->get<ControllerComponent>();
                if (playerBody && playerController && !playerController->isDead()) {
                    // Debug: print player Y position periodically
                    static int debugCounter = 0;
                    if (++debugCounter % 60 == 0) { // Print every 60 frames (~1 second)
                        std::cout << "DEBUG: Player Y position: " << playerBody->y << std::endl;
                    }
                    
                    // Check if player fell off the map (deathHeight = 3000.0f)
                    if (playerBody->y > 3000.0f) {
                        std::cout << "Player died by falling off the map! Y position: " << playerBody->y << std::endl;
                        
                        // Stop the physics body immediately to prevent infinite falling
                        auto playerPhysics = playerObj->get<Box2DPhysicsComponent>();
                        if (playerPhysics) {
                            playerPhysics->stopBody();
                        }
                        
                        playerController->die();
                        
                        // Restart timer on death
                        m_gameTimer = 0.0f;
                        m_timerRunning = true;
                        
                        // Stop background music
                        SoundManager::getInstance().stopMusic();
                        
                        // Show initials entry screen
                        m_enteringInitials = true;
                        m_playerInitials = "";
                        m_textInputActive = true;
                        SDL_StartTextInput();
                        return; // Don't continue with other updates
                    }
                }
            }
            
            // Update bullets (check out of bounds)
            updateBullets(deltaTime);
            
            // Check bullet-enemy collisions
            checkBulletCollisions();
            
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
            
            // If entering initials, only render the initials screen
            if(m_enteringInitials) {
                // Clear screen
                SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255); // Sky blue background
                SDL_RenderClear(renderer);
                
                // Render initials entry screen
                renderInitialsEntry(renderer);
                SDL_RenderPresent(renderer);
                return;
            }
            
            // If showing high scores, only render the high scores screen
            if(m_showingHighScores) {
                // Clear screen
                SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255); // Sky blue background
                SDL_RenderClear(renderer);
                
                // Render high scores screen
                renderHighScores(renderer);
                SDL_RenderPresent(renderer);
                return;
            }
            
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
            
            // Render score in top left corner
            renderScore(renderer);
            
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
                auto otherKillZone = otherObj->get<KillZoneComponent>();
                auto otherCheckpoint = otherObj->get<CheckpointComponent>();
                
                if(!otherBody) continue;
                
                // Special check for kill zone: check Y position directly (prevents tunneling)
                if(otherKillZone) {
                    // Kill zone spans from otherBody->y to otherBody->y + otherBody->height
                    // Player dies if their bottom (y + height) reaches or passes the kill zone top (y)
                    float playerBottom = playerBody->y + playerBody->height;
                    if(playerBottom >= otherBody->y) {
                        std::cout << "Player died by touching kill zone! Player Y: " << playerBody->y 
                                  << " (bottom: " << playerBottom 
                                  << "), KillZone Y: " << otherBody->y << " to " << (otherBody->y + otherBody->height) << std::endl;
                        
                        // Stop the physics body immediately to prevent continued movement
                        auto playerPhysics = playerObj->get<Box2DPhysicsComponent>();
                        if (playerPhysics) {
                            playerPhysics->stopBody();
                        }
                        
                        playerController->die();
                        
                        // Reset all game objects to initial positions
                        resetGameObjects();
                        
                        // Timer is restarted in resetGameObjects(), so no need to restart here
                        
                        // Stop background music
                        SoundManager::getInstance().stopMusic();
                        
                        // Show initials entry screen
                        m_enteringInitials = true;
                        m_playerInitials = "";
                        m_textInputActive = true;
                        SDL_StartTextInput();
                        
                        return; // Stop checking other collisions
                    }
                }
                
                // Check player collisions - USE FULL BODY SIZE (no scaling)
                if(CollisionSystem::checkCollision(playerBody, otherBody)) {
                    // Check if it's a kill zone - if so, player dies (backup check via AABB)
                    if(otherKillZone) {
                        std::cout << "Player died by touching kill zone (AABB collision)! Player Y: " << playerBody->y 
                                  << ", KillZone Y: " << otherBody->y << " to " << (otherBody->y + otherBody->height) << std::endl;
                        
                        // Stop the physics body immediately to prevent continued movement
                        auto playerPhysics = playerObj->get<Box2DPhysicsComponent>();
                        if (playerPhysics) {
                            playerPhysics->stopBody();
                        }
                        
                        playerController->die();
                        
                        // Reset all game objects to initial positions
                        resetGameObjects();
                        
                        // Timer is restarted in resetGameObjects(), so no need to restart here
                        
                        // Stop background music
                        SoundManager::getInstance().stopMusic();
                        
                        // Show initials entry screen
                        m_enteringInitials = true;
                        m_playerInitials = "";
                        m_textInputActive = true;
                        SDL_StartTextInput();
                        
                        return; // Stop checking other collisions
                    }
                    
                    // Check if it's a checkpoint - if so, award points and restart game
                    if(otherCheckpoint) {
                        std::cout << "Player reached checkpoint!" << std::endl;
                        
                        // Stop the physics body immediately
                        auto playerPhysics = playerObj->get<Box2DPhysicsComponent>();
                        if (playerPhysics) {
                            playerPhysics->stopBody();
                        }
                        
                        // Calculate points based on completion time
                        int timeBonus = calculateTimeBonus(m_gameTimer);
                        m_score += timeBonus;
                        
                        // Log completion time and points
                        int minutes = static_cast<int>(m_gameTimer) / 60;
                        int seconds = static_cast<int>(m_gameTimer) % 60;
                        std::cout << "Completion time: " << minutes << ":" << std::setfill('0') << std::setw(2) << seconds 
                                  << " - Awarded " << timeBonus << " points! Total score: " << m_score << std::endl;
                        
                        // Reset all game objects to initial positions
                        resetGameObjects();
                        
                        // Timer is restarted in resetGameObjects(), so no need to restart here
                        
                        // Restart background music
                        SoundManager::getInstance().playMusic();
                        
                        return; // Stop checking other collisions
                    }
                    
                    // Check if it's an enemy - if so, player dies and respawns
                    if(otherEnemy) {
                        std::cout << "Player died by enemy collision!" << std::endl;
                        
                        // Stop the physics body immediately to prevent continued movement
                        auto playerPhysics = playerObj->get<Box2DPhysicsComponent>();
                        if (playerPhysics) {
                            playerPhysics->stopBody();
                        }
                        
                        playerController->die();
                        
                        // Reset all game objects to initial positions
                        resetGameObjects();
                        
                        // Stop background music
                        SoundManager::getInstance().stopMusic();
                        
                        // Show initials entry screen
                        m_enteringInitials = true;
                        m_playerInitials = "";
                        m_textInputActive = true;
                        SDL_StartTextInput();
                        
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
                    // Draw green rectangle using the ACTUAL BodyComponent dimensions
                    SDL_Rect visualRect = mainView.getTransformedRect(
                        playerBody->x, playerBody->y, 
                        playerBody->width,  // Use actual width from BodyComponent
                        playerBody->height  // Use actual height from BodyComponent
                    );
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 64); // Semi-transparent green
                    SDL_RenderDrawRect(renderer, &visualRect);
                    
                    // Debug: Print dimensions occasionally to verify they're correct
                    static int debugCounter = 0;
                    if (debugCounter++ % 300 == 0) {
                        std::cout << "DEBUG: Green rectangle dimensions - width: " << playerBody->width 
                                  << ", height: " << playerBody->height << std::endl;
                    }
                }
            }
        }
        
        void renderScore(SDL_Renderer* renderer) {
            if (!m_font) return; // Can't render without font
            
            // Create score text
            std::string scoreText = "Score: " + std::to_string(m_score);
            
            // Create surface from text
            SDL_Color textColor = {255, 255, 255, 255}; // White text
            SDL_Surface* textSurface = TTF_RenderText_Solid(m_font, scoreText.c_str(), textColor);
            if (!textSurface) {
                return; // Failed to create text surface
            }
            
            // Create texture from surface
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (!textTexture) {
                SDL_FreeSurface(textSurface);
                return; // Failed to create texture
            }
            
            // Get text dimensions
            int textWidth = textSurface->w;
            int textHeight = textSurface->h;
            
            // Render score in top left corner (screen coordinates, not world coordinates)
            SDL_Rect destRect = {10, 10, textWidth, textHeight}; // 10 pixels from top-left
            SDL_RenderCopy(renderer, textTexture, NULL, &destRect);
            
            // Clean up score texture
            SDL_DestroyTexture(textTexture);
            SDL_FreeSurface(textSurface);
            
            // Format and render timer (MM:SS.mmm format)
            int minutes = static_cast<int>(m_gameTimer) / 60;
            int seconds = static_cast<int>(m_gameTimer) % 60;
            int milliseconds = static_cast<int>((m_gameTimer - static_cast<int>(m_gameTimer)) * 1000);
            
            char timerBuffer[32];
            std::snprintf(timerBuffer, sizeof(timerBuffer), "Time: %02d:%02d.%03d", minutes, seconds, milliseconds);
            std::string timerText = timerBuffer;
            
            // Create timer text surface
            SDL_Surface* timerSurface = TTF_RenderText_Solid(m_font, timerText.c_str(), textColor);
            if (!timerSurface) {
                return; // Failed to create timer surface
            }
            
            // Create texture from surface
            SDL_Texture* timerTexture = SDL_CreateTextureFromSurface(renderer, timerSurface);
            if (!timerTexture) {
                SDL_FreeSurface(timerSurface);
                return; // Failed to create texture
            }
            
            // Render timer below score (10 pixels below score, same left margin)
            SDL_Rect timerRect = {10, 10 + textHeight + 5, timerSurface->w, timerSurface->h};
            SDL_RenderCopy(renderer, timerTexture, NULL, &timerRect);
            
            // Clean up timer texture
            SDL_DestroyTexture(timerTexture);
            SDL_FreeSurface(timerSurface);
        }
        
        void renderInitialsEntry(SDL_Renderer* renderer) {
            if (!m_font) return;
            
            int screenWidth = 800;
            int screenHeight = 600;
            
            // Draw semi-transparent overlay
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180); // Semi-transparent black
            SDL_Rect overlay = {0, 0, screenWidth, screenHeight};
            SDL_RenderFillRect(renderer, &overlay);
            
            // Render win message or prompt text
            SDL_Color textColor = {255, 255, 255, 255}; // White
            std::string promptText;
            if (m_gameWon) {
                promptText = "YOU WIN! Enter Initials to Save Score";
            } else {
                promptText = "Enter Initials to Save Score";
            }
            SDL_Surface* promptSurface = TTF_RenderText_Solid(m_font, promptText.c_str(), textColor);
            if (promptSurface) {
                SDL_Texture* promptTexture = SDL_CreateTextureFromSurface(renderer, promptSurface);
                if (promptTexture) {
                    int promptX = (screenWidth - promptSurface->w) / 2;
                    int promptY = screenHeight / 2 - 80;
                    SDL_Rect promptRect = {promptX, promptY, promptSurface->w, promptSurface->h};
                    SDL_RenderCopy(renderer, promptTexture, NULL, &promptRect);
                    SDL_DestroyTexture(promptTexture);
                }
                SDL_FreeSurface(promptSurface);
            }
            
            // Render initials input field
            std::string displayText = "";
            for (int i = 0; i < 3; i++) {
                if (i < static_cast<int>(m_playerInitials.length())) {
                    displayText += m_playerInitials[i];
                } else {
                    displayText += "_";
                }
                if (i < 2) {
                    displayText += " ";
                }
            }
            
            SDL_Surface* initialsSurface = TTF_RenderText_Solid(m_font, displayText.c_str(), textColor);
            if (initialsSurface) {
                SDL_Texture* initialsTexture = SDL_CreateTextureFromSurface(renderer, initialsSurface);
                if (initialsTexture) {
                    int initialsX = (screenWidth - initialsSurface->w) / 2;
                    int initialsY = screenHeight / 2 - 20;
                    SDL_Rect initialsRect = {initialsX, initialsY, initialsSurface->w, initialsSurface->h};
                    SDL_RenderCopy(renderer, initialsTexture, NULL, &initialsRect);
                    SDL_DestroyTexture(initialsTexture);
                }
                SDL_FreeSurface(initialsSurface);
            }
            
            // Render instruction text
            std::string instructionText = "Press ENTER to submit (min 1 char)";
            SDL_Surface* instructionSurface = TTF_RenderText_Solid(m_font, instructionText.c_str(), textColor);
            if (instructionSurface) {
                SDL_Texture* instructionTexture = SDL_CreateTextureFromSurface(renderer, instructionSurface);
                if (instructionTexture) {
                    int instructionX = (screenWidth - instructionSurface->w) / 2;
                    int instructionY = screenHeight / 2 + 40;
                    SDL_Rect instructionRect = {instructionX, instructionY, instructionSurface->w, instructionSurface->h};
                    SDL_RenderCopy(renderer, instructionTexture, NULL, &instructionRect);
                    SDL_DestroyTexture(instructionTexture);
                }
                SDL_FreeSurface(instructionSurface);
            }
        }
        
        void saveScoreToXML(const std::string& initials, int score) {
            using namespace tinyxml2;
            
            XMLDocument doc;
            std::string filename = "scores.xml";
            
            // Load existing scores
            std::vector<std::pair<std::string, int>> scores; // initials, score pairs
            
            if (doc.LoadFile(filename.c_str()) == XML_SUCCESS) {
                XMLElement* root = doc.FirstChildElement("HighScores");
                if (root) {
                    XMLElement* scoreElem = root->FirstChildElement("Score");
                    while (scoreElem) {
                        const char* init = scoreElem->Attribute("initials");
                        int val = scoreElem->IntAttribute("value");
                        if (init && val >= 0) {
                            scores.push_back({std::string(init), val});
                        }
                        scoreElem = scoreElem->NextSiblingElement("Score");
                    }
                }
            }
            
            // Add new score
            scores.push_back({initials, score});
            
            // Sort by score (descending)
            std::sort(scores.begin(), scores.end(), 
                [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                    return a.second > b.second;
                });
            
            // Keep only top 10
            if (scores.size() > 10) {
                scores.resize(10);
            }
            
            // Store top 10 for display
            m_highScores = scores;
            
            // Create new XML document with top 10 scores
            doc.Clear();
            XMLElement* root = doc.NewElement("HighScores");
            doc.InsertFirstChild(root);
            
            for (const auto& scorePair : scores) {
                XMLElement* scoreEntry = doc.NewElement("Score");
                scoreEntry->SetAttribute("initials", scorePair.first.c_str());
                scoreEntry->SetAttribute("value", scorePair.second);
                
                // Add timestamp
                time_t now = time(0);
                char* dt = ctime(&now);
                std::string timeStr(dt);
                timeStr.pop_back(); // Remove newline
                scoreEntry->SetAttribute("date", timeStr.c_str());
                
                root->InsertEndChild(scoreEntry);
            }
            
            // Save file
            if (doc.SaveFile(filename.c_str()) == XML_SUCCESS) {
                std::cout << "Score saved: " << initials << " - " << score << std::endl;
            } else {
                std::cerr << "Failed to save score to XML" << std::endl;
            }
        }
        
        void loadHighScores() {
            using namespace tinyxml2;
            
            XMLDocument doc;
            std::string filename = "scores.xml";
            
            m_highScores.clear();
            
            if (doc.LoadFile(filename.c_str()) == XML_SUCCESS) {
                XMLElement* root = doc.FirstChildElement("HighScores");
                if (root) {
                    XMLElement* scoreElem = root->FirstChildElement("Score");
                    while (scoreElem) {
                        const char* init = scoreElem->Attribute("initials");
                        int val = scoreElem->IntAttribute("value");
                        if (init && val >= 0) {
                            m_highScores.push_back({std::string(init), val});
                        }
                        scoreElem = scoreElem->NextSiblingElement("Score");
                    }
                }
            }
            
            // Sort by score (descending)
            std::sort(m_highScores.begin(), m_highScores.end(), 
                [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                    return a.second > b.second;
                });
        }
        
        void renderHighScores(SDL_Renderer* renderer) {
            if (!m_font) return;
            
            int screenWidth = 800;
            int screenHeight = 600;
            
            // Draw semi-transparent overlay
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200); // Darker overlay
            SDL_Rect overlay = {0, 0, screenWidth, screenHeight};
            SDL_RenderFillRect(renderer, &overlay);
            
            SDL_Color textColor = {255, 255, 255, 255}; // White
            
            // Render title
            std::string titleText = "HIGH SCORES";
            SDL_Surface* titleSurface = TTF_RenderText_Solid(m_font, titleText.c_str(), textColor);
            if (titleSurface) {
                SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
                if (titleTexture) {
                    int titleX = (screenWidth - titleSurface->w) / 2;
                    int titleY = 100;
                    SDL_Rect titleRect = {titleX, titleY, titleSurface->w, titleSurface->h};
                    SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
                    SDL_DestroyTexture(titleTexture);
                }
                SDL_FreeSurface(titleSurface);
            }
            
            // Render scores (up to 10)
            int startY = 180;
            int lineHeight = 35;
            int maxScores = std::min(10, static_cast<int>(m_highScores.size()));
            
            for (int i = 0; i < maxScores; i++) {
                std::string scoreText = std::to_string(i + 1) + ". " + m_highScores[i].first + " - " + std::to_string(m_highScores[i].second);
                SDL_Surface* scoreSurface = TTF_RenderText_Solid(m_font, scoreText.c_str(), textColor);
                if (scoreSurface) {
                    SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
                    if (scoreTexture) {
                        int scoreX = (screenWidth - scoreSurface->w) / 2;
                        int scoreY = startY + (i * lineHeight);
                        SDL_Rect scoreRect = {scoreX, scoreY, scoreSurface->w, scoreSurface->h};
                        SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
                        SDL_DestroyTexture(scoreTexture);
                    }
                    SDL_FreeSurface(scoreSurface);
                }
            }
            
            // Render instruction text
            std::string instructionText = "Press ENTER to continue";
            SDL_Surface* instructionSurface = TTF_RenderText_Solid(m_font, instructionText.c_str(), textColor);
            if (instructionSurface) {
                SDL_Texture* instructionTexture = SDL_CreateTextureFromSurface(renderer, instructionSurface);
                if (instructionTexture) {
                    int instructionX = (screenWidth - instructionSurface->w) / 2;
                    int instructionY = screenHeight - 80;
                    SDL_Rect instructionRect = {instructionX, instructionY, instructionSurface->w, instructionSurface->h};
                    SDL_RenderCopy(renderer, instructionTexture, NULL, &instructionRect);
                    SDL_DestroyTexture(instructionTexture);
                }
                SDL_FreeSurface(instructionSurface);
            }
        }
        
        // Calculate points based on completion time
        int calculateTimeBonus(float timeInSeconds) {
            if (timeInSeconds >= 120.0f) {
                // Over 2 minutes = 0 points
                return 0;
            } else if (timeInSeconds >= 90.0f) {
                // 1:30-2 minutes = 25 points
                return 25;
            } else if (timeInSeconds >= 60.0f) {
                // 1 minute - 1:29 minutes = 50 points
                return 50;
            } else if (timeInSeconds >= 50.0f) {
                // 50-59 seconds = 70 points
                return 70;
            } else if (timeInSeconds >= 40.0f) {
                // 40-49 seconds = 80 points
                return 80;
            } else if (timeInSeconds >= 30.0f) {
                // 30-39 seconds = 90 points
                return 90;
            } else if (timeInSeconds >= 20.0f) {
                // 20-29 seconds = 100 points
                return 100;
            } else if (timeInSeconds >= 15.0f) {
                // 15-19 seconds = 125 points
                return 125;
            } else {
                // 0-14 seconds = 150 points
                return 150;
            }
        }
        
        void resetGame() {
            // Reset score
            m_score = 0;
            
            // Reset win state
            m_gameWon = false;
            
            // Reset and start timer
            m_gameTimer = 0.0f;
            m_timerRunning = true;
            
            // Reset player
            auto playerObj = findPlayer();
            if (playerObj) {
                auto playerController = playerObj->get<ControllerComponent>();
                if (playerController) {
                    playerController->respawn();
                }
            }
            
            // Restart music
            SoundManager::getInstance().playMusic();
        }
        
        // ========================
        // Box2D Demo Methods
        // ========================
        void createPlatform(float x, float y, float width, float height, SDL_Texture* platformTexture) {
            auto platform = std::make_unique<GameObject>();
            platform->add<BodyComponent>(x, y, width, height);
            // Create sprite with empty textureKey, then explicitly set texture to avoid sprite sheet confusion
            auto platformSprite = platform->add<SpriteComponent>("");
            if (platformTexture) {
                platformSprite->setTexture(platformTexture);
                int texWidth, texHeight;
                SDL_QueryTexture(platformTexture, NULL, NULL, &texWidth, &texHeight);
                std::cout << "createPlatform: Using platform texture " << texWidth << "x" << texHeight << " (full texture)" << std::endl;
            } else {
                std::cerr << "WARNING: createPlatform called with null platformTexture!" << std::endl;
            }
            auto platformPhysics = platform->add<Box2DPhysicsComponent>(Box2DPhysicsComponent::STATIC, 0.0f, 0.7f, 0.1f);
            platformPhysics->createBody(x, y, width, height);
            platform->add<SolidComponent>();
            m_gameObjects.push_back(std::move(platform));
        }
        
        void createBox2DDemoScene() {
            auto& textureManager = TextureManager::getInstance();
            SDL_Renderer* renderer = Engine::getRenderer();
            
            // Load textures
            textureManager.loadTexture(renderer, "assets/background.bmp", "background_texture");
            textureManager.loadTexture(renderer, "assets/character.bmp", "player_texture");
            textureManager.loadTexture(renderer, "assets/enemy.bmp", "enemy_texture");
            textureManager.loadTexture(renderer, "assets/tileset.bmp", "tile_texture");
            textureManager.loadTexture(renderer, "assets/platform.bmp", "platform_texture");
            
            SDL_Texture* platformTexture = textureManager.getTexture("platform_texture");
            SDL_Texture* tileTexture = textureManager.getTexture("tile_texture");
            
            // Create tiling background
            auto background = std::make_unique<GameObject>();
            background->add<TilingBackgroundComponent>("background_texture", 0.0f, 0.0f);
            m_gameObjects.push_back(std::move(background));
            
            // Platform layout settings
            const float platformHeight = 30.0f;
            const float platformY = 500.0f; // Y position for platforms
            const float platform1Width = 200.0f;
            const float platform1X = 50.0f;
            const float gapWidth = 150.0f; // Gap between platforms
            const float platform2X = platform1X + platform1Width + gapWidth;
            const float platform2Width = 300.0f;
            
            // Create first platform (empty - no enemies)
            createPlatform(platform1X, platformY, platform1Width, platformHeight, platformTexture);
            
            // Create second platform (with enemies)
            createPlatform(platform2X, platformY, platform2Width, platformHeight, platformTexture);
            
            // Create bottom ground/platform at bottom of screen for safety
            createPlatform(0, 550, 800, 50, platformTexture);
            
            // Create left wall with tileset
            auto leftWall = std::make_unique<GameObject>();
            leftWall->add<BodyComponent>(0, 0, 20, 600);
            auto leftWallSprite = leftWall->add<SpriteComponent>("tile_texture");
            if (tileTexture) {
                leftWallSprite->setTexture(tileTexture);
                leftWallSprite->setTile(1, 0, 16, 16); // Use different tile for wall
            }
            auto leftWallPhysics = leftWall->add<Box2DPhysicsComponent>(Box2DPhysicsComponent::STATIC, 0.0f, 0.7f, 0.1f);
            leftWallPhysics->createBody(0, 0, 20, 600);
            m_gameObjects.push_back(std::move(leftWall));
            
            // Create right wall with tileset
            auto rightWall = std::make_unique<GameObject>();
            rightWall->add<BodyComponent>(780, 0, 20, 600);
            auto rightWallSprite = rightWall->add<SpriteComponent>("tile_texture");
            if (tileTexture) {
                rightWallSprite->setTexture(tileTexture);
                rightWallSprite->setTile(1, 0, 16, 16);
            }
            auto rightWallPhysics = rightWall->add<Box2DPhysicsComponent>(Box2DPhysicsComponent::STATIC, 0.0f, 0.7f, 0.1f);
            rightWallPhysics->createBody(780, 0, 20, 600);
            m_gameObjects.push_back(std::move(rightWall));
            
            // Create player character on first platform (empty platform)
            const float playerStartX = platform1X + 50.0f; // Center-ish on first platform
            const float playerStartY = platformY - 80.0f; // Above the platform
            auto player = std::make_unique<GameObject>();
            
            // Tilesheet: 8 columns x 7 rows (56 frames total)
            // Each tile: 64x32 pixels (assuming 512x224 texture: 512/8=64, 224/7=32)
            const int tileWidth = 64;   // 512 / 8
            const int tileHeight = 32;  // 224 / 7
            
            // Match hitbox to sprite size
            const float playerWidth = static_cast<float>(tileWidth);   // 64 pixels (matches sprite frame width)
            const float playerHeight = static_cast<float>(tileHeight); // 32 pixels (matches sprite frame height)
            
            std::cout << "Player hitbox size: " << playerWidth << "x" << playerHeight << " pixels" << std::endl;
            
            player->add<BodyComponent>(playerStartX, playerStartY, playerWidth, playerHeight);
            auto playerSprite = player->add<SpriteComponent>("player_texture");
            SDL_Texture* playerTexture = textureManager.getTexture("player_texture");
            if (playerTexture) {
                loadCharacterSprite(playerSprite, playerTexture);
            }
            auto playerPhysics = player->add<Box2DPhysicsComponent>(Box2DPhysicsComponent::DYNAMIC, 1.0f, 0.3f, 0.3f);
            playerPhysics->createBody(playerStartX, playerStartY, playerWidth, playerHeight);
            m_gameObjects.push_back(std::move(player));
            // Track player as first dynamic body
            m_dynamicBodies.push_back(m_gameObjects.back().get());
            
            // Create enemies on the second platform
            const int numEnemies = 3;
            const float enemySpacing = (platform2Width - 100.0f) / (numEnemies + 1); // Space them evenly
            const float enemyY = platformY - 64.0f; // Above the platform
            
            // Enemy tilesheet: 664x64 pixels, 1 row x 8 columns (8 frames total)
            // Each frame: 664/8 = 83 pixels wide, 64 pixels tall
            const int enemyFrameWidth = 83;   // 664 / 8
            const int enemyFrameHeight = 64;  // 64 pixels tall
            const int enemyTotalFrames = 8;   // 8 frames in 1 row
            const int enemyFramesPerRow = 8;  // 8 frames per row
            
            for (int i = 0; i < numEnemies; ++i) {
                float enemyX = platform2X + 50.0f + (i + 1) * enemySpacing; // Position evenly on platform
                auto enemy = std::make_unique<GameObject>();
                enemy->add<BodyComponent>(enemyX, enemyY, enemyFrameWidth, enemyFrameHeight);
                auto enemySprite = enemy->add<SpriteComponent>("enemy_texture");
                SDL_Texture* enemyTexture = textureManager.getTexture("enemy_texture");
                if (enemyTexture) {
                    loadEnemySprite(enemySprite, enemyTexture);
                }
                auto enemyPhysics = enemy->add<Box2DPhysicsComponent>(Box2DPhysicsComponent::DYNAMIC, 1.0f, 0.3f, 0.3f);
                enemyPhysics->createBody(enemyX, enemyY, enemyFrameWidth, enemyFrameHeight);
                enemy->add<EnemyComponent>();
                m_gameObjects.push_back(std::move(enemy));
                m_dynamicBodies.push_back(m_gameObjects.back().get());
            }
        }
        
        // Helper function to load and configure character sprite
        // Character tilesheet: 8x7 grid (8 columns x 7 rows = 56 frames total)
        void loadCharacterSprite(SpriteComponent* sprite, SDL_Texture* texture) {
            if (!sprite || !texture) return;
            
            // Get actual texture dimensions to verify
            int texWidth, texHeight;
            SDL_QueryTexture(texture, NULL, NULL, &texWidth, &texHeight);
            std::cout << "Character texture loaded: " << texWidth << "x" << texHeight << " pixels" << std::endl;
            
            // Character tilesheet: 512x224 pixels, 8x7 grid (8 columns x 7 rows = 56 frames total)
            // Grid layout: 8 columns, 7 rows
            // Each frame: 512/8 = 64 pixels wide, 224/7 = 32 pixels tall
            const int characterFrameWidth = 64;   // 512 / 8 columns
            const int characterFrameHeight = 32;  // 224 / 7 rows
            const int characterTotalFrames = 56;  // 8 columns * 7 rows = 56 frames
            const int characterFramesPerRow = 8;  // 8 frames per row (8 columns)
            const int characterRows = 7;            // 7 rows in the grid
            
            sprite->setTexture(texture);
            // Configure character sprite sheet: 8x7 grid, 512x224 pixels, 56 frames total
            sprite->setSpriteSheet(characterFrameWidth, characterFrameHeight, characterTotalFrames, characterFramesPerRow, 10.0f);
            // Initialize with idle animation (row 0, top row)
            sprite->setAnimationRow(0, 8); // Start with idle animation
            
            std::cout << "Character sprite configured: " << characterFrameWidth << "x" << characterFrameHeight 
                      << " frames, 8x7 grid, starting at row 0" << std::endl;
        }
        
        // Helper function to load and configure enemy sprite
        void loadEnemySprite(SpriteComponent* sprite, SDL_Texture* texture) {
            if (!sprite || !texture) return;
            
            // Enemy tilesheet: 664x64 pixels, 1 row x 8 columns (8 frames total)
            // Each frame: 664/8 = 83 pixels wide, 64 pixels tall
            const int enemyFrameWidth = 83;   // 664 / 8
            const int enemyFrameHeight = 64;  // 64 pixels tall
            const int enemyTotalFrames = 8;    // 8 frames in 1 row
            const int enemyFramesPerRow = 8;   // 8 frames per row
            
            sprite->setTexture(texture);
            // Configure enemy sprite sheet: 664x64, 1 row x 8 columns, 8 frames total
            sprite->setSpriteSheet(enemyFrameWidth, enemyFrameHeight, enemyTotalFrames, enemyFramesPerRow, 8.0f);
        }
        
        bool isPlayerGrounded(Box2DPhysicsComponent* physics, BodyComponent* body) {
            if (!IsValid(physics->getBodyId()) || !body) return false;
            
            // Cast a short ray downward from the player's bottom edge
            // Box2D body position is at center, so we need to account for half height
            float halfHeight = (body->height / 2.0f) / 100.0f; // Convert to meters
            float checkDistance = 8.0f / 100.0f; // 8 pixels in meters
            b2Vec2 playerPos = b2Body_GetPosition(physics->getBodyId());
            b2Vec2 rayStart = {playerPos.x, playerPos.y + halfHeight}; // Bottom of player
            b2Vec2 rayEnd = {playerPos.x, playerPos.y + halfHeight + checkDistance};
            
            auto result = Box2DWorld::getInstance().rayCast(rayStart, rayEnd);
            return result.hit && result.fraction < 1.0f;
        }
        
        void handleBox2DInput() {
            auto& input = InputSystem::getInstance();
            
            // Find player character (first dynamic body with Box2D physics)
            GameObject* playerObj = nullptr;
            Box2DPhysicsComponent* playerPhysics = nullptr;
            BodyComponent* playerBody = nullptr;
            for (auto& obj : m_gameObjects) {
                // Skip background objects
                if (obj->get<TilingBackgroundComponent>()) continue;
                
                playerPhysics = obj->get<Box2DPhysicsComponent>();
                if (playerPhysics && playerPhysics->getBodyId().index1 != 0) {
                    playerObj = obj.get();
                    playerBody = obj->get<BodyComponent>();
                    break; // Use first dynamic body as "player"
                }
            }
            
            if (!playerPhysics || !playerBody) return;
            
            // Check if player is grounded for jumping
            bool grounded = isPlayerGrounded(playerPhysics, playerBody);
            
            // Get current velocity
            b2Vec2 currentVel = b2Body_GetLinearVelocity(playerPhysics->getBodyId());
            
            // WASD Controls
            const float moveSpeed = 300.0f; // Pixels per second (will be converted to meters)
            const float jumpImpulse = 500.0f; // Jump strength
            const float maxSpeed = 400.0f; // Max horizontal speed in pixels per second
            const float maxSpeedMeters = maxSpeed / 100.0f; // Convert to m/s
            
            // W - Jump (only when grounded)
            if ((input.isKeyJustPressed(SDL_SCANCODE_W) || input.isKeyJustPressed(SDL_SCANCODE_UP)) && grounded) {
                playerPhysics->applyImpulse(0, -jumpImpulse);
            }
            
            // A - Move left
            if (input.isKeyPressed(SDL_SCANCODE_A) || input.isKeyPressed(SDL_SCANCODE_LEFT)) {
                // Apply force left, but clamp to max speed
                // currentVel.x is in m/s, maxSpeedMeters is in m/s
                if (currentVel.x > -maxSpeedMeters) {
                    playerPhysics->applyForce(-moveSpeed * 2.0f, 0);
                }
            }
            
            // D - Move right
            if (input.isKeyPressed(SDL_SCANCODE_D) || input.isKeyPressed(SDL_SCANCODE_RIGHT)) {
                // Apply force right, but clamp to max speed
                if (currentVel.x < maxSpeedMeters) {
                    playerPhysics->applyForce(moveSpeed * 2.0f, 0);
                }
            }
            
            // S - Down (optional: could be used for crouch or fast fall)
            // For now, we'll leave it unused
            
            // Legacy controls still available
            // SPACE - Apply impulse upward (works even when not grounded)
            if (input.isKeyJustPressed(SDL_SCANCODE_SPACE)) {
                playerPhysics->applyImpulse(0, -500);
            }
            
            // F - Apply force to the right
            if (input.isKeyJustPressed(SDL_SCANCODE_F)) {
                playerPhysics->applyForce(200, 0);
            }
            
            // R - Perform raycast
            if (input.isKeyJustPressed(SDL_SCANCODE_R)) {
                performRaycast();
            }
            
            // Q - Perform AABB query
            if (input.isKeyJustPressed(SDL_SCANCODE_Q)) {
                performAABBQuery();
            }
            
            // C - Create new box
            if (input.isKeyJustPressed(SDL_SCANCODE_C)) {
                createDynamicBox(100 + (rand() % 600), 100, 30, 30);
                std::cout << "Created new box! Total dynamic bodies: " << m_dynamicBodies.size() << std::endl;
            }
            
            // X - Remove last box
            if (input.isKeyJustPressed(SDL_SCANCODE_X)) {
                removeLastDynamicBody();
            }
            
            // V - Set random velocity
            if (input.isKeyJustPressed(SDL_SCANCODE_V)) {
                float velX = (rand() % 200) - 100;
                float velY = (rand() % 200) - 100;
                playerPhysics->setLinearVelocity(velX, velY);
                std::cout << "Set velocity to: " << velX << ", " << velY << std::endl;
            }
        }
        
        void performRaycast() {
            // Cast ray from center of screen downward
            float startX = 400.0f / 100.0f; // Convert pixels to meters
            float startY = 100.0f / 100.0f;
            float endX = 400.0f / 100.0f;
            float endY = 600.0f / 100.0f;
            
            b2Vec2 start = {startX, startY};
            b2Vec2 end = {endX, endY};
            
            auto result = Box2DWorld::getInstance().rayCast(start, end);
            
            if (result.hit) {
                std::cout << "Raycast HIT! Point: (" << result.point.x << ", " << result.point.y 
                          << "), Fraction: " << result.fraction << std::endl;
                
                m_lastRaycastStart = b2Vec2{startX * 100.0f, startY * 100.0f}; // Convert back to pixels
                m_lastRaycastEnd = b2Vec2{result.point.x * 100.0f, result.point.y * 100.0f};
            } else {
                std::cout << "Raycast missed!" << std::endl;
                m_lastRaycastStart = b2Vec2{startX * 100.0f, startY * 100.0f};
                m_lastRaycastEnd = b2Vec2{endX * 100.0f, endY * 100.0f};
            }
            
            m_showRaycast = true;
            m_raycastTimer = 0.0f;
        }
        
        void performAABBQuery() {
            // Query area around center of screen
            b2AABB queryBox;
            queryBox.lowerBound = b2Vec2{300.0f / 100.0f, 200.0f / 100.0f};
            queryBox.upperBound = b2Vec2{500.0f / 100.0f, 400.0f / 100.0f};
            
            std::vector<b2BodyId> foundBodies;
            Box2DWorld::getInstance().queryAABB(queryBox, foundBodies);
            
            std::cout << "AABB Query found " << foundBodies.size() << " bodies" << std::endl;
            
            m_lastAABB = queryBox;
            m_showAABB = true;
            m_aabbTimer = 0.0f;
        }
        
        void createDynamicBox(float x, float y, float width, float height) {
            auto& textureManager = TextureManager::getInstance();
            auto box = std::make_unique<GameObject>();
            box->add<BodyComponent>(x, y, width, height);
            auto boxSprite = box->add<SpriteComponent>("tile_texture");
            SDL_Texture* tileTexture = textureManager.getTexture("tile_texture");
            if (tileTexture) {
                boxSprite->setTexture(tileTexture);
                boxSprite->setTile(0, 0, 16, 16); // Use tileset tile
            } else {
                // Fallback to colored rectangle if texture not loaded
                boxSprite = box->add<SpriteComponent>("", SDL_Color{0, 150, 255, 255});
            }
            auto physics = box->add<Box2DPhysicsComponent>(Box2DPhysicsComponent::DYNAMIC, 1.0f, 0.3f, 0.3f);
            physics->createBody(x, y, width, height);
            
            m_gameObjects.push_back(std::move(box));
            m_dynamicBodies.push_back(m_gameObjects.back().get());
        }
        
        void createDestructibleBox(float x, float y, float width, float height) {
            auto& textureManager = TextureManager::getInstance();
            SDL_Texture* tileTexture = textureManager.getTexture("tile_texture");
            
            // Create bottom box
            auto box1 = std::make_unique<GameObject>();
            box1->add<BodyComponent>(x, y, width, height);
            box1->add<SolidComponent>(); // Makes it solid for collision
            box1->add<DestructibleBoxComponent>(); // Marks it as destructible
            
            auto boxSprite1 = box1->add<SpriteComponent>("tile_texture");
            if (tileTexture) {
                boxSprite1->setTexture(tileTexture);
                boxSprite1->setTile(0, 0, 16, 16); // Use tileset tile
            } else {
                // Fallback to brown colored rectangle if texture not loaded
                boxSprite1 = box1->add<SpriteComponent>("", SDL_Color{139, 69, 19, 255}); // Brown color
            }
            
            // Create static body so player can stand on it
            auto physics1 = box1->add<Box2DPhysicsComponent>(Box2DPhysicsComponent::STATIC, 0.0f, 0.7f, 0.1f);
            physics1->createBody(x, y, width, height);
            
            m_gameObjects.push_back(std::move(box1));
            
            // Create top box (stacked on top of bottom box)
            float topBoxY = y - height; // Stack the second box on top
            auto box2 = std::make_unique<GameObject>();
            box2->add<BodyComponent>(x, topBoxY, width, height);
            box2->add<SolidComponent>(); // Makes it solid for collision
            box2->add<DestructibleBoxComponent>(); // Marks it as destructible
            
            auto boxSprite2 = box2->add<SpriteComponent>("tile_texture");
            if (tileTexture) {
                boxSprite2->setTexture(tileTexture);
                boxSprite2->setTile(0, 0, 16, 16); // Use tileset tile
            } else {
                // Fallback to brown colored rectangle if texture not loaded
                boxSprite2 = box2->add<SpriteComponent>("", SDL_Color{139, 69, 19, 255}); // Brown color
            }
            
            // Create static body for top box
            auto physics2 = box2->add<Box2DPhysicsComponent>(Box2DPhysicsComponent::STATIC, 0.0f, 0.7f, 0.1f);
            physics2->createBody(x, topBoxY, width, height);
            
            m_gameObjects.push_back(std::move(box2));
        }
        
        void createDestructibleBoxesOnPlatform(float platformX, float platformY, float platformWidth, int numBoxes, float boxWidth, float boxHeight) {
            // Create multiple destructible boxes evenly spaced on a platform
            float spacing = platformWidth / (numBoxes + 1); // Space between boxes
            float startX = platformX + spacing;
            
            for(int i = 0; i < numBoxes; i++) {
                float boxX = startX + (i * spacing);
                createDestructibleBox(boxX, platformY - boxHeight, boxWidth, boxHeight);
            }
        }
        
        void createDestructibleBoxesAtPositions(float platformY, float boxWidth, float boxHeight, const std::vector<float>& xPositions) {
            // Create destructible boxes at specific X positions
            for(float x : xPositions) {
                createDestructibleBox(x, platformY - boxHeight, boxWidth, boxHeight);
            }
        }
        
        void createKillZone(float x, float y, float width, float height) {
            // Create invisible kill zone that kills player on contact
            auto killZone = std::make_unique<GameObject>();
            killZone->add<BodyComponent>(x, y, width, height);
            killZone->add<KillZoneComponent>(); // Mark as kill zone
            
            // Create static physics body (invisible, no sprite)
            auto physics = killZone->add<Box2DPhysicsComponent>(Box2DPhysicsComponent::STATIC, 0.0f, 0.0f, 0.0f);
            physics->createBody(x, y, width, height);
            
            m_gameObjects.push_back(std::move(killZone));
            std::cout << "Kill zone created at Y=" << y << " (width=" << width << ", height=" << height << ")" << std::endl;
        }
        
        void resetGameObjects() {
            // Reset all game objects to their initial positions by reloading from XML
            std::cout << "Resetting game objects..." << std::endl;
            
            // Reload all game objects from XML
            auto newObjects = XMLComponentFactory::createFromXML(Engine::getRenderer(), "scene.xml");
            
            // Clear old objects
            m_gameObjects.clear();
            
            // Add reloaded objects
            m_gameObjects = std::move(newObjects);
            
            // Recreate destructible boxes and kill zone
            createDestructibleBox(200, 470, 40, 40);
            // Create destructible boxes on the long platform (platform at x=6200, y=500, width=800)
            // Only create first, middle, and last box with increased spacing
            std::vector<float> boxPositions = {6300.0f, 6800.0f, 7300.0f}; // First, middle, end positions (500px spacing)
            createDestructibleBoxesAtPositions(500, 40, 40, boxPositions);
            createKillZone(0, 3000, 10000, 100);
            
            // Reset player position
            auto player = findPlayer();
            if(player) {
                auto body = player->get<BodyComponent>();
                if(body) {
                    body->x = 100.0f; // Reset to spawn position
                    body->y = 400.0f;
                    body->velocityX = 0.0f;
                    body->velocityY = 0.0f;
                    
                    // Reset physics body
                    auto physics = player->get<Box2DPhysicsComponent>();
                    if(physics) {
                        physics->destroyBody();
                        physics->createBody(100.0f, 400.0f, body->width, body->height);
                    }
                }
            }
            
            // Start timer when player spawns
            m_gameTimer = 0.0f;
            m_timerRunning = true;
            
            std::cout << "Game objects reset complete." << std::endl;
        }
        
        void removeLastDynamicBody() {
            if (!m_dynamicBodies.empty()) {
                auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
                    [this](const std::unique_ptr<GameObject>& obj) {
                        return obj.get() == m_dynamicBodies.back();
                    });
                
                if (it != m_gameObjects.end()) {
                    m_gameObjects.erase(it);
                    m_dynamicBodies.pop_back();
                    std::cout << "Removed box! Remaining: " << m_dynamicBodies.size() << std::endl;
                }
            }
        }
        
        void drawRaycast(SDL_Renderer* renderer, const View& view) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            
            int startX = static_cast<int>(view.worldToScreenX(m_lastRaycastStart.x));
            int startY = static_cast<int>(view.worldToScreenY(m_lastRaycastStart.y));
            int endX = static_cast<int>(view.worldToScreenX(m_lastRaycastEnd.x));
            int endY = static_cast<int>(view.worldToScreenY(m_lastRaycastEnd.y));
            
            SDL_RenderDrawLine(renderer, startX, startY, endX, endY);
        }
        
        void drawAABB(SDL_Renderer* renderer, const View& view) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 128);
            
            float lowerX = m_lastAABB.lowerBound.x * 100.0f;
            float lowerY = m_lastAABB.lowerBound.y * 100.0f;
            float upperX = m_lastAABB.upperBound.x * 100.0f;
            float upperY = m_lastAABB.upperBound.y * 100.0f;
            
            SDL_Rect aabbRect = {
                static_cast<int>(view.worldToScreenX(lowerX)),
                static_cast<int>(view.worldToScreenY(lowerY)),
                static_cast<int>((upperX - lowerX)),
                static_cast<int>((upperY - lowerY))
            };
            
            SDL_RenderDrawRect(renderer, &aabbRect);
        }
        
        // ========================
        // Bullet System Methods
        // ========================
        void shootBullet(float x, float y, float directionX) {
            auto bullet = std::make_unique<GameObject>();
            
            // Bullet properties
            const float bulletWidth = 24.0f;  // 3x original size (8 * 3)
            const float bulletHeight = 24.0f; // 3x original size (8 * 3)
            const float bulletSpeed = 1600.0f; // pixels per second (16 m/s in Box2D)
            
            bullet->add<BodyComponent>(x, y, bulletWidth, bulletHeight);
            
            // Add bullet component
            auto bulletComp = bullet->add<BulletComponent>(directionX, bulletSpeed);
            
            // Add sprite with texture
            auto bulletSprite = bullet->add<SpriteComponent>("bullet_texture");
            
            // Add Box2D physics
            auto bulletPhysics = bullet->add<Box2DPhysicsComponent>(
                Box2DPhysicsComponent::DYNAMIC, 
                0.1f,  // low density
                0.0f,  // no friction
                0.0f   // no bounce
            );
            bulletPhysics->createCircleBody(x + bulletWidth / 2, y + bulletHeight / 2, bulletWidth / 2);
            
            // Set initial velocity
            float velX = directionX * (bulletSpeed / 100.0f); // Convert to m/s
            bulletPhysics->setLinearVelocity(velX, 0.0f);
            
            // Make bullet a sensor (passes through but detects collisions)
            // Note: Box2D C API sensor setup may vary
            
            m_gameObjects.push_back(std::move(bullet));
            m_bullets.push_back(m_gameObjects.back().get());
            
            // Play shoot sound effect
            SoundManager::getInstance().playSound("shoot_sound");
            
            std::cout << "Bullet shot at (" << x << ", " << y << ") direction: " << directionX << std::endl;
        }
        
        void handleShooting() {
            // Don't allow shooting if entering initials or showing high scores
            if (m_enteringInitials || m_showingHighScores) {
                return;
            }
            
            auto& input = InputSystem::getInstance();
            auto playerObj = findPlayer();
            
            if (!playerObj) return;
            
            auto playerBody = playerObj->get<BodyComponent>();
            if (!playerBody) return;
            
            // Check if player is dead - don't allow shooting when dead
            auto playerController = playerObj->get<ControllerComponent>();
            if (playerController && playerController->isDead()) {
                return;
            }
            
            // Check for mouse click or key press to shoot
            static bool mouseWasPressed = false;
            int mouseX, mouseY;
            Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
            bool mousePressed = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
            
            // Shoot on mouse click (left button) or J key
            // Check if shooting cooldown has expired (limit to 1 shot per second)
            if (((mousePressed && !mouseWasPressed) || input.isKeyJustPressed(SDL_SCANCODE_J)) && m_shootCooldown <= 0.0f) {
                // Determine direction based on player facing or mouse position
                float directionX = 1.0f; // Default to right
                
                // If using mouse, determine direction based on mouse position relative to player
                if (mousePressed) {
                    try {
                        View& view = Engine::getMainView();
                        float worldMouseX = view.screenToWorldX(mouseX);
                        // Validate the coordinate is finite
                        if (std::isfinite(worldMouseX) && std::isfinite(playerBody->x)) {
                            directionX = (worldMouseX > playerBody->x) ? 1.0f : -1.0f;
                        }
                    } catch (...) {
                        // If view transformation fails, use default direction
                        directionX = 1.0f;
                    }
                } else {
                    // Use player's velocity to determine facing direction
                    if (std::isfinite(playerBody->velocityX) && playerBody->velocityX < 0) {
                        directionX = -1.0f; // Moving left
                    }
                }
                
                // Validate player position before creating bullet
                if (!std::isfinite(playerBody->x) || !std::isfinite(playerBody->y)) {
                    std::cerr << "Warning: Invalid player position, cannot shoot bullet" << std::endl;
                    mouseWasPressed = mousePressed;
                    return;
                }
                
                // Get player position - spawn bullet on the side player is facing
                // Player sprite is 62x50, hitbox is 34x40, hitbox is bottom-aligned with 10px offset
                // Gun is typically in upper-middle of sprite (~15-20px from top of sprite)
                // Sprite top is at: playerBody->y - 10 (hitboxOffsetY)
                // Gun position from sprite top: ~15px (slightly higher than before)
                const float spriteTopY = playerBody->y - 10.0f; // hitboxOffsetY
                const float gunOffsetFromSpriteTop = 7.5f; // Approximate gun position on sprite
                
                float bulletX = (directionX > 0) ? 
                    playerBody->x + playerBody->width : 
                    playerBody->x;
                float bulletY = spriteTopY + gunOffsetFromSpriteTop;
                
                // Validate bullet position before creating
                if (std::isfinite(bulletX) && std::isfinite(bulletY)) {
                    shootBullet(bulletX, bulletY, directionX);
                    m_playerShootTimer = 0.3f; // Show shooting animation for 0.3 seconds
                    m_shootCooldown = 1.0f; // Set cooldown to 1 second (limit to 1 shot per second)
                    m_lastShootDirection = directionX; // Store direction for sprite flipping
                } else {
                    std::cerr << "Warning: Invalid bullet position, cannot shoot" << std::endl;
                }
            }
            
            mouseWasPressed = mousePressed;
        }
        
        void updatePlayerAnimations() {
            auto playerObj = findPlayer();
            if (!playerObj) return;
            
            auto playerSprite = playerObj->get<SpriteComponent>();
            if (!playerSprite) return;
            
            auto playerController = playerObj->get<ControllerComponent>();
            if (!playerController) return;
            
            auto playerBody = playerObj->get<BodyComponent>();
            if (!playerBody) return;
            
            // Determine facing direction based on movement or shooting
            bool facingLeft = false;
            
            // Check if shooting - use last shoot direction
            if (m_playerShootTimer > 0.0f && m_lastShootDirection != 0.0f) {
                facingLeft = (m_lastShootDirection < 0.0f);
            } else {
                // Check movement direction
                auto physics = playerObj->get<Box2DPhysicsComponent>();
                if (physics && IsValid(physics->getBodyId())) {
                    b2Vec2 vel = b2Body_GetLinearVelocity(physics->getBodyId());
                    facingLeft = (vel.x < -0.1f); // Moving left
                } else if (playerBody->velocityX < -0.1f) {
                    facingLeft = true; // Moving left
                }
            }
            
            // Set flip flag (flip when facing left)
            playerSprite->setFlipHorizontal(facingLeft);
            
            // Determine animation state: shooting > running > idle
            int newAnimationRow = 0;
            if (m_playerShootTimer > 0.0f) {
                // Shooting animation (row 5, which is the 6th row, 0-indexed)
                newAnimationRow = 5;
            } else if (playerController->isMoving()) {
                // Running animation (row 1, which is the 2nd row, 0-indexed)
                newAnimationRow = 1;
            } else {
                // Idle animation (row 0, which is the top row)
                newAnimationRow = 0;
            }
            
            // Only change animation row if it's different from current
            if (playerSprite->getAnimationRow() != newAnimationRow) {
                playerSprite->setAnimationRow(newAnimationRow, 8, true); // Reset frame when changing rows
            }
        }
        
        void updateBullets(float deltaTime) {
            View& view = Engine::getMainView();
            
            // Get camera bounds (world coordinates)
            float cameraCenterX = view.screenToWorldX(400); // Screen center X
            float cameraCenterY = view.screenToWorldY(300); // Screen center Y
            float cameraHalfWidth = 400.0f;  // Half screen width
            float cameraHalfHeight = 300.0f; // Half screen height
            
            // Validate camera coordinates before creating AABB
            if (!std::isfinite(cameraCenterX) || !std::isfinite(cameraCenterY)) {
                std::cerr << "Warning: Invalid camera coordinates, skipping bullet update" << std::endl;
                return;
            }
            
            // Create AABB for camera bounds
            b2AABB cameraBounds;
            float lowerX = (cameraCenterX - cameraHalfWidth - 100.0f) / 100.0f;
            float lowerY = (cameraCenterY - cameraHalfHeight - 100.0f) / 100.0f;
            float upperX = (cameraCenterX + cameraHalfWidth + 100.0f) / 100.0f;
            float upperY = (cameraCenterY + cameraHalfHeight + 100.0f) / 100.0f;
            
            // Validate AABB bounds before using them
            if (!std::isfinite(lowerX) || !std::isfinite(lowerY) || 
                !std::isfinite(upperX) || !std::isfinite(upperY)) {
                std::cerr << "Warning: Invalid AABB bounds, skipping bullet update" << std::endl;
                return;
            }
            
            // Clamp to reasonable values to prevent Box2D assertion errors
            const float maxCoord = 10000.0f; // Maximum reasonable coordinate
            lowerX = std::max(-maxCoord, std::min(maxCoord, lowerX));
            lowerY = std::max(-maxCoord, std::min(maxCoord, lowerY));
            upperX = std::max(-maxCoord, std::min(maxCoord, upperX));
            upperY = std::max(-maxCoord, std::min(maxCoord, upperY));
            
            cameraBounds.lowerBound = b2Vec2{lowerX, lowerY};
            cameraBounds.upperBound = b2Vec2{upperX, upperY};
            
            // Use AABB query to find bullets in camera area
            std::vector<b2BodyId> bulletsInView;
            Box2DWorld::getInstance().queryAABB(cameraBounds, bulletsInView);
            
            // Track bullets to remove (out of bounds)
            std::vector<GameObject*> bulletsToRemove;
            
            for (auto* bulletObj : m_bullets) {
                if (!bulletObj || !bulletObj->isActive) continue;
                
                auto bulletPhysics = bulletObj->get<Box2DPhysicsComponent>();
                if (!bulletPhysics || !IsValid(bulletPhysics->getBodyId())) continue;
                
                b2Vec2 bulletPos = b2Body_GetPosition(bulletPhysics->getBodyId());
                float bulletWorldX = bulletPos.x * 100.0f;
                float bulletWorldY = bulletPos.y * 100.0f;
                
                // Check if bullet is outside camera bounds with margin
                float margin = 200.0f;
                if (bulletWorldX < cameraCenterX - cameraHalfWidth - margin ||
                    bulletWorldX > cameraCenterX + cameraHalfWidth + margin ||
                    bulletWorldY < cameraCenterY - cameraHalfHeight - margin ||
                    bulletWorldY > cameraCenterY + cameraHalfHeight + margin) {
                    
                    bulletsToRemove.push_back(bulletObj);
                }
            }
            
            // Remove out-of-bounds bullets
            for (auto* bullet : bulletsToRemove) {
                auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
                    [bullet](const std::unique_ptr<GameObject>& obj) {
                        return obj.get() == bullet;
                    });
                
                if (it != m_gameObjects.end()) {
                    m_gameObjects.erase(it);
                    
                    // Remove from bullets list
                    m_bullets.erase(std::remove(m_bullets.begin(), m_bullets.end(), bullet), m_bullets.end());
                }
            }
        }
        
        void checkBulletCollisions() {
            // Get contacts from contact listener
            auto* contactListener = Box2DWorld::getInstance().getContactListener();
            if (!contactListener) return;
            
            auto contacts = contactListener->getContactsAndClear();
            
            std::vector<GameObject*> objectsToRemove;
            
            for (const auto& contact : contacts) {
                GameObject* objA = contact.objA;
                GameObject* objB = contact.objB;
                
                if (!objA || !objB) continue;
                
                BulletComponent* bullet = objA->get<BulletComponent>();
                EnemyComponent* enemy = objB->get<EnemyComponent>();
                DestructibleBoxComponent* box = objB->get<DestructibleBoxComponent>();
                
                // Check if A is bullet and B is enemy
                if (bullet && enemy) {
                    objectsToRemove.push_back(objA); // Bullet
                    objectsToRemove.push_back(objB); // Enemy
                    // Award points for killing enemy
                    m_score += 10;
                    std::cout << "Bullet hit enemy! Score: " << m_score << std::endl;
                    continue;
                }
                
                // Check if A is bullet and B is destructible box
                if (bullet && box) {
                    objectsToRemove.push_back(objA); // Bullet
                    objectsToRemove.push_back(objB); // Box
                    std::cout << "Bullet hit destructible box!" << std::endl;
                    continue;
                }
                
                // Check if A is enemy and B is bullet (reversed)
                bullet = objB->get<BulletComponent>();
                enemy = objA->get<EnemyComponent>();
                
                if (bullet && enemy) {
                    objectsToRemove.push_back(objA); // Enemy
                    objectsToRemove.push_back(objB); // Bullet
                    // Award points for killing enemy
                    m_score += 10;
                    std::cout << "Bullet hit enemy! Score: " << m_score << std::endl;
                    continue;
                }
                
                // Check if A is destructible box and B is bullet (reversed)
                box = objA->get<DestructibleBoxComponent>();
                
                if (bullet && box) {
                    objectsToRemove.push_back(objA); // Box
                    objectsToRemove.push_back(objB); // Bullet
                    std::cout << "Bullet hit destructible box!" << std::endl;
                }
            }
            
            // Use AABB queries and raycasting to check bullet-enemy collisions
            for (auto* bulletObj : m_bullets) {
                if (!bulletObj || !bulletObj->isActive) continue;
                
                auto bulletPhysics = bulletObj->get<Box2DPhysicsComponent>();
                if (!bulletPhysics || !IsValid(bulletPhysics->getBodyId())) continue;
                
                auto bulletBody = bulletObj->get<BodyComponent>();
                if (!bulletBody) continue;
                
                b2Vec2 bulletPos = b2Body_GetPosition(bulletPhysics->getBodyId());
                
                // Create AABB around bullet for overlap query
                float bulletHalfWidth = (bulletBody->width / 2.0f) / 100.0f;
                float bulletHalfHeight = (bulletBody->height / 2.0f) / 100.0f;
                
                b2AABB bulletAABB;
                float margin = 0.2f; // 20 pixels in meters - larger margin for fast bullets
                bulletAABB.lowerBound = b2Vec2{
                    bulletPos.x - bulletHalfWidth - margin,
                    bulletPos.y - bulletHalfHeight - margin
                };
                bulletAABB.upperBound = b2Vec2{
                    bulletPos.x + bulletHalfWidth + margin,
                    bulletPos.y + bulletHalfHeight + margin
                };
                
                // Query for overlapping bodies
                std::vector<b2BodyId> overlappingBodies;
                Box2DWorld::getInstance().queryAABB(bulletAABB, overlappingBodies);
                
                // Check if any overlapping body is an enemy
                for (b2BodyId bodyId : overlappingBodies) {
                    if (!IsValid(bodyId)) continue;
                    
                    void* userData = b2Body_GetUserData(bodyId);
                    if (!userData) continue;
                    
                    GameObject* hitObj = static_cast<GameObject*>(userData);
                    
                    if (hitObj && hitObj != bulletObj) {
                        EnemyComponent* enemy = hitObj->get<EnemyComponent>();
                        DestructibleBoxComponent* box = hitObj->get<DestructibleBoxComponent>();
                        
                        if (enemy) {
                            // Found collision with enemy!
                            if (std::find(objectsToRemove.begin(), objectsToRemove.end(), bulletObj) == objectsToRemove.end()) {
                                objectsToRemove.push_back(bulletObj);
                            }
                            if (std::find(objectsToRemove.begin(), objectsToRemove.end(), hitObj) == objectsToRemove.end()) {
                                objectsToRemove.push_back(hitObj);
                            }
                            // Award points for killing enemy
                            m_score += 10;
                            std::cout << "AABB query detected bullet-enemy collision! Score: " << m_score << std::endl;
                            break; // Only hit one enemy per bullet
                        } else if (box) {
                            // Found collision with destructible box!
                            if (std::find(objectsToRemove.begin(), objectsToRemove.end(), bulletObj) == objectsToRemove.end()) {
                                objectsToRemove.push_back(bulletObj);
                            }
                            if (std::find(objectsToRemove.begin(), objectsToRemove.end(), hitObj) == objectsToRemove.end()) {
                                objectsToRemove.push_back(hitObj);
                            }
                            std::cout << "AABB query detected bullet-box collision!" << std::endl;
                            break; // Only hit one box per bullet
                        }
                    }
                }
                
                // Also use raycasting as backup detection
                float rayLength = 0.15f; // 15 pixels in meters - longer ray for fast bullets
                b2Vec2 rayEnd = {
                    bulletPos.x + (bulletObj->get<BulletComponent>()->getDirectionX() * rayLength),
                    bulletPos.y
                };
                
                auto rayResult = Box2DWorld::getInstance().rayCast(bulletPos, rayEnd);
                
                if (rayResult.hit && IsValid(rayResult.bodyId)) {
                    void* userData = b2Body_GetUserData(rayResult.bodyId);
                    if (userData) {
                        GameObject* hitObj = static_cast<GameObject*>(userData);
                        
                        if (hitObj && hitObj != bulletObj) {
                            EnemyComponent* enemy = hitObj->get<EnemyComponent>();
                            DestructibleBoxComponent* box = hitObj->get<DestructibleBoxComponent>();
                            
                            if (enemy) {
                                if (std::find(objectsToRemove.begin(), objectsToRemove.end(), bulletObj) == objectsToRemove.end()) {
                                    objectsToRemove.push_back(bulletObj);
                                }
                                if (std::find(objectsToRemove.begin(), objectsToRemove.end(), hitObj) == objectsToRemove.end()) {
                                    objectsToRemove.push_back(hitObj);
                                }
                                // Award points for killing enemy
                                m_score += 10;
                                std::cout << "Raycast detected bullet-enemy collision! Score: " << m_score << std::endl;
                            } else if (box) {
                                if (std::find(objectsToRemove.begin(), objectsToRemove.end(), bulletObj) == objectsToRemove.end()) {
                                    objectsToRemove.push_back(bulletObj);
                                }
                                if (std::find(objectsToRemove.begin(), objectsToRemove.end(), hitObj) == objectsToRemove.end()) {
                                    objectsToRemove.push_back(hitObj);
                                }
                                std::cout << "Raycast detected bullet-box collision!" << std::endl;
                            }
                        }
                    }
                }
                
                // Fallback: Direct BodyComponent collision check
                // This uses BodyComponent positions for more reliable detection
                if (std::find(objectsToRemove.begin(), objectsToRemove.end(), bulletObj) == objectsToRemove.end()) {
                    // Get bullet center from BodyComponent
                    float bulletCenterX = bulletBody->x + bulletBody->width / 2.0f;
                    float bulletCenterY = bulletBody->y + bulletBody->height / 2.0f;
                    
                    for (auto& obj : m_gameObjects) {
                        if (!obj || obj.get() == bulletObj) continue;
                        
                        auto enemy = obj->get<EnemyComponent>();
                        auto box = obj->get<DestructibleBoxComponent>();
                        
                        // Skip if neither enemy nor destructible box
                        if (!enemy && !box) continue;
                        
                        auto targetBody = obj->get<BodyComponent>();
                        if (!targetBody) continue;
                        
                        // Get target center from BodyComponent
                        float targetCenterX = targetBody->x + targetBody->width / 2.0f;
                        float targetCenterY = targetBody->y + targetBody->height / 2.0f;
                        
                        // Simple distance check (circle-circle collision)
                        float dx = bulletCenterX - targetCenterX;
                        float dy = bulletCenterY - targetCenterY;
                        float distance = std::sqrt(dx * dx + dy * dy);
                        float bulletRadius = std::max(bulletBody->width, bulletBody->height) / 2.0f;
                        float targetRadius = std::max(targetBody->width, targetBody->height) / 2.0f;
                        float collisionDistance = bulletRadius + targetRadius;
                        
                        if (distance < collisionDistance) {
                            // Collision detected!
                            if (std::find(objectsToRemove.begin(), objectsToRemove.end(), bulletObj) == objectsToRemove.end()) {
                                objectsToRemove.push_back(bulletObj);
                            }
                            if (std::find(objectsToRemove.begin(), objectsToRemove.end(), obj.get()) == objectsToRemove.end()) {
                                objectsToRemove.push_back(obj.get());
                            }
                            if (enemy) {
                                // Award points for killing enemy
                                m_score += 10;
                                std::cout << "Direct BodyComponent collision detected: bullet hit enemy at distance " << distance << " Score: " << m_score << std::endl;
                            } else if (box) {
                                std::cout << "Direct BodyComponent collision detected: bullet hit destructible box at distance " << distance << std::endl;
                            }
                            break; // Only hit one target per bullet
                        }
                    }
                }
            }
            
            // Remove collided objects
            for (auto* obj : objectsToRemove) {
                auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
                    [obj](const std::unique_ptr<GameObject>& gameObj) {
                        return gameObj.get() == obj;
                    });
                
                if (it != m_gameObjects.end()) {
                    // Remove from bullet list if it's a bullet
                    if (obj->get<BulletComponent>()) {
                        m_bullets.erase(std::remove(m_bullets.begin(), m_bullets.end(), obj), m_bullets.end());
                    }
                    
                    m_gameObjects.erase(it);
                }
            }
        }
        
        std::vector<std::unique_ptr<GameObject>> m_gameObjects;
        
        // Box2D Demo member variables
        std::vector<GameObject*> m_dynamicBodies;
        std::vector<GameObject*> m_bullets;
        bool m_showRaycast = false;
        bool m_showAABB = false;
        b2Vec2 m_lastRaycastStart;
        b2Vec2 m_lastRaycastEnd;
        b2AABB m_lastAABB;
        float m_raycastTimer = 0.0f;
        float m_aabbTimer = 0.0f;
        float m_playerShootTimer = 0.0f; // Timer for shooting animation
        float m_shootCooldown = 0.0f; // Cooldown timer to limit shooting rate (1 shot per second)
        float m_lastShootDirection = 1.0f; // Last shooting direction (1.0 = right, -1.0 = left)
        float m_musicPauseTimer = 0.0f; // Timer for music pause after death (3 seconds)
        int m_score = 0; // Player score
        TTF_Font* m_font = nullptr; // Font for rendering score
        
        // Game timer system
        float m_gameTimer = 0.0f; // Elapsed time in seconds
        bool m_timerRunning = false; // Whether the timer is currently running
        
        // Initials entry system
        bool m_enteringInitials = false;
        std::string m_playerInitials = "";
        bool m_textInputActive = false;
        bool m_showingHighScores = false;
        bool m_gameWon = false; // Win state
        std::vector<std::pair<std::string, int>> m_highScores; // initials, score pairs
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