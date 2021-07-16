// THIS IS A CONVEYOR PR1;PR2;PR3
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
	//int fd = open ("fil2", O_RDONLY | 0644);
	pid_t pid = fork();
	if (pid == 0) {
		exit(100);
	}
	int status;
	waitpid(pid, &status, 0);
	fprintf (stderr, "%i", WEXITSTATUS(status));
    return 0;
}
