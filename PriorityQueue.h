#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

struct PQItem {
    float key;   
    int id;
};

class PriorityQueue {
public:
    PriorityQueue();
    PriorityQueue(const PriorityQueue& other);
    PriorityQueue& operator=(const PriorityQueue& other);
    ~PriorityQueue();

    void push(float key, int id);
    bool pop(PQItem& out);              
    bool peek(PQItem& out) const;
    int size() const { return size_; }
    bool empty() const { return size_ == 0; }
    void clear() { size_ = 0; }

private:
    void siftUp(int i);
    void siftDown(int i);
    PQItem* data_;
    int capacity_;
    int size_;
};

#endif
