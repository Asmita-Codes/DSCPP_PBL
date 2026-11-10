#include "PriorityQueue.h"

PriorityQueue::PriorityQueue() : data_(new PQItem[16]), capacity_(16), size_(0) {}

PriorityQueue::PriorityQueue(const PriorityQueue& o) : data_(new PQItem[o.capacity_]), capacity_(o.capacity_), size_(o.size_) {
    for (int i = 0; i < size_; ++i) data_[i] = o.data_[i];
}

PriorityQueue& PriorityQueue::operator=(const PriorityQueue& o) {
    if (this == &o) return *this;
    PQItem* fresh = new PQItem[o.capacity_];
    for (int i = 0; i < o.size_; ++i) fresh[i] = o.data_[i];
    delete[] data_;
    data_ = fresh;
    capacity_ = o.capacity_;
    size_ = o.size_;
    return *this;
}

PriorityQueue::~PriorityQueue() { delete[] data_; }

void PriorityQueue::siftUp(int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (data_[parent].key <= data_[i].key) break;
        PQItem t = data_[parent]; data_[parent] = data_[i]; data_[i] = t;
        i = parent;
    }
}

void PriorityQueue::siftDown(int i) {
    for (;;) {
        int l = 2 * i + 1, r = 2 * i + 2, smallest = i;
        if (l < size_ && data_[l].key < data_[smallest].key) smallest = l;
        if (r < size_ && data_[r].key < data_[smallest].key) smallest = r;
        if (smallest == i) break;
        PQItem t = data_[i]; data_[i] = data_[smallest]; data_[smallest] = t;
        i = smallest;
    }
}

void PriorityQueue::push(float key, int id) {
    if (size_ == capacity_) {
        PQItem* bigger = new PQItem[capacity_ * 2];
        for (int i = 0; i < size_; ++i) bigger[i] = data_[i];
        delete[] data_;
        data_ = bigger;
        capacity_ *= 2;
    }
    data_[size_].key = key;
    data_[size_].id = id;
    siftUp(size_);
    ++size_;
}

bool PriorityQueue::pop(PQItem& out) {
    if (size_ == 0) return false;
    out = data_[0];
    data_[0] = data_[--size_];
    siftDown(0);
    return true;
}

bool PriorityQueue::peek(PQItem& out) const {
    if (size_ == 0) return false;
    out = data_[0];
    return true;
}
