#include <ctype.h>
isalpha('a')

/* test macro substitution for system file include */
#define ctype_h <ctype.h>
#include ctype_h
isalpha('b')
