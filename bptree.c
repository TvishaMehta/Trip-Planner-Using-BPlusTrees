#include "bptree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Comparators

// Compare two integers — used as the nav-step key comparator
int compareInt(void* a, void* b) { return (*(int*)a - *(int*)b); }

// Compare two datetime strings — used as the trip-activity key comparator
int compareDateTime(void* a, void* b) { return strcmp((char*)a, (char*)b); }

// Node and tree creation

// Allocate a fresh node and zero everything out
BPTreeNode* createNode(int leaf) {
  BPTreeNode* node = (BPTreeNode*)malloc(sizeof(BPTreeNode));
  if (!node) {
    printf("Out of memory!\n");
    exit(1);
  }

  node->n = 0;
  node->leaf = leaf;
  node->next = NULL;

  for (int i = 0; i < M - 1; i++) {
    node->keys[i] = NULL;
    node->data[i] = NULL;
  }
  for (int i = 0; i < M; i++) {
    node->child[i] = NULL;
  }
  return node;
}

// Allocate a new B+ tree and attach a single empty leaf as the root
BPTree* createTree(int (*compare)(void*, void*)) {
  BPTree* tree = (BPTree*)malloc(sizeof(BPTree));
  if (!tree) {
    printf("Out of memory!\n");
    exit(1);
  }
  tree->root = createNode(1); // start life as one empty leaf
  tree->compare = compare;
  return tree;
}

// Traversal helpers

// Walk down the leftmost spine until we hit a leaf
BPTreeNode* getLeftmost(BPTreeNode* root) {
  if (!root) return NULL;
  while (!root->leaf) root = root->child[0];
  return root;
}

// Walk down the rightmost spine until we hit a leaf
BPTreeNode* getRightmost(BPTreeNode* root) {
  if (!root) return NULL;
  while (!root->leaf) root = root->child[root->n];
  return root;
}

// Print every key in every leaf (handy for debugging)
void traverseLeaves(BPTreeNode* root) {
  BPTreeNode* leaf = getLeftmost(root);
  while (leaf) {
    for (int i = 0; i < leaf->n; i++)
      printf("[leaf key %d] ", *((int*)leaf->keys[i]));
    printf("\n");
    leaf = leaf->next;
  }
}

// Search

// Find the leaf node where a given key should live
BPTreeNode* findLeaf(BPTreeNode* root, void* key, int (*compare)(void*, void*)) {
    // why function pointer? -> this is because we may compare int in case of nav step numbers or strings in case of date and time
  if (!root) return NULL;
  BPTreeNode* curr = root;
  while (!curr->leaf) {
    int i = 0;
    // move right as long as key >= curr->keys[i]
    while (i < curr->n && compare(key, curr->keys[i]) >= 0) i++;
    curr = curr->child[i];
  }
  return curr;
}

// Return the data pointer stored alongside a key, or NULL if not found
void* search(BPTree* tree, void* key) {
  if (!tree || !tree->root) return NULL;
  BPTreeNode* leaf = findLeaf(tree->root, key, tree->compare);
  if (!leaf) return NULL;
  for (int i = 0; i < leaf->n; i++) {
    if (tree->compare(leaf->keys[i], key) == 0) return leaf->data[i];
  }
  return NULL;
}

// Insertion

// Figure out which slot a new key should go into (for sorted insertion)
int findInsertIndex(BPTreeNode* node, void* key, int (*compare)(void*, void*)) {
  int i = node->n - 1;
  while (i >= 0 && compare(key, node->keys[i]) < 0) i--;
  return i + 1;
}

// Split the i-th child of parent when it is already full
void splitChild(BPTreeNode* parent, int index) {
  BPTreeNode* child = parent->child[index];
  BPTreeNode* sibling = createNode(child->leaf);

  int mid = (M - 1) / 2; // index of the median key

  sibling->n = child->n - mid - 1;

  // Copy the right half of child into sibling
  for (int i = 0; i < sibling->n; i++) {
    sibling->keys[i] = child->keys[mid + 1 + i];
    sibling->data[i] = child->data[mid + 1 + i];
  }

  if (!child->leaf) {
    // Internal node: bring along child pointers too
    for (int i = 0; i <= sibling->n; i++)
      sibling->child[i] = child->child[mid + 1 + i];
  }

  // In a B+ tree the median key is copied up (not moved) for leaf splits
  void* pushUpKey = child->keys[mid];

  child->n = mid;

  // Link the new sibling into the leaf chain
  if (child->leaf) {
    sibling->next = child->next;
    child->next = sibling;
    // For leaves the median key stays in the right child, so keep a copy
    // sibling->keys[sibling->n] = pushUpKey; /* already there, no adjustment
    // needed

    // shift right
    for (int i = sibling->n; i > 0; i--) {
      sibling->keys[i] = sibling->keys[i - 1];
      sibling->data[i] = sibling->data[i - 1];
    }

    // insert median at front
    sibling->keys[0] = pushUpKey;
    sibling->data[0] = child->data[mid];  // important!

    sibling->n++;
  }

  // Slide parent's existing keys and children one spot to the right
  for (int i = parent->n; i > index; i--) {
    parent->keys[i] = parent->keys[i - 1];
    parent->data[i] = parent->data[i - 1];
    parent->child[i + 1] = parent->child[i];
  }

  parent->keys[index] = pushUpKey;
  parent->child[index + 1] = sibling;
  parent->n++;
}

// Insert into a node that is guaranteed to have room
void insertNonFull(BPTreeNode* node, void* key, void* data,
                   int (*compare)(void*, void*)) {
  int i = node->n - 1;

  if (node->leaf) {
    // Shift existing entries right to make a gap for the new one
    while (i >= 0 && compare(key, node->keys[i]) < 0) {
      node->keys[i + 1] = node->keys[i];
      node->data[i + 1] = node->data[i];
      i--;
    }
    node->keys[i + 1] = key;
    node->data[i + 1] = data;
    node->n++;
  } else {
    // Find the right child to descend into
    while (i >= 0 && compare(key, node->keys[i]) < 0) i--;
    i++;

    // If that child is full, split it before going in
    if (node->child[i]->n == M - 1) {
      splitChild(node, i);
      if (compare(key, node->keys[i]) > 0) i++;
    }
    insertNonFull(node->child[i], key, data, compare);
  }
}

// Public insert: handles the root-full case by growing the tree upward
void insert(BPTree* tree, void* key, void* data) {
  if (!tree) return;
  BPTreeNode* root = tree->root;

  if (root->n == M - 1) {
    // Root is full — create a new root above it
    BPTreeNode* newRoot = createNode(0);
    newRoot->child[0] = root;
    splitChild(newRoot, 0);
    tree->root = newRoot;
    insertNonFull(newRoot, key, data, tree->compare);
  } else {
    insertNonFull(root, key, data, tree->compare);
  }
}

// memory cleanup

// Recursively free a subtree (keys and data at leaves)
void freeBPTreeNode(BPTreeNode* node) {
  if (!node) return;

  if (!node->leaf) {
    // Recurse into children first
    for (int i = 0; i <= node->n; i++) freeBPTreeNode(node->child[i]);
  }

  // Free the key copies
  for (int i = 0; i < node->n; i++) free(node->keys[i]);

  // Data (NavNode* / TripNode*) is freed by the caller, but if it's a leaf the keys ARE owned by this node

  free(node);
}

// Free the whole tree wrapper along with every node
void freeBPTree(BPTree* tree) {
  if (!tree) return;
  freeBPTreeNode(tree->root);
  free(tree);
}



// Delete implementation

int minKeys() {
    return (M - 1) / 2;   // for M=4 to 1
}

// Find parent
BPTreeNode* findParent(BPTreeNode* root, BPTreeNode* child, int* idx) {
    if (!root || root->leaf) return NULL;

    for (int i = 0; i <= root->n; i++) {
        if (root->child[i] == child) {
            *idx = i;
            return root;
        }
        BPTreeNode* res = findParent(root->child[i], child, idx);
        if (res) return res;
    }
    return NULL;
}

// Remove from leaf
void removeFromLeaf(BPTreeNode* leaf, int pos) {
    for (int i = pos; i < leaf->n - 1; i++) {
        leaf->keys[i] = leaf->keys[i + 1];
        leaf->data[i] = leaf->data[i + 1];
    }
    leaf->n--;
}

// Borrow from left (leaf)
void borrowFromLeftLeaf(BPTreeNode* leaf, BPTreeNode* left,
                        BPTreeNode* parent, int idx) {

    for (int i = leaf->n; i > 0; i--) {
        leaf->keys[i] = leaf->keys[i - 1];
        leaf->data[i] = leaf->data[i - 1];
    }

    leaf->keys[0] = left->keys[left->n - 1];
    leaf->data[0] = left->data[left->n - 1];

    leaf->n++;
    left->n--;

    parent->keys[idx - 1] = leaf->keys[0];
}

// Borrow from right (leaf)
void borrowFromRightLeaf(BPTreeNode* leaf, BPTreeNode* right,
                         BPTreeNode* parent, int idx) {

    leaf->keys[leaf->n] = right->keys[0];
    leaf->data[leaf->n] = right->data[0];
    leaf->n++;

    for (int i = 0; i < right->n - 1; i++) {
        right->keys[i] = right->keys[i + 1];
        right->data[i] = right->data[i + 1];
    }

    right->n--;

    parent->keys[idx] = right->keys[0];
}

// Merge leaf
void mergeLeaf(BPTreeNode* left, BPTreeNode* right,
               BPTreeNode* parent, int idx) {

    for (int i = 0; i < right->n; i++) {
        left->keys[left->n + i] = right->keys[i];
        left->data[left->n + i] = right->data[i];
    }

    left->n += right->n;
    left->next = right->next;

    free(right);

    for (int i = idx; i < parent->n - 1; i++) {
        parent->keys[i] = parent->keys[i + 1];
        parent->child[i + 1] = parent->child[i + 2];
    }
    parent->n--;
}

// Forward
void fixInternal(BPTree* tree, BPTreeNode* node);

// Handle leaf underflow
void handleLeafUnderflow(BPTree* tree, BPTreeNode* leaf) {
    int idx;
    BPTreeNode* parent = findParent(tree->root, leaf, &idx);
    
    if (!parent) return;

    BPTreeNode* left = (idx > 0) ? parent->child[idx - 1] : NULL;
    BPTreeNode* right = (idx < parent->n) ? parent->child[idx + 1] : NULL;

    int min = minKeys();

    if (left && left->n > min) {
        borrowFromLeftLeaf(leaf, left, parent, idx);
        return;
    }

    if (right && right->n > min) {
        borrowFromRightLeaf(leaf, right, parent, idx);
        return;
    }

    if (left) {
        mergeLeaf(left, leaf, parent, idx - 1);
        fixInternal(tree, parent);
    } else {
        mergeLeaf(leaf, right, parent, idx);
        fixInternal(tree, parent);
    }
}

// Fix internal
void fixInternal(BPTree* tree, BPTreeNode* node) {
    if (node == tree->root) {
        if (node->n == 0) {
            tree->root = node->child[0];
            free(node);
        }
        return;
    }

    if (node->n >= minKeys()) return;

    int idx;
    BPTreeNode* parent = findParent(tree->root, node, &idx);

    BPTreeNode* left = (idx > 0) ? parent->child[idx - 1] : NULL;
    BPTreeNode* right = (idx < parent->n) ? parent->child[idx + 1] : NULL;

    int min = minKeys();

    // Borrow from left
    if (left && left->n > min) {
        for (int i = node->n; i > 0; i--) {
            node->keys[i] = node->keys[i - 1];
            node->child[i + 1] = node->child[i];
        }

        node->child[1] = node->child[0];
        node->keys[0] = parent->keys[idx - 1];

        node->child[0] = left->child[left->n];
        parent->keys[idx - 1] = left->keys[left->n - 1];

        left->n--;
        node->n++;
        return;
    }

    // Borrow from right
    if (right && right->n > min) {
        node->keys[node->n] = parent->keys[idx];
        node->child[node->n + 1] = right->child[0];

        parent->keys[idx] = right->keys[0];

        for (int i = 0; i < right->n - 1; i++) {
            right->keys[i] = right->keys[i + 1];
            right->child[i] = right->child[i + 1];
        }
        right->child[right->n - 1] = right->child[right->n];
        right->n--;

        node->n++;
        return;
    }

    // Merge
    if (left) {
        left->keys[left->n] = parent->keys[idx - 1];

        for (int i = 0; i < node->n; i++) {
            left->keys[left->n + 1 + i] = node->keys[i];
            left->child[left->n + 1 + i] = node->child[i];
        }
        left->child[left->n + node->n + 1] = node->child[node->n];

        left->n += node->n + 1;

        free(node);

        for (int i = idx - 1; i < parent->n - 1; i++) {
            parent->keys[i] = parent->keys[i + 1];
            parent->child[i + 1] = parent->child[i + 2];
        }
        parent->n--;

        fixInternal(tree, parent);
    }
}

// main delete
void delete(BPTree* tree, void* key) {
    if (!tree || !tree->root) return;

    BPTreeNode* leaf = findLeaf(tree->root, key, tree->compare);
    if (!leaf) return;

    int pos = -1;
    for (int i = 0; i < leaf->n; i++) {
        if (tree->compare(leaf->keys[i], key) == 0) {
            pos = i;
            break;
        }
    }

    if (pos == -1) return;

    removeFromLeaf(leaf, pos);

    if (leaf == tree->root) {
        if (leaf->n == 0) {
            free(leaf);
            tree->root = NULL;
        }
        return;
    }

    if (leaf->n < minKeys()) {
        handleLeafUnderflow(tree, leaf);
    }
}