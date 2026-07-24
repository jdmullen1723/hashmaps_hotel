#include "direct_addressing.h" //Give access to CAPACITY, and indirectly to the Guest struct
#include <string.h> //Allows us to use string functions like strncpy

/**
This line creates an array called guest_list of size capacity
that can contain then type Guest.
Static here means that this array is limited to internal linkage. 
This means it can only be accessed by code within this .c file.
 */
static Guest guest_list[CAPACITY];

/*
The fucntion to add a guest to the guest_list array declared above.
*/
void insert_guest(int guest_number, const char *name) {
    int index = guest_number - 1; //Array starts at 0, but guest_number starts at 1. So we want number 1 in index 0 and so on
    guest_list[index].guest_number = guest_number; //Declares that the Guest at the index will have a guest_number equal to the guest_number parameter
    /*
    Copies the name passed in to the guest_list[index].name. 
    It copies a maximum of NAME_MAX - 1 characters to not exceed the memory allocation.
     */
    strncpy(guest_list[index].name, name, NAME_MAX - 1); 
    /*
    This ensures that the last character of the inputed string is always the terminator.
    It's at position NAME_MAX - 1, not NAME_MAX because the indexing of strings starts at 0 not 1. 
    */
    guest_list[index].name[NAME_MAX - 1] = '\0'; 
}

/*
The function to lookup a guest given their guest_number. 
It checks if the slot if NULL, if not it returns the adress to the guest that is there.
*/
Guest *lookup_guest(int guest_number) {
    int index = guest_number - 1;
    if (guest_list[index].guest_number == 0) {
        return NULL;
    }
    return &guest_list[index];
}