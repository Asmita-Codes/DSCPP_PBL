
#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include <string>
#include <vector>

struct ScoreRow {
    std::string name;
    int score;
    int secs;
    int difficulty;   
};

struct BstNode {
    char name[24];
    int score;
    int secs;
    int difficulty;
    BstNode* left;
    BstNode* right;
};

class Leaderboard {
public:
    Leaderboard();
    Leaderboard(const Leaderboard&) = delete;
    Leaderboard& operator=(const Leaderboard&) = delete;
    ~Leaderboard();

    void add(const std::string& name, int score, int secs, int difficulty);
    std::vector<ScoreRow> top(int n) const;     
    int size() const { return size_; }
    int height() const;
    void clear();
    void seedSamples();                         

    bool load(const char* path);                
    bool save(const char* path) const;

private:
    BstNode* root_;
    int size_;
};

#endif
