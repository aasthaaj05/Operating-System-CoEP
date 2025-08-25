/*
Write a program that opens 3 files, and shows that the FD obtained are 3, 4, 5 respectively.  Use argv. Filename: threefds.c
*/

#include <fcntl.h>   
#include <unistd.h>  
#include <stdio.h>   
#include <stdlib.h>  

int main(int argc, char *argv[]) {

    int fd1 = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC);
    int fd2 = open(argv[2], O_CREAT | O_WRONLY | O_APPEND);
    int fd3 = open(argv[3], O_CREAT | O_RDWR);
    
    printf("%d %d %d", fd1, fd2, fd3);

}

