#ifndef NAVNODE_H
#define NAVNODE_H

#include "bptree.h"

#define NAV_DIR_SIZE 100
#define LOC_SIZE     50

// Each navigation step lives in a leaf of the trip node's navRoot B+ tree. The key is stepNo (int), so steps are always stored in order.
typedef struct NavNode {
    int  stepNo;
    char direction[NAV_DIR_SIZE];
    float distance;
} NavNode;

// core logic (called internally or from fileio)
void addDirectionCore(BPTree* navTree, char dir[], float dist);
void insertDirectionCore(BPTree* navTree, int pos, char dir[], float dist);
void deleteDirectionCore(BPTree* navTree, int stepNo);
void updateDirectionCore(BPTree* navTree, int stepNo, char newDir[], float newDist);
int  searchDirection(BPTree* navTree, char dir[]);

// UI wrappers (called from menu)
void addDirectionUI(BPTree* navTree);
void insertDirectionUI(BPTree* navTree);
void deleteDirectionUI(BPTree* navTree);
void updateDirectionUI(BPTree* navTree);

void inputNavigationSteps(BPTree* navTree, char from[], char to[]);

// display & cleanup
void displayNavigation(BPTree* navTree);
void freeNavTree(BPTree* navTree);

// internal helpers
NavNode* createNavNode(int stepNo, char direction[], float distance);
int      getMaxStep(BPTree* navTree);
void     shiftSteps(BPTree* navTree, int pos);

#endif
