#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <termios.h>

#define MAX_ARGS 64
#define MAX_PATH_LENGTH 4096
#define MAX_JOBS 100
#define HISTORY_SIZE 100

typedef struct job {
    pid_t pid;
    int job_id;
    char *command;
    int status; // 0=running, 1=stopped, 2=done
} job_t;

char *prompt = NULL;
char *custom_path = NULL;
job_t jobs[MAX_JOBS];
int job_count = 0;
int next_job_id = 1;
char *history[HISTORY_SIZE];
int history_count = 0;
pid_t foreground_pid = 0;

void sigint_handler(int sig);
void sigtstp_handler(int sig);
void sigchld_handler(int sig);
void setup_signals();
void execute_command(char **args);
void execute_pipeline(char ***commands, int num_commands);
void handle_cd(char **args);
void handle_path(char **args);
void handle_ps1(char **args);
void handle_fg(char **args);
void handle_bg(char **args);
void handle_jobs(char **args);
void handle_history(char **args);
int find_executable(char *command, char *full_path);
void setup_redirection(char **args, int *input_fd, int *output_fd);
char **tokenize_input(char *input, int *arg_count);
char ***parse_pipeline(char **args, int *num_commands);
void free_tokens(char **tokens);
void free_pipeline(char ***pipeline, int num_commands);
void add_job(pid_t pid, char *command);
void remove_job(int job_id);
job_t *find_job_by_pid(pid_t pid);
job_t *find_job_by_id(int job_id);
void update_job_status();
void add_to_history(char *command);

void sigint_handler(int sig) {
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGINT);
    }
    printf("\n");
}

void sigtstp_handler(int sig) {
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGTSTP);
        job_t *job = find_job_by_pid(foreground_pid);
        if (job) {
            job->status = 1;
            printf("\n[%d]+ Stopped %s\n", job->job_id, job->command);
        }
        foreground_pid = 0;
    }
    printf("\n");
}

void sigchld_handler(int sig) {
    pid_t pid;
    int status;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        job_t *job = find_job_by_pid(pid);
        if (job) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                job->status = 2;
            }
        }
    }
}

void setup_signals() {
    struct sigaction sa_int, sa_tstp, sa_chld;
    
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);
    
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = 0;
    sigaction(SIGTSTP, &sa_tstp, NULL);
    
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa_chld, NULL);
}

void execute_pipeline(char ***commands, int num_commands) {
    if (num_commands == 1) {
        execute_command(commands[0]);
        return;
    }
    
    int pipes[num_commands - 1][2];
    pid_t pids[num_commands];
    
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return;
        }
    }
    
    // Build command string for job tracking
    char pipeline_cmd[512] = {0};
    for (int i = 0; i < num_commands; i++) {
        for (int j = 0; commands[i][j] != NULL; j++) {
            strncat(pipeline_cmd, commands[i][j], sizeof(pipeline_cmd) - strlen(pipeline_cmd) - 1);
            if (commands[i][j+1] != NULL) {
                strncat(pipeline_cmd, " ", sizeof(pipeline_cmd) - strlen(pipeline_cmd) - 1);
            }
        }
        if (i < num_commands - 1) {
            strncat(pipeline_cmd, " | ", sizeof(pipeline_cmd) - strlen(pipeline_cmd) - 1);
        }
    }
    
    for (int i = 0; i < num_commands; i++) {
        pids[i] = fork();
        
        if (pids[i] == 0) {
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            if (i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            char full_path[PATH_MAX];
            if (find_executable(commands[i][0], full_path)) {
                execv(full_path, commands[i]);
                perror("execv");
            }
            exit(EXIT_FAILURE);
        } else if (pids[i] < 0) {
            perror("fork");
            return;
        }
    }
    
    for (int i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Add the first process as a job (representative of the pipeline)
    add_job(pids[0], pipeline_cmd);
    
    foreground_pid = pids[0];
    int status;
    int stopped = 0;
    
    for (int i = 0; i < num_commands; i++) {
        waitpid(pids[i], &status, WUNTRACED);
        if (WIFSTOPPED(status)) {
            stopped = 1;
        }
    }
    
    if (stopped) {
        job_t *job = find_job_by_pid(pids[0]);
        if (job) {
            job->status = 1;
            printf("\n[%d]+ Stopped %s\n", job->job_id, job->command);
        }
    } else {
        // Pipeline completed normally, remove from job list
        job_t *job = find_job_by_pid(pids[0]);
        if (job) {
            remove_job(job->job_id);
        }
    }
    
    foreground_pid = 0;
}

void execute_command(char **args) {
    if (!args[0]) return;

    pid_t pid = fork();
    
    if (pid == 0) {
        int input_fd = -1, output_fd = -1;
        
        setup_redirection(args, &input_fd, &output_fd);
        
        char full_path[PATH_MAX];
        if (find_executable(args[0], full_path)) {
            execv(full_path, args);
            perror("execv");
        }
        
        if (input_fd != -1) close(input_fd);
        if (output_fd != -1) close(output_fd);
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Add job to job list
        char cmd_str[256] = {0};
        for (int i = 0; args[i] != NULL; i++) {
            strncat(cmd_str, args[i], sizeof(cmd_str) - strlen(cmd_str) - 1);
            if (args[i+1] != NULL) {
                strncat(cmd_str, " ", sizeof(cmd_str) - strlen(cmd_str) - 1);
            }
        }
        add_job(pid, cmd_str);
        
        foreground_pid = pid;
        int status;
        waitpid(pid, &status, WUNTRACED);
        
        // Check if process was stopped
        if (WIFSTOPPED(status)) {
            job_t *job = find_job_by_pid(pid);
            if (job) {
                job->status = 1;
                printf("\n[%d]+ Stopped %s\n", job->job_id, job->command);
            }
        } else {
            // Process completed normally, remove from job list
            job_t *job = find_job_by_pid(pid);
            if (job) {
                remove_job(job->job_id);
            }
        }
        foreground_pid = 0;
    } else {
        perror("fork");
    }
}

void handle_cd(char **args) {
    if (args[1] == NULL) {
        char *home = getenv("HOME");
        if (home == NULL) {
            return;
        }
        if (chdir(home) != 0) {
            perror("cd");
        }
    } else {
        if (chdir(args[1]) != 0) {
            perror("cd");
        }
    }
}

void handle_path(char **args) {
    if (args[1] == NULL) {
        if (custom_path) {
            printf("PATH=%s\n", custom_path);
        } else {
            char *env_path = getenv("PATH");
            if (env_path) {
                printf("PATH=%s\n", env_path);
            } else {
                printf("PATH not set\n");
            }
        }
    } else {
        free(custom_path);
        custom_path = strdup(args[1]);
        if (custom_path == NULL) {
            perror("strdup");
        }
    }
}

void handle_ps1(char **args) {
    if (args[1] == NULL) {
        if (prompt) {
            printf("PS1=%s\n", prompt);
        } else {
            printf("PS1=\\w$\n");
        }
    } else if (strcmp(args[1], "\\w$") == 0) {
        free(prompt);
        prompt = NULL;
    } else {
        free(prompt);
        prompt = strdup(args[1]);
        if (prompt == NULL) {
            perror("strdup");
        }
    }
}

void handle_fg(char **args) {
    update_job_status();
    
    job_t *job = NULL;
    if (args[1] == NULL) {
        for (int i = job_count - 1; i >= 0; i--) {
            if (jobs[i].status == 1) {
                job = &jobs[i];
                break;
            }
        }
    } else {
        int job_id = atoi(args[1]);
        job = find_job_by_id(job_id);
    }
    
    if (job && job->status == 1) {
        printf("%s\n", job->command);
        job->status = 0;
        foreground_pid = job->pid;
        kill(job->pid, SIGCONT);
        
        int status;
        waitpid(job->pid, &status, WUNTRACED);
        
        if (WIFSTOPPED(status)) {
            job->status = 1;
            printf("\n[%d]+ Stopped %s\n", job->job_id, job->command);
        } else {
            remove_job(job->job_id);
        }
        foreground_pid = 0;
    }
}

void handle_bg(char **args) {
    update_job_status();
    
    job_t *job = NULL;
    if (args[1] == NULL) {
        for (int i = job_count - 1; i >= 0; i--) {
            if (jobs[i].status == 1) {
                job = &jobs[i];
                break;
            }
        }
    } else {
        int job_id = atoi(args[1]);
        job = find_job_by_id(job_id);
    }
    
    if (job && job->status == 1) {
        job->status = 0;
        printf("[%d]+ %s &\n", job->job_id, job->command);
        kill(job->pid, SIGCONT);
    }
}

void handle_jobs(char **args) {
    update_job_status();
    
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].status != 2) {
            printf("[%d]%c %s %s\n", 
                jobs[i].job_id, 
                (i == job_count - 1) ? '+' : ' ',
                (jobs[i].status == 0) ? "Running" : "Stopped",
                jobs[i].command);
        }
    }
}

void handle_history(char **args) {
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

int find_executable(char *command, char *full_path) {
    if (strchr(command, '/') != NULL) {
        if (access(command, X_OK) == 0) {
            strncpy(full_path, command, PATH_MAX - 1);
            full_path[PATH_MAX - 1] = '\0';
            return 1;
        }
        return 0;
    }

    char *search_path = custom_path;
    if (search_path == NULL) {
        search_path = getenv("PATH");
        if (search_path == NULL) {
            search_path = "/usr/bin:/bin:/usr/sbin:/sbin";
        }
    }

    char *path_copy = strdup(search_path);
    if (path_copy == NULL) {
        perror("strdup");
        return 0;
    }

    char *dir = strtok(path_copy, ":");
    int found = 0;
    
    while (dir != NULL) {
        snprintf(full_path, PATH_MAX, "%s/%s", dir, command);
        if (access(full_path, X_OK) == 0) {
            found = 1;
            break;
        }
        dir = strtok(NULL, ":");
    }
    
    free(path_copy);
    return found;
}

void setup_redirection(char **args, int *input_fd, int *output_fd) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            if (args[i + 1] == NULL) {
                exit(EXIT_FAILURE);
            }
            *input_fd = open(args[i + 1], O_RDONLY);
            if (*input_fd == -1) {
                exit(EXIT_FAILURE);
            }
            if (dup2(*input_fd, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            args[i] = NULL;
        } else if (strcmp(args[i], ">") == 0) {
            if (args[i + 1] == NULL) {
                exit(EXIT_FAILURE);
            }
            *output_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (*output_fd == -1) {
                perror("open output");
                exit(EXIT_FAILURE);
            }
            if (dup2(*output_fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            args[i] = NULL;
        } else if (strcmp(args[i], ">>") == 0) {
            if (args[i + 1] == NULL) {
                exit(EXIT_FAILURE);
            }
            *output_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (*output_fd == -1) {
                perror("open output");
                exit(EXIT_FAILURE);
            }
            if (dup2(*output_fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            args[i] = NULL;
        }
    }
}

char **tokenize_input(char *input, int *arg_count) {
    char **tokens = malloc(MAX_ARGS * sizeof(char *));
    if (tokens == NULL) {
        perror("malloc");
        return NULL;
    }

    int count = 0;
    char *token = strtok(input, " \t\n");
    
    while (token != NULL && count < MAX_ARGS - 1) {
        tokens[count] = strdup(token);
        if (tokens[count] == NULL) {
            perror("strdup");
            for (int i = 0; i < count; i++) {
                free(tokens[i]);
            }
            free(tokens);
            return NULL;
        }
        count++;
        token = strtok(NULL, " \t\n");
    }
    
    tokens[count] = NULL;
    *arg_count = count;
    return tokens;
}

char ***parse_pipeline(char **args, int *num_commands) {
    int pipe_count = 0;
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipe_count++;
        }
    }
    
    *num_commands = pipe_count + 1;
    char ***commands = malloc(*num_commands * sizeof(char **));
    if (commands == NULL) {
        perror("malloc");
        return NULL;
    }
    
    int cmd_idx = 0;
    int start = 0;
    
    for (int i = 0; args[i] != NULL || cmd_idx < *num_commands; i++) {
        if (args[i] == NULL || strcmp(args[i], "|") == 0) {
            int cmd_len = i - start;
            commands[cmd_idx] = malloc((cmd_len + 1) * sizeof(char *));
            if (commands[cmd_idx] == NULL) {
                perror("malloc");
                for (int j = 0; j < cmd_idx; j++) {
                    free(commands[j]);
                }
                free(commands);
                return NULL;
            }
            
            for (int j = 0; j < cmd_len; j++) {
                commands[cmd_idx][j] = args[start + j];
            }
            commands[cmd_idx][cmd_len] = NULL;
            
            cmd_idx++;
            start = i + 1;
            
            if (args[i] == NULL) break;
        }
    }
    
    return commands;
}

void free_tokens(char **tokens) {
    if (tokens == NULL) return;
    
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

void free_pipeline(char ***pipeline, int num_commands) {
    if (pipeline == NULL) return;
    
    for (int i = 0; i < num_commands; i++) {
        free(pipeline[i]);
    }
    free(pipeline);
}

void add_job(pid_t pid, char *command) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        jobs[job_count].job_id = next_job_id++;
        jobs[job_count].command = strdup(command);
        jobs[job_count].status = 0;
        job_count++;
    }
}

void remove_job(int job_id) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].job_id == job_id) {
            free(jobs[i].command);
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            break;
        }
    }
}

job_t *find_job_by_pid(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            return &jobs[i];
        }
    }
    return NULL;
}

job_t *find_job_by_id(int job_id) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].job_id == job_id) {
            return &jobs[i];
        }
    }
    return NULL;
}

void update_job_status() {
    for (int i = job_count - 1; i >= 0; i--) {
        if (jobs[i].status == 2) {
            remove_job(jobs[i].job_id);
        }
    }
}

void add_to_history(char *command) {
    if (history_count < HISTORY_SIZE) {
        history[history_count] = strdup(command);
        history_count++;
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(command);
    }
}

int main() {
    char input[1024];
    char cwd[PATH_MAX];
    
    setup_signals();
    
    while (1) {
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("getcwd");
            printf("$ ");
        } else {
            if (prompt == NULL) {
                printf("%s$ ", cwd);
            } else {
                printf("%s", prompt);
            }
        }
        
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }
        
        input[strcspn(input, "\n")] = '\0';
        
        if (strlen(input) == 0) {
            continue;
        }
        
        if (strcmp(input, "exit") == 0) {
            break;
        }
        
        add_to_history(input);
        
        int arg_count = 0;
        char **args = tokenize_input(input, &arg_count);
        if (args == NULL || arg_count == 0) {
            free_tokens(args);
            continue;
        }
        
        if (strcmp(args[0], "cd") == 0) {
            handle_cd(args);
        } else if (strcmp(args[0], "PATH") == 0) {
            handle_path(args);
        } else if (strcmp(args[0], "PS1") == 0) {
            handle_ps1(args);
        } else if (strcmp(args[0], "fg") == 0) {
            handle_fg(args);
        } else if (strcmp(args[0], "bg") == 0) {
            handle_bg(args);
        } else if (strcmp(args[0], "jobs") == 0) {
            handle_jobs(args);
        } else if (strcmp(args[0], "history") == 0) {
            handle_history(args);
        } else {
            int num_commands;
            char ***pipeline = parse_pipeline(args, &num_commands);
            if (pipeline) {
                execute_pipeline(pipeline, num_commands);
                free_pipeline(pipeline, num_commands);
            }
        }
        
        free_tokens(args);
    }
    
    free(prompt);
    free(custom_path);
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }
    for (int i = 0; i < job_count; i++) {
        free(jobs[i].command);
    }
    
    return 0;
}
