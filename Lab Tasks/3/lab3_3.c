//Write a small C program showing how output redirection can be done by closing (1) and opening a file.

#include<unistd.h>
#include<fcntl.h>

int main(){
    close(1);
    open("output.txt", O_WRONLY|O_CREAT);
    write(1, "This goes to output.txt\n", 23);
}
