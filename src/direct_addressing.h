#ifndef DIRECT_ADDRESS_H
#define DIRECT_ADDRESS_H

#include "guest.h" //essentially imports guest.h

//for direct addressing the CAPACITY is fixed at the number of guests. CAPACITY = number of guests = 1000
#define CAPACITY 1000 

//We define these functions here, but their bodies are written in direct_addressing.c
void insert_guest(int guest_number, const char *name); //Adds a guest to array
Guest *lookup_guest(int guest_number); //Looks up guest within array

#endif