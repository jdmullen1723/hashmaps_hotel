#include <assert.h> //Import from compiler library for assert function
#include <stdio.h> //Import from compiler library --> allows for printf function
#include <string.h> //Import that allows strmcp or "string compare"
#include "../src/direct_addressing.h" //Import from elsewhere in this repo, imports "direct_addressing.h"

int main(void) {
    //First paragraph calls insert_guest then runs tests to see if it was actually inserted
    insert_guest(1, "Test Guest One"); //Inserts "Test Guest One" at index 0 in array
    Guest *found = lookup_guest(1);
    assert(found != NULL);
    assert(found->guest_number == 1);
    assert(strcmp(found->name, "Test Guest One") == 0);

    //We check to see that there is no guest at guest number 500. We never inserted a guest, so there should be none there.
    Guest *missing = lookup_guest(500);
    assert(missing == NULL);

    //Same test as first paragraph just in a different position
    insert_guest(1000, "Last Guest");
    Guest *last = lookup_guest(1000);
    assert(last != NULL);
    assert(last->guest_number == 1000);
    assert(strcmp(last->name, "Last Guest") == 0);

    printf("All direct addressing tests passed.\n");
    return 0;
}