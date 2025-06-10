#include <stdint.h>
#include <stddef.h>
#include "size.h"

void itoa(unsigned int n, char s[]);
void reverse(char s[]);
unsigned int wildcard(char *mask,char *filename);
void touppercase(char *string);
void wildcard_rename(char *name,char *mask,char *out);
void tohex(uint32_t hex,char *buf);
int tokenize_line(char *linebuf,char *tokens[][BUF_SIZE],char *split);
unsigned int regexp(char *filename,char *mask);
int tcp_getline(int socket,char *buf);
int getvaluefromtimestring(char *str);
void createtimestring(int time,char *b);
void removenewline(char *line);

