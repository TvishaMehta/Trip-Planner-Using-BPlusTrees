#ifndef TRIP_H
#define TRIP_H

#include "navigation.h"

#define TYPE_SIZE 20
#define LOCATION_SIZE 50
#define DATETIME_SIZE 20
#define DETAILS_SIZE 100
#define NAME_SIZE 50

// Forward declaration so TripNode can reference Trip
typedef struct Trip Trip;

// One activity in the itinerary. navRoot is a B+ tree keyed by step number — stores all the navigation directions needed to reach this spot.
typedef struct TripNode {
  char type[TYPE_SIZE];
  char location[LOCATION_SIZE];
  char datetime[DATETIME_SIZE];
  float cost;
  char details[DETAILS_SIZE];
  BPTree* navRoot; // B+ tree of NavNode* entries for this activity
} TripNode;

// Small BST node used internally by findRepeatedLocations. Keeps a visit count per unique location name.
typedef struct LocNode {
  char location[LOCATION_SIZE];
  int count;
  struct LocNode* left;
  struct LocNode* right;
} LocNode;

// node / activity management
TripNode* createTripNode(char type[], char location[], char datetime[],
                         float cost, char details[]);
void addActivity(BPTree* tripRoot, TripNode* newNode);
void deleteActivity(BPTree* tripRoot, char datetime[]);
void displayTrip(BPTree* tripRoot);

// search
TripNode* searchByLocation(BPTree* tripRoot, char location[]);
TripNode* searchByFirstLocation(BPTree* tripRoot, char location[]);

// C.1  path finding
void findPathInTrip(BPTree* tripRoot, char src[], char dest[]);
void findPathAllTrips(Trip* head, char src[], char dest[]);

// C.2  repeated locations
void findRepeatedLocations(BPTree* tripRoot);

// C.3  hotel cost sort
void displayHotelsSortedByCost(BPTree* tripRoot);

// range search
void rangeSearch(BPTree* tripRoot, char d1[], char d2[]);

// misc helpers
void updateNode(TripNode* target);
void showTripSummary(BPTree* tripRoot);
void freeTripTree(BPTree* tripRoot);

// UI wrappers
void addActivitySortedUI(BPTree* tripRoot);
void addActivityEndUI(BPTree* tripRoot);
void updateActivityUI(BPTree* tripRoot);
void deleteActivityUI(BPTree* tripRoot);
void findPathUI(BPTree* tripRoot);
void rangeSearchUI(BPTree* tripRoot);

void inputNavigation(BPTree* tripRoot, TripNode* tar);
void updateNavAfterDelete(TripNode* prev, TripNode* next);
void getPrevNext(BPTree* tripRoot, TripNode* tar, TripNode** prevNode,
                 TripNode** nextNode);
#endif
