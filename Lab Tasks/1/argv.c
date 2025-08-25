//rite a C program that uses arguments to main(i.e. argc, argv).  Print all the arguments in main(). Learn the exact meaning of argc, argv. (filename: argv.c)
//Draw a diagram (neat, detailed) showing argc, and argv as data structures.  (filename: argv.jpeg)

#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("Number of arguments (argc): %d\n", argc);
    printf("Arguments passed (argv):\n");

    for (int i = 0; i < argc; i++) {
        printf("argv[%d] = %s\n", i, argv[i]);
    }

    return 0;
}
