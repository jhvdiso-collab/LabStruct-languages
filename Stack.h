#ifndef STACK_H
#define STACK_H

struct StackNode {
    int data;
    StackNode* next;
};

void push(StackNode*& top, int value);
int pop(StackNode*& top);
bool isEmpty(StackNode* top);
void freeStack(StackNode*& top);

#endif