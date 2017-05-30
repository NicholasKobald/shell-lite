/*
* Nicholas Kobald
* @Date:   24-May-2017
* @Email:  nick.kobald@gmail.com
*
* TODO:
*      -NEXT: Fix filenames (including .uvsrc)
*      -need to be printing error to stderr
*      -all my execute_command functions need to do some error checking
*      -extra work??
*           background process?
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

#define max_args 10 //well, 9, but we're tokenizing the command itself
#define max_line_length 80
#define max_prompt_length 10
#define max_directory_len 10
#define max_directory_string_len 100

/* File IO */
int read_default_dirs(FILE *fp, char default_directories[max_directory_len][max_directory_string_len]);
int load_prompt(FILE *fp, char prefix[10]);

/* Debugging and print functions */
void print_args(char **args, int num_tokens);
void print_default_dirs(char default_directories[max_directory_len][max_directory_string_len], int count);
void print_tokens(char tokens[max_args][max_line_length], int num_tokens);
void print_variable_args(char *args[]);
void dump_searched_in(char *args[max_args], char default_directories[max_directory_len][max_directory_string_len], char candidate_path[max_line_length], int num_dir, char target_command[max_line_length]);

/* String logic and house keeping */
int validate_tokens(char tokens[max_args][max_line_length], int num_tokens);
int populate_exec_args(char** args, char tokens[max_args][max_line_length], int num_tokens);
int tokenize_command_string(char str[max_line_length], char tokenized[max_args][max_line_length]);
void normalize_space(char current_command_string[max_line_length]);
void init(char ccs[max_line_length], char ts[max_args][max_line_length]);

/* Command and directory logic */
int check_path(char *arg[]);
int find_command(char *args[max_args], char default_directories[max_directory_len][max_directory_string_len], char candidate_path[max_line_length], int num_dir);
int check_command_type(char* args[]);
int execute_command(char **args);
int complete_local_path( char *args[max_args], char completed_path[max_line_length]);
int complete_home_path(char *args[max_args], char completed_path[max_line_length]);
int handle_command(char *args[max_args], char default_directories[max_directory_len][max_directory_string_len], char completed_path[max_line_length],  int cmd_type, int num_dir);

int execute_vanilla_command(char *args[]);
int execute_do_out_command(char *args[]);
int execute_do_pipe_command(char *args[]);

typedef enum {
    DEFAULT,
	DO_PIPE,
    DO_OUT
} control_flag;

control_flag current_control_flag = DEFAULT;

/*
 * if control flag is none-default, pipe_index will be the Index
 * of the '::' symbol. Otherwise, pipe_index will be -1.
 */
int PIPE_INDEX = -1;

extern char **environ;

int main() {
    FILE *fp;
    fp = fopen(".uvshrc", "r"); //TODO: change this to the right name. Also close this file.

    char default_directories[max_directory_len][max_directory_string_len];
    char prompt[max_prompt_length];
    load_prompt(fp, prompt);
    int num_directories = read_default_dirs(fp, default_directories);

    char current_command_string[max_line_length];
    char tokenized_string[max_args][max_line_length];
    char *exec_args[max_args];
    char completed_path[max_line_length];
    char completed_path_target[max_line_length];

	int command_type;
    int num_tokens;
    int valid = 0;
    int valid_pipe_command = 0;

    for (;;) {
        current_control_flag = DEFAULT;
        init(current_command_string, tokenized_string);
        printf("%s", prompt);
        fgets(current_command_string, max_line_length, stdin);
        normalize_space(current_command_string);
        num_tokens = tokenize_command_string(current_command_string, tokenized_string);

        if (num_tokens > 0 && populate_exec_args(exec_args, tokenized_string, num_tokens) == 1) {
            command_type = check_command_type(exec_args);
            valid = handle_command(exec_args, default_directories, completed_path,  command_type, num_directories);
            if (current_control_flag == DO_PIPE) {
                command_type = check_command_type(&exec_args[PIPE_INDEX + 1]);
                valid_pipe_command = handle_command(&exec_args[PIPE_INDEX + 1], default_directories, completed_path_target, command_type, num_directories);
            }
            if (valid == 1 && (current_control_flag != DO_PIPE || valid_pipe_command == 1))
                execute_command(exec_args);
        }
    }
}

/*
 *  @param args - pointer to a char *. First address contains the command,
 *                  followed by a list of argvoid dump_searched_in(char *args[max_args], char default_directories[max_directory_len][max_directory_string_len], char candidate_path[max_line_length], int num_dir) {
uements
 *  @side effect:
 *          modify pointer of args[0] to point at a the absolute path of an
 *          executeable.
 *
 *  @side effect:
 *          WILL EXIT PROGRAM IF VALUE OF CMD IS -1
 */
int handle_command(char *args[max_args], char default_directories[max_directory_len][max_directory_string_len], char completed_path[max_line_length], int cmd_type, int num_dir)  {
    int i;
    int ret_val = 0;
    for (i = 0; i < max_line_length; i++) {
        completed_path[i] = '\0';
    }
    if (cmd_type == -1)
        exit(EXIT_SUCCESS);

    //abs path case
    if (cmd_type == 0)
        ret_val = check_path(args);

    //local directory case
    if (cmd_type == 1)
        ret_val = complete_local_path(args, completed_path);

    //home '~' case.
    if (cmd_type == 2)
        ret_val = complete_home_path(args, completed_path);

    //inferred path case (look in directories)
    if (cmd_type == 3)
        ret_val = find_command(args, default_directories, completed_path, num_dir);

    if (ret_val == 0) {
        fprintf(stderr, "%s: command not found\n", args[0]);
        return ret_val;
    }
    return ret_val;
}

int check_path(char *args[]) {
    if (access(args[0], X_OK) != -1) {
        return 1;
    }
    return 0;
}

/*
 * modify args to have a complete path.
 */
int complete_home_path(char *args[max_args], char completed_path[max_line_length]) {
    strcpy(completed_path, getenv("HOME"));
    strcat(completed_path, args[0] + 1); // +1 to remove the tilde
    if (access(completed_path, X_OK) != -1) {
        args[0] = completed_path;
        return 1;
    }
    return 0;
}

/*
 * Given a local path, ie starting with a '.', complete it by getting the CWD and executing an absolute path
 */
int complete_local_path(char *args[max_args], char completed_path[max_line_length]) {
    getcwd(completed_path, max_line_length);
    strcat(completed_path, args[0] + 1); // + 1 to skip the dot
    if (access(completed_path, X_OK) != -1) {
        args[0] = completed_path;
        return 1;
    }
    return 0;
}

int find_command(char *args[max_args], char default_directories[max_directory_len][max_directory_string_len], char candidate_path[max_line_length], int num_dir) {
    char target_command[max_line_length];
    int i;
    for (i = 0; i < max_line_length; i++) {
        candidate_path[i] = '\0';
        target_command[i] = '\0';
    }

    strcpy(target_command, "/");
    strcat(target_command, args[0]);

    for (i = 0; i < num_dir; i++) {
        strcpy(candidate_path, default_directories[i]);
        strcat(candidate_path, target_command);
        if (access(candidate_path, X_OK) != -1) {
            args[0] = candidate_path;
            return 1;
        }
    }
    dump_searched_in(args, default_directories, candidate_path, num_dir, target_command);
    return 0;
}

void dump_searched_in(char *args[max_args], char default_directories[max_directory_len][max_directory_string_len],
                char candidate_path[max_line_length], int num_dir, char target_command[max_line_length]) {
    int i;
    for (i = 0; i < num_dir; i++) {
        strcpy(candidate_path, default_directories[i]);
        strcat(candidate_path, target_command);
        fprintf(stderr, "Searched for: %s\n", candidate_path);
    }
    return;
}

/*
 * So we can get commands of different forms.
 *  Cases:
 *    Absolute Path.
 *      if it starts with a '/' ie, /bin/ls we can assume it's an absolute path
 *	and check for the existence and execute it immediately.
 *    Local Path
 *      if it starts with a '.' like ./uvshr we can assume it refers to the local directory
 *    Home Path
 *      an arguement can also start with a '~', which points to the users home directory.
 *
 *    In cases 2 & 3 we have to augment the path to be complete, and then execute as normal.
 *
 *    The final use case is with no path information. In that case, we loop over directories in default dirs
 *     and check if the specified command exists in any of them.
 *
 *    @return:
 *           0 if absolute path
 *           1 if relative path
 *           2 if home path
 *           3 if it's the name of an executable only (use default directories)
 */
int check_command_type(char* args[]) {
    if (strcmp(args[0], "exit") == 0)
        return -1;

    if (*args[0] == '/') {
        return 0;
    }  else  if (args[0][0] == '.') {
        return 1;
    }  else if (args[0][0] == '~') {
        return 2;
    } else {
        return 3;
    }
}

/*
 * Should be safe since fgets should never have more than the trailing newline.
 *    @side-effect
 *          remove trailing \n from fgets, and normalize whitespace from arguement.
 *          ie 'ls   -l' -> 'ls -l'
 */
void normalize_space(char current_command_string[max_line_length]) {
    int i, x;
    for (i = 0; i < max_line_length; i++) {
        if (current_command_string[i] == '\n') {
            current_command_string[i] = '\0';
        }
    }
    /* Borrowed and modified code from the following sources for this loop
     * https://stackoverflow.com/questions/36950552/0-evaluates-false-0-evaluates-true/36950573
     * https://stackoverflow.com/questions/17770202/remove-extra-whitespace-from-a-string-in-c
     */
    for(i = x = 0; current_command_string[i]; ++i) {
        if(!isspace(current_command_string[i]) || (i>0 && !isspace(current_command_string[i - 1]))) {
            current_command_string[x++] = current_command_string[i];
        }
    }
    current_command_string[x] = '\0';
}

int read_default_dirs(FILE *fp, char default_directories[max_directory_len][max_directory_string_len]) {
    int i, j;
    for (i = 0; i < max_directory_len; i++) {
        for (j = 0; j < max_directory_string_len; j++) {
            default_directories[i][j] = '\0';
        }
    }

    int count = 0;
    while(fgets(default_directories[count], max_directory_string_len, fp) != NULL) {
        normalize_space(default_directories[count++]);
    }
    return count;
}

int populate_exec_args(char** args, char tokens[max_args][max_line_length], int num_tokens) {
    int i, start;
    if (strcmp(tokens[0], "do-pipe") == 0) {
        current_control_flag = DO_PIPE;
        start = 1;
    } else if (strcmp(tokens[0], "do-out")  == 0) {
        current_control_flag = DO_OUT;
        start = 1;
    } else {
        current_control_flag = DEFAULT;
        start = 0;
    }
    if (validate_tokens(tokens, num_tokens) == 0) {
        return 0;
    }
    for (i = start; i < num_tokens; i++) {
        args[i - start] = tokens[i];
        if (strcmp(args[i - start], "::") ==  0) {
            PIPE_INDEX = i - start;
        }
    }
    args[i - start] = '\0';
    return 1;
}

int validate_tokens(char tokens[max_args][max_line_length], int num_tokens) {
    int i;
    int seen_pipe_symbol = 0;
    for (i = 0; i < num_tokens; i++) {
        if (strcmp(tokens[i], "::") == 0) {
            seen_pipe_symbol += 1;
        }
    }
    if (seen_pipe_symbol == 1 && current_control_flag == DEFAULT) {
        fprintf(stderr, "Pipe not specified and encountered '::' token.\n");
        return 0;
    }
    if (strcmp(tokens[i  - 1], "::") == 0) {
        fprintf(stderr, "Encountered '::' symbol, but no target provided\n");
        return 0;
    }
    if (seen_pipe_symbol > 1) {
        fprintf(stderr,  "Nested pipes not supported.\n");

        return 0;
    }
    if (seen_pipe_symbol == 0 && (current_control_flag == DO_PIPE || current_control_flag == DO_OUT)) {
        fprintf(stderr, "do-pipe or do-out specified, but no '::' symbol provided.\n");
        return 0;
    }
    return 1;
}

void init(char ccs[max_line_length], char ts[max_args][max_line_length]) {
    int i, j;
    PIPE_INDEX = -1;
    for (i = 0; i < max_line_length; i++) {
        ccs[i] = '\0';
    }
    for (i = 0; i < max_args; i++) {
        for (j = 0; j < max_line_length; j++) {
            ts[i][j] = '\0';
        }
    }
}

int tokenize_command_string(char str[max_line_length], char tokenized[max_args][max_line_length]) {
    int i;
    int count = 0;
    int last_none_space_index = 0;
    for (i = 0; i < max_line_length; i++) {
        count++;
        if (str[i] == '\0') {
            break;
        }
        if (str[i] != ' ') {
            last_none_space_index = i;
        }
    }
    int current_token = 0;
    int j = 0;
    char null_term = '\0';
    for (i = 0; i < last_none_space_index; i++) {
        j = 0;
        while (str[i] != ' ' && str[i] != '\0') {
            strcpy(&tokenized[current_token][j], &str[i]);
            j++;
            i++;
      }
        strcpy(&tokenized[current_token][j], &null_term);
        current_token++;
    }
    return current_token;
}

void print_args(char **args, int num_tokens) {
    int i;
    printf("\n====Printing Args:====\n");
    printf("Index split at:%d\n", PIPE_INDEX);
    for (i = 0; i < num_tokens; i++) {
        printf("ArgNo. %d||%s\n", i, args[i]);
    }
    printf("============DONE========\n");
}

/*
    this function is heavily influenced by examples provided
    for this assignment written by Mike Zastre
*/
int execute_vanilla_command(char *args[]) {
    pid_t pid;
    int status;
    if ((pid = fork()) == 0) {
        if (execve(args[0], args, environ) < 0) {
            fprintf(stderr, "\t Err. Exec failed when run on %s.\n", args[0]);
        }
    } else if (pid < 0) {
        fprintf(stderr, "\t Err. Failed to spawn child process %s.\n", args[0]);
    }
    wait(&status);
    return 0;
}

/*
    this function is heavily influenced by examples provided
    for this assignment written by Mike Zastre
*/
int execute_do_out_command(char *args[]) {
    int pid, fd;
    int status;
    if ((pid = fork()) == 0) {
        fd = open(args[PIPE_INDEX + 1] , O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
        if (fd == -1) {
            fprintf(stderr, "\t Err. Unable to open %s for writing.\n", args[PIPE_INDEX + 1]);
            return 0;
        }
        dup2(fd, 1);
        dup2(fd, 2);
        args[PIPE_INDEX] = '\0';
        if (execve(args[0], args, environ) < 0) {
            fprintf(stderr, "\t Err. Exec failed on child process %s.\n", args[0]);
        }
    } else if (pid < 0) {
        fprintf(stderr, "\t Err. Failed to spawn child process %s.\n", args[0]);
    }
    waitpid(pid, &status, 0);
    return 1;
}

/*
    this function is heavily influenced by examples provided
    by example code posted by Mike Zastre
*/
int execute_do_pipe_command(char *args[]) {
    args[PIPE_INDEX] = '\0';

    int pid_head, pid_tail, status;
    int fd[2];

    pipe(fd);

    if ((pid_head = fork()) == 0) {
        dup2(fd[1], 1);
        close(fd[0]);
        if (execve(args[0], args, environ) < 0) {
            fprintf(stderr, "\t Err. Exec failed on proccess: %s.\n", args[0]);
        }
    } else if (pid_head < 0) {
        fprintf(stderr, "\t Err. Failed to launch process %s.\n", args[0]);
    }

    if ((pid_tail = fork()) == 0) {
        dup2(fd[0], 0);
        close(fd[1]);
        if (execve(args[PIPE_INDEX + 1], &args[PIPE_INDEX + 1], environ) < 0) {
            fprintf(stderr, "\t Err. Failed on process %s.\n", args[PIPE_INDEX + 1]);
        }
    } else if (pid_head < 0) {
        fprintf(stderr, "\t Err.  Failed to launch process %s.\n", args[PIPE_INDEX + 1]);
    }

    close(fd[0]);
    close(fd[1]);
    waitpid(pid_head, &status, 0);
    waitpid(pid_tail, &status, 0);

    return 1;
}


int  execute_command(char *args[]) {
    if (current_control_flag == DEFAULT) {
        return execute_vanilla_command(args);
    }
    if (current_control_flag == DO_OUT) {
        return execute_do_out_command(args);
    }
    if (current_control_flag == DO_PIPE) {
        return execute_do_pipe_command(args);
    }
    return 0;
}


void print_tokens(char tokens[max_args][max_line_length], int num_tokens) {
    int i, j;
    for (i = 0; i < num_tokens; i++) {
        j = 0;
        while (tokens[i][j] != '\0') {
            printf("%c", tokens[i][j]);
            j++;
        }
        printf("\n");
    }
}

void print_default_dirs(char default_directories[max_directory_len][max_directory_string_len], int count) {
    int i;
    for (i = 0; i < count; i++) {
        printf("DirNo %d: %s\n", i, default_directories[i]);
    }
}

int load_prompt(FILE *fp, char prefix[max_prompt_length]) {
    int i;
    for (i = 0; i < 10; i++) prefix[i] = '\0';
	if (fgets(prefix, 10, fp)==NULL) return 0;
    prefix[strlen(prefix)-1] = '\0';
    return 1;
}

void print_variable_args(char *args[]) {
    int i;
    printf("\n===begin Printing Args.====\n");
    for (i = 0; args[i] != '\0'; i++) {
        printf("ArgNo. %d: %s\n", i, args[i]);
    }
    printf("============\n");
}
