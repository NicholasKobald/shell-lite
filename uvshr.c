/*
* Nicholas Kobald
* @Date:   24-May-2017
* @Email:  nick.kobald@gmail.com
*
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

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
int find_command(char default_directories[max_directory_len][max_directory_string_len], char tokenized_string[max_args][max_line_length]);
int  execute_command(char **args, int num_tokens);
void populate_exec_args(char** args, char tokens[max_args][max_line_length], int num_tokens);
void print_args(char **args, int num_tokens);
void strip_newline(char current_command_string[max_line_length]);

int main() {
    FILE *fp;
    fp = fopen(".uvshr", "r");
    char default_directories[max_directory_len][max_directory_string_len];
    char prompt[max_prompt_length];
    int num_tokens;

    load_prompt(fp, prompt);
    read_default_dirs(fp, default_directories);

    char current_command_string[max_line_length];
    char tokenized_string[max_args][max_line_length];
    char *exec_args[max_args]; //need both cuz reasons.

    //main loop
    for(;;) {
        init(current_command_string, tokenized_string);
        printf("%s", prompt);
        fgets(current_command_string, max_line_length, stdin);
        strip_newline(current_command_string);
        num_tokens = tokenize_command_string(current_command_string, tokenized_string);
        populate_exec_args(exec_args, tokenized_string, num_tokens);
        execute_command(exec_args, num_tokens);
        fflush(stdout);
    }
}

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

    while(fgets(default_directories[count++], max_directory_string_len, fp) != NULL) {}
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
    printf("First arg is: %s\n", args[0]);
    printf("Printing Arguements: ");
    for (i = 0; i < num_tokens; i++) {
        printf("%s\n", args[i]);
    }
    printf("---done\n");
}

/*
 * original author Amanda Chase
 * Adapted for use in this assignment.
 */
int  execute_command(char *args[], int num_tokens) {
    pid_t pid;
    int status;
    print_args(args, num_tokens);
    char *envp[] = {0};
    char *test_args[] = {"/bin/ls", "-l", 0};
    printf("Test args look like this:\n");
    int i;
    int count = 0;
    for(i = 0; test_args[i] != '\0'; i++) {
        count++;
        printf("%s\n", test_args[i]);
    }
    printf("Looped %d times.\n", count);
    count = 0;
    printf("args look like this:\n");
    for(i = 0; args[i] != '\0'; i++) {
        count++;
        printf("%s\n", args[i]);
    }
    printf("Looped %d time\n", count);
    printf("Fin testing.");
    if ((pid = fork()) == 0) {
        printf("Childs starting.\n");
        execve(args[0], args, envp);
        printf("This shouldn't happen\n");
    }

    printf("Waiting for child (this is parent)");
    while (wait(&status) > 0) {
        printf("Was maybe a success.");
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
    int i, j;
    for (i = 0; i < count; i++) {
        printf("%s", default_directories[i]);
    }
}

int load_prompt(FILE *fp, char prefix[max_prompt_length]) {
    int i;
    for (i = 0; i < 10; i++) prefix[i] = '\0';
	if (fgets(prefix, 10, fp)==NULL) return 0;
    prefix[strlen(prefix)-1] = '\0';
    return 1;
}
