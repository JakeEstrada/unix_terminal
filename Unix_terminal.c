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
#include <termios.h>
#include <ctype.h>
#include <pwd.h>

#define MAX_LINE_LENGTH 512
#define MAX_HISTORY 1000
#define clear() printf("\033[H\033[J")  

// Global variables for command history
char *command_history[MAX_HISTORY];
int history_count = 0;
int current_history = 0;

// Function to save command to history
void save_to_history(const char *command) {
    if (strlen(command) == 0) return;
    
    // Free the oldest command if we've reached max history
    if (history_count >= MAX_HISTORY) {
        free(command_history[0]);
        for (int i = 0; i < history_count - 1; i++) {
            command_history[i] = command_history[i + 1];
        }
        history_count--;
    }
    
    // Save the new command
    command_history[history_count] = strdup(command);
    history_count++;
    current_history = history_count;
}

// Function to get shortened path
void get_shortened_path(char *path, char *short_path, int max_depth) {
    char *home = getenv("HOME");
    if (home == NULL) {
        struct passwd *pw = getpwuid(getuid());
        if (pw != NULL) {
            home = pw->pw_dir;
        }
    }
    
    // First, resolve any .. in the path
    char resolved_path[PATH_MAX];
    if (realpath(path, resolved_path) == NULL) {
        strcpy(resolved_path, path);
    }
    
    // Check if we're in the home directory
    if (home != NULL && strncmp(resolved_path, home, strlen(home)) == 0) {
        // Replace home directory with ~
        sprintf(short_path, "~%s", resolved_path + strlen(home));
    } else {
        // For paths outside home, show the last max_depth components
        char *components[PATH_MAX];
        int comp_count = 0;
        char *p = resolved_path;
        
        // Split path into components
        while ((p = strchr(p, '/')) != NULL) {
            p++; // Skip the '/'
            if (*p != '\0') {
                components[comp_count++] = p;
            }
        }
        
        if (comp_count <= max_depth) {
            // If we have fewer components than max_depth, show the full path
            strcpy(short_path, resolved_path);
        } else {
            // Show the last max_depth components
            short_path[0] = '/';
            int pos = 1;
            for (int i = comp_count - max_depth; i < comp_count; i++) {
                char *next_slash = strchr(components[i], '/');
                int len = next_slash ? next_slash - components[i] : strlen(components[i]);
                strncpy(short_path + pos, components[i], len);
                pos += len;
                if (i < comp_count - 1) {
                    short_path[pos++] = '/';
                }
            }
            short_path[pos] = '\0';
        }
    }
}

// Function to handle arrow key navigation
void handle_arrow_keys(char *line, int *line_pos) {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return;
    if (c == '[') {
        if (read(STDIN_FILENO, &c, 1) != 1) return;
        if (c == 'A') { // Up arrow
            if (current_history > 0) {
                // Clear current line
                printf("\r\033[K");
                // Move to previous command
                current_history--;
                strcpy(line, command_history[current_history]);
                *line_pos = strlen(line);
                // Reprint prompt and command
                char short_path[PATH_MAX];
                get_shortened_path(getcwd(NULL, 0), short_path, 3);
                printf("\033[38;5;239mHome@bread > \033[38;5;51m%s$ \033[0m%s", short_path, line);
                fflush(stdout);
            }
        } else if (c == 'B') { // Down arrow
            if (current_history < history_count - 1) {
                // Clear current line
                printf("\r\033[K");
                // Move to next command
                current_history++;
                strcpy(line, command_history[current_history]);
                *line_pos = strlen(line);
                // Reprint prompt and command
                char short_path[PATH_MAX];
                get_shortened_path(getcwd(NULL, 0), short_path, 3);
                printf("\033[38;5;239mHome@bread > \033[38;5;51m%s$ \033[0m%s", short_path, line);
                fflush(stdout);
            } else if (current_history == history_count - 1) {
                // Clear current line
                printf("\r\033[K");
                // Clear the line when we reach the end of history
                line[0] = '\0';
                *line_pos = 0;
                // Reprint prompt
                char short_path[PATH_MAX];
                get_shortened_path(getcwd(NULL, 0), short_path, 3);
                printf("\033[38;5;239mHome@bread > \033[38;5;51m%s$ \033[0m", short_path);
                fflush(stdout);
            }
        }
    }
}

// Function to set terminal to raw mode
void set_terminal_raw() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to restore terminal settings
void restore_terminal() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

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
    int line_pos = 0;
    
    // Set terminal to raw mode
    set_terminal_raw();
    
    while (1) {
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("getcwd() error");
            continue;
        }
        
        char short_path[PATH_MAX];
        get_shortened_path(cwd, short_path, 3);
        printf("\033[38;5;239mHome@bread > \033[38;5;51m%s$ \033[0m", short_path);
        fflush(stdout);
        
        // Read input character by character
        line_pos = 0;
        memset(line, 0, sizeof(line));  // Clear the line buffer
        
        while (1) {
            char c;
            if (read(STDIN_FILENO, &c, 1) != 1) continue;
            
            if (c == '\n') {
                printf("\n");
                line[line_pos] = '\0';
                break;
            } else if (c == '\033') { // Escape sequence
                handle_arrow_keys(line, &line_pos);
            } else if (c == 127 || c == '\b') { // Backspace
                if (line_pos > 0) {
                    line_pos--;
                    printf("\b \b");
                    fflush(stdout);
                }
            } else if (isprint(c)) {
                if (line_pos < MAX_LINE_LENGTH - 1) {
                    line[line_pos++] = c;
                    printf("%c", c);
                    fflush(stdout);
                }
            }
        }
        
        // Only save non-empty commands to history
        if (strlen(line) > 0) {
            save_to_history(line);
        }
        
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
    
    // Restore terminal settings before exit
    restore_terminal();
    return 0;
}
