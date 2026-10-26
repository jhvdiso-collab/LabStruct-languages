#include "AVLTree.h"
#include <iostream>
#include <algorithm>

using namespace std;

int getHeight(Node* n) {
    if (n == nullptr) {
        return 0;
    }
    return n->height;
}

int getBalance(Node* n) {
    if (n == nullptr) {
        return 0;
    }
    return getHeight(n->right) - getHeight(n->left);
}

void updateHeight(Node* n) {
    if (n) {
        n->height = 1 + max(getHeight(n->left), getHeight(n->right));
    }
}

Node* rotateRight(Node* p) {
    Node* q = p->left;
    p->left = q->right;
    q->right = p;

    updateHeight(p);
    updateHeight(q);

    return q;
}

Node* rotateLeft(Node* q) {
    Node* p = q->right;
    q->right = p->left;
    p->left = q;

    updateHeight(q);
    updateHeight(p);

    return p;
}

Node* balance(Node* p) {
    updateHeight(p);

    if (getBalance(p) == 2) {
        if (getBalance(p->right) < 0) {
            p->right = rotateRight(p->right);
        }
        return rotateLeft(p);
    }

    if (getBalance(p) == -2) {
        if (getBalance(p->left) > 0) {
            p->left = rotateLeft(p->left);
        }
        return rotateRight(p);
    }
    return p;
}

Node* insert(Node* p, int k) {
    if (!p) {
        return new Node(k);
    }

    if (k < p->key) {
        p->left = insert(p->left, k);
    } else if (k > p->key) {
        p->right = insert(p->right, k);
    } else {
        return p;
    }
    return balance(p);
}

Node* findMin(Node* p) {
    while (p->left != nullptr) {
        p = p->left;
    }
    return p;
}

Node* remove(Node* p, int k) {
    if (!p) {
        return nullptr;
    }

    if (k < p->key) {
        p->left = remove(p->left, k);
    } else if (k > p->key) {
        p->right = remove(p->right, k);
    } else {
        if (!p->left || !p->right) {
            Node* temp = p->left;
            if (temp == nullptr) {
                temp = p->right;
            }
            delete p;
            return temp;
        }
        Node* min = findMin(p->right);
        p->key = min->key;
        p->right = remove(p->right, p->key);
    }
    return balance(p);
}

void printInOrder(Node* root) {
    if (root == nullptr) {
        return;
    }
    printInOrder(root->left);
    cout << root->key << " ";
    printInOrder(root->right);
}

void freeMemory(Node* root) {
    if (!root) {
        return;
    }
    freeMemory(root->left);
    freeMemory(root->right);
    delete root;
}
bool searchAVL(const Node* root, int key) {
    while (root) {
        if (root->key == key) {
            return true;
        }
        if (key < root->key) {
            root = root->left;
        } else {
            root = root->right;
        }
    }
    return false;
}
