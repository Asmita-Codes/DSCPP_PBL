
#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include "GameTypes.h"

struct Edge {
    int to;       
    int weight;   
};

class Graph {
public:
    Graph(int cols, int rows);

    void buildCity();                               

    int cols() const { return cols_; }
    int rows() const { return rows_; }
    bool inBounds(Vec2i t) const;
    TileType tile(Vec2i t) const;                   
    void setTile(Vec2i t, TileType type);           
    bool walkable(Vec2i t) const;                   
    bool isBuilding(Vec2i t) const { return tile(t) == TileType::Building; }
    bool canStand(Vec2 p, float radius) const;      
    static int tileCost(TileType t);

    
    std::vector<Vec2i> bfs(Vec2i from, Vec2i to, int* explored = nullptr) const;
    std::vector<Vec2i> dijkstra(Vec2i from, Vec2i to, int* explored = nullptr, int* totalCost = nullptr) const;

    std::vector<Vec2i> tilesOfType(TileType t) const;
    int nodeCount() const { return cols_ * rows_; }
    int edgeCount() const;
    unsigned version() const { return version_; }   

private:
    int idx(Vec2i t) const { return t.y * cols_ + t.x; }
    void rebuildEdges(Vec2i t);                    
    std::vector<Vec2i> buildPath(const std::vector<int>& prev, int goalIdx) const;

    int cols_, rows_;
    std::vector<TileType> tiles_;
    std::vector<std::vector<Edge>> adj_;            
    unsigned version_ = 0;
};

#endif
