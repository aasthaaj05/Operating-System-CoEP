#include<stdio.h>
#include<unistd.h>

int main(){
    printf("PID: %d\n", getpid());
    for (int i = 0; i < 100000000; i++) {
        printf("Working... %d\n", i);
        sleep(1); 
    }
    return 0;
}

