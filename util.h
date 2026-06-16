#ifndef UTIL_H
#define UTIL_H

/* Trim leading/trailing whitespace and capitalise the first letter */
void normalize_str(char* str);

/* Prompt the user for a valid activity type */
void inpType(char type[]);

/* Prompt the user for a valid datetime string (YYYY-MM-DD HH:MM) */
void inpDateTime(char datetime[]);

#endif
