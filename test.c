#include  <stdio.h>
#include  <sys/types.h>
#include <errno.h>

#define INPUT_BUFFER 80
#define MAX_ARGS 9
#define MAX_PROMPT_LEN 10
#define MAX_DIRS 10


void  parsed(const char *line, const char delim, char **argv)
{
     while (*line != '\0') {       /* if not the end of line ....... */ 
          while (*line == ' ' || *line == '\t' || *line == '\n')
               *line++ = '\0';     /* replace white spaces with 0    */
          *argv++ = line;          /* save the argument position     */
          while (*line != '\0' && *line != delim && 
                 *line != '\t' && *line != '\n') 
               line++;             /* skip the argument until ...    */
     }
     *argv = '\0';                 /* mark the end of argument list  */
}

void parse(const char* parseme, const char* delim, char** output) {
    char* token;
    char* str = dup2(parseme);
    int i;
    for(i = 0; (token = strsep(&str, delim)); i++) {        
        output[i] = token;
    }
}


int  execute_cmd(char **argv, char **envp)
{
     pid_t  pid;
     int    statusl;
     
     // Fork a child process 
     if ((pid = fork()) < 0) { //Failed!     
          fprintf(stderr, "*** ERROR: forking child process failed\n");
          return -1; 
     }
     //Successful!
     else if (pid == 0) {          
          if (execve(*argv, argv) < 0) {     /* execute the command  */
               fprintf(stderr, "*** ERROR: exec failed\n");
               return -11; 
          }
     }
     else {                                  /* for the parent:      */
          while (wait(&status) != pid)       /* wait for completion  */
               ;
     }

     return 0;
}

int main( int argc, char *argv[] )
{
     char raw_input[INPUT_BUFFER];   
     char exec_argv[MAX_ARGS];
     const char exec_path[] = {"PATH=/bin:/usr/bin", (char*)0};


     // And we go on, and on, and on
     for(;;) {                     
          // Make sure the output buffer is clear first 
		  fflush(stdout);
          
          // Display prompt
          printf("¯\\_(ツ)_/¯ ");  
		  
          //Read in from the command line
          fgets(raw_input, INPUT_BUFFER, stdin);
          printf("\n");
         
          //tokenize the input and store result in exec_argv
          parse(raw_input, ' ', exec_argv);       
          
         
          //Do we need to exit? 
          if (strcmp(exit_argv[0], "exit") == 0)  
                break;
          
          //No, let's execute the command then 
          if(execute_cmd(exec_argv, exec_path) < 0)
                break;  
     }

     return 0;
}

                
