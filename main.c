#include <stdio.h> //For printf command
#include <string.h>
#include "src/direct_addressing.h"
#include <stdlib.h>

int main(void) {
    /*
    This paragraph opens the data file "hotel_california.psv" in reading mode "r".
    The file is NULL checked, then declared toi have opened succesfully is not NULL.
    */
    FILE *file = fopen("data/hotel_california.psv", "r"); 
    if (file == NULL) {
        printf("Could not open the data file.\n");
        return 1;
    }
    printf("File opened successfully.\n");

    /*
    This array "line" is the space allocated for the fget functions
    to store their strings as they are called in the loop.
    */
    char line[256]; //declares an array of 256 characters named "line"

    //Reads and discards the header (guest_number|name|phone)
    fgets(line, sizeof(line), file);

    int count = 0;

    /*
    fgets reads through the file while moving the cursor character by charcter. 
    It stops when it hits a newline \n.
    It stores the line is has just read as "line", a string
    This while loop calls fgets on every line and tokenizes each line by value.
    */
    while (fgets(line, sizeof(line), file) != NULL) {
        //Removes the newline \n at the end of the line so the data reads as one clean line
        //If youz don't do this first, reading the datafile with fgets will reveal a \n every line
        line[strcspn(line, "\n")] = '\0';

        //strtok splits the lines in the datafile into smaller strings.
        //The marker for where to split is chosen as "|"
        char *number_str = strtok(line, "|");
        //The NULL below means carry on tokenizing from the line where you left off
        char *name = strtok(NULL, "|"); 
        
        int guest_number = atoi(number_str); //Atoi turns the string of number_str into the integer guest_number, as insert guest requires int as an input
        insert_guest(guest_number, name); //Inserts the guest at the index corresponding to their address
        count++; //Increments the count
    }

    fclose(file);

    printf("Loaded %d guests.\n", count); //See count to verify all guests loaded

    Guest *g = lookup_guest(1000);
    if (g != NULL) printf("Found: %d = %s\n", g->guest_number, g->name);
    else printf("Guest 1000 not found \n");

    Guest *missing = lookup_guest(1001);
    if (missing != NULL) printf("FOund: %d = %s\n", missing->guest_number, missing->name);
    else printf("Guest 1001 not found");

    return 0;
}