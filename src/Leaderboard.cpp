#include "Leaderboard.h"
#include <cstdio>
#include <cstring>

namespace {

BstNode* insertNode(BstNode* root, BstNode* fresh) {
    if (!root) return fresh;
    if (fresh->score < root->score) root->left = insertNode(root->left, fresh);
    else                            root->right = insertNode(root->right, fresh);
    return root;
}

void freeTree(BstNode* n) {
    if (!n) return;
    freeTree(n->left);
    freeTree(n->right);
    delete n;
}

void collectDesc(const BstNode* n, std::vector<ScoreRow>& out) {
    if (!n) return;
    collectDesc(n->right, out);                 
    out.push_back({n->name, n->score, n->secs, n->difficulty});
    collectDesc(n->left, out);
}

int heightOf(const BstNode* n) {
    if (!n) return 0;
    int l = heightOf(n->left), r = heightOf(n->right);
    return 1 + (l > r ? l : r);
}

std::string sanitize(std::string s) {
    for (char& c : s)
        if (c == ' ' || c == '\t' || c == '\n') c = '_';
    if (s.empty()) s = "Player";
    if (s.size() > 20) s.resize(20);
    return s;
}

} 

Leaderboard::Leaderboard() : root_(nullptr), size_(0) {}

Leaderboard::~Leaderboard() { clear(); }

void Leaderboard::clear() {
    freeTree(root_);
    root_ = nullptr;
    size_ = 0;
}

void Leaderboard::add(const std::string& name, int score, int secs, int difficulty) {
    BstNode* n = new BstNode;
    std::string clean = sanitize(name);
    std::strncpy(n->name, clean.c_str(), sizeof n->name - 1);
    n->name[sizeof n->name - 1] = '\0';
    n->score = score;
    n->secs = secs;
    n->difficulty = difficulty;
    n->left = n->right = nullptr;
    root_ = insertNode(root_, n);
    ++size_;
}

std::vector<ScoreRow> Leaderboard::top(int n) const {
    std::vector<ScoreRow> rows;
    collectDesc(root_, rows);
    if ((int)rows.size() > n) rows.resize((size_t)n);
    return rows;
}

int Leaderboard::height() const { return heightOf(root_); }

void Leaderboard::seedSamples() {
    add("Sample_A", 18550, 5130, 2);
    add("Sample_B", 9840, 2600, 1);
    add("Sample_C", 4200, 1500, 0);
}

bool Leaderboard::load(const char* path) {
    std::FILE* f = std::fopen(path, "r");
    if (!f) return false;
    clear();
    char line[128];
    while (std::fgets(line, sizeof line, f)) {
        char name[24];
        int score = 0, secs = 0, diff = 1;
        int got = std::sscanf(line, "%23s %d %d %d", name, &score, &secs, &diff);
        if (got >= 3) add(name, score, secs, got == 4 ? diff : 1);
    }
    std::fclose(f);
    return true;
}

bool Leaderboard::save(const char* path) const {
    std::FILE* f = std::fopen(path, "w");
    if (!f) return false;
    for (const ScoreRow& r : top(size_)) std::fprintf(f, "%s %d %d %d\n", r.name.c_str(), r.score, r.secs, r.difficulty);
    std::fclose(f);
    return true;
}
