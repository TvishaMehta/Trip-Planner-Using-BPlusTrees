#include "tripList.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "menu.h"
#include "trip.h"
#include "util.h"

/* Running counter so every new trip gets a unique ID */
static int nextUID = 1;

/*
   CORE OPERATIONS */

/* Append a new Trip to the end of the linked list */
Trip* createTrip(Trip** tripHead, char name[]) {
  Trip* newTrip = (Trip*)malloc(sizeof(Trip));
  if (!newTrip) {
    printf("Memory allocation failed!\n");
    return NULL;
  }

  newTrip->uid = nextUID++;
  normalize_str(name);
  strcpy(newTrip->name, name);
  newTrip->head =
      createTree(compareDateTime); /* activities sorted by datetime */
  newTrip->next = NULL;

  if (*tripHead == NULL) {
    *tripHead = newTrip;
  } else {
    Trip* tail = *tripHead;
    while (tail->next) tail = tail->next;
    tail->next = newTrip;
  }

  printf("Trip '%s' created (ID %d).\n", name, newTrip->uid);
  return newTrip;
}

/* Remove a trip by UID and free all its data */
void deleteTrip(Trip** tripHead, int uid) {
  Trip* curr = *tripHead;
  Trip* prev = NULL;

  while (curr && curr->uid != uid) {
    prev = curr;
    curr = curr->next;
  }

  if (!curr) {
    printf("Trip %d not found.\n", uid);
    return;
  }

  if (!prev)
    *tripHead = curr->next;
  else
    prev->next = curr->next;

  freeTripTree(curr->head); /* free every TripNode + their navRoots */
  free(curr);
  printf("Trip %d deleted.\n", uid);
}

/* Look up a trip by UID */
Trip* findTrip(Trip* tripHead, int uid) {
  while (tripHead) {
    if (tripHead->uid == uid) return tripHead;
    tripHead = tripHead->next;
  }
  return NULL;
}

/* Print a one-line summary for every trip */
void listAllTripsSummaryUI(Trip* tripHead) {
  if (!tripHead) {
    printf("No trips available.\n");
    return;
  }

  printf("\n===== All Trips =====\n");
  while (tripHead) {
    printf("  [ID %d] %s\n", tripHead->uid, tripHead->name);
    showTripSummary(tripHead->head);
    tripHead = tripHead->next;
  }
}

/* Free all trips in the list */
void freeAllTrips(Trip** tripHead) {
  Trip* curr = *tripHead;
  while (curr) {
    Trip* next = curr->next;
    freeTripTree(curr->head);
    free(curr);
    curr = next;
  }
  *tripHead = NULL;
}

/* ============================================================
   UI WRAPPERS
   ============================================================ */

void createTripUI(Trip** tripHead) {
  char name[NAME_SIZE];
  printf("Enter trip name: ");
  scanf(" %[^\n]", name);
  createTrip(tripHead, name);
}

void deleteTripUI(Trip** tripHead) {
  listAllTripsSummaryUI(*tripHead);
  int uid;
  printf("Enter trip ID to delete: ");
  scanf("%d", &uid);
  deleteTrip(tripHead, uid);
}

/*
 * Let the user pick a trip, then hand control to the trip-level menu
 * so they can add/delete/view activities and navigation steps.
 */
void selectTripUI(Trip* tripHead) {
  if (!tripHead) {
    printf("No trips available.\n");
    return;
  }

  listAllTripsSummaryUI(tripHead);

  int uid;
  printf("Enter trip ID to select: ");
  scanf("%d", &uid);

  Trip* chosen = findTrip(tripHead, uid);
  if (!chosen) {
    printf("Trip not found.\n");
    return;
  }

  printf("Selected: %s\n", chosen->name);
  tripMenu(&chosen->head);
}

/* Look for a source→destination path across ALL trips */
void findPathGlobalUI(Trip* tripHead) {
  char src[LOCATION_SIZE], dest[LOCATION_SIZE];
  printf("Source     : ");
  scanf(" %[^\n]", src);
  printf("Destination: ");
  scanf(" %[^\n]", dest);
  findPathAllTrips(tripHead, src, dest);
}

/* Run a range search on a user-selected trip */
void rangeSearchGlobalUI(Trip* tripHead) {
  if (!tripHead) {
    printf("No trips available.\n");
    return;
  }
  listAllTripsSummaryUI(tripHead);

  int uid;
  printf("Enter trip ID for range search: ");
  scanf("%d", &uid);

  Trip* chosen = findTrip(tripHead, uid);
  if (!chosen) {
    printf("Trip not found.\n");
    return;
  }

  rangeSearchUI(chosen->head);
}
