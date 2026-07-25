#include <stdio.h> //For printf command
#include "src/direct_addressing.h"

int main(void) {
    //Inserts the first two guests manually as a trial
    insert_guest(1, "Pamela James"); 
    insert_guest(2, "Debra Thompson");

    Guest *g_1 = lookup_guest(1);
    if (g_1 != NULL) {
        printf("Guest %d: %s\n", g_1->guest_number, g_1->name);
    } else {
        printf("Guest 1 not found\n");
    }

    Guest *g_2 = lookup_guest(2);
    if (g_2 != NULL) {
        printf("Guest %d: %s\n", g_2->guest_number, g_2->name);
    } else {
        printf("Guest 2 not found\n");
    }

    Guest *missing = lookup_guest(500);
    if (missing != NULL) {
        printf("Guest %d: %s\n", missing->guest_number, missing->name);
    } else {
        printf("Guest 500 not found\n");
    }

    return 0;
}