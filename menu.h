#ifndef MENU_H
#define MENU_H

#include "trip.h"
#include "tripList.h"

// Entry point — runs the top-level menu loop
void mainMenu(Trip** tripHead);

// Second-level menu for managing activities in a chosen trip
void tripMenu(BPTree** head);

// Third-level menu for navigation steps on a specific activity
void navigationMenu(BPTree* tripRoot);

#endif
