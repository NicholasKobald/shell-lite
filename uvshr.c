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
int  execute_command(char **args);

int main() {
    FILE *fp;
    fp = fopen(".uvshr", "r");
    char default_directories[max_directory_len][max_directory_string_len];
    char prompt[max_prompt_length];

    load_prompt(fp, prompt);
    read_default_dirs(fp, default_directories);

    char current_command_string[max_line_length];
    char tokenized_string[max_args][max_line_length];

    //main loop
    for(;;) {
        init(current_command_string, tokenized_string);
        printf("%s", prompt);
        fgets(current_command_string, max_line_length, stdin);
        tokenize_command_string(current_command_string, tokenized_string);
        printf("Calling execute command");

        //execute_command(tokenize_command_string);
        printf("Fin.");

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

/*
 * original author Amanda Chase
 * Adapted for use in this assignment.
 */
int  execute_command(char **args) {
     pid_t pid;
     int status;

     if ((pid = fork()) < 0) {
          fprintf(stderr, "*** ERROR: forking child process failed\n");
          return -1;
     } else if (pid == 0 && execve(*args, args, '\0') < 0) {
           fprintf(stderr, "*** ERROR: exec failed\n");
           return -1;
     } else {
          while (wait(&status) != pid);
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
