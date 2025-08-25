/*
Write a shell. The shell should have following features:

Read commands in a loop and run commands without complete path names and handle arguments+options for commands (1 marks)
show prompt with current working directory in the prompt and allow user to change the prompt to a particular string and revert back to CWD prompt (1 marks)
This should be done with following two commands, with specified syntax:
PS1="whatever string you want "
PS1="\w$"
This also mandates that the "cd" command should work. Note that "cd" is always an internal command of shell, and not an executable. It affects the cwd of the shell itself.
Handle all possible user input errors (2 marks)
exit gracefully on typing "exit" or ctrl-D  (0.5 marks)
allow users to set a variable called PATH which is used for searching the list of executables (just like in the case of 'bash' shell)   (2 marks)
This should be done with a command that has following syntax
PATH=/usr/bin:/bin:/sbin
This combined with first point means that you will NOT be using execvp(), but rather implement a version of execvp() on your own.
Implement input redirection and output redirection  (0.5 marks)
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<limits.h>

#define MAX_ARGS 64
#define MAX_PATH_LENGTH 4096

char *prompt=NULL;
char *path=NULL;

void execute_command(char **args);
void handle_cd(char **args);
void handle_path(char **args);
void handle_ps1(char **args);
int find_executable(char *command, char *full_path);
void setup_redirection(char **args, int *input_fd, int *output_fd);

void execute_command(char **args){
    if (!args[0]) return; // No command

    pid_t pid=fork();
    
    if(pid==0){  //Child
        int input_fd=-1, output_fd=-1;
        
        //redirection
        setup_redirection(args, &input_fd, &output_fd);
        
        //Find executable in PATH
        char full_path[PATH_MAX];
        if(find_executable(args[0], full_path)){
            execv(full_path, args);
            perror("execv");
        }else{
            fprintf(stderr, "Command not found: %s\n", args[0]);
        }
        exit(EXIT_FAILURE);
    }else if(pid > 0){  //Parent
        wait(NULL);
    } else {
        perror("fork");
    }
}

void handle_cd(char **args){
    if(args[1]==NULL){
        chdir(getenv("HOME"));
    }else{
        if(chdir(args[1])!=0){
            perror("cd");
        }
    }
}

void handle_path(char **args){
    if(args[1]==NULL){
        printf("PATH=%s\n", path);
    }else{
        free(path);
        path=strdup(args[1]);
    }
}

void handle_ps1(char **args){
    if(args[1]==NULL){
        printf("PS1=%s\n", prompt ? prompt : "\\w$");
    }else if(strcmp(args[1], "\\w$")==0){
        free(prompt);
        prompt=NULL;
    }else{
        free(prompt);
        prompt=strdup(args[1]);
    }
}

int find_executable(char *command, char *full_path){
    //if command contains a path
    if(strchr(command, '/')!=NULL){
        if(access(command, X_OK)==0){
            strcpy(full_path, command);
            return 1;
        }
        return 0;
    }
    
    // fallback to system PATH if our path is NULL
    if(!path){
        char *envp = getenv("PATH");
        if(envp) path = strdup(envp);
        else path = strdup("/usr/bin:/bin:/usr/sbin:/sbin");
    }

    char *path_copy=strdup(path);
    char *dir=strtok(path_copy, ":");
    
    while(dir!=NULL){
        snprintf(full_path, PATH_MAX, "%s/%s", dir, command);
        if(access(full_path, X_OK)==0){
            free(path_copy);
            return 1;
        }
        dir=strtok(NULL, ":");
    }
    free(path_copy);
    return 0;
}

void setup_redirection(char **args, int *input_fd, int *output_fd) {
    for(int i=0; args[i]!=NULL; i++){
        if(strcmp(args[i],"<")==0){  //Input redirection
            if(args[i+1]==NULL){
                fprintf(stderr,"Error: no input file specified\n");
                exit(EXIT_FAILURE);
            }
            *input_fd=open(args[i+1], O_RDONLY);
            if(*input_fd==-1){
                perror("open input");
                exit(EXIT_FAILURE);
            }
            dup2(*input_fd, STDIN_FILENO);
            args[i]=NULL;
            break; // stop parsing after redirection
        }
        else if(strcmp(args[i], ">")==0 || strcmp(args[i], ">>")==0){  //Output redirection
            if(args[i+1]==NULL){
                fprintf(stderr,"Error: no output file specified\n");
                exit(EXIT_FAILURE);
            }
            int flags = O_WRONLY | O_CREAT;
            if(strcmp(args[i], ">>")==0)
                flags |= O_APPEND;
            else
                flags |= O_TRUNC;

            *output_fd=open(args[i+1], flags, 0644);
            if(*output_fd==-1){
                perror("open output");
                exit(EXIT_FAILURE);
            }
            dup2(*output_fd, STDOUT_FILENO);
            args[i]=NULL;
            break; // stop parsing after redirection
        }
    }
}

int main(){
    char input[1024];
    char *args[MAX_ARGS];
    char cwd[PATH_MAX];
    
    //default PATH
    path=strdup("/usr/bin:/bin:/usr/sbin:/sbin");
    
    while(1){
        //cwd for prompt
        if(getcwd(cwd, sizeof(cwd))){
            if(prompt==NULL){
                //Default prompt with CWD
                printf("%s$ ",cwd);
            }else{
                printf("%s ", prompt);
            }
        }else{
            perror("getcwd");
            printf("$ ");
        }
        
        //Read input
        if(fgets(input, sizeof(input), stdin)==NULL){
            printf("\n");
            break;  
        }
        
        //Remove newline
        input[strcspn(input, "\n")]='\0';
        
        //Skip empty input
        if(strlen(input)==0){
            continue;
        }
        
        //exit command
        if(strcmp(input, "exit")==0){
            break;
        }
        
        //Handle PS1="value" and PATH="value"
        if(strncmp(input, "PS1=", 4)==0){
            char *value=input+4;
            
            //Remove surrounding quotes 
            if(value[0]== '"' && value[strlen(value)-1]=='"'){
                value[strlen(value)-1]='\0';
                value++;
            }
            
            char *ps1_args[]={"PS1", value, NULL};
            handle_ps1(ps1_args);
            continue;
        }else if(strncmp(input, "PATH=", 5)==0){
            char *value=input+5;
            
            //Remove surrounding quotes
            if(value[0]=='"' && value[strlen(value)-1]=='"'){
                value[strlen(value)-1]='\0';
                value++;
            }
            
            char *path_args[]={"PATH", value, NULL};
            handle_path(path_args);
            continue;
        }

        //Tokenize input
        char *token=strtok(input, " ");
        int i=0;
        while(token!=NULL && i<MAX_ARGS-1){
            args[i++]=token;
            token=strtok(NULL, " ");
        }
        args[i]=NULL;
        
        //builtin commands
        if(strcmp(args[0], "cd")==0){
            handle_cd(args);
            continue;
        }
        
        if(strcmp(args[0], "PATH")==0){
            handle_path(args);
            continue;
        }
        
        if(strcmp(args[0], "PS1")==0){
            handle_ps1(args);
            continue;
        }
        
        //external command
        execute_command(args);
    }
    free(prompt);
    free(path);
}

