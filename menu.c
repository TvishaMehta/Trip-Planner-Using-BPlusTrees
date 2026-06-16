#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "menu.h"
#include "trip.h"
#include "tripList.h"
#include "navigation.h"
#include "util.h"
#include "fileio.h"

extern Trip* tripHead;   // defined in main.c

// main menu

void mainMenu(Trip** head) {
    int choice = 0;

    while (choice != 7) {
        printf("\nTRAVEL PLANNER\n");
        printf("  1. Create new trip\n");
        printf("  2. Select a trip\n");
        printf("  3. List all trips\n");
        printf("  4. Delete a trip\n");
        printf("  5. Find path (search across all trips)\n");
        printf("  6. Range search (across all trips)\n");
        printf("  7. Exit\n");
        printf("Enter choice: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1:
                createTripUI(head);
                saveAllTripsToFile(*head, "data.txt");
                break;
            case 2:
                selectTripUI(*head);
                saveAllTripsToFile(*head, "data.txt");
                break;
            case 3:
                listAllTripsSummaryUI(*head);
                break;
            case 4:
                deleteTripUI(head);
                saveAllTripsToFile(*head, "data.txt");
                break;
            case 5:
                findPathGlobalUI(*head);
                break;
            case 6:
                rangeSearchGlobalUI(*head);
                break;
            case 7:
                printf("Saving and exiting. Goodbye!\n");
                break;
            default:
                printf("Invalid choice, please try again.\n");
        }
    }
}

// Trip menu:  For handling activities within a single trip

void tripMenu(BPTree** head) {
    int choice = 0;

    while (choice != 11) {
        printf("\nTRIP MENU\n");
        printf("  1.  Add activity (sorted by date)\n");
        printf("  2.  Add activity (append to end)\n");
        printf("  3.  Update an activity\n");
        printf("  4.  Delete an activity\n");
        printf("  5.  Display full trip\n");
        printf("  6.  Navigation menu (manage directions)\n");
        printf("  7.  Hotels sorted by cost\n");
        printf("  8.  Find repeated locations\n");
        printf("  9.  Find path within trip\n");
        printf("  10. Range search (by date)\n");
        printf("  11. Back\n");
        printf("Enter choice: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1:
                addActivitySortedUI(*head);
                saveAllTripsToFile(tripHead, "data.txt");
                break;
            case 2:
                addActivityEndUI(*head);
                saveAllTripsToFile(tripHead, "data.txt");
                break;
            case 3:
                updateActivityUI(*head);
                saveAllTripsToFile(tripHead, "data.txt");
                break;
            case 4:
                deleteActivityUI(*head);
                saveAllTripsToFile(tripHead, "data.txt");
                break;
            case 5:
                displayTrip(*head);
                break;
            case 6:
                navigationMenu(*head);
                break;
            case 7:
                displayHotelsSortedByCost(*head);
                break;
            case 8:
                findRepeatedLocations(*head);
                break;
            case 9:
                findPathUI(*head);
                break;
            case 10:
                rangeSearchUI(*head);
                break;
            case 11:
                return;
            default:
                printf("Invalid choice, please try again.\n");
        }
    }
}

// Navigation menu: For handling direction for a single activity node

void navigationMenu(BPTree* tripRoot) {
    if (!tripRoot || !tripRoot->root) {
        printf("Trip is empty — add activities first.\n");
        return;
    }

    // Displaying summary to help user pick activity
    showTripSummary(tripRoot);
    char loc[LOCATION_SIZE];
    printf("Enter location to manage navigation for: ");
    scanf(" %[^\n]", loc);

    TripNode* node = searchByLocation(tripRoot, loc);
    if (!node) {
        printf("Location '%s' not found.\n", loc);
        return;
    }

    int choice = 0;
    while (choice != 7) {
        printf("\nNAVIGATION MENU: %s \n", node->location);
        printf("  1. Display navigation\n");
        printf("  2. Add direction (append)\n");
        printf("  3. Insert direction at position\n");
        printf("  4. Delete direction\n");
        printf("  5. Update direction\n");
        printf("  6. Search for a direction\n");
        printf("  7. Back\n");
        printf("Enter choice: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1:
                displayNavigation(node->navRoot);
                break;
            case 2:
                addDirectionUI(node->navRoot);
                saveAllTripsToFile(tripHead, "data.txt");
                break;
            case 3:
                insertDirectionUI(node->navRoot);
                saveAllTripsToFile(tripHead, "data.txt");
                break;
            case 4:
                deleteDirectionUI(node->navRoot);
                saveAllTripsToFile(tripHead, "data.txt");
                break;
            case 5:
                updateDirectionUI(node->navRoot);
                saveAllTripsToFile(tripHead, "data.txt");
                break;
            case 6: {
                char dir[NAV_DIR_SIZE];
                printf("Direction to search: ");
                scanf(" %[^\n]", dir);
                if (searchDirection(node->navRoot, dir))
                    printf("Found '%s' in navigation.\n", dir);
                else
                    printf("'%s' not found.\n", dir);
                break;
            }
            case 7:
                return;
            default:
                printf("Invalid choice.\n");
        }
    }
}
