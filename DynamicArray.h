#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

struct DynamicArray {
    int* data;
    int size;
    int capacity;
};

void initArray(DynamicArray& arr, int initialCapacity = 4);
void pushBack(DynamicArray& arr, int value);
void popBack(DynamicArray& arr);
void printArray(const DynamicArray& arr);
void freeArray(DynamicArray& arr);

void insertAt(DynamicArray& arr, int index, int value);
int getAt(const DynamicArray& arr, int index);
int eraseAt(DynamicArray& arr, int index);
void setAt(DynamicArray& arr, int index, int value);
int arrayLength(const DynamicArray& arr);
#endif