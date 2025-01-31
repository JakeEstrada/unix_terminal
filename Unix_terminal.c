#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>  
#include <limits.h> 
#include <linux/limits.h>

#define MAX_LINE_LENGTH 512
#define clear() printf("\033[H\033[J")  

void print_help() {
    printf("\n\n        Home Bread Help Menu    \n");
    printf("************************************************\n");
    printf(" Home Bread is a spoof of home brew.\n");
    printf(" For people who love home-made bread.\n");
    printf(" Meant to mimic the use cases of a typical terminal,\n");
    printf(" but hot and fresh with a soft doughy center!!\n");
    printf("************************************************\n");
    printf("Available Commands:\n");
    printf("-----------------------------------------------\n");
    printf("cd <path>       - Change the current directory to <path>\n");
    printf("mkdir <dir>     - Create a new directory named <dir>\n");
    printf("pwd             - Print the current working directory\n");
    printf("ls              - List the files in the current directory\n");
    printf("echo <text>     - Display <text> on the terminal\n");
    printf("mv <src> <dest> - Move/rename a file from <src> to <dest>\n");
    printf("kill <pid>      - Terminate the process with the given <pid>\n");
    printf("ps              - Display the current active processes\n");
    printf("ping <url>      - Ping a network host with the given URL\n");
    printf("more <file>     - View the content of <file> one page at a time\n");
    printf("diff <file1> <file2> - Compare two files and show the differences\n");
    printf("!!              - Repeat the last command executed\n");
    printf("which <program> - Locate the program's executable path\n");
    printf("exit            - Exit the shell\n");
    printf("help            - Display this help menu\n");
    printf("-----------------------------------------------\n");
    printf(" Additional Features:\n");
    printf(" - Pipes (|): Combine two commands where the output of the first\n");
    printf("   command serves as the input to the second (e.g., ls | wc -l)\n");
    printf(" - Redirection (>): Redirect the output of a command to a file\n");
    printf("   (e.g., ls > output.txt)\n");
    printf("************************************************\n");
}


// Function to parse the input
void parse(char *line, char **argv) {
    while (*line != '\0') {   // not EOLN
        while (*line == ' ' || *line == '\t' || *line == '\n') {
            *line++ = '\0';   // replace white spaces with null
        }
        *argv++ = line;       // save the argument position
        while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n') {
            line++;           // skip the argument until ...
        }
    }
    *argv = '\0';             // mark the end of the argument list
}

// Function to execute commands
void execute(char **argv) {
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {   // fork a child process
        printf("*** ERROR: forking child process failed\n");
        exit(1);
    } else if (pid == 0) {      // for the child process
        if (execvp(argv[0], argv) < 0) {   // execute the command
            printf("*** ERROR: exec failed\n");
            exit(1);
        }
    } else {                    // for the parent
        while (wait(&status) != pid);    // wait for completion
    }
}

// prints SPACE where there are spaces with echo
void handle_echo(char **argv) {
    for (int i = 1; argv[i] != NULL; i++) {
        if (i > 1) {
            printf("SPACE");
        }
        printf("%s", argv[i]);
    }
    printf("\n");
}

// Function to handle ECHO (all caps)
void handle_ECHO(char **argv) {
    for (int i = 1; argv[i] != NULL; i++) {
        printf("%s\n", argv[i]);
    }
}

// Function to handle redirection ls < output.txt
void execute_with_redirection(char **argv, char *filename, int direction) {
    pid_t pid;
    int status;
    int fd;

    if ((pid = fork()) < 0) {
        printf("*** ERROR: forking child process failed\n");
        exit(1);
    } else if (pid == 0) {
        // Redirection for output >
        if (direction == 1) {
            fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);  // Open the file for writing
            if (fd < 0) {
                perror("open");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);  // Redirect stdout to the file
        } 
        // Redirection for input <
        else if (direction == 0) {
            fd = open(filename, O_RDONLY);  // Open the file for reading
            if (fd < 0) {
                perror("open");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);  // Redirect stdin to the file
        }
        close(fd);

        execvp(argv[0], argv);  // Execute the command
        perror("execvp failed");
        exit(1);
    } else {  // parent process
        while (wait(&status) != pid);  // waiting for child process
    }
}

// Function is in place because i was having issues with white spaces
// the terminal reads these the same -> "     ls -l     " == "ls -l"
char* trim_whitespace(char* str) {
    while (*str == ' ') str++;  // Trim leading spaces
    char* end = str + strlen(str) - 1;
    while (end > str && *end == ' ') end--;  // Trim trailing spaces
    *(end + 1) = '\0';
    return str;
}
// Function to handle pipes
void execute_with_pipe(char **cmd1, char **cmd2) {
    int pipefd[2];
    pid_t pid1, pid2;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    // First child - first command
    if ((pid1 = fork()) == 0) {
        dup2(pipefd[1], STDOUT_FILENO);  // send output to pipe
        close(pipefd[0]);  // Close read end 
        close(pipefd[1]);  // Close write end
        execvp(cmd1[0], cmd1);  // Execute 
        perror("execvp");
        exit(1);
    }

    // Second child - second command
    if ((pid2 = fork()) == 0) {
        dup2(pipefd[0], STDIN_FILENO);  // send output to pipe
        close(pipefd[1]);  // Close write 
        close(pipefd[0]);  // Close read 
        execvp(cmd2[0], cmd2);  // Execute
        perror("execvp");
        exit(1);
    }

    //Parent process
    //close both ends of the pipe and wait for children
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}


// Function to handle built-in commands 
int handle_builtin_commands(char **argv) {
    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL) {
            printf("cd: expected argument\n");
        } else {
            if (chdir(argv[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    } else if (strcmp(argv[0], "pwd") == 0) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
        return 1;
    } else if (strcmp(argv[0], "kill") == 0) {
        if (argv[1] == NULL) {
            printf("kill: expected pid\n");
        } else {
            pid_t pid = atoi(argv[1]);
            if (kill(pid, SIGKILL) != 0) {
                perror("kill");
            }
        }
        return 1;
    } else if (strcmp(argv[0], "echo") == 0) {
        handle_echo(argv);
        return 1;
    } else if (strcmp(argv[0], "ECHO") == 0) {
        handle_ECHO(argv);
        return 1;
    }
    return 0;  
}


int main(void) {
    char line[1024];  
    char lastCommand[1024] = ""; 
    char cwd[MAX_LINE_LENGTH];  
    char *argv[64];    



    
    while (1) {
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("getcwd() error");
            continue;
        }
        printf("\033[38;5;239mHome@bread > \033[38;5;51m%s$ \033[0m", cwd);
        fgets(line, sizeof(line), stdin);
        line[strlen(line) - 1] = '\0'; // Remove trailing newline

        // Handle the !! command
        if (strcmp(line, "!!") == 0) {
            if (strlen(lastCommand) == 0) {
                printf("*** Error: No previous commands to repeat!\n");
                continue;
            } else {
                strcpy(line, lastCommand);
            }
        }

        // Store the current command
        strcpy(lastCommand, line);

        // Handle pipe
        char *cmd1[64], *cmd2[64];
        char *pipe_position = strstr(line, "|");
        if (pipe_position != NULL) {
            *pipe_position = '\0';
            char *left_command = trim_whitespace(line);
            char *right_command = trim_whitespace(pipe_position + 1);
            parse(left_command, cmd1);
            parse(right_command, cmd2);
            execute_with_pipe(cmd1, cmd2);
            continue;
        }

        // Handle redirection
        char *redir_position = strstr(line, ">");
        if (redir_position != NULL) {
            *redir_position = '\0';
            char *left_command = trim_whitespace(line);
            char *filename = trim_whitespace(redir_position + 1);
            parse(left_command, argv);
            execute_with_redirection(argv, filename, 1);
            continue;
        }

        // Parse and handle commands
        parse(line, argv);

        if (handle_builtin_commands(argv)) {
            continue;
        }

        if (strcmp(argv[0], "exit") == 0) {
            exit(0);
        }

        if (strcmp(argv[0], "help") == 0) {
            print_help();
            continue;
        }

        execute(argv);
    }

    return 0;
}
