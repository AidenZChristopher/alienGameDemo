This project is a component-based 2D platformer engine built using C++17 and SDL2.
It demonstrates a modular game architecture where gameplay logic is divided into self-contained components (physics, rendering, behavior, etc.) that can be dynamically composed into GameObject entities.

🎮 Overview

The engine supports:

Player movement with gravity, jumping, and collision handling

Animated sprite sheets

Moving and static platforms
<img width="986" height="735" alt="Screenshot 2025-11-08 205844" src="https://github.com/user-attachments/assets/f697b43d-6244-432f-88b2-9479e583158f" />

Patrolling and bouncing enemies
<img width="1004" height="772" alt="Screenshot 2025-11-08 205826" src="https://github.com/user-attachments/assets/95d2ace5-370e-4be6-94d5-9f642488f38a" />
<img width="796" height="612" alt="Screenshot 2025-11-08 205918" src="https://github.com/user-attachments/assets/9219bbbc-876c-4211-a2b1-15874272a883" />

A following camera system

A centralized input and texture management system

⚙️ Features
🧱 Entity-Component System (ECS)

Each GameObject can have multiple components attached to define its behavior, such as:

BodyComponent – Tracks position, size, and velocity

SpriteComponent – Handles textures and sprite sheet animations

ControllerComponent – Manages player input and movement

PhysicsComponent – Adds gravity and physics-based motion

PatrolBehaviorComponent / BounceBehaviorComponent – Controls enemy AI movement

SolidComponent / EnemyComponent – Marks objects for collision or enemy logic

🎮 Input System

A centralized InputSystem uses SDL keyboard state tracking to detect key presses and transitions:

isKeyPressed(key)

isKeyJustPressed(key)

🎨 Rendering System

TextureManager loads, caches, and manages all textures.

Missing textures are replaced with a magenta placeholder.

SpriteComponent supports static textures and animated multi-row sprite sheets.

📷 Camera System

A lightweight Camera class centers the viewport on the player and converts world coordinates into screen coordinates for smooth scrolling and tracking.

⚡ Collision Handling

The CollisionSystem provides:

Bounding-box collision detection

Platform collision resolution

Support for moving and static platforms

Accurate vertical and horizontal collision responses

🧩 XML Factory Simulation

The XMLComponentFactory simulates XML-based object creation, generating:

The Player

Ground and moving platforms

Patrolling and flying enemies

🧭 Controls
Key	Action
A / ←	Move Left
D / →	Move Right
Space / ↑	Jump
Esc / Close Window	Quit

🗂️ Asset Management Using XML

Assets can be loaded from XML or dynamically enumerated from a folder.

Textures are stored in a map structure that associates asset names with file paths.

This system ensures efficient retrieval and reuse of textures throughout the engine.

🪟 View Class Implementation

Added a View class to handle camera transformations and viewport logic.

The View tracks:

Center position

Scale

Rotation angle (optional)

The engine stores a static View instance initialized during startup.

The sprite draw function now accounts for the View transformation.
⏱️ Frame Rate Limiting & Delta Time

The engine includes a frame rate limiter to maintain a consistent FPS.

The target frame rate can be statically configured.

If frame processing exceeds the ideal duration, the engine skips waiting to catch up.

A static deltaTime variable stores the duration of the last frame, ensuring consistent movement and animation speeds across all systems.

🧰 Tech Stack

Language: C++17

Framework: SDL2

Architecture: Entity-Component System (ECS)

🚀 Future Improvements

Multiple camera views

Configurable physics and collision layers

Enhanced XML-driven level editor
