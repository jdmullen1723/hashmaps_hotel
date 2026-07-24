#ifndef DIRECT_ADDRESS_H
#define DIRECT_ADDRESS_H

#include "guest.h" //essentially imports guest.h

//for direct addressing the CAPACITY is fixed at the number of guests. CAPACITY = number of guests = 1000
#define CAPACITY 1000 

void insert_guest(int guest_number, const char *name);
Guest *lookup_guest(int guest_number);

#endif