#include "Database.h"
#include <algorithm>
#include <climits>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>

void check(bool ok) { if (!ok) throw std::runtime_error("Check failed"); }
template<class F> void throws(F f) {
    bool caught = false;
    try { f(); } catch (const std::out_of_range&) { caught = true; }
    check(caught);
}
int validate(const Node* p, long long lo, long long hi, std::vector<int>& values) {
    if (!p) return 0;
    check(lo < p->key && p->key < hi);
    int l = validate(p->left, lo, p->key, values);
    values.push_back(p->key);
    int r = validate(p->right, p->key, hi, values);
    check(std::abs(l-r) <= 1);
    check(p->height == 1 + std::max(l, r));
    return p->height;
}
int main() {
    try {
        DynamicArray a{}; initArray(a, 0);
        for (int i = 0; i < 100; ++i) pushBack(a, i);
        insertAt(a, 0, -1); insertAt(a, a.size, 100); insertAt(a, 51, 999);
        check(eraseAt(a, 51) == 999); setAt(a, 0, -2);
        check(getAt(a, 0) == -2 && arrayLength(a) == 102);
        throws([&]{ getAt(a, -1); }); throws([&]{ insertAt(a, 103, 0); });
        freeArray(a); pushBack(a, 42); check(getAt(a, 0) == 42); freeArray(a);
        SinglyNode* f = nullptr;
        insertSingly(f, 0, 2); insertSingly(f, 0, 1); insertSingly(f, 2, 3);
        check(findSingly(f, 2) == 1); check(eraseSinglyValue(f, 2));
        check(eraseSingly(f, 1) == 3); check(eraseSingly(f, 0) == 1);
        throws([&]{ eraseSingly(f, 0); }); check(!eraseSinglyValue(f, 9)); freeList(f);
        DoublyList l{};
        std::vector<int> model;
        std::mt19937 random(2026);
        for (int i = 0; i < 1000; ++i) {
            if (model.empty() || random()%2) {
                int pos = random()%(model.size()+1); int value = random()%100;
                insertDoubly(l, pos, value); model.insert(model.begin()+pos, value);
            } else {
                int pos = random()%model.size();
                check(eraseDoubly(l, pos) == model[pos]); model.erase(model.begin()+pos);
            }
            std::vector<int> forward, backward;
            DoubleNode* previous = nullptr;
            for (auto p = l.head; p; p = p->next) { check(p->prev == previous); forward.push_back(p->data); previous = p; }
            check(previous == l.tail);
            for (auto p = l.tail; p; p = p->prev) backward.push_back(p->data);
            check(forward == model); std::reverse(backward.begin(), backward.end()); check(backward == model);
        }
        freeDoublyList(l); check(!l.head && !l.tail);
        StackNode* s = nullptr; Queue q{};
        throws([&]{ pop(s); }); throws([&]{ popQueue(q); });
        for (int v : {-1, 0, INT_MAX}) { push(s, v); pushQueue(q, v); }
        for (int v : {INT_MAX, 0, -1}) check(pop(s) == v);
        for (int v : {-1, 0, INT_MAX}) check(popQueue(q) == v);
        check(!q.head && !q.tail); freeStack(s); freeQueue(q);
        Node* tree = nullptr; std::set<int> keys;
        for (int i = 0; i < 5000; ++i) {
            int key = static_cast<int>(random()%300)-150;
            if (random()%2) { tree = insert(tree, key); keys.insert(key); }
            else { tree = remove(tree, key); keys.erase(key); }
            std::vector<int> values;
            validate(tree, LLONG_MIN, LLONG_MAX, values);
            check(values == std::vector<int>(keys.begin(), keys.end()));
            check(searchAVL(tree, key) == (keys.count(key) != 0));
        }
        for (int key : keys) tree = remove(tree, key);
        check(!tree); freeMemory(tree);
        std::cout << "PASS: arrays, lists, stack, queue; 5000 AVL mutations with BST, height and balance invariants\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
