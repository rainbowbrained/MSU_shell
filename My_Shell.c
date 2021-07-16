#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>	
#include <string.h>	
#include <ctype.h>	
#include <limits.h>
#include "utilities.h"

#define INVITE "------ This is MYSHELL ------\nPlease, enter commands:\n"

unsigned long long int children_to_wait = 0;
char buf[1024];
int fd = STDIN_FILENO;

void sighndlr(int sig) {
	if (sig == SIGINT) {
		if (fd > 2) close (fd);
		kill(0, SIGINT);
	}
	while (children_to_wait) {
		wait (NULL);
		children_to_wait--;
	}
	if (fd > 2) close (fd);
	fprintf (stdout,  "------ Have a lovely day! Terminating ------\n\n");
	exit (0);
} 

#define NEXTSTR getchar(); while ((c != '\n')&&(c != EOF)) { getchar();} \
		i = 0; buf[0] = '\0'; background_flag = 0; goto continue_reading_string;
int main (int argc, char * argv[]) {
	if (write(STDOUT_FILENO, INVITE, sizeof(INVITE)) == -1) {
		perror ("Can not write an invitation");
		return errno;
	}
	
	unsigned long long int i = 0;
	long long int brackets = 0;
	if (argc > 1) {
		if ((fd = open(argv[1], O_RDONLY, 0644)) == -1) {
			perror ("Opening file with tests");
			argc = 1;
		}
		dup (STDIN_FILENO);
		dup2(fd, STDIN_FILENO);
	}
	signal (SIGINT, sighndlr);
	signal (SIGKILL, sighndlr);
	char background_flag = 0; //programms work simultaneously
	int c = getchar();

continue_reading_string:
	while (c != EOF) {
		while (c != '\n') {
			if ((background_flag == 0)&&(c == '&')) {
				buf[i] = c;
				i++;
				if ((c = getchar()) != '&') {
					i--;
					background_flag = 1;
					goto stop_reading_string;
					++i;
					if (i == INT_MAX) {
						fprintf (stderr, "Too long string\n");
						NEXTSTR;
					}
				}
			}
			if (c == ')') brackets--;
			if (c == '(') brackets++;
			if (brackets < 0) {
				fprintf (stderr, "Too long string\n");
				NEXTSTR;
			}
			buf[i] = c;
			++i;
			if (i == INT_MAX) {
				fprintf (stderr, "Too long string\n");
				NEXTSTR;
			}
            c = getchar();
		}
		if (brackets != 0) { 
			fprintf (stderr, "Brackets are disbalanced\n");
			NEXTSTR;
		}
stop_reading_string:
		buf[i] = '\0';
		pid_t pid = fork();
		if (pid == 0) {
			signal (SIGINT, SIG_DFL);
			signal (SIGKILL, SIG_DFL);
			if (argc > 1) fprintf(stderr, "executing: %s\n", buf);
			my_shell_exec(buf, i, STDIN_FILENO, STDOUT_FILENO);
			if (argc > 1) sleep (1);
		} else if (pid < 0) {
			perror("Can not create a son process");
			while (children_to_wait) wait (NULL);
			if (fd > 2) close (fd);
	       	return errno;
		}
		if (!background_flag) {
			waitpid(pid, NULL, 0);
		} else children_to_wait++;
		i = 0;
		background_flag = 0;
		c = getchar();
		if ((argc > 1)&&(buf[0] == 'r')&&(buf[1] == 'm')) c = EOF;
		buf[0] = '\0';
	}
	
	while (children_to_wait) {
		wait (NULL);
		children_to_wait--;
	}
	if (fd > 2) close (fd);
	fprintf (stdout,  "------ Have a lovely day! Terminating ------\n");
	return 0;
}
