// THIS IS A CONVEYOR PR1|PR2|PR3
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>	
#include <dirent.h>	
#include <string.h>	
#include <ctype.h>	
#include <limits.h>	

int main (void) //./prog, p, f
{
	//int fd1 = open ("polis.c", O_RDWR, 0644),fd2 = open ("fil2", O_RDWR, 0644),fd3 = open ("file", O_RDWR, 0644), safe, safe1;
    int fd1[2];
    pipe (fd1); //[1] - write, [0] - read
    int fd2[2];
    pipe (fd2);
    int pid1, pid2, pid3;
    
	//fcntl(fd1[0], F_SETFL, fcntl(fd1[0], F_GETFL, 0) | O_NONBLOCK);
	//fcntl(fd2[0], F_SETFL, fcntl(fd2[0], F_GETFL, 0) | O_NONBLOCK);
    if ((pid1 = fork()) == 0){
		int safe1 = dup(STDOUT_FILENO);
		dup(STDOUT_FILENO);
		dup(STDIN_FILENO);
		dup2(fd1[1], STDOUT_FILENO);
		
		close(fd1[1]);
        close(fd2[0]);
        close(fd1[0]);
        close(fd2[1]);
        fprintf (stderr, "1\n");
        execlp("./slow_pr", "./slow_pr", NULL);
        perror("execl()");
        dup2(safe1, STDOUT_FILENO);
        return errno;
    }
    if ((pid2 = fork()) == 0){
		int safe1 = dup(STDOUT_FILENO);
		dup(STDOUT_FILENO);
		int safe = dup(STDIN_FILENO);
		dup(STDIN_FILENO);
		
		dup2(fd1[0], STDIN_FILENO);
        dup2(fd2[1], STDOUT_FILENO);
        close(fd1[1]);
        close(fd2[0]);
        close(fd1[0]);
        close(fd2[1]);
        //dup2(safe1, STDOUT_FILENO);
        fprintf (stderr, "2\n");
        execlp("tail", "tail", "-2", NULL);
        perror("execl()");
        dup2(safe, STDIN_FILENO);
        dup2(safe1, STDIN_FILENO);
        return errno;
	}
	
	if ((pid3 = fork()) == 0){
		int safe = dup(STDIN_FILENO);
		dup(STDIN_FILENO);
		dup2(fd2[0], STDIN_FILENO);
        /*close(fd1[1]);
        close(fd2[0]);
        close(fd1[0]);
        close(fd2[1]);*/
        fprintf (stderr, "3\n");
        execlp("grep", "grep", "pid", NULL);
        perror("execl()");
        dup2(safe, STDIN_FILENO);
        return errno;
    }
    close(fd1[1]);
    close(fd2[0]);
    close(fd1[0]);
    close(fd2[1]);
    waitpid(pid1, NULL, 0);
    fprintf (stderr, "!1");
    waitpid(pid2, NULL, 0);
	fprintf (stderr, "!2");
    waitpid(pid3, NULL, 0);
	fprintf (stderr, "!3");
	
	
    return 0;
}
