#include <stdint.h>
#include <stddef.h>
#include "size.h"

void itoa(unsigned int n, char s[]);
void reverse(char s[]);
void ToUppercase(char *string);
void WildcardRename(char *name,char *mask,char *out);
void ToHex(uint32_t hex,char *buf);
int TokenizeLine(char *linebuf,char *tokens[][BUF_SIZE],char *split);
void WildcardRename(char *name,char *mask,char *out);
unsigned int regexp(char *filename,char *mask);
int TCPReadLine(int socket,char *buf);
int GetValueFromTimeString(char *str);
void CreateTimeString(int time,char *b);
void RemoveNewLine(char *line);

