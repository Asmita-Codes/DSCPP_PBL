#include "Queue.h"

Queue::Queue(int capacity)
    : data_(new int[capacity > 1 ? capacity : 2]), capacity_(capacity > 1 ? capacity : 2), head_(0), size_(0) {}

Queue::Queue(const Queue& other)
    : data_(new int[other.capacity_]), capacity_(other.capacity_), head_(0), size_(other.size_) {
    for (int i = 0; i < size_; ++i) data_[i] = other.data_[(other.head_ + i) % other.capacity_];
}

Queue& Queue::operator=(const Queue& other) {
    if (this == &other) return *this;
    int* fresh = new int[other.capacity_];
    for (int i = 0; i < other.size_; ++i) fresh[i] = other.data_[(other.head_ + i) % other.capacity_];
    delete[] data_;
    data_ = fresh;
    capacity_ = other.capacity_;
    head_ = 0;
    size_ = other.size_;
    return *this;
}

Queue::~Queue() { delete[] data_; }

void Queue::grow() {
    int newCap = capacity_ * 2;
    int* bigger = new int[newCap];
    for (int i = 0; i < size_; ++i) bigger[i] = data_[(head_ + i) % capacity_];  // unwrap into a straight line
    delete[] data_;
    data_ = bigger;
    capacity_ = newCap;
    head_ = 0;
}

void Queue::enqueue(int value) {
    if (size_ == capacity_) grow();
    data_[(head_ + size_) % capacity_] = value;
    ++size_;
}

bool Queue::dequeue(int& out) {
    if (size_ == 0) return false;
    out = data_[head_];
    head_ = (head_ + 1) % capacity_;
    --size_;
    return true;
}

bool Queue::peek(int& out) const {
    if (size_ == 0) return false;
    out = data_[head_];
    return true;
}

bool Queue::at(int index, int& out) const {
    if (index < 0 || index >= size_) return false;
    out = data_[(head_ + index) % capacity_];
    return true;
}

void Queue::clear() {
    head_ = 0;
    size_ = 0;
}
