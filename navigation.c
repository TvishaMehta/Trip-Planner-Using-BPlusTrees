#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "navigation.h"
#include "util.h"

typedef struct TripNode TripNode;
//Node creation

NavNode* createNavNode(int stepNo, char direction[], float distance) {
    NavNode* newNode = (NavNode*)malloc(sizeof(NavNode));
    if (!newNode) {
        printf("Memory allocation failed!\n");
        return NULL;
    }
    newNode->stepNo   = stepNo;
    newNode->distance = distance;
    normalize_str(direction);
    strcpy(newNode->direction, direction);
    return newNode;
}

// Internal helpers

// Find the highest step number currently in the tree. Returns 0 if the tree is empty so the first step gets number 1.
int getMaxStep(BPTree* navTree) {
    if (!navTree || !navTree->root) return 0;
    BPTreeNode* rightmost = getRightmost(navTree->root);
    if (!rightmost || rightmost->n == 0) return 0;
    int* key = (int*)rightmost->keys[rightmost->n - 1];
    return *key;
}

// Bump every step number >= pos up by one. This makes room before we insert at position pos.
void shiftSteps(BPTree* navTree, int pos) {
    if (!navTree || !navTree->root) return;
    BPTreeNode* leaf = getLeftmost(navTree->root);
    while (leaf) {
        for (int i = 0; i < leaf->n; i++) {
            int* key = (int*)leaf->keys[i];
            if (*key >= pos) {
                (*key)++;
                NavNode* nav = (NavNode*)leaf->data[i];
                nav->stepNo  = *key;
            }
        }
        leaf = leaf->next;
    }
}

// Core logic

// Append a new step at the end (step number = max + 1)
void addDirectionCore(BPTree* navTree, char direction[], float distance) {
    if (!navTree) return;
    int stepNo = getMaxStep(navTree) + 1;

    NavNode* newNode = createNavNode(stepNo, direction, distance);
    if (!newNode) return;

    int* key = (int*)malloc(sizeof(int));
    *key     = stepNo;
    insert(navTree, key, newNode);
}

// Insert a step at a specific position, shifting later steps right
void insertDirectionCore(BPTree* navTree, int pos,
                         char direction[], float distance) {
    if (!navTree) return;
    if (pos <= 0) {
        printf("Position must be >= 1.\n");
        return;
    }

    shiftSteps(navTree, pos);

    NavNode* newNode = createNavNode(pos, direction, distance);
    if (!newNode) return;

    int* key = (int*)malloc(sizeof(int));
    *key     = pos;
    insert(navTree, key, newNode);
}

// Remove a step by step number
void deleteDirectionCore(BPTree* navTree, int stepNo) {
    if (!navTree || !navTree->root) return;

    NavNode* nav = (NavNode*)search(navTree, &stepNo);
    if (!nav) {
        printf("Step %d not found.\n", stepNo);
        return;
    }

    delete(navTree, &stepNo);
    free(nav);
    printf("Step %d deleted.\n", stepNo);
}

// Scan all leaves for a direction string — returns 1 if found
int searchDirection(BPTree* navTree, char dir[]) {
    if (!navTree || !navTree->root) return 0;
    BPTreeNode* leaf = getLeftmost(navTree->root);
    while (leaf) {
        for (int i = 0; i < leaf->n; i++) {
            NavNode* nav = (NavNode*)leaf->data[i];
            if (strcmp(nav->direction, dir) == 0)
                return 1;
        }
        leaf = leaf->next;
    }
    return 0;
}

// Update direction text and distance for a given step number
void updateDirectionCore(BPTree* navTree, int stepNo,
                         char newDir[], float newDist) {
    NavNode* nav = (NavNode*)search(navTree, &stepNo);
    if (!nav) {
        printf("Step %d not found.\n", stepNo);
        return;
    }
    strcpy(nav->direction, newDir);
    nav->distance = newDist;
    printf("Step %d updated.\n", stepNo);
}

// UI wrappers

void addDirectionUI(BPTree* navTree) {
    char  dir[NAV_DIR_SIZE];
    float dist;

    printf("Direction: ");
    scanf(" %[^\n]", dir);
    normalize_str(dir);

    printf("Distance (km): ");
    scanf("%f", &dist);
    while (dist <= 0) {
        printf("Distance must be positive. Try again: ");
        scanf("%f", &dist);
    }

    addDirectionCore(navTree, dir, dist);
    printf("Direction added.\n");
}

void insertDirectionUI(BPTree* navTree) {
    int   pos;
    char  dir[NAV_DIR_SIZE];
    float dist;

    printf("Current navigation:\n");
    displayNavigation(navTree);

    printf("Insert at position: ");
    scanf("%d", &pos);
    while (pos <= 0) {
        printf("Position must be >= 1. Try again: ");
        scanf("%d", &pos);
    }

    printf("Direction: ");
    scanf(" %[^\n]", dir);
    normalize_str(dir);

    printf("Distance (km): ");
    scanf("%f", &dist);
    while (dist <= 0) {
        printf("Distance must be positive. Try again: ");
        scanf("%f", &dist);
    }

    insertDirectionCore(navTree, pos, dir, dist);
    printf("Direction inserted.\n");
}

void deleteDirectionUI(BPTree* navTree) {
    printf("Current navigation:\n");
    displayNavigation(navTree);

    int stepNo;
    printf("Step number to delete: ");
    scanf("%d", &stepNo);
    deleteDirectionCore(navTree, stepNo);
}

void updateDirectionUI(BPTree* navTree) {
    int   stepNo;
    char  newDir[NAV_DIR_SIZE];
    float dist;

    printf("Current navigation:\n");
    displayNavigation(navTree);

    printf("Step number to update: ");
    scanf("%d", &stepNo);

    printf("New direction: ");
    scanf(" %[^\n]", newDir);
    normalize_str(newDir);

    printf("New distance (km): ");
    scanf("%f", &dist);
    while (dist <= 0) {
        printf("Distance must be positive. Try again: ");
        scanf("%f", &dist);
    }

    updateDirectionCore(navTree, stepNo, newDir, dist);
}

// Ask for a from/to pair then collect individual steps


void inputNavigationSteps(BPTree* navTree, char from[], char to[]) {
    int   n;
    char  dir[NAV_DIR_SIZE];
    float dist;

    printf("\nEnter navigation steps for %s -> %s\n", from, to);
    printf("How many steps? ");
    scanf("%d", &n);
    while (n <= 0) {
        printf("Must be at least 1. Try again: ");
        scanf("%d", &n);
    }

    for (int i = 0; i < n; i++) {
        printf("Step %d direction: ", i + 1);
        scanf(" %[^\n]", dir);
        normalize_str(dir);

        printf("Step %d distance (km): ", i + 1);
        scanf("%f", &dist);
        while (dist < 0) {
            printf("Cannot be negative. Try again: ");
            scanf("%f", &dist);
        }

        addDirectionCore(navTree, dir, dist);
    }
}

// Display and cleanup

// Walk the leaf chain and print every step in order
void displayNavigation(BPTree* navTree) {
    if (!navTree || !navTree->root) {
        printf("No navigation steps yet.\n");
        return;
    }

    BPTreeNode* leaf = getLeftmost(navTree->root);
    int printed = 0;

    while (leaf) {
        for (int i = 0; i < leaf->n; i++) {
            NavNode* nav = (NavNode*)leaf->data[i];
            printf("  Step %d: %s (%.2f km)\n",
                   nav->stepNo, nav->direction, nav->distance);
            printed = 1;
        }
        leaf = leaf->next;
    }

    if (!printed)
        printf("No navigation steps yet.\n");
}

// Free the nav tree wrapper (NavNode data is freed elsewhere)
void freeNavTree(BPTree* navTree) {
    if (!navTree) return;
    // NavNode* data pointers are freed by freeBPTreeNode only if the caller frees them before calling this. Here we free the tree structure; caller is responsible for NavNode data.
    freeBPTree(navTree);
}
