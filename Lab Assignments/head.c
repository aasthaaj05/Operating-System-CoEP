/*
Write an application in xv6 Operating System (not kernel). 

It can be either of the following: cp, head, tail, (or any other that you discuss with the faculty) 

Submit: cp.c,  head.c, tail.c, etc. 

Also submit Modified Makefile 
*/

#include"types.h"
#include"stat.h"
#include"user.h"
#include"fcntl.h"

int main(int argc, char *argv[]){
    int fd, n, i;
    char buf[256];
    int line_count=0;
    int lines_to_show=10;

    if(argc<2){
        exit();
    }

    if((fd=open(argv[1], O_RDONLY))<0){
        exit();
    }

    while((n=read(fd, buf, sizeof(buf)))> 0 && line_count<lines_to_show){
        for(i=0;i<n;i++) {
            if(buf[i]=='\n') {
                line_count++;
                if(line_count>=lines_to_show){
                    break;
                }
            }
            printf(1, "%c", buf[i]);
        }
    }
    close(fd);
    exit();
}

