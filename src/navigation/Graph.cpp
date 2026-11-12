#include "Graph.h"
#include <algorithm>
#include "PriorityQueue.h"
#include "Queue.h"

namespace {
const Vec2i kDirs[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
}

Graph::Graph(int cols, int rows)
    : cols_(cols), rows_(rows), tiles_((size_t)cols * rows, TileType::Building), adj_((size_t)cols * rows) {}

bool Graph::inBounds(Vec2i t) const { return t.x >= 0 && t.y >= 0 && t.x < cols_ && t.y < rows_; }

TileType Graph::tile(Vec2i t) const { return inBounds(t) ? tiles_[idx(t)] : TileType::Building; }

bool Graph::walkable(Vec2i t) const {
    TileType k = tile(t);
    return k == TileType::Road || k == TileType::Rubble;
}

int Graph::tileCost(TileType t) { return t == TileType::Rubble ? 3 : 1; }

bool Graph::canStand(Vec2 p, float radius) const {
    for (int dx : {-1, 1})
        for (int dy : {-1, 1})
            if (!walkable(toTile(Vec2(p.x + dx * radius, p.y + dy * radius)))) return false;
    return true;
}

void Graph::rebuildEdges(Vec2i t) {
    if (!inBounds(t)) return;
    std::vector<Edge>& list = adj_[idx(t)];
    list.clear();
    for (Vec2i d : kDirs) {
        Vec2i n = t + d;
        if (walkable(n)) list.push_back({idx(n), tileCost(tile(n))});
    }
}

void Graph::setTile(Vec2i t, TileType type) {
    if (!inBounds(t)) return;
    tiles_[idx(t)] = type;
    rebuildEdges(t);                                
    for (Vec2i d : kDirs) rebuildEdges(t + d);      
    ++version_;
}

void Graph::buildCity() {
    std::fill(tiles_.begin(), tiles_.end(), TileType::Building);
  
    for (int r = 0; r < rows_; ++r)
        for (int c = 0; c < cols_; ++c)
            if (r % 10 < 2 || c % 12 < 2) tiles_[idx({c, r})] = TileType::Road;
    for (int r = 0; r < rows_; ++r)
        for (int c = 0; c < cols_; ++c) rebuildEdges({c, r});
    ++version_;
}

int Graph::edgeCount() const {
    int n = 0;
    for (const auto& list : adj_) n += (int)list.size();
    return n;
}

std::vector<Vec2i> Graph::tilesOfType(TileType t) const {
    std::vector<Vec2i> out;
    for (int y = 0; y < rows_; ++y)
        for (int x = 0; x < cols_; ++x)
            if (tiles_[idx({x, y})] == t) out.push_back({x, y});
    return out;
}

std::vector<Vec2i> Graph::buildPath(const std::vector<int>& prev, int goalIdx) const {
    std::vector<Vec2i> path;
    for (int i = goalIdx; i != -1; i = prev[i]) path.push_back({i % cols_, i / cols_});
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<Vec2i> Graph::bfs(Vec2i from, Vec2i to, int* explored) const {
    if (explored) *explored = 0;
    if (!inBounds(from) || !walkable(to)) return {};
    std::vector<int> prev((size_t)nodeCount(), -2);   
    Queue frontier;
    int start = idx(from), goal = idx(to), cur = 0, count = 0;
    prev[start] = -1;
    frontier.enqueue(start);
    while (frontier.dequeue(cur)) {
        ++count;
        if (cur == goal) break;
        for (const Edge& e : adj_[cur])
            if (prev[e.to] == -2) { prev[e.to] = cur; frontier.enqueue(e.to); }
    }
    if (explored) *explored = count;
    if (prev[goal] == -2) return {};
    return buildPath(prev, goal);
}

std::vector<Vec2i> Graph::dijkstra(Vec2i from, Vec2i to, int* explored, int* totalCost) const {
    if (explored) *explored = 0;
    if (totalCost) *totalCost = -1;
    if (!inBounds(from) || !walkable(to)) return {};
    const float INF = 1e9f;
    std::vector<float> dist((size_t)nodeCount(), INF);
    std::vector<int> prev((size_t)nodeCount(), -2);
    PriorityQueue pq;
    int start = idx(from), goal = idx(to), count = 0;
    dist[start] = 0.f;
    prev[start] = -1;
    pq.push(0.f, start);
    PQItem it;
    while (pq.pop(it)) {
        int u = it.id;
        if (it.key > dist[u]) continue;               
        ++count;
        if (u == goal) break;
        for (const Edge& e : adj_[u]) {
            float nd = dist[u] + (float)e.weight;
            if (nd < dist[e.to]) { dist[e.to] = nd; prev[e.to] = u; pq.push(nd, e.to); }
        }
    }
    if (explored) *explored = count;
    if (dist[goal] >= INF) return {};
    if (totalCost) *totalCost = (int)dist[goal];
    return buildPath(prev, goal);
}
