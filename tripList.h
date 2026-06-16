#ifndef TRIPLIST_H
#define TRIPLIST_H

#include "trip.h"

/*
 * The outer linked list of trips.
 * Each Trip holds its own B+ tree (head) of TripNode activities.
 */
typedef struct Trip {
    int  uid;
    char name[NAME_SIZE];
    BPTree* head;       /* B+ tree of TripNode*, keyed by datetime */
    struct Trip* next;
} Trip;

/* --- trip-level operations --- */
Trip*  createTrip(Trip** tripHead, char name[]);
void  deleteTrip(Trip** tripHead, int uid);
Trip* findTrip(Trip* tripHead, int uid);
void  listAllTripsSummaryUI(Trip* tripHead);
void  freeAllTrips(Trip** tripHead);

/* --- UI wrappers --- */
void createTripUI(Trip** tripHead);
void deleteTripUI(Trip** tripHead);
void selectTripUI(Trip* tripHead);
void findPathGlobalUI(Trip* tripHead);
void rangeSearchGlobalUI(Trip* tripHead);

#endif
