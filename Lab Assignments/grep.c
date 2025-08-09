/*
This program is a simple version of the "grep" command.

Write a program without using printf, scanf, getchar, putchar (and related standard I/O functions) , without using FILE, fopen, etc; and only using open(), read(), write(), close(), lseek(): 

which reads a text file, specified as command line argument (argv[1]) and 

reads a word from standard input and

prints all lines from the text file containing given word (here word is a character sequence, it need not be separated with space). See how grep works.
*/

#include<unistd.h>
#include<fcntl.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]){
    char buf[BUF_SIZE], word[BUF_SIZE], *p, *line;
    int fd, n, i, word_len=0, in_line=0;

    //Read search word from stdin
    while ((n=read(0, word+word_len, 1))>0 && word[word_len]!='\n') word_len++;
    word[word_len]=0;

    //Open file
    if((fd=open(argv[1], O_RDONLY))<0) return 1;

    //Search file
    line=buf;
    while((n=read(fd, buf, BUF_SIZE))>0) {
        for(p=buf; p<buf+n; p++) {
            if(!in_line)line=p;
            if(*p=='\n'){
                in_line=0;
                continue;
            }
            in_line=1;
            for(i=0; i<word_len && p+i<buf+n; i++)
                if(p[i]!=word[i]) break;
            if(i==word_len){
                write(1, line, p-line+word_len + 1);
                p+=word_len-1;
            }
        }
    }
    close(fd);
    return 0;
}
