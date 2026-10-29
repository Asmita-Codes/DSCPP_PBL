// Graph.h - the CITY AS A GRAPH (adjacency list) + BFS and Dijkstra path finding.
// Concepts: graph (nodes = tiles, edges = walkable neighbours), adjacency list, BFS (uses our Queue),
//           Dijkstra (uses our PriorityQueue), weighted edges (rubble is slower), editing the graph at
//           runtime (barricades) and keeping the edge lists consistent.
#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include "GameTypes.h"

struct Edge {
    int to;       // index of the neighbour node
    int weight;   // cost of stepping onto that neighbour (road 1, rubble 3)
};

class Graph {
public:
    Graph(int cols, int rows);

    void buildCity();                               // default road layout, everything else is buildings

    int cols() const { return cols_; }
    int rows() const { return rows_; }
    bool inBounds(Vec2i t) const;
    TileType tile(Vec2i t) const;                   // out of bounds counts as Building
    void setTile(Vec2i t, TileType type);           // updates the node AND the edge lists around it
    bool walkable(Vec2i t) const;                   // Road or Rubble
    bool isBuilding(Vec2i t) const { return tile(t) == TileType::Building; }
    bool canStand(Vec2 p, float radius) const;      // circle vs. tile collision for movement
    static int tileCost(TileType t);

    // Both return the path from `from` to `to` (inclusive). Empty = unreachable.
    // `explored` (optional) receives the number of nodes taken out of the frontier (to compare algorithms).
    std::vector<Vec2i> bfs(Vec2i from, Vec2i to, int* explored = nullptr) const;
    std::vector<Vec2i> dijkstra(Vec2i from, Vec2i to, int* explored = nullptr, int* totalCost = nullptr) const;

    std::vector<Vec2i> tilesOfType(TileType t) const;
    int nodeCount() const { return cols_ * rows_; }
    int edgeCount() const;
    unsigned version() const { return version_; }   // changes whenever the graph changes

private:
    int idx(Vec2i t) const { return t.y * cols_ + t.x; }
    void rebuildEdges(Vec2i t);                     // recompute the outgoing edges of one node
    std::vector<Vec2i> buildPath(const std::vector<int>& prev, int goalIdx) const;

    int cols_, rows_;
    std::vector<TileType> tiles_;
    std::vector<std::vector<Edge>> adj_;            // adj_[node] = edges to WALKABLE neighbours
    unsigned version_ = 0;
};

#endif
