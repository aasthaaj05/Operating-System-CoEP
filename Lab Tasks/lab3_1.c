//Show that exec() can fail, for at least 2 different reasons

#include<unistd.h>
#include<stdio.h>
#include<errno.h>

int main(){
    //Non existent program
    if(execl("/nonexistent/path", "nonexistent", NULL) == -1){
        perror("execl failed (nonexistent path)");
    }

    //No execute permission
    if(execl("/etc/passwd", "passwd", NULL) == -1){
        perror("execl failed (no execute permission)");
    }
}
