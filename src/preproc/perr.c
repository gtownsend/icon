/*
 * The functions in this file print error messages.
 */
#include "../preproc/preproc.h"
#include "../preproc/pproto.h"
extern char *progname;

/*
 * Prototypes for static functions.
 */
static void rm_files (void);


/*
 * File list.
 */
struct finfo_lst {
   char *name;                  /* file name */
   FILE *file;                  /* file */
   struct finfo_lst *next;      /* next entry in list */
   };

static struct finfo_lst *file_lst = NULL;

/*
 * errt1 - error message in one string, location indicated by a token.
 */
void errt1(t, s)
struct token *t;
char *s;
   {
   errfl1(t->fname, t->line, s);
   }

/*
 * errfl1 - error message in one string, location given by file and line.
 */
void errfl1(f, l, s)
char *f;
int l;
char *s;
   {
   fflush(stdout);
   fprintf(stderr, "%s: File %s; Line %d: %s\n", progname, f, l, s);
   rm_files();
   exit(EXIT_FAILURE);
   }

/*
 * err1 - error message in one string, no location given
 */
void err1(s)
char *s;
   {
   fflush(stdout);
   fprintf(stderr, "%s: %s\n", progname, s);
   rm_files();
   exit(EXIT_FAILURE);
   }

/*
 * errt2 - error message in two strings, location indicated by a token.
 */
void errt2(t, s1, s2)
struct token *t;
char *s1;
char *s2;
   {
   errfl2(t->fname, t->line, s1, s2);
   }

/*
 * errfl2 - error message in two strings, location given by file and line.
 */
void errfl2(f, l, s1, s2)
char *f;
int l;
char *s1;
char *s2;
   {
   fflush(stdout);
   fprintf(stderr, "%s: File %s; Line %d: %s%s\n", progname, f, l, s1, s2);
   rm_files();
   exit(EXIT_FAILURE);
   }

/*
 * err2 - error message in two strings, no location given
 */
void err2(s1, s2)
char *s1;
char *s2;
   {
   fflush(stdout);
   fprintf(stderr, "%s: %s%s\n", progname, s1, s2);
   rm_files();
   exit(EXIT_FAILURE);
   }

/*
 * errt3 - error message in three strings, location indicated by a token.
 */
void errt3(t, s1, s2, s3)
struct token *t;
char *s1;
char *s2;
char *s3;
   {
   errfl3(t->fname, t->line, s1, s2, s3);
   }

/*
 * errfl3 - error message in three strings, location given by file and line.
 */
void errfl3(f, l, s1, s2, s3)
char *f;
int l;
char *s1;
char *s2;
char *s3;
   {
   fflush(stdout);
   fprintf(stderr, "%s: File %s; Line %d: %s%s%s\n", progname, f, l,
       s1, s2, s3);
   rm_files();
   exit(EXIT_FAILURE);
   }

/*
 * addrmlst - add a file name to the list of files to be removed if
 *   an error occurs.
 */
void addrmlst(fname, f)
char *fname;
FILE *f;
   {
   struct finfo_lst *id;

   id = NewStruct ( finfo_lst );
   id->name = fname;
   id->file = f;
   id->next = file_lst;
   file_lst = id;
   }

/*
 * rm_files - remove files that must be cleaned up in the event of an
 *   error.
 */
static void rm_files()
   {
   while (file_lst != NULL) {
      fclose ( file_lst->file );
      remove(file_lst->name);
      file_lst = file_lst->next;
      }
   }
