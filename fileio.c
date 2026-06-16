#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileio.h"
#include "trip.h"
#include "navigation.h"

/*
File format (plain text, pipe-separated):
TRIP|<uid>|<name>
ACTIVITY|<type>|<location>|<datetime>|<cost>|<details>
NAVCOUNT|<n>
NAV|<direction>|<distance>
... (n NAV lines)
... more ACTIVITY blocks ...
 ENDTRIP
... more TRIP blocks ...
*/

// save

void saveAllTripsToFile(Trip* head, char filename[]) {
    if (!head) {
        // Nothing to write — leave any existing file untouched
        return;
    }

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: could not open '%s' for writing.\n", filename);
        return;
    }

    while (head) {
        fprintf(fp, "TRIP|%d|%s\n", head->uid, head->name);

        // Walk the leaf chain of the trip B+ tree
        if (head->head && head->head->root) {
            BPTreeNode* leaf = getLeftmost(head->head->root);
            while (leaf) {
                for (int i = 0; i < leaf->n; i++) {
                    TripNode* node = (TripNode*)leaf->data[i];
                    fprintf(fp, "ACTIVITY|%s|%s|%s|%.2f|%s\n",
                            node->type, node->location,
                            node->datetime, node->cost, node->details);

                    // Count how many nav steps this activity has
                    int navCount = 0;
                    if (node->navRoot && node->navRoot->root) {
                        BPTreeNode* navLeaf = getLeftmost(node->navRoot->root);
                        while (navLeaf) {
                            navCount += navLeaf->n;
                            navLeaf   = navLeaf->next;
                        }
                    }
                    fprintf(fp, "NAVCOUNT|%d\n", navCount);

                    // Write each nav step
                    if (navCount > 0) {
                        BPTreeNode* navLeaf = getLeftmost(node->navRoot->root);
                        while (navLeaf) {
                            for (int j = 0; j < navLeaf->n; j++) {
                                NavNode* nav = (NavNode*)navLeaf->data[j];
                                fprintf(fp, "NAV|%s|%.2f\n",
                                        nav->direction, nav->distance);
                            }
                            navLeaf = navLeaf->next;
                        }
                    }
                }
                leaf = leaf->next;
            }
        }

        fprintf(fp, "ENDTRIP\n");
        head = head->next;
    }

    fclose(fp);
}

// load

void loadAllTripsFromFile(Trip** head, char filename[]) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return;   // fine — file just doesn't exist yet on first run

    char line[256];
    Trip*     currentTrip = NULL;
    TripNode* lastNode    = NULL;

    while (fgets(line, sizeof(line), fp)) {
        // Strip trailing newline / carriage-return
        int len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        if (strncmp(line, "TRIP|", 5) == 0) {
            char name[NAME_SIZE];
            int  id;
            sscanf(line, "TRIP|%d|%49[^\n]", &id, name);
            Trip * newTrip = createTrip(head, name);

            // Point currentTrip to the one we just added (always at tail)
            if(!currentTrip) {
                currentTrip = newTrip;
            } else {
                currentTrip->next = newTrip;
                currentTrip = currentTrip->next;
            }

            lastNode = NULL;

        } else if (strncmp(line, "ACTIVITY|", 9) == 0) {
            char  type[TYPE_SIZE], loc[LOCATION_SIZE];
            char  dt[DATETIME_SIZE], details[DETAILS_SIZE];
            float cost;

            sscanf(line, "ACTIVITY|%19[^|]|%49[^|]|%19[^|]|%f|%99[^\n]",
                   type, loc, dt, &cost, details);

            TripNode* node = createTripNode(type, loc, dt, cost, details);
            if (node && currentTrip)
                addActivity(currentTrip->head, node);

            lastNode = node;

        } else if (strncmp(line, "NAV|", 4) == 0) {
            char  dir[NAV_DIR_SIZE];
            float dist;
            sscanf(line, "NAV|%99[^|]|%f", dir, &dist);
            if (lastNode)
                addDirectionCore(lastNode->navRoot, dir, dist);

        }
        // NAVCOUNT and ENDTRIP lines are structural — we skip them
    }

    fclose(fp);
}
