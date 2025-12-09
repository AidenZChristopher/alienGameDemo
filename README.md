# Component-Based 2D Game Engine

A comprehensive 2D game engine built using C++17, SDL2, and Box2D featuring a modular Entity-Component System (ECS) architecture with realistic physics simulation.

## 🎮 Overview

This engine demonstrates a modern game architecture where gameplay logic is divided into self-contained components that can be dynamically composed into GameObject entities. It features full Box2D physics integration, sprite animation, XML-based level loading, and a robust component system.

## ✨ Features

### 🧱 **Entity-Component System (ECS)**
- **Modular Design**: Each GameObject can have multiple components defining behavior
- **Dynamic Composition**: Components can be added/removed at runtime
- **Clean Separation**: Rendering, physics, and logic are isolated in separate components

### ⚙️ **Box2D Physics Integration**
- **Realistic Physics**: Full Box2D integration for accurate collision and movement
- **Dynamic & Static Bodies**: Support for both movable and fixed objects
- **Physics Queries**:
  - **Raycasting**: Detect intersections with physics bodies
  - **AABB Querying**: Find bodies within specified areas
  - **Contact Listening**: Real-time collision detection and response
- **Runtime Manipulation**: Add/remove physics bodies during gameplay

### 🎨 **Rendering System**
- **Animated Sprite Sheets**: Multi-row sprite sheet support with frame-based animation
- **Texture Management**: Centralized texture loading and caching
- **Camera System**: Smooth following camera with viewport transformations
- **Tiling Backgrounds**: Infinite scrolling background support

### 🎯 **Gameplay Systems**
- **Player Controller**: Movement, jumping, and physics-based interaction
- **Reactive Enemy AI**: Vision cone detection, hearing system, and dynamic state-based behavior (Patrol, Alert, Chase)
- **Bullet System**: Projectile physics with collision detection
- **Destructible Objects**: Boxes that can be destroyed by bullets
- **Sound Event System**: Broadcasts sound events that enemies can detect and react to

### 📁 **Asset & Level Management**
- **XML-Based Level Loading**: Create levels using XML configuration files
- **Texture Factory**: Load and manage textures from asset definitions
- **Component Factory**: Dynamic object creation from XML specifications

## 📸 Screenshots

<img width="517" height="351" alt="Screenshot 2025-12-02 022218" src="https://github.com/user-attachments/assets/529935a1-1894-4b20-85eb-9dc7573fad63" />
<img width="439" height="286" alt="Screenshot 2025-12-02 022206" src="https://github.com/user-attachments/assets/156a83b8-9514-4287-a5be-851e02bee84d" />
<img width="959" height="726" alt="Screenshot 2025-12-02 022156" src="https://github.com/user-attachments/assets/b9092f88-fd07-4bca-97f6-62a7cd064b26" />
<img width="987" height="770" alt="Screenshot 2025-12-02 022137" src="https://github.com/user-attachments/assets/a2cdab40-5108-4401-8545-08883243eac1" />

## 🏗️ Architecture

### Core Components

Each GameObject can have multiple components attached to define its behavior:

| Component | Description |
|-----------|-------------|
| **BodyComponent** | Tracks position, size, and velocity |
| **SpriteComponent** | Handles textures and sprite sheet animations |
| **ControllerComponent** | Manages player input and movement |
| **PhysicsComponent** | Adds gravity and physics-based motion |
| **Box2DPhysicsComponent** | Full Box2D physics integration |
| **PatrolBehaviorComponent** | Controls patrolling enemy AI movement |
| **BounceBehaviorComponent** | Controls bouncing enemy movement |
| **VisionComponent** | Implements vision cone detection for enemies (300 unit range, configurable angle) |
| **HearingComponent** | Detects sound events within hearing range (350 unit range) |
| **ReactiveAIComponent** | State-based AI system (Patrol, Alert, Chase) that responds to vision and hearing |
| **SolidComponent** | Marks objects for collision |
| **EnemyComponent** | Marks objects as enemies |
| **BulletComponent** | Manages projectile behavior |
| **DestructibleBoxComponent** | Marks objects as destructible |

### System Managers

| System | Description |
|--------|-------------|
| **InputSystem** | Centralized input management with key state tracking |
| **TextureManager** | Loads, caches, and manages all textures |
| **CollisionSystem** | Provides bounding-box collision detection and resolution |
| **Box2DWorld** | Manages the Box2D physics world |
| **XMLComponentFactory** | Creates objects from XML configuration |

## 🎮 Controls

| Key | Action |
|-----|--------|
| **W / ↑** | Jump |
| **A / ←** | Move Left |
| **D / →** | Move Right |
| **Space** | Jump / Apply Impulse |
| **Left Click / J** | Shoot Bullet |
| **C** | Spawn New Box |
| **X** | Remove Last Box |
| **R** | Perform Raycast |
| **Q** | Perform AABB Query |
| **F** | Apply Force (Right) |
| **V** | Set Random Velocity |
| **Esc** | Quit Game |

## 📁 Technical Implementation

### Asset Management
Assets can be loaded from XML or dynamically enumerated from a folder. Textures are stored in a map structure that associates asset names with file paths, ensuring efficient retrieval and reuse throughout the engine.

### View Class
The View class handles camera transformations and viewport logic, tracking:
- Center position
- Scale
- Rotation angle (optional)

The engine stores a static View instance initialized during startup, and all rendering accounts for View transformations.

### Frame Rate Management
The engine includes a frame rate limiter to maintain consistent FPS. A static `deltaTime` variable stores the duration of the last frame, ensuring consistent movement and animation speeds across all systems.

### Frame Rate Management
The engine includes a frame rate limiter to maintain consistent FPS. A static `deltaTime` variable stores the duration of the last frame, ensuring consistent movement and animation speeds across all systems.

### Reactive AI System

The engine features a sophisticated reactive AI system that allows enemies to detect and respond to the player through vision and hearing:

#### Vision System
- **VisionComponent**: Implements a vision cone detection system
  - **View Distance**: 300 units (configurable via XML)
  - **View Angle**: Configurable field of view angle
  - **Facing Detection**: Automatically determines enemy facing direction based on movement
  - **Line of Sight**: Calculates distance and angle to player to determine visibility

#### Hearing System
- **HearingComponent**: Detects sound events within a specified range
  - **Hearing Range**: 350 units (configurable via XML)
  - **Sound Broadcasting**: Game events (e.g., shooting) broadcast sound events at their location
  - **Position Tracking**: Stores the position of detected sounds for AI navigation

#### AI State Machine
- **ReactiveAIComponent**: Implements a three-state AI system:
  - **Patrol State**: Default behavior, follows patrol paths or bouncing patterns
  - **Alert State**: Triggered when a sound is heard but player is not visible; enemy moves toward sound location
  - **Chase State**: Triggered when player is visible; enemy actively pursues the player
- **State Transitions**:
  - Patrol → Alert: Sound detected within hearing range
  - Alert → Chase: Player becomes visible
  - Alert → Patrol: Alert timer expires without seeing player
  - Chase → Patrol: Player moves out of vision range and no recent sounds

#### Integration
- Works seamlessly with existing behavior components (`PatrolBehaviorComponent`, `BounceBehaviorComponent`)
- Behavior components automatically yield control when AI enters Alert or Chase states
- Supports both ground-based and flying enemies
- Fully configurable via XML level definitions

