#include "Inventory.h"
#include <cstring>

Inventory::Inventory() : head_(nullptr), size_(0) {}

Inventory::~Inventory() { clear(); }

void Inventory::clear() {
    while (head_) {
        ItemNode* next = head_->next;
        delete head_;
        head_ = next;
    }
    size_ = 0;
}

ItemNode* Inventory::find(const char* name) const {
    for (ItemNode* n = head_; n; n = n->next)
        if (std::strcmp(n->name, name) == 0) return n;
    return nullptr;
}

void Inventory::set(const char* name, int count) {
    if (ItemNode* n = find(name)) { n->count = count; return; }
    ItemNode* node = new ItemNode;
    std::strncpy(node->name, name, sizeof node->name - 1);
    node->name[sizeof node->name - 1] = '\0';
    node->count = count;
    node->next = nullptr;
    if (!head_) head_ = node;                       
    else { ItemNode* t = head_; while (t->next) t = t->next; t->next = node; }
    ++size_;
}

void Inventory::add(const char* name, int n) {
    if (ItemNode* node = find(name)) node->count += n;
    else set(name, n);
}

bool Inventory::consume(const char* name, int n) {
    ItemNode* node = find(name);
    if (!node || node->count < n) return false;
    node->count -= n;
    return true;
}

int Inventory::count(const char* name) const {
    const ItemNode* node = find(name);
    return node ? node->count : 0;
}

void Inventory::resetDefaults() {
    clear();
    set("Rifle", 60);
    set("Pistol", 30);
    set("Medkit", 2);
    set("Grenade", 2);
    set("Barrier", 5);
    set("Torch", 1);
}
