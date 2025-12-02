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
#include <box2d/box2d.h>

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
            
            // Enable color keying to make white (255, 255, 255) transparent
            // This removes the white background from sprites
            Uint32 colorKey = SDL_MapRGB(surface->format, 255, 255, 255);
            SDL_SetColorKey(surface, SDL_TRUE, colorKey);
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
            
            SDL_Rect destRect = view.getTransformedRect(body->x, body->y, body->width, body->height);
            
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
                    SDL_RenderCopy(renderer, m_texture, &srcRect, &destRect);
                }
                // For animated sprite sheets (characters, enemies)
                else if(m_usingSpriteSheet && m_animated) {
                    // Ensure sprite dimensions are valid
                    if (m_spriteWidth <= 0 || m_spriteHeight <= 0) {
                        // Fallback to rendering entire texture if dimensions not set
                        SDL_RenderCopy(renderer, m_texture, NULL, &destRect);
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
            
            // Add Box2D physics component to enemies
            auto enemyPhysics = obj->add<Box2DPhysicsComponent>(
                Box2DPhysicsComponent::DYNAMIC,
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
            Box2DWorld::getInstance().shutdown();
            m_gameObjects.clear();
            TextureManager::getInstance().cleanup();
            Engine::getInstance().shutdown();
        }
        
    private:
        void update(float deltaTime) {
            // Update Box2D physics world first
            Box2DWorld::getInstance().update(deltaTime);
            
            // Handle shooting
            handleShooting();
            
            // Update shooting timer
            if (m_playerShootTimer > 0.0f) {
                m_playerShootTimer -= deltaTime;
                if (m_playerShootTimer < 0.0f) m_playerShootTimer = 0.0f;
            }
            
            // Update player animations based on state
            updatePlayerAnimations();
            
            // Update all game objects using proper deltaTime
            for(auto& obj : m_gameObjects) {
                if(obj->isActive) {
                    obj->update(deltaTime);
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
        
        // ========================
        // Box2D Demo Methods
        // ========================
        void createPlatform(float x, float y, float width, float height, SDL_Texture* tileTexture) {
            auto platform = std::make_unique<GameObject>();
            platform->add<BodyComponent>(x, y, width, height);
            auto platformSprite = platform->add<SpriteComponent>("tile_texture");
            if (tileTexture) {
                platformSprite->setTexture(tileTexture);
                platformSprite->setTile(2, 3, 16, 16); // Use ground tile from tileset
            }
            auto platformPhysics = platform->add<Box2DPhysicsComponent>(Box2DPhysicsComponent::STATIC, 0.0f, 0.7f, 0.1f);
            platformPhysics->createBody(x, y, width, height);
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
            createPlatform(platform1X, platformY, platform1Width, platformHeight, tileTexture);
            
            // Create second platform (with enemies)
            createPlatform(platform2X, platformY, platform2Width, platformHeight, tileTexture);
            
            // Create bottom ground/platform at bottom of screen for safety
            createPlatform(0, 550, 800, 50, tileTexture);
            
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
            // Original tile ratio: 64:32 = 2:1 (width:height)
            const int tileWidth = 64;   // 512 / 8
            const int tileHeight = 32;  // 224 / 7
            const float originalRatio = static_cast<float>(tileWidth) / static_cast<float>(tileHeight); // 2.0 (2:1 ratio)
            
            // Keep character height and calculate width to maintain original 2:1 ratio
            const float playerHeight = 64.0f; // Keep at current height
            const float playerWidth = playerHeight * originalRatio; // 64 * 2.0 = 128 pixels (maintains 2:1 ratio)
            
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
            auto box = std::make_unique<GameObject>();
            box->add<BodyComponent>(x, y, width, height);
            box->add<SolidComponent>(); // Makes it solid for collision
            box->add<DestructibleBoxComponent>(); // Marks it as destructible
            
            auto boxSprite = box->add<SpriteComponent>("tile_texture");
            SDL_Texture* tileTexture = textureManager.getTexture("tile_texture");
            if (tileTexture) {
                boxSprite->setTexture(tileTexture);
                boxSprite->setTile(0, 0, 16, 16); // Use tileset tile
            } else {
                // Fallback to brown colored rectangle if texture not loaded
                boxSprite = box->add<SpriteComponent>("", SDL_Color{139, 69, 19, 255}); // Brown color
            }
            
            // Create static body so player can stand on it
            auto physics = box->add<Box2DPhysicsComponent>(Box2DPhysicsComponent::STATIC, 0.0f, 0.7f, 0.1f);
            physics->createBody(x, y, width, height);
            
            m_gameObjects.push_back(std::move(box));
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
            const float bulletWidth = 8.0f;
            const float bulletHeight = 8.0f;
            const float bulletSpeed = 600.0f; // pixels per second
            
            bullet->add<BodyComponent>(x, y, bulletWidth, bulletHeight);
            
            // Add bullet component
            auto bulletComp = bullet->add<BulletComponent>(directionX, bulletSpeed);
            
            // Add sprite (yellow/orange bullet)
            auto bulletSprite = bullet->add<SpriteComponent>("", SDL_Color{255, 200, 0, 255});
            
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
            
            std::cout << "Bullet shot at (" << x << ", " << y << ") direction: " << directionX << std::endl;
        }
        
        void handleShooting() {
            auto& input = InputSystem::getInstance();
            auto playerObj = findPlayer();
            
            if (!playerObj) return;
            
            auto playerBody = playerObj->get<BodyComponent>();
            if (!playerBody) return;
            
            // Check for mouse click or key press to shoot
            static bool mouseWasPressed = false;
            int mouseX, mouseY;
            Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
            bool mousePressed = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
            
            // Shoot on mouse click (left button) or J key
            if ((mousePressed && !mouseWasPressed) || input.isKeyJustPressed(SDL_SCANCODE_J)) {
                // Determine direction based on player facing or mouse position
                float directionX = 1.0f; // Default to right
                
                // If using mouse, determine direction based on mouse position relative to player
                if (mousePressed) {
                    View& view = Engine::getMainView();
                    float worldMouseX = view.screenToWorldX(mouseX);
                    directionX = (worldMouseX > playerBody->x) ? 1.0f : -1.0f;
                } else {
                    // Use player's velocity to determine facing direction
                    if (playerBody->velocityX < 0) {
                        directionX = -1.0f; // Moving left
                    }
                }
                
                // Get player position - spawn bullet on the side player is facing
                float bulletX = (directionX > 0) ? 
                    playerBody->x + playerBody->width : 
                    playerBody->x;
                float bulletY = playerBody->y + playerBody->height / 2;
                
                shootBullet(bulletX, bulletY, directionX);
                m_playerShootTimer = 0.3f; // Show shooting animation for 0.3 seconds
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
            
            // Create AABB for camera bounds
            b2AABB cameraBounds;
            cameraBounds.lowerBound = b2Vec2{
                (cameraCenterX - cameraHalfWidth - 100.0f) / 100.0f,  // Add margin
                (cameraCenterY - cameraHalfHeight - 100.0f) / 100.0f
            };
            cameraBounds.upperBound = b2Vec2{
                (cameraCenterX + cameraHalfWidth + 100.0f) / 100.0f,
                (cameraCenterY + cameraHalfHeight + 100.0f) / 100.0f
            };
            
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
                    std::cout << "Bullet hit enemy!" << std::endl;
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
                    std::cout << "Bullet hit enemy!" << std::endl;
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
                            std::cout << "AABB query detected bullet-enemy collision!" << std::endl;
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
                                std::cout << "Raycast detected bullet-enemy collision!" << std::endl;
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
                                std::cout << "Direct BodyComponent collision detected: bullet hit enemy at distance " << distance << std::endl;
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