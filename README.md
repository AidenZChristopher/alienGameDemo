This project is a component-based 2D platformer engine built using C++17 and SDL2. It demonstrates a modular game architecture where gameplay logic is divided into self-contained components (physics, rendering, behavior, etc.) that can be dynamically composed into GameObject entities.

The engine supports:

Player movement with gravity, jumping, and collision handling

Moving and static platforms
<img width="987" height="776" alt="Screenshot 2025-11-01 235148" src="https://github.com/user-attachments/assets/1309ecf8-6247-4974-a118-408d9bbfd5c5" />

Animated sprite sheets

Patrolling and bouncing enemies
<img width="990" height="770" alt="Screenshot 2025-11-01 235258" src="https://github.com/user-attachments/assets/42d3aaa6-197c-4422-ad9a-61364ad4fce5" />

A following camera system

A centralized input and texture management system

Features
🧱 Entity-Component System (ECS)

Each GameObject can have multiple components attached to define its behavior, such as:

BodyComponent – Tracks position, size, and velocity

SpriteComponent – Handles textures and sprite sheet animations

ControllerComponent – Manages player input and movement

PhysicsComponent – Adds gravity to objects

PatrolBehaviorComponent / BounceBehaviorComponent – Controls enemy movement patterns

SolidComponent / EnemyComponent – Marks objects for collision or enemy logic

🎮 Input System

Centralized InputSystem uses SDL keyboard state tracking to detect key presses and key transitions (isKeyPressed, isKeyJustPressed).

🎨 Rendering System

TextureManager loads, caches, and manages textures.
If a texture is missing, a magenta placeholder texture is automatically generated.
SpriteComponent supports both static textures and animated multi-row sprite sheets.

📷 Camera

A lightweight Camera class centers the viewport on the player, converting world coordinates into screen coordinates.

⚙️ Collision Handling

CollisionSystem provides bounding-box collision detection and platform collision resolution. It supports static and moving platforms, ensuring proper vertical and horizontal collision responses.

📦 XML Factory Simulation

XMLComponentFactory simulates XML-based object loading. It creates:

The player object

Ground platforms

Moving platforms

Enemies (patrolling and flying)

Controls
Key	Action
A / ←	Move Left
D / →	Move Right
Space / ↑	Jump
Esc / Close Window	Quit
