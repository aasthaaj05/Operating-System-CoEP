//Show that you can call execl(), execvp() and execlp() 

#include<unistd.h>

int main(){
    char *args[]={"ls", "-l", NULL};
    
    //execl()- needs full path and list of args
    execl("/bin/ls", "ls", "-l", NULL);
    
    //execvp()- uses PATH and takes array
    execvp("ls", args);
    
    //execlp()- uses PATH and takes list
    execlp("ls", "ls", "-l", NULL);
}
