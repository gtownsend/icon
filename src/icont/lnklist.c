/*
 * lnklist.c -- functions for handling file linking.
 */

#include "../h/gsupport.h"
#include "tproto.h"
#include "lfile.h"

/*
 * Prototype.
 */
static	struct lfile *alclfile	(char *name);

struct lfile *lfiles;
struct invkl *invkls;

/*
 * addinvk adds an "invokable" name to the list.
 *  n==1 if name is an identifier; otherwise it is a string literal.
 */
void addinvk(name, n)
char *name;
int n;
   {
   struct invkl *p;

   if (n == 1) {			/* if identifier, must be "all" */
      if (strcmp(name, "all") != 0) {
         tfatal("invalid operand to invocable", name);
         return;
         }
      else
         name = "0";			/* "0" represents "all" */
      }
   else if (!isalpha(name[0]) && (name[0] != '_'))
      return;				/* if operator, ignore */

   p = alloc(sizeof(struct invkl));
   if (!p)
      tsyserr("not enough memory for invocable list");
   p->iv_name = salloc(name);
   p->iv_link = invkls;
   invkls = p;
   }

/*
 * alclfile allocates an lfile structure for the named file, fills
 *  in the name and returns a pointer to it.
 */
static struct lfile *alclfile(name)
char *name;
   {
   struct lfile *p;

   p = alloc(sizeof(struct lfile));
   if (!p)
      tsyserr("not enough memory for file list");
   p->lf_link = NULL;
   p->lf_name = salloc(name);
   return p;
   }

/*
 * addlfile creates an lfile structure for the named file and add it to the
 *  end of the list of files (lfiles) to generate link instructions for.
 */
void addlfile(name)
char *name;
   {
   struct lfile *nlf, *p;

   nlf = alclfile(name);
   if (lfiles == NULL) {
      lfiles = nlf;
      }
   else {
      p = lfiles;
      while (p->lf_link != NULL) {
         p = p->lf_link;
         }
      p->lf_link = nlf;
      }
   }
