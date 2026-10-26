#ifndef BST_H
#define BST_H

struct BSTNode {
    int key;
    BSTNode* left;
    BSTNode* right;

    BSTNode(int k) : key(k), left(nullptr), right(nullptr) {}
};

BSTNode* insertBST(BSTNode* p, int k);
bool searchBST(BSTNode* p, int k);
void printBSTInOrder(BSTNode* root);
void freeBST(BSTNode* root);

#endif