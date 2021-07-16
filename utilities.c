#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>	
#include <dirent.h>	
#include <string.h>	
#include <ctype.h>	
#include <limits.h>

#define MAX_ARGS 256
#define SAFE(msg, call) do{if((call) == -1) perror(msg); exit(1);}while(0)

struct Redirect {
    unsigned long long int max_file_amount; 
    unsigned long long int num_of_file;
	int * fd_array;
	int * open_mode;
};

struct Unit {
	struct Redirect * files;
	char * command;
	unsigned long long int command_length;
};

int in(char symbol, char *mas);
struct Redirect* RedInit(void);
int UnitInit(struct Unit* pt);
void RedDeinit(struct Redirect *pt);
void UnitDeinit(struct Unit *pt);
void printRed (struct Redirect *pt);
void printUnit (struct Unit *pt);
int my_shell_exec (char*input, unsigned long long int length,int input_fd, int output_fd);
int PrepareUnit(struct Unit *pt, char * input, unsigned long long int length);
int FillFiles (struct Unit * unit) ;
int conveyor (char*input, int fd_in, int fd_out, struct Unit*unit_array, char**expr_fork);

char separators[] = "|&;()";
char numbers[] = "0123456789";

//The function returns 1 if the symbol is in the given array of symbols, or 0 if it is not
int in(char symbol, char *mas) {
    for (int i = 0; i < strlen(mas); i++) {
        if (symbol == mas[i])
            return 1;
    }
    return 0;
}

struct Redirect* RedInit(void) {
	struct Redirect *pt = (struct Redirect*)malloc(sizeof(struct Redirect));
	
	if (pt == NULL) {
		perror("Can not allocate memory for files");
		return NULL;
	}
	pt->max_file_amount = 1024;
	pt->num_of_file = 0;
    pt->fd_array = (int*) malloc (sizeof(int)*pt->max_file_amount);
    if (pt->fd_array == NULL) {
		perror("Can not allocate memory for files");
		free (pt);
		return NULL;
	}
	pt->open_mode = (int*) malloc (sizeof(int)*pt->max_file_amount);
    if (pt->open_mode == NULL) {
		perror("Can not allocate memory for files");
		free (pt->fd_array);
		free (pt);
		return NULL;
	}
    for (int file_i = 0; file_i < 1024; file_i++) {
		pt->fd_array[file_i] = 0;
	}
	return pt;
}

int UnitInit(struct Unit* pt) {
	if ((pt->files = RedInit()) == NULL) {
		return -1;
	}
	pt->command_length = 1024;
    pt->command = (char*) malloc (sizeof(char)*pt->command_length);
    if (pt->command == NULL) {
		perror ("Can not allocate memory for command");
		return errno;
	}
    for (int file_i = 0; file_i < 1024; file_i++) {
		pt->command[file_i] = 0;
	}
	return 0;
}

void RedDeinit(struct Redirect *pt)
{
	for (int i = 0; i < pt->num_of_file; i++) close(pt->fd_array[i]);
	free(pt->fd_array);
	free(pt->open_mode);
	return;
}

void UnitDeinit(struct Unit *pt)
{
	RedDeinit(pt->files);
	free(pt->files);
	free(pt->command);
	return;
}

void printRed (struct Redirect *pt)
{
	for (unsigned long long int i = 0; i < pt->num_of_file; i++) {
		fprintf (stderr, "%lli| fd: %i\n", i, pt->fd_array[i]);
	}
}

void printUnit (struct Unit *pt)
{
	fprintf(stderr, "----- command: '%s' -----\n", pt->command);
	printRed (pt->files);
}

#define FREEMEM for (int l = 0; l < cur_fork; l++) {free(expr_fork[l]);} free(expr_fork);\
				for (i = 0; i < cur; i++) UnitDeinit(&unit_array[i]); free (unit_array);\

int my_shell_exec(char*input, unsigned long long int length, int input_fd, int output_fd){
    unsigned long long int i = 0, expr_i = 0; 
    unsigned long long int j = 0, max_num_of_units = 1024, cur = 0;
    struct Unit * unit_array = (struct Unit*)malloc(sizeof(struct Unit)*max_num_of_units);
    if (unit_array == NULL) {
		perror ("Can not allocate memory for unit array");
		exit(EXIT_FAILURE);
	}
	char tmpbuf[length + 1]; //for units
	char expression[length + 2]; // for polis
    
    //table for expressions in brackets
    int max_forks = 100, cur_fork = 0;
    char** expr_fork = (char**) malloc(sizeof(char*)*max_forks); //for express. in brackets
    if (expr_fork  == NULL) {
		perror ("Can not allocate memory for an array with epressions with brackets");
		free(unit_array);
		exit(EXIT_FAILURE);
	}
	
my_shell_exec_next:
	while (i < length) {
		j = 0;
		while ((input[i] == ' ')&&(i < length)) i++; //skip spaces in the beggining
		while ((i < length)&&(!in(input[i], separators))) {
			tmpbuf[j] = input[i];
			j++;
			i++;
		}
		tmpbuf[j] = '\0';
		
		if ((i < length)&&(input[i] == ')')) {
			fprintf(stderr, "Brackets are disbalanced\n");
			FREEMEM;
			exit(EXIT_FAILURE);
		}
		
		// came to an expression with brackets
		if ((i < length)&&(input[i] == '(')) {
			while ((i > 0)&&(!in(input[i], separators))) i--;
			if ((in(input[i], separators))&&(input[i] != '(')&&(input[i] != ')')) i++;
				
			expr_fork[cur_fork] = (char *)malloc(sizeof(char)*(length + 1));
			if (expr_fork[cur_fork] == NULL) {
				perror ("Can not allocate memory for an expression in brackets\n");
				FREEMEM;
				exit(EXIT_FAILURE);	
			}
			
			int open_bracket = 0;
			char bracket_flag = 1;
			unsigned long long int fork_i = 0;
			while ((i < length)&&bracket_flag) {
				if (input[i] == ')') open_bracket--;
				if (input[i] == '(') open_bracket++;
				expr_fork[cur_fork][fork_i] = input[i];
				i++;
				fork_i++;
				if (open_bracket == 0) bracket_flag = 0;
			}
			while ((i < length)&&(!in(input[i], separators))) {
				expr_fork[cur_fork][fork_i] = input[i];
				fork_i++;
				i++;
			}
			expr_fork[cur_fork][fork_i] = '\0';
			if (open_bracket != 0) { 
				fprintf(stderr, "Brackets are disbalanced\n");
				cur_fork++;
				FREEMEM;
				exit(EXIT_FAILURE);	
			}
			expression[expr_i] = 'f';
			expr_i++;
			char buf_num[256];
			sprintf (buf_num, "%i%c", cur_fork, '\0');
			
			int iiii = 0;
			for (iiii = 0; iiii < strlen (buf_num); iiii++) 
				expression[expr_i + iiii] = buf_num[iiii];
			expr_i += iiii;
		
			cur_fork++;
			if (cur_fork == max_forks) {
				expr_fork = (char**) realloc (expr_fork, max_forks*2);
				max_forks *= 2;
			}
			fork_i = 0;
		} else {
		
			if (tmpbuf[0] == '\0') { strcpy(tmpbuf, "false\0"); j = 5;}
			if (UnitInit(&unit_array[cur]) != 0) {
				fprintf(stderr, "Can not create a unit\n");
				FREEMEM;
				exit(EXIT_FAILURE);	
			}
			if (PrepareUnit(&unit_array[cur], tmpbuf, j) != 0) {
				// go to the next unit
				j = 0;
				while (in(input[i], separators)) i++;
				goto my_shell_exec_next;
			}
			//printUnit(&unit_array[cur]);
			char buf_num[256];
			sprintf (buf_num, "%lli%c", cur, '\0');
			int iii = 0;
			for (iii = 0; iii < strlen (buf_num); iii++) 
				expression[expr_i + iii] = buf_num[iii];
			expr_i += iii;
			cur++;
		}
		
		if ((i < length)&&(input[i] == ';')) {
		    expression[expr_i] = ';';
		    expr_i++;
		} else if ((i < length - 1)&&(input[i] == '&')&&(input[i+1] == '&')) {
		    expression[expr_i] = '/';
		    expr_i++;
		    i++;
		} else if (((i < length)&&(input[i] == '&'))&&(((i < length - 1) \
								&&(input[i+1] != '&'))||(i = length - 1))) {
		    expression[expr_i] = '&';
		    expr_i++;
		    goto my_exec_ending;
		} else if ((i < length - 1)&&(input[i] == '|')&&(input[i+1] == '|')) {
		    expression[expr_i] = '=';
		    expr_i++;
		    i++;
		} else if (((i < length)&&(input[i] == '|'))&&(((i < length - 1) \
								&&(input[i+1] != '|'))||(i = length - 1))) {
		    expression[expr_i] = '|';
		    expr_i++;
		}
		i++;
	}

my_exec_ending:
	expression[expr_i] = '\0';
	int status = 0;
	if (expression[0] != '\0') {
		conveyor (expression, input_fd, output_fd, unit_array, expr_fork);
	} else {
		exit(WEXITSTATUS(status));
	}
	FREEMEM;
	return 0;
}

//  The function gets one whole unit nd prepares it to the work. It fills information
//  about files, commands, arguements and other, initializes struct Redirect, fills
//  the array of file descriptors and so on. In case of success return 0, otherwise
//  returns errno or -1. 
int PrepareUnit(struct Unit *pt, char * input, unsigned long long int length) {
	unsigned long long int i = 0, i1 = 0;
	while ((i < length)&&(input[i] == ' ')) i++;
	if (i == length) {
		fprintf (stderr,"Empty unit\n");
		return -1;
	}
	while ((length > 0)&&(input[length] == ' ')) length--;
	
	int open_mode = 0;
	char flag = 0;
	unsigned long long int file_i = 0, max_file_name = 256;
	char * filename = (char*) malloc (sizeof(char)*max_file_name);
	if (filename == NULL) {
		perror ("Can not allocate memory");
		return errno;
	}
	for (unsigned long long int j = 0; j < max_file_name; j++) filename[j] = '\0';
	while (i < length) {
	    if (flag == 0) {
            pt->command[i1] = input[i];
            if ((i1 > 0)&&(input[i] == ' ')&&(input[i - 1] != ' ')) {
				pt->command[i1] = '\0';
				while ((i < length-1)&&(input[i+1] == ' ')) i++;
			}
		    if (i == pt->command_length) {
			    pt->command = (char*)realloc(pt->command, \
										sizeof(char)*pt->command_length*2);
				pt->command_length = pt->command_length*2;
		    }
		    i1++;
		}
	    if (((input[i] == '>')||(input[i] == '<')||(i == length-1))&&(flag == 0)) {
			if ((pt->command[i1-1] == '\0')||(pt->command[i1-1] == ' ') \
					||(pt->command[i1-1] == '>')||(pt->command[i1-1] == '<')) i1--;
			pt->command[i1] = '\0';
			i1++;
			pt->command[i1] = '\0'; //two in row - end of command
			flag = 1;
		}
	    
	    if (input[i] == '>') {
			i++;
            while ((i < length)&&(input[i] == ' ')) i++;
            if (input[i] == '>') {
  	            open_mode = O_APPEND | O_WRONLY | O_CREAT;
  	            i++;
  	            
		    } else open_mode = O_TRUNC | O_WRONLY | O_CREAT;
			if (i == length) {
				fprintf (stderr, "Empty filename");
			}
			while ((i < length)&&(input[i] == ' ')) i++;
			file_i = 0;
			while ((i < length)&&(input[i] != '<')&&(input[i] != '>')) {
				filename[file_i] = input[i];
				file_i++;
				i++;
				if (file_i == max_file_name) {
					max_file_name = max_file_name*2;
					filename = (char *) realloc (filename, max_file_name);
				}
			}
			while (filename[file_i-1] == ' ') file_i--;
			filename[file_i] = '\0';
			if (filename[0] != '\0') {
				//operate with the info
				if ((pt->files->fd_array[pt->files->num_of_file] = \
										open(filename, open_mode, 0644)) == -1) {
					perror ("Opening file");
					fprintf (stderr, "file's name: %s\n", filename);
					(pt->files->num_of_file)--;
				}	
				pt->files->open_mode[pt->files->num_of_file] = open_mode;
				pt->files->num_of_file++;
				if (pt->files->num_of_file == pt->files->max_file_amount) {
					pt->files->fd_array = (int*)realloc(pt->files->fd_array, \
										sizeof(int)*pt->files->max_file_amount*2);
					pt->files->open_mode = (int*)realloc(pt->files->open_mode, \
										sizeof(int)*pt->files->max_file_amount*2);
					pt->files->max_file_amount = pt->files->max_file_amount*2;
				}
				//end of operation
				i--;
			}
		}
		
		if (input[i] == '<') {
			for (unsigned long long int j = 0; j < max_file_name; j++) filename[j] = '\0';
			i++;
			while ((i < length)&&(input[i] == ' ')) i++;
			if (i == length) {
				perror ("Syntax error: '>' and '<' must me followed by a filename");
				free(filename);
				return -1;
			}
            open_mode = O_RDONLY | O_CREAT;
		    if (i == length) {
				perror ( "Syntax error: '>' and '<' must me followed by a filename");
				free(filename);
				return -1;
			}
			file_i = 0;
			while ((i < length)&&(input[i] != '<')&&(input[i] != '>')) {
				filename[file_i] = input[i];
				file_i++;
				if (file_i == max_file_name) {
					max_file_name = max_file_name*2;
					filename = (char *) realloc (filename, max_file_name);
				}
				i++;
			}
			while (filename[file_i-1] == ' ') file_i--;
			filename[file_i] = '\0';
			if (filename[0] != '\0') {
				//operate with the info
				if ((pt->files->fd_array[pt->files->num_of_file] = \
									open(filename, open_mode, 0644)) == -1) {
					perror ("Can not open the file");
					(pt->files->num_of_file)--;
				}	
				pt->files->open_mode[pt->files->num_of_file] = open_mode;
				pt->files->num_of_file++;
				if (pt->files->num_of_file == pt->files->max_file_amount) {
					pt->files->fd_array = (int*)realloc(pt->files->fd_array, \
											sizeof(int)*pt->files->max_file_amount*2);
					pt->files->open_mode = (int*)realloc(pt->files->open_mode, \
											sizeof(int)*pt->files->max_file_amount*2);
					pt->files->max_file_amount = pt->files->max_file_amount*2;
				}
				//end of operation
				i--;
			}
		}
		i++;
    }
    free(filename);
	return 0;
}

//
#define CLOSEFD for (int j = 0; j < num_of_pipes ; j++) { \
			if (fd[j][1] > 2) close(fd[j][1]); if (fd[j][0] > 2) close (fd[j][0]);}
#define DUPSTD(stream) if (dup(stream) == -1) { \
								perror ("Can not create a copy of stream");return -1;}
int conveyor (char * input, int fd_in, int fd_out, struct Unit * unit_array, char ** expr_fork) {
	unsigned long long int i = 0, length = strlen(input), cur = 0, num_of_childs = 0;
	int tmp_i = 0, num_of_pipes = 2, cur_pipe = 0, cur_fork = 0;
	int status;
	pid_t pid;
	char tmpbuf[16];
	for (i = 0; i < length; i++) {
		if ((input[i] == '|')||(input[i] == '=')||(input[i] == '/') \
						||(input[i] == ';')) num_of_pipes++;
	}
	i = 0;
	
	int fd[num_of_pipes][2]; //[1] - write, [0] - read
	
	for (tmp_i = 0; tmp_i < num_of_pipes; tmp_i++) {
		pipe(fd[tmp_i]);
		dup(STDOUT_FILENO);
		dup(STDIN_FILENO);
	}
	fd[0][0] = fd_in;
	fd[num_of_pipes-1][1] = fd_out;
	tmp_i = 0;

	while (i < length) {
		if (input[i] != 'f') {
			tmp_i = 0;
			while ((i < length)&&(in(input[i],numbers))) {
				tmpbuf[tmp_i] = input[i];
				i++;
				tmp_i++;
			}
			tmpbuf[tmp_i] = '\0';
			tmp_i = 0;
			cur = atoll(tmpbuf);
			//input and output
			if ((i < length)&&((input[i] == ';')||(input[i] == '/')||(input[i] == '='))) {
				if (fd[cur_pipe][0] > 2) close (fd[cur_pipe][0]);
				if (fd[cur_pipe + 1][1] > 2) close (fd[cur_pipe + 1][1]);
				fd[cur_pipe + 1][1] = fd_out;
				fd[cur_pipe][0] = fd_in;
			}
			for (int k = 0; k < unit_array[cur].files->num_of_file; k++) {
				char redir1 = 1, redir2 = 1;
				int tmpfd = unit_array[cur].files->fd_array[k];
				if (redir1&&(unit_array[cur].files->open_mode[k] == (O_RDONLY|O_CREAT))) {
				    if (fd[cur_pipe][0] > 2) close (fd[cur_pipe][0]);
				    fd[cur_pipe][0] = tmpfd;
				    redir1--;
			    }
				else if (redir2) {
					if (fd[cur_pipe + 1][1] > 2) close (fd[cur_pipe + 1][1]);
					if (fd[cur_pipe + 1][0] > 2) close (fd[cur_pipe + 1][0]);
					fd[cur_pipe + 1][1] = tmpfd;
					fd[cur_pipe + 1][0] = STDIN_FILENO;
					redir2--;
				}
				if ((redir1)&&(redir2)) break;
			}
			
			if ((pid = fork()) == 0) {
				//child
				char *args[MAX_ARGS];
				int args_i = 0, cur_arg = 1;
				unsigned long long int l_com = 1;
				while (!((unit_array[cur].command[l_com] == '\0') \
						&&((unit_array[cur].command[l_com-1] == '\0')))) {
					l_com++;
				}
				args[0] = unit_array[cur].command;
					
				while (args_i < l_com - 1) {
					while ((args_i < l_com - 1) \
								&&(unit_array[cur].command[args_i] != '\0')) args_i++;
					if (args_i < l_com - 1) {
						args[cur_arg] = unit_array[cur].command + args_i + 1;
						cur_arg++;
						args_i++;
					}
				}
				args[cur_arg] = NULL;
				int p = 0;
				while ((unit_array[cur].command[p] != ' ') \
							&&(p < strlen(unit_array[cur].command))) {
					p++;
				}
				//int save_in, save_out;
				dup2(fd[cur_pipe][0], STDIN_FILENO);
				dup2(fd[cur_pipe + 1][1], STDOUT_FILENO);
				CLOSEFD;
				execvp (args[0],args);
				perror ("Can not execute the programm");
				exit(EXIT_FAILURE);

			} else if (pid == -1) {
				perror ("Can not create a son process");
			}
		} else if (input[i] == 'f') {
			i++; //skip 'f'
			while ((i < length)&&(in(input[i],numbers))) {
				tmpbuf[tmp_i] = input[i];
				i++;
				tmp_i++;
			}
			tmpbuf[tmp_i] = '\0';
			tmp_i = 0;
			cur_fork = atoll(tmpbuf);
			
			//input and output
			if ((i < length)&&((input[i] == ';')||(input[i] == '/')||(input[i] == '='))) {
				if (fd[cur_pipe][0] > 2) close (fd[cur_pipe][0]);
				if (fd[cur_pipe + 1][1] > 2) close (fd[cur_pipe + 1][1]);
				fd[cur_pipe + 1][1] = fd_out;
				fd[cur_pipe][0] = fd_in;
			}
			
			//case of (...) > file1 < file2 
			unsigned long long int k = strlen(expr_fork[cur_fork]) - 1;
			char redir1 = 1, redir2 = 1;
			while ((expr_fork[cur_fork][k] != ')')&&(redir1||redir2)) {
				//from the end
				while ((expr_fork[cur_fork][k] != '>')&&(expr_fork[cur_fork][k] != '<') \
							&&((expr_fork[cur_fork][k] != ')'))) k--;
				if (expr_fork[cur_fork][k] == ')') {
					expr_fork[cur_fork][k+1] = '\0';
					k++;
					redir1 = 0;
					redir2 = 0;
				} else if ((expr_fork[cur_fork][k] == '<')&&(redir1)) {
					int max_length_tmpbuf = 256, i_tmp = 0, save_k = k+1;
					while ((save_k < strlen(expr_fork[cur_fork])) \
							&&(expr_fork[cur_fork][save_k] == ' ')) save_k++;
					
					char *filename = (char*) malloc (sizeof(char)*max_length_tmpbuf);
					while ((save_k < strlen(expr_fork[cur_fork])) \
								&&(expr_fork[cur_fork][save_k] != '>') \
										&&(expr_fork[cur_fork][save_k] != '<')) {
						filename[i_tmp] = expr_fork[cur_fork][save_k]; 
						save_k ++;
						i_tmp++;
						if (i_tmp == max_length_tmpbuf) {
							max_length_tmpbuf *= 2;
							filename = (char*)realloc(filename, \
										sizeof(char)*max_length_tmpbuf);
						}
					}
					filename[i_tmp] = '\0';
					
					if (filename[0] != '\0') {
						int fd_fork;
						//operate with the info
						if ((fd_fork = open(filename, O_RDONLY | O_CREAT, 0644)) == -1) {
							perror ("Can not open the file");
						} else {
							fd[cur_pipe][0] = fd_fork;
							redir1 = 0;
						}
					}
					free(filename);
					
				} else if ((expr_fork[cur_fork][k] == '>')&&(redir2)) {
					int fork_open_mode = 0;
					if ((k > 0)&&(expr_fork[cur_fork][k-1] == '>')) 
							fork_open_mode = O_APPEND | O_WRONLY | O_CREAT;
					else fork_open_mode = O_TRUNC | O_WRONLY | O_CREAT;
					
					int max_length_tmpbuf = 256, i_tmp = 0, save_k = k+1;
					while ((save_k < strlen(expr_fork[cur_fork])) \
							&&(expr_fork[cur_fork][save_k] == ' ')) save_k++;
						
					char *filename = (char*) malloc (sizeof(char)*max_length_tmpbuf);
					while ((save_k < strlen(expr_fork[cur_fork])) \
								&&(expr_fork[cur_fork][save_k] != '>') \
										&&(expr_fork[cur_fork][save_k] != '<')) {
						filename[i_tmp] = expr_fork[cur_fork][save_k]; 
						save_k ++;
						i_tmp++;
						if (i_tmp == max_length_tmpbuf) {
							max_length_tmpbuf *= 2;
							filename = (char*)realloc(filename, \
										sizeof(char)*max_length_tmpbuf);
						}
					}
					filename[i_tmp] = '\0';
					if (filename[0] != '\0') {
						int fd_fork;
						//operate with the info
						if ((fd_fork = open(filename, fork_open_mode, 0644)) == -1) {
							perror ("Can not open the file");
						} else {
							fd[cur_pipe + 1][1] = fd_fork;
							redir2 = 0;
						}
					}
					free(filename);
				}
				k--;
			}
			expr_fork[cur_fork][k] = '\0';
			if ((pid = fork()) == 0) {
				//child
				my_shell_exec (expr_fork[cur_fork] + 1, strlen(expr_fork[cur_fork]), \
												fd[cur_pipe][0], fd[cur_pipe + 1][1]);
				
			} else if (pid == -1) {
				perror ("Can not create a son process");
			}
			cur_fork++;
		}
		
		cur_pipe++;
		if (i < length) {
			// check next symbol
			if (input[i] == ';') {
				waitpid (pid, &status, 0);
		    } else if (input[i] == '=') {
				waitpid (pid, &status, 0);
				if (WEXITSTATUS(status) == EXIT_SUCCESS) {
					CLOSEFD;
					exit(EXIT_SUCCESS);
				}
			} else if (input[i] == '/') {
				waitpid (pid, &status, 0);
				if (WEXITSTATUS(status) == EXIT_FAILURE) {
					CLOSEFD;
					exit(EXIT_FAILURE);
				}
			} else num_of_childs++;
		} 
		i++;
	}
	CLOSEFD;
	//wait for every child
	num_of_childs++;
	//fprintf (stderr, "number of children to wait = %lli\n", num_of_childs);
	while (num_of_childs) {
		wait(&status);
		num_of_childs--;
	}
	exit(WEXITSTATUS(status));
}

