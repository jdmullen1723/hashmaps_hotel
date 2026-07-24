#ifndef GUEST_H
#define GUEST_H

#define NAME_MAX 64

typedef struct {
    int guest_number;
    char name[NAME_MAX];
} Guest;

#endif