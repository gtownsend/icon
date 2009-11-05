/*
 * alloc.c -- allocation routines for the Icon compiler
 */

#include "../h/gsupport.h"

/*
 * salloc - allocate and initialize string
 */

char *salloc(s)
char *s;
   {
   register char *s1;

   s1 = (char *)malloc(strlen(s) + 1);
   if (s1 == NULL) {
      fprintf(stderr, "salloc(%d): out of memory\n", (int)strlen(s) + 1);
      exit(EXIT_FAILURE);
      }
   return strcpy(s1, s);
   }

/*
 * alloc - allocate n bytes
 */

pointer alloc(n)
unsigned int n;
   {
   register pointer a;

   if (n == 0)				/* Work-around for 0 allocation */
      n = 1;

   a = calloc(n, sizeof(char));
   if (a == NULL) {
      fprintf(stderr, "alloc(%d): out of memory\n", (int)n);
      exit(EXIT_FAILURE);
      }
   return a;
   }
