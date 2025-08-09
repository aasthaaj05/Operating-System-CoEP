//Learn chdir() and getcwd() C function. Write a program using it.  Use "man chdir" and "man getcwd" commands to learn. (filename: getcwd.c)

#include <stdio.h>
#include <unistd.h>
#include <limits.h>

int main() {
    char cwd[PATH_MAX];

    // Get and print the current directory
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current directory: %s\n", cwd);
    } else {
        perror("getcwd error");
        return 1;
    }

    // Change directory to parent
    if (chdir("..") == 0) {
        printf("Directory changed to parent.\n");
    } else {
        perror("chdir error");
        return 1;
    }

    // Get and print the new current directory
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("New current directory: %s\n", cwd);
    } else {
        perror("getcwd error");
        return 1;
    }

    return 0;
}

