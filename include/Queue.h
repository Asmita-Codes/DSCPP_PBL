#ifndef QUEUE_H
#define QUEUE_H

class Queue {
public:
    explicit Queue(int capacity = 16);
    Queue(const Queue& other);
    Queue& operator=(const Queue& other);
    ~Queue();

    void enqueue(int value);                 
    bool dequeue(int& out);                  
    bool peek(int& out) const;               
    bool at(int index, int& out) const;      
    int size() const { return size_; }
    bool empty() const { return size_ == 0; }
    void clear();

private:
    void grow();
    int* data_;
    int capacity_;
    int head_;
    int size_;
};

#endif
