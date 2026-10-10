#ifndef INVENTORY_H
#define INVENTORY_H

struct ItemNode {
    char name[24];
    int count;
    ItemNode* next;
};

class Inventory {
public:
    Inventory();
    Inventory(const Inventory&) = delete;
    Inventory& operator=(const Inventory&) = delete;
    ~Inventory();

    void clear();
    void resetDefaults();                           
    void set(const char* name, int count);
    void add(const char* name, int n);
    bool consume(const char* name, int n = 1);      
    int count(const char* name) const;
    int size() const { return size_; }
    const ItemNode* head() const { return head_; }  

private:
    ItemNode* find(const char* name) const;
    ItemNode* head_;
    int size_;
};

#endif
