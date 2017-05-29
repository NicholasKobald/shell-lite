#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main(int argc, char *argv[]) {
    char *cmd_head[] = { "/bin/ls", "-1", 0 };
    char *cmd_tail[] = { "/usr/bin/wc", "-l", 0 };
    char *envp[] = { 0 };
    int status;
    int pid_head, pid_tail;
    int fd[2];

    pipe(fd);

    /* After this point, we now have a pipe in the form of two file
     * descriptors. The end of the pipe from which we can read is
     * fd[0], and the end of the pipe to which we write is fd[1].
     *
     * Any child created after this point will have these file descriptors
     * for the pipe.
     */

    printf("parent: setting up piped commands...\n");
    if ((pid_head = fork()) == 0) {
        printf("child (head): re-routing plumbing; STDOUT to pipe.\n");
        dup2(fd[1], 1);
        close(fd[0]);
        execve(cmd_head[0], cmd_head, envp);
        printf("child (head): SHOULDN'T BE HERE.\n");
    }

    if ((pid_tail = fork()) == 0) {
        printf("child (tail): re-routing plumbing; pipe to STDIN.\n");
        dup2(fd[0], 0);
        close(fd[1]);
        execve(cmd_tail[0], cmd_tail, envp);
        printf("child (tail): SHOULDN'T BE HERE.\n");
    }

    /* One last detail: At this point we are running within code used
     * by the parent. The parent does *not* need the pipe after the
     * children are started, but the file system does not know this
     * and so detects additional links to the open pipe. Therefore the
     */
    close(fd[0]);
    close(fd[1]);

    printf("parent: waiting for child (head) to finish...\n");
    waitpid(pid_head, &status, 0);
    printf("parent: child (head) is finished.\n");

    printf("parent: waiting for child (tail) to finish...\n");
    waitpid(pid_tail, &status, 0);
    printf("parent: child (tail) is finished.\n");
    return 1;
}
