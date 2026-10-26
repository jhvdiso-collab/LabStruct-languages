#include "DoublyLinkedList.h"
#include <iostream>
#include <stdexcept>

using namespace std;

void initDoublyList(DoublyList& list) {
    list.head = nullptr;
    list.tail = nullptr;
}

void pushFrontDoubly(DoublyList& list, int value) {
    DoubleNode* newNode = new DoubleNode{value, nullptr, list.head};
    if (list.head != nullptr) {
        list.head->prev = newNode;
    } else {
        list.tail = newNode;
    }
    list.head = newNode;
}

void pushBackDoubly(DoublyList& list, int value) {
    DoubleNode* newNode = new DoubleNode{value, list.tail, nullptr};
    if (list.tail != nullptr) {
        list.tail->next = newNode;
    } else {
        list.head = newNode;
    }
    list.tail = newNode;
}

void popFrontDoubly(DoublyList& list) {
    if (list.head == nullptr) {
        return;
    }
    DoubleNode* temp = list.head;
    list.head = list.head->next;
    if (list.head != nullptr) {
        list.head->prev = nullptr;
    } else {
        list.tail = nullptr;
    }
    delete temp;
}

void printDoublyForward(const DoublyList& list) {
    DoubleNode* curr = list.head;
    cout << "nullptr <-> ";
    while (curr != nullptr) {
        cout << curr->data << " <-> ";
        curr = curr->next;
    }
    cout << "nullptr\n";
}

void printDoublyBackward(const DoublyList& list) {
    DoubleNode* curr = list.tail;
    cout << "nullptr <-> ";
    while (curr != nullptr) {
        cout << curr->data << " <-> ";
        curr = curr->prev;
    }
    cout << "nullptr\n";
}

void freeDoublyList(DoublyList& list) {
    while (list.head != nullptr) {
        popFrontDoubly(list);
    }
}
namespace {
DoubleNode* doublyAt(const DoublyList& list, int index) {
    if (index < 0) {
        throw std::out_of_range("List index");
    }
    DoubleNode* node = list.head;
    for (int i = 0; i < index && node != nullptr; i++) {
        node = node->next;
    }
    if (!node) {
        throw std::out_of_range("List index");
    }
    return node;
}
int unlinkDoubly(DoublyList& list, DoubleNode* node) {
    int value = node->data;
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        list.head = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        list.tail = node->prev;
    }
    delete node;
    return value;
}
} // namespace
int doublyLength(const DoublyList& list) {
    int n = 0;
    for (DoubleNode* p = list.head; p; p = p->next) {
        ++n;
    }
    return n;
}
int getDoubly(const DoublyList& list, int index) {
    return doublyAt(list, index)->data;
}
int findDoubly(const DoublyList& list, int value) {
    int i = 0;
    for (DoubleNode* p = list.head; p; p = p->next, ++i) {
        if (p->data == value) {
            return i;
        }
    }
    return -1;
}
void insertDoubly(DoublyList& list, int index, int value) {
    if (index < 0) {
        throw out_of_range("List index");
    }
    if (index == 0) {
        pushFrontDoubly(list, value);
        return;
    }
    DoubleNode* prev = doublyAt(list, index - 1);
    DoubleNode* node = new DoubleNode{value, prev, prev->next};
    if (prev->next) {
        prev->next->prev = node;
    } else {
        list.tail = node;
    }
    prev->next = node;
}
int eraseDoubly(DoublyList& list, int index) {
    return unlinkDoubly(list, doublyAt(list, index));
}
bool eraseDoublyValue(DoublyList& list, int value) {
    for (DoubleNode* p = list.head; p; p = p->next) {
        if (p->data == value) {
            unlinkDoubly(list, p);
            return true;
        }
    }
    return false;
}
int popBackDoubly(DoublyList& list) {
    if (!list.tail) {
        throw std::out_of_range("Empty list");
    }
    return unlinkDoubly(list, list.tail);
}
