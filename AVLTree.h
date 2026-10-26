#ifndef AVL_TREE_H
#define AVL_TREE_H

struct Node {
    int key;
    int height;
    Node* left;
    Node* right;

    Node(int k) : key(k), height(1), left(nullptr), right(nullptr) {}
};

int getHeight(Node* n);
int getBalance(Node* n);
void updateHeight(Node* n);

Node* rotateRight(Node* p);
Node* rotateLeft(Node* q);
Node* balance(Node* p);

Node* insert(Node* p, int k);
Node* findMin(Node* p);
Node* remove(Node* p, int k);

void printInOrder(Node* root);
void freeMemory(Node* root);

bool searchAVL(const Node* root, int key);
#endif