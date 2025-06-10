/* generate password */

#include <stdio.h>
#include <unistd.h>
#include <crypt.h>
#include <stdlib.h>
#include <string.h>
#include "size.h"

#define _XOPEN_SOURCE

int main(int argc,char **argv)
{

if(argc < 2) {			/* no arguments */
	printf("generate password for AdventureMUD\n\n");
	printf("\genpass [username] [password]\n");
	exit(0);
}

printf("%s\n",crypt(argv[2],argv[1]));

exit(0);
}

