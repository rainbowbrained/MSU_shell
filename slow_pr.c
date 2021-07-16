#include <stdio.h>
#include <unistd.h>	
#include <ctype.h>	

int main (void) //./prog, p, f
{
	for (int i = 0; i < 5; i++) {
		fprintf (stdout,"pid = %i, cur time = %i: I am still working!\n", getpid(), i);
		sleep(1);
	}
    return 0;
}
