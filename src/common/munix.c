/*
 *  munix.c -- special common code from Unix
 *
 *  (Originally used only under Unix, but now on all platforms.)
 */

#include "../h/gsupport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
 *  relfile(prog, mod) -- find related file.
 *
 *  Given that prog is the argv[0] by which this program was executed,
 *  and assuming that it was set by the shell or other equally correct
 *  invoker, relfile finds the location of a related file and returns
 *  it in an allocated string.  It takes the location of prog, appends
 *  mod, and canonizes the result; thus if argv[0] is icont or its path,
 *  relfile(argv[0],"/../iconx") finds the location of iconx.
 */
char *relfile(char *prog, char *mod) {
   static char baseloc[MaxPath];
   char buf[MaxPath];

   if (baseloc[0] == 0) {		/* if argv[0] not already found */

      #if CYGWIN
         char posix_prog[_POSIX_PATH_MAX + 1];
         cygwin_conv_to_posix_path(prog, posix_prog);
         prog = posix_prog;
      #endif				/* CYGWIN */

      if (findexe(prog, baseloc, sizeof(baseloc)) == NULL) {
         fprintf(stderr, "cannot find location of %s\n", prog);
         exit(EXIT_FAILURE);
         }
      if (followsym(baseloc, buf, sizeof(buf)) != NULL)
         strcpy(baseloc, buf);
   }

   strcpy(buf, baseloc);		/* start with base location */
   strcat(buf, mod);			/* append adjustment */
   canonize(buf);			/* canonize result */
   if (mod[strlen(mod)-1] == '/')	/* if trailing slash wanted */
      strcat(buf, "/");			/* append to result */
   return salloc(buf);			/* return allocated string */
   }

/*
 *  findexe(prog, buf, len) -- find absolute executable path, given argv[0]
 *
 *  Finds the absolute path to prog, assuming that prog is the value passed
 *  by the shell in argv[0].  The result is placed in buf, which is returned.
 *  NULL is returned in case of error.
 */

char *findexe(char *name, char *buf, size_t len) {
   int n;
   char *s;

   if (name == NULL)
      return NULL;

   /* if name does not contain a slash, search $PATH for file */
   if (strchr(name, '/') != NULL)
      strcpy(buf, name);
   else if (findonpath(name, buf, len) == NULL)
      return NULL;

   /* if path is not absolute, prepend working directory */
   if (buf[0] != '/') {
      n = strlen(buf) + 1;
      memmove(buf + len - n, buf, n);
      if (getcwd(buf, len - n) == NULL)
         return NULL;
      s = buf + strlen(buf);
      *s = '/';
      memcpy(s + 1, buf + len - n, n);
      }
   canonize(buf);
   return buf;
   }

/*
 *  findonpath(name, buf, len) -- find name on $PATH
 *
 *  Searches $PATH (using POSIX 1003.2 rules) for executable name,
 *  writing the resulting path in buf if found.
 */
char *findonpath(char *name, char *buf, size_t len) {
   int nlen, plen;
   char *path, *next, *sep, *end;
   struct stat status;

   nlen = strlen(name);
   path = getenv("PATH");

   if (path == NULL || *path == '\0')
      path = ".";
   #if CYGWIN
      else {
         char *posix_path;
         posix_path = alloca(cygwin_win32_to_posix_path_list_buf_size(path));
         cygwin_win32_to_posix_path_list(path, posix_path);
         path = posix_path;
         }
   #endif				/* CYGWIN */

   end = path + strlen(path);
   for (next = path; next <= end; next = sep + 1) {
      sep = strchr(next, ':');
      if (sep == NULL)
         sep = end;
      plen = sep - next;
      if (plen == 0) {
         next = ".";
         plen = 1;
         }
      if (plen + 1 + nlen + 1 > len)
         return NULL;
      memcpy(buf, next, plen);
      buf[plen] = '/';
      strcpy(buf + plen + 1, name);
      if (access(buf, X_OK) == 0) {
         if (stat(buf, &status) == 0 && S_ISREG(status.st_mode))
            return buf;
         }
      }
   return NULL;
   }

/*
 *  followsym(name, buf, len) -- follow symlink to final destination.
 *
 *  If name specifies a file that is a symlink, resolves the symlink to
 *  its ultimate destination, and returns buf.  Otherwise, returns NULL.
 *
 *  Note that symlinks in the path to name do not make it a symlink.
 *
 *  buf should be long enough to hold name.
 */

#define MAX_FOLLOWED_LINKS 24

char *followsym(char *name, char *buf, size_t len) {
   int i, n;
   char *s, tbuf[MaxPath];

   strcpy(buf, name);

   for (i = 0; i < MAX_FOLLOWED_LINKS; i++) {
      if ((n = readlink(buf, tbuf, sizeof(tbuf) - 1)) <= 0)
         break;
      tbuf[n] = 0;

      if (tbuf[0] == '/') {
         if (n < len)
            strcpy(buf, tbuf);
         else
            return NULL;
         }
      else {
         s = strrchr(buf, '/');
         if (s != NULL)
            s++;
         else
            s = buf;
         if ((s - buf) + n < len)
            strcpy(s, tbuf);
         else
            return NULL;
         }
      canonize(buf);
      }

   if (i > 0 && i < MAX_FOLLOWED_LINKS)
      return buf;
   else
      return NULL;
   }

/*
 *  canonize(path) -- put file path in canonical form.
 *
 *  Rewrites path in place, and returns it, after excising fragments of
 *  "." or "dir/..".  All leading slashes are preserved but other extra
 *  slashes are deleted.  The path never grows longer except for the
 *  special case of an empty path, which is rewritten to be ".".
 *
 *  No check is made that any component of the path actually exists or
 *  that inner components are truly directories.  From this it follows
 *  that if "foo" is any file path, canonizing "foo/.." produces the path
 *  of the directory containing "foo".
 */

char *canonize(char *path) {
   int len;
   char *root, *end, *in, *out, *prev;

   /* initialize */
   root = path;				/* set barrier for trimming by ".." */
   end = path + strlen(path);		/* set end of input marker */
   while (*root == '/')			/* preserve all leading slashes */
      root++;
   in = root;				/* input pointer */
   out = root;				/* output pointer */

   /* scan string one component at a time */
   while (in < end) {

      /* count component length */
      for (len = 0; in + len < end && in[len] != '/'; len++)
         ;

      /* check for ".", "..", or other */
      if (len == 1 && *in == '.')	/* just ignore "." */
         in++;
      else if (len == 2 && in[0] == '.' && in[1] == '.') {
         in += 2;			/* skip over ".." */
         /* find start of previous component */
         prev = out;
         if (prev > root)
            prev--;			/* skip trailing slash */
         while (prev > root && prev[-1] != '/')
            prev--;			/* find next slash or start of path */
         if (prev < out - 1
         && (out - prev != 3 || strncmp(prev, "../", 3) != 0)) {
            out = prev;		/* trim trailing component */
            }
         else {
            memcpy(out, "../", 3);	/* cannot trim, so must keep ".." */
            out += 3;
            }
         }
      else {
         memmove(out, in, len);		/* copy component verbatim */
         out += len;
         in += len;
         *out++ = '/';			/* add output separator */
         }

      while (in < end && *in == '/')	/* consume input separators */
         in++;
      }

   /* final fixup */
   if (out > root)
      out--;				/* trim trailing slash */
   if (out == path)
      *out++ = '.';			/* change null path to "." */
   *out++ = '\0';
   return path;				/* return result */
   }
