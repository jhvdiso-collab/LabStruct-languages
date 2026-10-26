#include "DynamicArray.h"
#include <iostream>
#include <stdexcept>
#include <climits>
#include <algorithm>

using namespace std;

void initArray(DynamicArray& arr, int initialCapacity) {
    if (initialCapacity < 0) {
        throw invalid_argument("Negative capacity");
    }
    initialCapacity = max(1, initialCapacity);
    arr.data = new int[initialCapacity];
    arr.size = 0;
    arr.capacity = initialCapacity;
}

void pushBack(DynamicArray& arr, int value) {
    if (arr.size == arr.capacity) {
        if (arr.capacity > INT_MAX / 2) {
            throw length_error("Array too large");
        }
        int newCapacity = max(1, arr.capacity * 2);
        int* newData = new int[newCapacity];
        for (int i = 0; i < arr.size; i++) {
            newData[i] = arr.data[i];
        }
        delete[] arr.data;
        arr.data = newData;
        arr.capacity = newCapacity;
    }
    arr.data[arr.size++] = value;
}

void popBack(DynamicArray& arr) {
    if (arr.size > 0) {
        arr.size--;
    }
}

void printArray(const DynamicArray& arr) {
    cout << "[ ";
    for (int i = 0; i < arr.size; i++) {
        cout << arr.data[i] << " ";
    }
    cout << "] (size: " << arr.size << ", capacity: " << arr.capacity << ")\n";
}

void freeArray(DynamicArray& arr) {
    delete[] arr.data;
    arr.data = nullptr;
    arr.size = 0;
    arr.capacity = 0;
}
void insertAt(DynamicArray& arr, int index, int value) {
    if (index < 0 || index > arr.size) {
        throw std::out_of_range("Array index");
    }
    pushBack(arr, value);
    for (int i = arr.size - 1; i > index; --i) {
        arr.data[i] = arr.data[i - 1];
    }
    arr.data[index] = value;
}
int getAt(const DynamicArray& arr, int index) {
    if (index < 0 || index >= arr.size) {
        throw std::out_of_range("Array index");
    }
    return arr.data[index];
}
int eraseAt(DynamicArray& arr, int index) {
    int value = getAt(arr, index);
    for (int i = index; i + 1 < arr.size; ++i) {
        arr.data[i] = arr.data[i + 1];
    }
    --arr.size;
    return value;
}
void setAt(DynamicArray& arr, int index, int value) {
    getAt(arr, index);
    arr.data[index] = value;
}
int arrayLength(const DynamicArray& arr) {
    return arr.size;
}
