#ifndef ACTIONSTACK_H
#define ACTIONSTACK_H

#include <string>
#include "GameTypes.h"

enum class ActionType { BarricadePlaced = 1, BarricadeRemoved = 2 };

struct Action {
    ActionType type;
    Vec2i tile;
};

class ActionStack {
public:
    ActionStack();
    ActionStack(const ActionStack&) = delete;
    ActionStack& operator=(const ActionStack&) = delete;
    ~ActionStack();

    void push(const Action& a);
    bool pop(Action& out);
    bool peek(Action& out) const;
    int size() const { return size_; }
    void clear() { size_ = 0; }
    std::string describeTop() const;   

private:
    Action* data_;
    int capacity_;
    int size_;
};

#endif
