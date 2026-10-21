==========================================================================
 DYNAMIC NAVIGATION AND ENTITY MANAGEMENT SYSTEM   (version 2)
 2D top-down zombie survival campaign  -  C++17 + SFML 3  -  starts FULLSCREEN
==========================================================================

HOW TO BUILD AND RUN  (Windows, MSYS2 + SFML 3 installed)
--------------------------------------------------------------------------
EASIEST:  double-click   build_and_run.bat   (inside this folder).
          It builds the game and starts it. If MSYS2 is not in C:\msys64, edit
          the PATH line at the top of the .bat file.

OR in the "MSYS2 UCRT64" terminal, from INSIDE this folder (ls must show
main.cpp and Game.cpp):

  g++ -std=c++17 -O2 *.cpp -o game.exe -lsfml-graphics -lsfml-window -lsfml-system -static-libgcc -static-libstdc++
  ./game.exe

Other ways to start it:
  ./game.exe --windowed   start in a window
  ./game.exe --demo       console demo: BFS vs Dijkstra (no window)

F11 switches fullscreen / windowed at any time. The 16:9 layout is kept, with
black bars if your screen has a different shape.
Font: font.ttf in this folder if present, otherwise Windows Arial.


CONTROLS
--------------------------------------------------------------------------
  WASD / arrows ... move               SHIFT ........ sprint (stamina)
  Left mouse ...... shoot              1 / 2 ........ Rifle / Pistol
  Right mouse ..... place / remove barricade on a road tile
  E ............... throw grenade at the mouse (it also hurts YOU)
  U ............... undo last barricade action            (stack)
  3 ............... medkit            5 or F ....... flashlight
  B ............... zombie AI: BFS <-> Dijkstra
  TAB ............. show every zombie's path
  F5 / F9 ......... save / load       F11 .......... fullscreen
  ESC or P ........ pause             ENTER ........ start / next mission / again


THE GAME (campaign of 3 missions)
--------------------------------------------------------------------------
  Mission 1  SAFE ZONE   - reach the safe zone.
  Mission 2  FUEL RUN    - collect 3 fuel cans (the yellow route guides you to
                           the nearest one), then reach the Evacuation Point.
  Mission 3  LAST STAND  - survive 75 s (a BOSS arrives), then reach the Helipad.
  Score, kills and health carry over between missions.

  Zombies:  Walker | Runner (fast, weak) | Tank (slow, tough) |
            Spitter (keeps its distance and spits acid) | BOSS (charges every 6 s)

  NEW LOGIC in version 2
    * NOISE: shots and explosions are heard. Zombies within earshot (rifle 650 px,
      pistol 380 px, grenade 750 px) hunt you even outside their normal sight range.
    * BARRICADES HAVE HEALTH: a zombie with no route to you walks to the nearest
      barricade and chews through it (Tank and Boss are faster). The graph changes
      again and everyone re-plans.
    * COMBO: kills within 3 s of each other build a score multiplier (up to x5).
    * AIR DROPS: a supply crate lands near you every 45 s.
    * Rubble (brown patches) slows everyone by half. BFS ignores the cost,
      Dijkstra walks around it - press B to compare.
    * Sprint with stamina, grenades with friendly fire, 3 difficulty levels,
      3 bonus objectives per mission (+200 each), undo, save/load, leaderboard.


FILES  -  GAME LOGIC (no SFML anywhere in these)
--------------------------------------------------------------------------
  GameTypes.h        Vec2/Vec2i, constants, enums, SaveData record
  Entities.h         Zombie, Pickup, Bullet, Acid, Grenade, Explosion, Input, Status
  Game.h/.cpp        ALL game rules: movement, waves, AI, combat, noise, combo,
                     barricade damage, missions flow, save/load glue
  Mission.h/.cpp     the campaign: table of MissionDef records + progress state

  DATA STRUCTURES (hand written)
  Graph.h/.cpp       graph as adjacency list; BFS (uses Queue); Dijkstra (uses
                     PriorityQueue); weighted tiles; editing the graph at runtime
  Queue.h/.cpp       circular array QUEUE (waves, BFS frontier)
  PriorityQueue.h/.cpp   binary MIN-HEAP (threat queue, Dijkstra)
  Inventory.h/.cpp   singly LINKED LIST
  ActionStack.h/.cpp STACK for undo
  Leaderboard.h/.cpp BINARY SEARCH TREE + C file I/O
  GameSave.h/.cpp    save/load, validate-then-apply (format v3)
  PathfindingDemo.cpp  console BFS vs Dijkstra demo (no header)

FILES  -  SFML (drawing and input; they only READ the game state)
--------------------------------------------------------------------------
  Gfx.h/.cpp         window, FULLSCREEN, letterboxing, drawing helpers
  Renderer.h/.cpp    city, entities, HUD, minimap, sidebar
  Screens.h/.cpp     menu, leaderboard, help, pause / mission-clear / game-over
  App.h/.cpp         event loop, keyboard + mouse -> game, screen switching
  main.cpp           entry point (--windowed, --demo)

HOW THE PIECES FIT
--------------------------------------------------------------------------
  keyboard/mouse -> App -> Input struct -> Game::update(dt, input)
  Game (read only) -> Renderer / Screens -> Gfx -> the SFML window
  The logic files never include <SFML/...>, so the rules can be tested without
  opening a window (that is how they were tested).


WHAT IS TESTED
--------------------------------------------------------------------------
  Logic: 43 gameplay checks (all 3 missions, boss, noise, combo, spitter,
  barricade chewing, undo rules, save/load incl. corrupt and old-version files,
  3 long random soak runs) + data-structure tests, all under AddressSanitizer and
  UndefinedBehaviorSanitizer with no errors.
  Integration: the whole program (logic + App + Renderer + Screens) was linked
  and run through scripted play-throughs, death and restart, and every screen
  and overlay was drawn - using a stand-in for the SFML library.
  NOT tested here: the real SFML library calls and real fullscreen. Please run
  the game once on your PC and report the first compiler error, if any.

