#include "trip.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "navigation.h"
#include "tripList.h"
#include "util.h"

// Node creation

TripNode* createTripNode(char type[], char location[], char datetime[],
                         float cost, char details[]) {
  TripNode* newNode = (TripNode*)malloc(sizeof(TripNode));
  if (!newNode) {
    printf("Memory allocation failed!\n");
    return NULL;
  }

  strcpy(newNode->type, type);
  normalize_str(location);
  strcpy(newNode->location, location);
  strcpy(newNode->datetime, datetime);
  newNode->cost = cost;
  strcpy(newNode->details, details);

  // Every activity gets its own empty nav tree
  newNode->navRoot = createTree(compareInt);
  return newNode;
}

// Activity operations

// Insert a TripNode into the B+ trip tree keyed by datetime. Because datetime strings sort lexicographically in YYYY-MM-DD HH:MM format, the B+ tree automatically keeps activities in chronological order.

void addActivity(BPTree* tripRoot, TripNode* newNode) {
  if (!tripRoot || !newNode) return;

  char* key = (char*)malloc(DATETIME_SIZE);
  if (!key) return;
  strcpy(key, newNode->datetime);
  insert(tripRoot, key, newNode);
}

// Remove an activity by its datetime key and free its memory
void deleteActivity(BPTree* tripRoot, char datetime[]) {
  if (!tripRoot || !tripRoot->root) return;

  TripNode* t = (TripNode*)search(tripRoot, datetime);
  if (!t) {
    printf("Activity not found.\n");
    return;
  }

  delete(tripRoot, datetime);
  freeBPTree(t->navRoot); /* clean up the navigation sub-tree */
  free(t);
  printf("Activity deleted.\n");
}

// Search

// Returns the first leaf-order match for a location (no disambiguation)
TripNode* searchByFirstLocation(BPTree* tripRoot, char location[]) {
  normalize_str(location);
  if (!tripRoot || !tripRoot->root) return NULL;

  BPTreeNode* curr = getLeftmost(tripRoot->root);
  while (curr) {
    for (int i = 0; i < curr->n; i++) {
      TripNode* t = (TripNode*)curr->data[i];
      if (strcmp(t->location, location) == 0) return t;
    }
    curr = curr->next;
  }
  return NULL;
}

// Find an activity by location. If there are multiple matches (same place visited more than once) we list them and let the user pick which one they mean.
TripNode* searchByLocation(BPTree* tripRoot, char location[]) {
  if (!tripRoot || !tripRoot->root) return NULL;
  normalize_str(location);

  TripNode* matches[100];
  int count = 0;

  BPTreeNode* curr = getLeftmost(tripRoot->root);
  while (curr) {
    for (int i = 0; i < curr->n; i++) {
      TripNode* t = (TripNode*)curr->data[i];
      if (strcmp(t->location, location) == 0) matches[count++] = t;
    }
    curr = curr->next;
  }

  if (count == 0) return NULL;
  if (count == 1) return matches[0];

  printf("\nMultiple entries for '%s':\n", location);
  for (int i = 0; i < count; i++) {
    printf("  %d. %s (%s) — %s\n", i + 1, matches[i]->location,
           matches[i]->datetime, matches[i]->type);
  }

  int choice;
  printf("Select occurrence (1-%d): ", count);
  if (scanf("%d", &choice) != 1) {
    while (getchar() != '\n');
    return NULL;
  }
  if (choice < 1 || choice > count) {
    printf("Invalid choice.\n");
    return NULL;
  }
  return matches[choice - 1];
}

// Display

void displayTrip(BPTree* tripRoot) {
  if (!tripRoot || !tripRoot->root) {
    printf("Trip is empty.\n");
    return;
  }

  BPTreeNode* curr = getLeftmost(tripRoot->root);
  int idx = 1;
  while (curr) {
    for (int i = 0; i < curr->n; i++) {
      TripNode* t = (TripNode*)curr->data[i];
      printf("\nActivity %d \n", idx++);
      printf("Type     : %s\n", t->type);
      printf("Location : %s\n", t->location);
      printf("DateTime : %s\n", t->datetime);
      printf("Cost     : %.2f\n", t->cost);
      printf("Details  : %s\n", t->details);
      printf("Navigation:\n");
      displayNavigation(t->navRoot);
    }
    curr = curr->next;
  }
}

void showTripSummary(BPTree* tripRoot) {
  if (!tripRoot || !tripRoot->root) {
    printf("  (No activities yet)\n");
    return;
  }

  BPTreeNode* curr = getLeftmost(tripRoot->root);
  int count = 1;
  printf("\nTrip Summary\n");
  while (curr) {
    for (int i = 0; i < curr->n; i++) {
      TripNode* t = (TripNode*)curr->data[i];
      printf("  %d. [%s] %s  (%s)\n", count++, t->type, t->location,
             t->datetime);
    }
    curr = curr->next;
  }
  printf("\n");
}

// Range search: Given two date strings D1 and D2, print all activities whose datetime falls in [D1, D2] inclusive.

void rangeSearch(BPTree* tripRoot, char d1[], char d2[]) {
  if (!tripRoot || !tripRoot->root) {
    printf("Trip is empty.\n");
    return;
  }

  printf("\nActivities from %s to %s \n", d1, d2);

  BPTreeNode* leaf = findLeaf(tripRoot->root, d1, tripRoot->compare);
  int found = 0;
  int idx = 1;

  while (leaf) {
    for (int i = 0; i < leaf->n; i++) {
      TripNode* t = (TripNode*)leaf->data[i];
      if (strcmp(t->datetime, d2) > 0) return;
      // Lexicographic comparison works correctly for YYYY-MM-DD HH:MM
      if (strcmp(t->datetime, d1) >= 0) {
        printf("\n  Activity %d:\n", idx++);
        printf("  Type     : %s\n", t->type);
        printf("  Location : %s\n", t->location);
        printf("  DateTime : %s\n", t->datetime);
        printf("  Cost     : %.2f\n", t->cost);
        printf("  Details  : %s\n", t->details);
        found = 1;
      }
    }
    leaf = leaf->next;
  }

  if (!found) printf("No activities found in this date range.\n");
}

// C.1  Path finding

// Check whether there is a path from src to dest within a single trip. If found, print every activity (with navigation) between them.

void findPathInTrip(BPTree* tripRoot, char src[], char dest[]) {
  if (!tripRoot || !tripRoot->root) {
    printf("Trip is empty.\n");
    return;
  }
  normalize_str(src);
  normalize_str(dest);

  // check that src appears before dest
  BPTreeNode* curr = getLeftmost(tripRoot->root);
  int foundSrc = 0;
  int foundDest = 0;
  BPTreeNode* startLeaf = NULL;
  int startIdx = 0;

  while (curr && !foundDest) {
    for (int i = 0; i < curr->n; i++) {
      TripNode* t = (TripNode*)curr->data[i];
      if (!foundSrc && strcmp(t->location, src) == 0) {
        foundSrc = 1;
        startLeaf = curr;
        startIdx = i;
      }
      if (foundSrc && strcmp(t->location, dest) == 0) {
        foundDest = 1;
        break;
      }
    }
    if (!foundDest) curr = curr->next;
  }

  if (!foundSrc) {
    printf("Source '%s' not found.\n", src);
    return;
  }
  if (!foundDest) {
    printf("Destination '%s' not found after source.\n", dest);
    return;
  }

  // print from src to dest
  printf("\nTRAVEL PATH: %s → %s \n", src, dest);
  int step = 1;
  int printing = 0;
  curr = startLeaf;

  while (curr) {
    for (int i = (curr == startLeaf ? startIdx : 0); i < curr->n; i++) {
      TripNode* t = (TripNode*)curr->data[i];

      if (!printing && strcmp(t->location, src) == 0) printing = 1;

      if (printing) {
        printf("\n  Step %d:\n", step++);
        printf("  Type     : %s\n", t->type);
        printf("  Location : %s\n", t->location);
        printf("  DateTime : %s\n", t->datetime);
        printf("  Cost     : %.2f\n", t->cost);
        printf("  Details  : %s\n", t->details);
        if (t->navRoot && t->navRoot->root) {
          printf("  Navigation:\n");
          displayNavigation(t->navRoot);
        }
        if (strcmp(t->location, dest) == 0) {
          printf("\nDestination reached.\n");
          return;
        }
      }
    }
    curr = curr->next;
  }
}

// Check all trips in the list for a src to dest path
void findPathAllTrips(Trip* head, char src[], char dest[]) {
  if (!head) {
    printf("No trips loaded.\n");
    return;
  }
  while (head) {
    printf("\nChecking trip: %s\n", head->name);
    findPathInTrip(head->head, src, dest);
    head = head->next;
  }
}

// C.2  Repeated locations

// BST helpers for counting location visits
static LocNode* addLoc(LocNode* root, char location[]) {
  if (!root) {
    LocNode* n = (LocNode*)malloc(sizeof(LocNode));
    strcpy(n->location, location);
    n->count = 1;
    n->left = n->right = NULL;
    return n;
  }
  int cmp = strcmp(location, root->location);
  if (cmp == 0)
    root->count++;
  else if (cmp < 0)
    root->left = addLoc(root->left, location);
  else
    root->right = addLoc(root->right, location);
  return root;
}

static void freeLocTree(LocNode* root) {
  if (!root) return;
  freeLocTree(root->left);
  freeLocTree(root->right);
  free(root);
}

static void printRepeated(LocNode* root, int* flag) {
  if (!root) return;
  printRepeated(root->left, flag);
  if (root->count > 1) {
    printf("  %-30s visited %d times\n", root->location, root->count);
    *flag = 1;
  }
  printRepeated(root->right, flag);
}

void findRepeatedLocations(BPTree* tripRoot) {
  if (!tripRoot || !tripRoot->root) {
    printf("Trip is empty.\n");
    return;
  }

  LocNode* freqRoot = NULL;
  BPTreeNode* curr = getLeftmost(tripRoot->root);
  while (curr) {
    for (int i = 0; i < curr->n; i++) {
      TripNode* t = (TripNode*)curr->data[i];
      freqRoot = addLoc(freqRoot, t->location);
    }
    curr = curr->next;
  }

  int flag = 0;
  printf("\nLocations Visited More Than Once\n");
  printRepeated(freqRoot, &flag);
  if (!flag) printf("No repeated locations.\n");

  freeLocTree(freqRoot);
}

// C.3  Hotels sorted by descending order of cost

void displayHotelsSortedByCost(BPTree* tripRoot) {
  if (!tripRoot || !tripRoot->root) {
    printf("Trip is empty.\n");
    return;
  }

  TripNode* hotels[100];
  int count = 0;

  BPTreeNode* curr = getLeftmost(tripRoot->root);
  while (curr) {
    for (int i = 0; i < curr->n; i++) {
      TripNode* t = (TripNode*)curr->data[i];
      // Case-insensitive match for "hotel" / "Hotel"
      char tmp[TYPE_SIZE];
      strcpy(tmp, t->type);
      for (int j = 0; tmp[j]; j++)
        tmp[j] = (char)tolower((unsigned char)tmp[j]);
      if (strcmp(tmp, "hotel") == 0 && count < 100) hotels[count++] = t;
    }
    curr = curr->next;
  }

  if (count == 0) {
    printf("No hotel entries in this trip.\n");
    return;
  }

  // Bubble sort descending by cost
  for (int i = 0; i < count - 1; i++) {
    for (int j = i + 1; j < count; j++) {
      if (hotels[i]->cost < hotels[j]->cost) {
        TripNode* tmp = hotels[i];
        hotels[i] = hotels[j];
        hotels[j] = tmp;
      }
    }
  }

  printf("\nHotels (Most Expensive First)\n");
  for (int i = 0; i < count; i++) {
    printf("  %d. %s  (%s)\n", i + 1, hotels[i]->location, hotels[i]->datetime);
    printf("     Cost    : %.2f\n", hotels[i]->cost);
    printf("     Details : %s\n", hotels[i]->details);
  }
}

// update helpers

void updateNode(TripNode* target) {
  if (!target) return;
  printf("\nUpdating activity at %s\n", target->location);
  printf("  1. Update Type\n");
  printf("  2. Update Cost\n");
  printf("  3. Update Details\n");
  printf("Choice: ");
  int choice;
  // Check if scanf successfully reads an integer
  if (scanf("%d", &choice) != 1) {
    while ((getchar()) != '\n'); // Clear the bad input
    printf("Invalid input. Please enter a number.\n");
    return; // Exit ONLY if input was bad
  }

  while ((getchar()) != '\n'); // Clear the newline after a successful read
  if (choice == 1) {
    inpType(target->type);
  } else if (choice == 2) {
    printf("New cost: ");
    scanf("%f", &target->cost);
  } else if (choice == 3) {
    printf("New details: ");
    scanf(" %[^\n]", target->details);
  } else {
    printf("Invalid choice.\n");
  }
}

// cleanup

// Walk every leaf, free each TripNode (and its navRoot tree),  then free the B+ tree structure itself.
void freeTripTree(BPTree* tripRoot) {
  if (!tripRoot) return;

  BPTreeNode* leaf = getLeftmost(tripRoot->root);
  while (leaf) {
    for (int i = 0; i < leaf->n; i++) {
      TripNode* t = (TripNode*)leaf->data[i];
      if (t) {
        freeBPTree(t->navRoot);
        free(t);
      }
      leaf->data[i] = NULL;
    }
    leaf = leaf->next;
  }

  freeBPTree(tripRoot);
}

// UI wrappers

void addActivitySortedUI(BPTree* tripRoot) {
  char type[TYPE_SIZE], location[LOCATION_SIZE];
  char datetime[DATETIME_SIZE], details[DETAILS_SIZE];
  float cost;

  inpType(type);

  printf("Location: ");
  scanf(" %[^\n]", location);
  normalize_str(location);

  inpDateTime(datetime);

  printf("Cost: ");
  scanf("%f", &cost);
  while (cost < 0) {
    printf("Cost cannot be negative. Try again: ");
    scanf("%f", &cost);
  }

  printf("Details: ");
  scanf(" %[^\n]", details);

  TripNode* node = createTripNode(type, location, datetime, cost, details);
  if (node) {
    addActivity(tripRoot, node);
    inputNavigation(tripRoot, node);
  }

  printf("\nActivity added successfully.\n");
}

// Get the activity with the latest datetime (rightmost leaf)
static TripNode* getLastActivity(BPTree* tripRoot) {
  if (!tripRoot || !tripRoot->root) return NULL;
  BPTreeNode* last = getRightmost(tripRoot->root);
  if (!last || last->n == 0) return NULL;
  return (TripNode*)last->data[last->n - 1];
}

void addActivityEndUI(BPTree* tripRoot) {
  char type[TYPE_SIZE], location[LOCATION_SIZE];
  char datetime[DATETIME_SIZE], details[DETAILS_SIZE];
  float cost;

  inpType(type);

  printf("Location: ");
  scanf(" %[^\n]", location);
  normalize_str(location);

  TripNode* last = getLastActivity(tripRoot);
  if (last)
    printf("Last activity is at: %s - new datetime must come after this.\n",
           last->datetime);

  inpDateTime(datetime);
  while (last && strcmp(datetime, last->datetime) <= 0) {
    printf("Datetime must be AFTER %s. Try again.\n", last->datetime);
    inpDateTime(datetime);
  }

  printf("Cost: ");
  scanf("%f", &cost);
  if (scanf("%f", &cost) != 1) {
    printf("Invalid cost. Enter numeric value.\n");
    return;
  }
  while (cost < 0) {
    printf("Cost cannot be negative. Try again: ");
    scanf("%f", &cost);
  }

  printf("Details: ");
  scanf(" %[^\n]", details);

  TripNode* node = createTripNode(type, location, datetime, cost, details);
  if (node) {
    addActivity(tripRoot, node);
    inputNavigation(tripRoot, node);
  }
  printf("\nActivity added successfully.\n");
}

void updateActivityUI(BPTree* tripRoot) {
  if (!tripRoot || !tripRoot->root) {
    printf("Trip is empty.\n");
    return;
  }

  showTripSummary(tripRoot);

  char loc[LOCATION_SIZE];
  printf("Enter location to update: ");
  scanf(" %[^\n]", loc);
  normalize_str(loc);

  TripNode* target = searchByLocation(tripRoot, loc);
  if (!target) {
    printf("Location not found.\n");
    return;
  }
  updateNode(target);
}

void deleteActivityUI(BPTree* tripRoot) {
  if (!tripRoot || !tripRoot->root) {
    printf("Trip is empty.\n");
    return;
  }

  showTripSummary(tripRoot);

  char loc[LOCATION_SIZE];
  printf("Enter location to delete: ");
  scanf(" %[^\n]", loc);
  while(getchar() != '\n');
  normalize_str(loc);
  TripNode* target = searchByLocation(tripRoot, loc);
  if (!target) {
    printf("\n[X] Error: Location '%s' not found in your itinerary.\n", loc);
    return;
  }
  TripNode* prev = NULL;
  TripNode* next = NULL;

  getPrevNext(tripRoot, target, &prev, &next);

  char deletedName[LOCATION_SIZE];
  strcpy(deletedName, target->location);

  deleteActivity(tripRoot, target->datetime);
  printf("  SUCCESS: '%s' has been removed.\n", deletedName);
  if (prev && next) {
    printf("\nRECALIBRATING PATH:\n");
    printf("Since you removed the stop in between,\n");
    printf("please provide new directions from [%s] directly to [%s]:\n",
            prev->location, next->location);

    updateNavAfterDelete(prev, next);

    printf("\n[+] Navigation updated successfully.\n");
  } else {
    printf("\n(No navigation update required for this change.)\n");
  }
}

void findPathUI(BPTree* tripRoot) {
  char src[LOCATION_SIZE], dest[LOCATION_SIZE];
  printf("Source location     : ");
  scanf(" %[^\n]", src);
  printf("Destination location: ");
  scanf(" %[^\n]", dest);
  findPathInTrip(tripRoot, src, dest);
}

void rangeSearchUI(BPTree* tripRoot) {
  char d1[DATETIME_SIZE], d2[DATETIME_SIZE];
  printf("From date ->  ");
  inpDateTime(d1);
  printf("To   date -> ");
  inpDateTime(d2);
  while (strcmp(d1, d2) > 0) {
    printf("From date must be earlier than To date. Please re-enter.\n");
    printf("From date ->  ");
    inpDateTime(d1);
    printf("To   date -> ");
    inpDateTime(d2);
  }
  rangeSearch(tripRoot, d1, d2);
}
void getPrevNext(BPTree* tripRoot, TripNode* tar, TripNode** prevNode,
                 TripNode** nextNode) {
  
  // BPTreeNode* leaf = findLeaf(tripRoot->root, tar->datetime, tripRoot->compare); 
  // cant use this as we need to find prev BptreeNode as well for prevNode if tar is the first entry in leaf
  BPTreeNode* leaf = getLeftmost(tripRoot->root);
  BPTreeNode* prevLeaf = NULL;

  BPTreeNode* tarLeaf = NULL;
  int tarPos = -1;

  int found = 0;

  while (leaf && !found) {
    for (int i = 0; i < leaf->n; i++) {
      TripNode* t = (TripNode*)leaf->data[i];

      if (t == tar) {
        tarLeaf = leaf;
        tarPos = i;
        found = 1;
        break;
      }
    }

    if (!found) {
      prevLeaf = leaf;
      leaf = leaf->next;
    }
  }

  if (!tarLeaf) {
    printf("Error: target TripNode not found in trip tree.\n");
    *prevNode = NULL;
    *nextNode = NULL;
    return;
  }
  //  PREV
  if (tarPos > 0) {
    *prevNode = (TripNode*)tarLeaf->data[tarPos - 1];
  } else if (prevLeaf && prevLeaf->n > 0) {
    *prevNode = (TripNode*)prevLeaf->data[prevLeaf->n - 1];
  } else {
    *prevNode = NULL;
  }

  // NEXT
  if (tarPos < tarLeaf->n - 1) {
    *nextNode = (TripNode*)tarLeaf->data[tarPos + 1];
  } else if (tarLeaf->next) {
    *nextNode = (TripNode*)tarLeaf->next->data[0];
  } else {
    *nextNode = NULL;
  }
}
void inputNavigation(BPTree* tripRoot, TripNode* tar) {
  TripNode *prevNode = NULL, *nextNode = NULL;
  getPrevNext(tripRoot, tar, &prevNode, &nextNode);

  char from[LOC_SIZE], to[LOC_SIZE];

  if (prevNode) {
    strcpy(from, prevNode->location);
    strcpy(to, tar->location);
    freeBPTree(tar->navRoot);
    tar->navRoot = createTree(compareInt);
    inputNavigationSteps(tar->navRoot, from, to);
  }

  if (nextNode) {
    char nextFrom[LOC_SIZE];
    char nextTo[LOC_SIZE];
    strcpy(nextFrom, tar->location);
    strcpy(nextTo, nextNode->location);

    freeBPTree(nextNode->navRoot);

    nextNode->navRoot = createTree(compareInt);
    inputNavigationSteps(nextNode->navRoot, nextFrom, nextTo);
  }
}

void updateNavAfterDelete(TripNode* prev, TripNode* next) {
  if (!prev || !next) return;

  if (next->navRoot) freeBPTree(next->navRoot);

  next->navRoot = createTree(compareInt);

  inputNavigationSteps(next->navRoot, prev->location, next->location);
}