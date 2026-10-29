// PathfindingDemo.cpp - console demo that compares BFS and Dijkstra on a small map.
//
// Two ways to use this file:
//   1) Inside the game build (g++ *.cpp ...): it only provides runPathfindingDemo(),
//      which main.cpp calls when you start the game with:  game.exe --demo
//   2) As a stand-alone program (no SFML needed):
//        g++ -std=c++17 -DPATHFINDING_DEMO_MAIN PathfindingDemo.cpp Graph.cpp Queue.cpp PriorityQueue.cpp -o demo.exe
//
// Map legend:  #=building  .=road  ~=rubble (cost 3)  X=barricade  S=start  G=goal  *=path

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>
#include "Graph.h"

namespace {

const char* kMap[] = {
    "##############",
    "#S...~~~.....#",   // top corridor: 15 steps, but crosses 3 rubble tiles
    "#.##########.#",
    "#.##########.#",
    "#.##########.#",
    "#.##########G#",
    "#............#",   // bottom corridor: 17 steps, no rubble
    "##############",
};
const int kRows = 8, kCols = 14;

void printMap(const Graph& g, const std::vector<Vec2i>& path, Vec2i s, Vec2i t) {
    std::vector<std::string> out(kRows, std::string(kCols, ' '));
    for (int y = 0; y < kRows; ++y)
        for (int x = 0; x < kCols; ++x) {
            TileType k = g.tile({x, y});
            out[y][x] = k == TileType::Building ? '#' : k == TileType::Rubble ? '~' : k == TileType::Barricade ? 'X' : '.';
        }
    for (Vec2i p : path) out[p.y][p.x] = '*';
    out[s.y][s.x] = 'S';
    out[t.y][t.x] = 'G';
    for (const auto& row : out) std::printf("    %s\n", row.c_str());
}

int rubbleOn(const Graph& g, const std::vector<Vec2i>& path) {
    int n = 0;
    for (Vec2i p : path) if (g.tile(p) == TileType::Rubble) ++n;
    return n;
}

double microsPerCall(const Graph& g, bool useDijkstra, Vec2i s, Vec2i t) {
    const int N = 2000;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        if (useDijkstra) g.dijkstra(s, t);
        else g.bfs(s, t);
    }
    return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / N;
}

void compare(const Graph& g, Vec2i s, Vec2i t, const char* title) {
    std::printf("\n=== %s ===\n", title);
    int exB = 0, exD = 0, cost = 0;
    auto pb = g.bfs(s, t, &exB);
    auto pd = g.dijkstra(s, t, &exD, &cost);
    if (pb.empty()) { std::printf("  No route exists (the goal is sealed off).\n"); return; }

    int bfsCost = 0;
    for (size_t i = 1; i < pb.size(); ++i) bfsCost += Graph::tileCost(g.tile(pb[i]));

    std::printf("  BFS      : %2d steps, cost %2d, rubble tiles crossed %d, nodes explored %3d, %.2f us/call\n",
                (int)pb.size() - 1, bfsCost, rubbleOn(g, pb), exB, microsPerCall(g, false, s, t));
    printMap(g, pb, s, t);
    std::printf("  Dijkstra : %2d steps, cost %2d, rubble tiles crossed %d, nodes explored %3d, %.2f us/call\n",
                (int)pd.size() - 1, cost, rubbleOn(g, pd), exD, microsPerCall(g, true, s, t));
    printMap(g, pd, s, t);
    if (cost < bfsCost) std::printf("  -> Dijkstra found a CHEAPER route (BFS only minimises steps, not cost).\n");
    else std::printf("  -> Same cost: here the fewest steps is also the cheapest route.\n");
}

}  // namespace

int runPathfindingDemo() {
    std::printf("Pathfinding demo: BFS (Queue) vs Dijkstra (PriorityQueue)\n");
    Graph g(kCols, kRows);
    for (int y = 0; y < kRows; ++y)
        for (int x = 0; x < kCols; ++x) {
            char c = kMap[y][x];
            g.setTile({x, y}, c == '#' ? TileType::Building : c == '~' ? TileType::Rubble : TileType::Road);
        }
    Vec2i s{1, 1}, t{12, 5};
    std::printf("Graph: %d nodes, %d edges\n", g.nodeCount(), g.edgeCount());

    compare(g, s, t, "1. Original map");

    g.setTile({12, 2}, TileType::Barricade);  // the player blocks the top corridor with a barricade
    compare(g, s, t, "2. Barricade at (12,2): the top route disappears, both re-route  [dynamic navigation]");

    g.setTile({12, 6}, TileType::Barricade);  // ...and then the last way into the goal
    compare(g, s, t, "3. Goal fully sealed");

    std::printf("\nWhy it matters: zombies re-run the chosen algorithm every time the graph changes.\n");
    return 0;
}

#ifdef PATHFINDING_DEMO_MAIN
int main() { return runPathfindingDemo(); }
#endif
