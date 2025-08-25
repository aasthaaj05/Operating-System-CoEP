/*
Write a program that demonstrates read(0, ...) and write(1, ...) works.  In this write a function mygetchar() and myputchar() which works like getchar() and putchar() respectively.   Filename: getputchar.c
*/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

char mygetchar() {
    char c;
    if (read(0, &c, 1) == 1)
        return c;
    else
        return -1;
}

void myputchar(char c) {
    write(1, &c, 1);
}

int main() {
    char c;

    while ((c = mygetchar()) != -1) {
        myputchar(c);  
    }
}

