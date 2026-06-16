#ifndef BPTREE_H
#define BPTREE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Order of the B+ tree — each node holds at most M-1 keys
#define M 4

typedef struct BPTreeNode {
  int n;             // how many keys are currently stored
  void* keys[M - 1]; // the actual key values (generic pointers)
  struct BPTreeNode*
      child[M];            // child pointers (only used in internal nodes)
  struct BPTreeNode* next; // chain linking leaf nodes left-to-right
  int leaf;                // 1 if this is a leaf, 0 if it's an internal node
  void* data[M - 1];       // data — only meaningful at leaf nodes
} BPTreeNode;

typedef struct BPTree {
  BPTreeNode* root;
  int (*compare)(void*, void*); // pluggable comparator for generic keys
} BPTree;

// tree/node construction
BPTree* createTree(int (*compare)(void*, void*));
BPTreeNode* createNode(int leaf);

// built-in comparators
int compareInt(void* a, void* b);
int compareDateTime(void* a, void* b);

// core operations
void* search(BPTree* tree, void* key);
void insert(BPTree* tree, void* key, void* data);
void delete (BPTree* tree, void* key);

// internal helpers
BPTreeNode* findLeaf(BPTreeNode* root, void* key, int (*compare)(void*, void*));
void insertNonFull(BPTreeNode* node, void* key, void* data,
                   int (*compare)(void*, void*));
void splitChild(BPTreeNode* parent, int index);
int findInsertIndex(BPTreeNode* node, void* key, int (*compare)(void*, void*));
void traverseLeaves(BPTreeNode* root);

// traversal helpers
BPTreeNode* getLeftmost(BPTreeNode* root);
BPTreeNode* getRightmost(BPTreeNode* root);

// memory cleanup
void freeBPTreeNode(BPTreeNode* node);
void freeBPTree(BPTree* tree);

#endif
