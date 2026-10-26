#include "SinglyLinkedList.h"
#include <iostream>
#include <stdexcept>

using namespace std;

void pushFront(SinglyNode*& head, int value) {
    SinglyNode* newNode = new SinglyNode{value, head};
    head = newNode;
}

void pushBack(SinglyNode*& head, int value) {
    SinglyNode* newNode = new SinglyNode{value, nullptr};
    if (head == nullptr) {
        head = newNode;
        return;
    }
    SinglyNode* curr = head;
    while (curr->next != nullptr) {
        curr = curr->next;
    }
    curr->next = newNode;
}

void popFront(SinglyNode*& head) {
    if (head == nullptr) {
        return;
    }
    SinglyNode* temp = head;
    head = head->next;
    delete temp;
}

void printList(SinglyNode* head) {
    SinglyNode* curr = head;
    while (curr != nullptr) {
        cout << curr->data << " -> ";
        curr = curr->next;
    }
    cout << "nullptr\n";
}

void freeList(SinglyNode*& head) {
    while (head != nullptr) {
        popFront(head);
    }
}
int singlyLength(const SinglyNode* head) {
    int length = 0;
    while (head != nullptr) {
        length++;
        head = head->next;
    }
    return length;
}

int getSingly(const SinglyNode* head, int index) {
    if (index < 0) {
        throw out_of_range("List index");
    }
    for (int i = 0; i < index && head != nullptr; i++) {
        head = head->next;
    }
    if (head == nullptr) {
        throw out_of_range("List index");
    }
    return head->data;
}

int findSingly(const SinglyNode* head, int value) {
    int index = 0;
    while (head != nullptr) {
        if (head->data == value) {
            return index;
        }
        head = head->next;
        index++;
    }
    return -1;
}

void insertSingly(SinglyNode*& head, int index, int value) {
    if (index < 0) {
        throw out_of_range("List index");
    }
    if (index == 0) {
        pushFront(head, value);
        return;
    }
    SinglyNode* previous = head;
    for (int i = 0; i < index - 1 && previous != nullptr; i++) {
        previous = previous->next;
    }
    if (previous == nullptr) {
        throw out_of_range("List index");
    }
    previous->next = new SinglyNode{value, previous->next};
}

int eraseSingly(SinglyNode*& head, int index) {
    if (index < 0 || head == nullptr) {
        throw out_of_range("List index");
    }
    if (index == 0) {
        int value = head->data;
        popFront(head);
        return value;
    }
    SinglyNode* previous = head;
    for (int i = 0; i < index - 1 && previous != nullptr; i++) {
        previous = previous->next;
    }
    if (previous == nullptr || previous->next == nullptr) {
        throw out_of_range("List index");
    }
    SinglyNode* node = previous->next;
    int value = node->data;
    previous->next = node->next;
    delete node;
    return value;
}

bool eraseSinglyValue(SinglyNode*& head, int value) {
    int index = findSingly(head, value);
    if (index < 0) {
        return false;
    }
    eraseSingly(head, index);
    return true;
}
