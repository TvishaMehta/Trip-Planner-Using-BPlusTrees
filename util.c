#include "util.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Trim leading/trailing whitespace in-place, then upper-case the very first letter so location names are stored consistently (e.g. "mumbai" → "Mumbai").
int is_space(char c) {
    return (c == ' ' || c == '\t' || c == '\n' ||
            c == '\r' || c == '\v' || c == '\f');
}
void normalize_str(char* str) {
    if (!str || str[0] == '\0') return;

    //  Strip leading whitespace 
    int start = 0;
    while (str[start] && is_space(str[start]))
        start++;

    int len = strlen(str + start);

    // Strip trailing whitespace
    while (len > 0 && is_space(str[start + len - 1]))
        len--;

    // Shift the trimmed string to position 0
    memmove(str, str + start, len);
    str[len] = '\0';

    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
    // Capitalise first character
    if (len > 0)
        str[0] = toupper(str[0]);
    
}
int isValidType(char type[]) {

    normalize_str(type);

    if (strcmp(type, "Hotel") == 0 ||
        strcmp(type, "Flight") == 0 ||
        strcmp(type, "Transport") == 0 ||
        strcmp(type, "Tourist") == 0 ||
        strcmp(type, "Start") == 0) {

        return 1;
    }

    return 0;
}

int isValidDateTime(char dt[]) {
    // Length must be exactly 16
    if (strlen(dt) != 16) return 0;

    // Check fixed positions
    if (dt[4] != '-' || dt[7] != '-' || dt[10] != ' ' || dt[13] != ':')
        return 0;

    // Check all other positions are digits
    for (int i = 0; i < 16; i++) {
        if (i == 4 || i == 7 || i == 10 || i == 13) continue;
        if (!isdigit(dt[i])) return 0;
    }

    // Extract values
    int year, month, day, hour, minute;
    sscanf(dt, "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute);

    // Basic range checks
    if (month < 1 || month > 12) return 0;
    if (day < 1 || day > 31) return 0;
    if (hour < 0 || hour > 23) return 0;
    if (minute < 0 || minute > 59) return 0;
    if(month==2 && day>29) return 0;

    return 1; // valid
}
// Ask the user to pick an activity type. Accepted values: Flight, Hotel, Tourist, Transport, Start.
void inpType(char type[]) {
    printf("Type (Start/Hotel/Flight/Tourist/Transport): ");
    scanf(" %[^\n]", type);

    while(!isValidType(type)) {
        printf("Invalid type! Enter again (Start/Hotel/Flight/Tourist/Transport): ");
        scanf(" %[^\n]", type);
    }
    while(getchar() != '\n');
}

// Ask the user for a datetime in the format YYYY-MM-DD HH:MM. We do a basic length check but keep validation light.
void inpDateTime(char dt[]) {
    printf("Enter datetime (YYYY-MM-DD HH:MM): ");
    scanf(" %[^\n]", dt);

    while(!isValidDateTime(dt)) {
        printf("Invalid datetime! Enter again (YYYY-MM-DD HH:MM): ");
        scanf(" %[^\n]", dt);
    }
}
void pauseScreen() {
    printf("\nPress Enter to continue...");
    while (getchar() != '\n');
    getchar();
}

