#include <stdio.h>
#include "menu.h"
#include "tripList.h"
#include "fileio.h"

// Global head — externed in menu.c and tripList.c
Trip* tripHead = NULL;

int main(void) {
    // Load any trips saved from a previous session
    loadAllTripsFromFile(&tripHead, "data.txt");

    // Hand off to the interactive menu
    mainMenu(&tripHead);

    // Persist everything the user changed before we exit
    saveAllTripsToFile(tripHead, "data.txt");

    // Clean up heap memory
    freeAllTrips(&tripHead);

    return 0;
}
