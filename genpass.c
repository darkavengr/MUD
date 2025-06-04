/* generate password */

#include <stdio.h>
#include <unistd.h>
#include <crypt.h>
#include <stdlib.h>
#include <string.h>

#define _XOPEN_SOURCE

int main(int argc,char **argv)
{
char *buf[255];

if(argc < 2) {			/* no arguments */
	printf("generate password for AdventureMUD\n\n");
	printf("\ngenpass [username] [password]\n");
	exit(0);
}

strcpy(buf,argv[1]);
printf("%s\n",crypt(argv[2],buf));

exit(0);
}

