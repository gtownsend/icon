/*
 * This file contains pathfind(), fparse(), makename(), and smatch().
 */
#include "../h/gsupport.h"

static char *pathelem	(char *s, char *buf);
static char *tryfile	(char *buf, char *dir, char *name, char *extn);

/*
 *  Define symbols for building file names.
 *  1. Prefix: the characters that terminate a file name prefix
 *  2. FileSep: the char to insert after a dir name, if any
 *  3. DefPath: the default IPATH/LPATH
 *  4. PathSep: allowable IPATH/LPATH separators
 *
 *  All platforms use POSIX forms of file paths.
 *  MS Windows implementations canonize local forms before parsing.
 */

#define Prefix "/"
#define FileSep '/'
#define PathSep " :"
#define DefPath ""

/*
 * pathfind(buf,path,name,extn) -- find file in path and return name.
 *
 *  pathfind looks for a file on a path, begining with the current
 *  directory.  Details vary by platform, but the general idea is
 *  that the file must be a readable simple text file.  pathfind
 *  returns buf if it finds a file or NULL if not.
 *
 *  buf[MaxPath] is a buffer in which to put the constructed file name.
 *  path is the IPATH or LPATH value, or NULL if unset.
 *  name is the file name.
 *  extn is the file extension (.icn or .u1) to be appended, or NULL if none.
 */
char *pathfind(buf, path, name, extn)
char *buf, *path, *name, *extn;
   {
   char *s;
   char pbuf[MaxPath];

   if (tryfile(buf, (char *)NULL, name, extn))	/* try curr directory first */
      return buf;
   if (!path)				/* if no path, use default */
      path = DefPath;

   #if CYGWIN
      s = alloca(cygwin_win32_to_posix_path_list_buf_size(path));
      cygwin_win32_to_posix_path_list(path, s);
   #else				/* CYGWIN */
      s = path;
   #endif				/* CYGWIN */

   while ((s = pathelem(s, pbuf)) != 0)		/* for each path element */
      if (tryfile(buf, pbuf, name, extn))	/* look for file */
         return buf;
   return NULL;				/* return NULL if no file found */
   }

/*
 * pathelem(s,buf) -- copy next path element from s to buf.
 *
 *  Returns the updated pointer s.
 */
static char *pathelem(s, buf)
char *s, *buf;
   {
   char c;

   while ((c = *s) != '\0' && strchr(PathSep, c))
      s++;
   if (!*s)
      return NULL;
   while ((c = *s) != '\0' && !strchr(PathSep, c)) {
      *buf++ = c;
      s++;
      }

   #ifdef FileSep
      /*
       * We have to append a path separator here.
       *  Seems like makename should really be the one to do that.
       */
      if (!strchr(Prefix, buf[-1])) {	/* if separator not already there */
         *buf++ = FileSep;
         }
   #endif				/* FileSep */

   *buf = '\0';
   return s;
   }

/*
 * tryfile(buf, dir, name, extn) -- check to see if file is readable.
 *
 *  The file name is constructed in buf from dir + name + extn.
 *  findfile returns buf if successful or NULL if not.
 */
static char *tryfile(buf, dir, name, extn)
char *buf, *dir, *name, *extn;
   {
   FILE *f;
   makename(buf, dir, name, extn);
   if ((f = fopen(buf, "r")) != NULL) {
      fclose(f);
      return buf;
      }
   else
      return NULL;
   }

/*
 * fparse - break a file name down into component parts.
 *  Result is a pointer to a struct of static pointers good until the next call.
 */
struct fileparts *fparse(s)
char *s;
   {
   static char buf[MaxPath+2];
   static struct fileparts fp;
   int n;
   char *p, *q;

   #if CYGWIN
      char posix_s[_POSIX_PATH_MAX + 1];
      cygwin_conv_to_posix_path(s, posix_s);
      s = posix_s;
   #endif				/* CYGWIN */

   q = s;
   fp.ext = p = s + strlen(s);
   while (--p >= s) {
      if (*p == '.' && *fp.ext == '\0')
         fp.ext = p;
      else if (strchr(Prefix,*p)) {
         q = p+1;
         break;
         }
      }

   fp.dir = buf;
   n = q - s;
   strncpy(fp.dir,s,n);
   fp.dir[n] = '\0';
   fp.name = buf + n + 1;
   n = fp.ext - q;
   strncpy(fp.name,q,n);
   fp.name[n] = '\0';
   p = fp.ext;
   fp.ext = fp.name + n + 1;
   strcpy(fp.ext, p);

   return &fp;
   }

/*
 * makename - make a file name, optionally substituting a new dir and/or ext
 */
char *makename(dest,d,name,e)
char *dest, *d, *name, *e;
   {
   struct fileparts fp;
   fp = *fparse(name);
   if (d != NULL)
      fp.dir = d;
   if (e != NULL)
      fp.ext = e;
   sprintf(dest,"%s%s%s",fp.dir,fp.name,fp.ext);
   return dest;
   }

/*
 * smatch - case-insensitive string match - returns nonzero if they match
 */
int smatch(s,t)
char *s, *t;
   {
   char a, b;
   for (;;) {
      while (*s == *t)
         if (*s++ == '\0')
            return 1;
         else
            t++;
      a = *s++;
      b = *t++;
      if (isupper(a))  a = tolower(a);
      if (isupper(b))  b = tolower(b);
      if (a != b)
         return 0;
      }
   }

#if MSWIN

FILE *pathOpen(fname, mode)
   char *fname;
   char *mode;
   {
   char buf[MaxPath];
   int i;

   for (i = 0; fname[i] != '\0'; i++) {
      if (fname[i] == '/' || fname[i] == ':' || fname[i] == '\\') {
         /* fname contains an explicit path */
         return fopen(fname, mode);
         }
      }

   if (!pathfind(buf, getenv("PATH"), fname, NULL))
      return 0;

   return fopen(buf, mode);
   }

#endif					/* MSWIN */
