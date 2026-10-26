#include "Stack.h"
#include <stdexcept>

using namespace std;

void push(StackNode*& top, int value) {
    StackNode* newNode = new StackNode{value, top};
    top = newNode;
}

int pop(StackNode*& top) {
    if (isEmpty(top)) {
        throw out_of_range("Empty stack");
    }
    StackNode* temp = top;
    int value = temp->data;
    top = top->next;
    delete temp;
    return value;
}

bool isEmpty(StackNode* top) {
    return top == nullptr;
}

void freeStack(StackNode*& top) {
    while (!isEmpty(top)) {
        pop(top);
    }
}
