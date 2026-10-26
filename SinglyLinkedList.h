#ifndef SINGLY_LINKED_LIST_H
#define SINGLY_LINKED_LIST_H

struct SinglyNode {
    int data;
    SinglyNode* next;
};

void pushFront(SinglyNode*& head, int value);
void pushBack(SinglyNode*& head, int value);
void popFront(SinglyNode*& head);
void printList(SinglyNode* head);
void freeList(SinglyNode*& head);

int singlyLength(const SinglyNode* head);
int getSingly(const SinglyNode* head, int index);
int findSingly(const SinglyNode* head, int value);
void insertSingly(SinglyNode*& head, int index, int value);
int eraseSingly(SinglyNode*& head, int index);
bool eraseSinglyValue(SinglyNode*& head, int value);
#endif