/*
* Nicholas Kobald
* @Date:   24-May-2017
* @Email:  nick.kobald@gmail.com
*
* TODO:
*       -fix bug with variable number of args
*       -allow recursive uvshr.c calls
*       -try not to cry
*       -fix whitespace bug
*       -cry a lot
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>


#define max_args 10 //well, 9, but we're tokenizing the command itself
#define max_line_length 80
#define max_prompt_length 10
#define max_directory_len 10
#define max_directory_string_len 100

int load_prompt(FILE *fp, char prefix[10]);
void init(char ccs[max_line_length], char ts[max_args][max_line_length]);
int tokenize_command_string(char str[max_line_length], char tokenized[max_args][max_line_length]);
void print_default_dirs(char default_directories[max_directory_len][max_directory_string_len], int count);
int read_default_dirs(FILE *fp, char default_directories[max_directory_len][max_directory_string_len]);
void print_tokens(char tokens[max_args][max_line_length], int num_tokens);
int  execute_command(char **args);
void populate_exec_args(char** args, char tokens[max_args][max_line_length], int num_tokens);
void print_args(char **args, int num_tokens);
int check_command_type(char* args[]);
void strip_newline(char current_command_string[max_line_length]);
void handle_command(char *args[max_args], char default_directories[max_directory_len][max_directory_string_len], int cmd_type, int num_dir);
int find_command(char *args[max_args], char default_directories[max_directory_len][max_directory_string_len], char candidate_path[max_line_length], int num_dir);



int main() {
    FILE *fp;
    fp = fopen(".uvshr", "r");
    char default_directories[max_directory_len][max_directory_string_len];
    char prompt[max_prompt_length];
    int num_tokens;

    load_prompt(fp, prompt);
    int num_directories = read_default_dirs(fp, default_directories);

    char current_command_string[max_line_length];
    char tokenized_string[max_args][max_line_length];
    char *exec_args[max_args]; //need both cuz reasons.
	int command_type;
    //main loop
    for(;;) {
        init(current_command_string, tokenized_string);
        printf("%s", prompt);
        fgets(current_command_string, max_line_length, stdin);
        strip_newline(current_command_string);
        num_tokens = tokenize_command_string(current_command_string, tokenized_string);
        populate_exec_args(exec_args, tokenized_string, num_tokens);

        if (num_tokens > 0) {
            command_type = check_command_type(exec_args);
            handle_command(exec_args, default_directories, command_type, num_directories);
        }
    }
}

/*
 *   The intention of this function is to handle the 'logic' of getting to the right executable, and executing it.
 *       @param args - array of arrays.
 *       @param default_directories - default directories to look for a command in
 *       @param cmd_type - int value specifying how to determine where the executable is.
 *               see function 'check command type'.
 *
 */
void handle_command(char *args[max_args], char default_directories[max_directory_len][max_directory_string_len], int cmd_type, int num_dir)  {
   // printf("In handle command. CMD TYPE: %d", cmd_type);
    if (cmd_type == 0) { //args holds an absolute path.
        execute_command(args);
        return;
    }

    if (cmd_type == 3) {
        char candidate_path[max_line_length];
        if (find_command(args, default_directories, candidate_path, num_dir) == 1) {
            execute_command(args);
        }
    }
}

int find_command(char *args[max_args], char default_directories[max_directory_len][max_directory_string_len], char candidate_path[max_line_length], int num_dir) {
    int i;

    char target_command[max_line_length];

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
    return 0;
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
 */
void strip_newline(char current_command_string[max_line_length]) {
    int i;
    for (i = 0; i < max_line_length; i++) {
        if (current_command_string[i] == '\n') {
            current_command_string[i] = '\0';
        }
    }
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
        strip_newline(default_directories[count++]);
    }
    //print_default_dirs(default_directories, count);
    return count;
}

void populate_exec_args(char** args, char tokens[max_args][max_line_length], int num_tokens) {
    int i;
    for (i = 0; i < num_tokens; i++) {
        args[i] = tokens[i];
    }
    args[i] = '\0';

    /*
    printf("Printing string.\n");
    for (i = 0; i < num_tokens; i++) {
        printf("%s", args[i]);
        printf("\n");
    } */
}

void init(char ccs[max_line_length], char ts[max_args][max_line_length]) {
    int i, j;
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
    printf("First arg is: \n%s\n", args[0]);
    printf("First arg is: \n%s\n", args[0]);
    printf("====Printing Arguements:====\n");
    for (i = 0; i < num_tokens; i++) {
        printf("ArgNo. %d:%s\n", i, args[i]);
    }
    printf("First arg still us: \n%s\n", args[0]);
    printf("---done-----\n");
}

int  execute_command(char *args[]) {
    pid_t pid;
    int status;
    //print_args(args, num_tokens);
    char *envp[] = {0};
    //char *test_args[] = {"/bin/ls", "-l", 0};

    if ((pid = fork()) == 0) {
        printf("%d", execve(args[0], args, envp));
        printf("This is the child now..");
        exit(EXIT_FAILURE);
    }

    while (wait(&status) > 0) {
     //   printf("Was maybe a success.");
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
