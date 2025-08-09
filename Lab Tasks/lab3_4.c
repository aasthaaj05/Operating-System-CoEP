//Write a program that calls fork() twice, or thrice, and one/two of them inside some if-else. Then draw a diagram showing how the fork calls work. 

#include <unistd.h>
#include <stdio.h>

int main(){
    pid_t pid1, pid2;
    
    printf("Parent (PID %d)\n", getpid());
    
    pid1=fork();
    
    if(pid1==0){
        //Child 1
        printf("Child 1 (PID %d)\n", getpid());
        pid2=fork();
        
        if(pid2==0){
            //Grandchild
            printf("Grandchild (PID %d)\n", getpid());
        }else{
            //Still Child 1
            printf("Child 1 created Grandchild %d\n", pid2);
        }
    } else{
        //Parent
        printf("Parent created Child 1 %d\n", pid1);
        fork();  //Create Child 2
        printf("Process %d exiting\n", getpid());
    }
    
    return 0;
}
