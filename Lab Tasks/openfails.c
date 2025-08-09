/*
Write a program that shows that open() fails due to three different reasons. For this, use the concept that open() returns -1 on failure, and the global variable errno is set to indicate the reason of failure. Read "man 2 open". Embed the filenames in the code, do not use argv.  Filename: openfails.c
*/

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main() {
    int fd;
        
    //Create a pre existing file with O_EXCL (doesnt open as we are trying to create an already existing file)
    fd = open("file1.txt", O_CREAT | O_EXCL);
    if (fd == -1) {
        printf("Error: %d\n", errno);
    }

    //Permission denied 
    fd = open("/root", O_RDONLY);
    if (fd == -1) {
        printf("Error: %d\n", errno);
    }

    //Invalid path
    fd = open("/invalid/path/to/file.txt", O_RDONLY);
    if (fd == -1) {
        printf("Error: %d\n", errno);
    }
}

