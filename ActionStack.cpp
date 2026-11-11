#include "ActionStack.h"

ActionStack::ActionStack() : data_(new Action[8]), capacity_(8), size_(0) {}

ActionStack::~ActionStack() { delete[] data_; }

void ActionStack::push(const Action& a) {
    if (size_ == capacity_) {                      
        Action* bigger = new Action[capacity_ * 2];
        for (int i = 0; i < size_; ++i) bigger[i] = data_[i];
        delete[] data_;
        data_ = bigger;
        capacity_ *= 2;
    }
    data_[size_++] = a;
}

bool ActionStack::pop(Action& out) {
    if (size_ == 0) return false;
    out = data_[--size_];
    return true;
}

bool ActionStack::peek(Action& out) const {
    if (size_ == 0) return false;
    out = data_[size_ - 1];
    return true;
}

std::string ActionStack::describeTop() const {
    Action a;
    if (!peek(a)) return "(empty)";
    const char* what = a.type == ActionType::BarricadePlaced ? "Barricade placed" : "Barricade removed";
    return std::string(what) + " (" + std::to_string(a.tile.x) + "," + std::to_string(a.tile.y) + ")";
}
