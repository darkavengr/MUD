#include <stdint.h>
#include <stddef.h>
#include "size.h"

void ToUppercase(char *string,char *out);
int TokenizeLine(char *linebuf,char *tokens[][BUF_SIZE],char *split);
unsigned int regexp(char *filename,char *mask);
int GetValueFromTimeString(char *str);
void CreateTimeString(int time,char *b);
void RemoveNewLine(char *line);
void WildcardToSQLWildcard(char *wildcard,char *out);

