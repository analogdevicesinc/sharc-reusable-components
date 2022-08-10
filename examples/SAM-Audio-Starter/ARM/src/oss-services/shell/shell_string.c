#include <string.h>
#include <stdlib.h>

#include "shell.h"

char *shell_strdup(const char *str)
{
   size_t size;
   char *copy;

   size = strlen(str) + 1;
   if ((copy = SHELL_MALLOC(size)) == NULL)
      return(NULL);

   (void)memcpy(copy, str, size);
   return(copy);
}

char *shell_strndup(const char *s, size_t n)
{
  char *result;
  size_t size = strlen(s);

  if (n < size)
    size = n;

  result = (char *)SHELL_MALLOC(size + 1);
  if (!result)
    return 0;

  result[size] = '\0';
  return ((char *)memcpy(result, s, size));
}
