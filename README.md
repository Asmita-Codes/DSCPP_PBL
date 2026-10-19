# Dynamic Navigation and Entity Management System

A 2D top-down zombie survival game written in **C++17** with **SFML 3**.
The player fights through a three-mission campaign in a city whose road network is a **graph that changes while you play**: placing or destroying barricades edits the graph, and every zombie re-plans its route using **BFS** or **Dijkstra's algorithm**.

The project was built to demonstrate hand-written data structures (graph, queue, priority queue / min-heap, linked list, stack, binary search tree) inside a real-time game.


## Features

- **Full-screen gameplay** with a compact HUD along the bottom edge: Health, Weapon, Ammo, Medkits, Grenades, Score and Level, all read live from the game state.
- **Dynamic navigation**: the city is a tile graph (adjacency list). Barricades add or remove edges at runtime, and zombies recompute their paths.
- **Two pathfinding algorithms**: BFS (fewest steps, ignores terrain cost) and Dijkstra (cheapest route, walks around rubble). Press **B** to switch.
- **Five zombie types**: Walker, Runner (fast, weak), Tank (slow, tough), Spitter (keeps its distance and spits acid), and a **Boss** that charges.
- **Noise system**: gunshots and explosions are heard. Zombies within earshot start hunting you even outside their sight range (the rifle is louder than the pistol).
- **Destructible barricades**: a zombie with no route to you walks to the nearest barricade and chews through it.
- **Combo scoring**: kills within 3 seconds of each other build a multiplier up to x5.
- **Survival timer** in Level 3, shown on screen and paused whenever the game is paused.
- Grenades with friendly fire, sprinting with stamina, a flashlight, air drops, rubble that slows movement, three difficulty levels, three bonus objectives per mission, undo, save/load and a persistent leaderboard.

---

## The Campaign

| Level | Mission | Objective |
|---|---|---|
| 1 | SAFE ZONE | Reach the safe zone. |
| 2 | FUEL RUN | Collect 3 fuel cans (the yellow route guides you to the nearest one), then reach the Evacuation Point. |
| 3 | LAST STAND | Survive 75 seconds (a Boss arrives), then reach the Helipad. |

Score, kills and health carry over between missions.

### Scoring

- Killing a zombie gives that zombie's points multiplied by the current combo multiplier (x1 to x5).
- Each pickup collected: +25.
- Each bonus objective completed: +200.
- Completing a mission: +500, plus a time bonus (300 minus the seconds spent, never below 0), plus 100 × the mission number (counting from 0).

---

## Controls

| Key / Button | Action |
|---|---|
| `W A S D` / Arrow keys | Move |
| `Shift` | Sprint (uses stamina) |
| Left mouse | Shoot |
| Right mouse | Place / remove a barricade on a road tile |
| `1` / `2` | Switch to Rifle / Pistol |
| `3` | Use a medkit |
| `E` | Throw a grenade at the mouse position (it can hurt you too) |
| `5` or `F` | Flashlight on/off |
| `U` | Undo the last barricade action |
| `B` | Switch zombie AI between BFS and Dijkstra |
| `Tab` | Show every zombie's current path |
| `F5` / `F9` | Save / Load |
| `Esc` or `P` | Pause / resume |
| `M` (while paused) | Return to the main menu |
| `Enter` | Start / next mission / play again |
| `F11` | Toggle fullscreen |

### Main menu

Start Campaign, Continue (load a save) / **Resume Game** (when a game is in progress), Difficulty, Leaderboard, How to Play, Fullscreen and Exit.

---

## Building and Running (Windows)

### 1. Install the tools

Install [MSYS2](https://www.msys2.org/), open the **MSYS2 UCRT64** terminal and run:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-sfml
```

SFML **3** is required.

### 2. Build

In the MSYS2 UCRT64 terminal, go to the project folder (the one containing `App.cpp` and `main.cpp`) and run:

```bash
g++ -std=c++17 -O2 *.cpp -o game.exe -lsfml-graphics -lsfml-window -lsfml-system -static-libgcc -static-libstdc++
```

Or simply double-click **`build_and_run.bat`**. It builds and starts the game. If MSYS2 is not installed in `C:\msys64`, edit the PATH line at the top of the file.

### 3. Run

```bash
./game.exe              # starts in fullscreen
./game.exe --windowed   # starts in a window
./game.exe --demo       # console demo: BFS vs Dijkstra (no window)
```

Run the game from the project folder so it can find `assets/font.ttf`. If the font is missing, it falls back to Windows Arial.

---

## Data Structures

All data structures are written by hand. None of them use the STL container they imitate.

| Structure | File | Used for |
|---|---|---|
| Graph (adjacency list) | `Graph.h/.cpp` | City map, BFS and Dijkstra, editing the map at runtime |
| Queue (circular array) | `Queue.h/.cpp` | Zombie wave queue, BFS frontier |
| Priority queue (binary min-heap) | `PriorityQueue.h/.cpp` | Most dangerous zombie first, Dijkstra |
| Singly linked list | `Inventory.h/.cpp` | Player inventory |
| Stack | `ActionStack.h/.cpp` | Undo for barricade actions |
| Binary search tree | `Leaderboard.h/.cpp` | Leaderboard (stored in `leaderboard.txt`) |

---

## Project Structure

**Game logic (no SFML in these files)**

| File | Purpose |
|---|---|
| `GameTypes.h` | Vectors, constants, enums, save record |
| `Entities.h` | Zombie, Pickup, Bullet, Acid, Grenade, Explosion, Input, Status |
| `Game.h/.cpp` | All game rules: movement, waves, AI, combat, noise, combo, barricade damage, mission flow |
| `Mission.h/.cpp` | Campaign table (mission definitions) and mission progress |
| `GameSave.h/.cpp` | Save / load with validation |
| `PathfindingDemo.cpp` | Console demo comparing BFS and Dijkstra |

**SFML (drawing and input; these only read the game state)**

| File | Purpose |
|---|---|
| `Gfx.h/.cpp` | Window, fullscreen, letterboxing, drawing helpers |
| `Renderer.h/.cpp` | City, entities, minimap |
| `Screens.h/.cpp` | Main menu, leaderboard, help screen |
| `App.h/.cpp` | Event loop, input, screen switching, full-screen gameplay HUD, Level 3 survival timer |
| `main.cpp` | Entry point (`--windowed`, `--demo`) |

**How the pieces fit**

```
keyboard / mouse -> App -> Input -> Game::update(dt, input)
Game (read-only) -> Renderer / Screens / App HUD -> Gfx -> SFML window
```

Because the logic files never include SFML, the game rules can be tested without opening a window.

**Earlier C version (not part of the current build)**

`graph.c`, `inventory.c`, `leaderboard.c`, `priority_queue.c`, `zombie_queue.c`, `action_stack.c`, `save_load.c` and their headers (`game_types.h`, `priority_queue.h`, `zombie_queue.h`, `action_stack.h`, `save_load.h`) belong to an earlier C version of the project. The build command above compiles only the `.cpp` files, so these are kept for reference only.

> **Note:** `CMakeLists.txt` also belongs to the earlier version and does not build the current game. Use the `g++` command or `build_and_run.bat` instead.

---

## Generated Files

| File | Created by |
|---|---|
| `savegame.txt` | `F5` (save) |
| `leaderboard.txt` | Finishing or losing a run |

---

## Team

1. ASMITA : GUI and application flow
2. NEHA NEGI : Navigation and game logic
3. LAKSHYA GUSAIN : Data Management and Testing
4. FATIMA SUGHRA : Core Structure

Graphic Era — Project Based Learning (PBL)

---

## License and Credits

- Built with [SFML](https://www.sfml-dev.org/).
- The font license is in `assets/FONT_LICENSE.txt`.
