#ifndef FILEIO_H
#define FILEIO_H

#include "tripList.h"

void saveAllTripsToFile(Trip* head, char filename[]);
void loadAllTripsFromFile(Trip** head, char filename[]);

#endif
