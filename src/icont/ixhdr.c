/*
 * ixhdr.c -- bootstrap header for icode files
 *
 * (used as an alternative to ShellHeader)
 */

#include "../h/gsupport.h"
#include "tproto.h"

#include "../h/header.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

static	void	doiconx		(char **s, char *t);
static	void	hsyserr		(char *av, char *file);

static char patchpath[MaxPath + 18] = "%PatchStringHere->";

#ifdef HardWiredPaths
   static char *refpath = RefPath;
#endif					/* HardWiredPaths */

int main(argc, argv)
int argc;
char **argv;
   {
   char fullpath[256];
   char *argvx[1000];

   #if CYGWIN
      char name[_POSIX_PATH_MAX + 1];
   #else				/* CYGWIN */
      char *name;
   #endif				/* CYGWIN */

   /*
    * Abort if we've been invoked with setuid or setgid privileges.
    *  Allowing such usage would open a huge security hole, because
    *  there is no way to ensure that the right iconx will interpret
    *  the right user program.
    */
   if (getuid() != geteuid() || getgid() != getegid())
      hsyserr(argv[0], ": cannot run an Icon program setuid/setgid");

   #if CYGWIN
      cygwin_conv_to_posix_path(argv[0], name);
   #else				/* CYGWIN */
      name = argv[0];		/* name of icode file */
   #endif				/* CYGWIN */

   do 
      argvx[argc+1] = argv[argc];
   while (argc--);
   argv = argvx;

   /*
    * If the name contains any slashes, execute the file as named.
    *  Otherwise, search the path to find out where the file really is.
    */
   if (index(name, '/'))
      doiconx(argv, name);
#if MSWIN
   else if (chkcmd(name))
      doiconx(argv, name);
#endif					/* MSWIN */
   else if (findcmd(fullpath, name, "PATH"))
      doiconx(argv, fullpath);
   else
      hsyserr("iconx: icode file not found: ", fullpath);
   }

/*
 * doiconx(argv, file) - execute iconx, passing file as argv[1].
 *
 *  To find the interpreter, first check the environment variable ICONX.
 *  If it defines a path, it had better work, else we abort.
 *
 *  If there's no $ICONX, but there's a hardwired path, try that.
 *  If THAT doesn't work, try searching $PATH for an "iconx" program.
 *  If nothing works, abort.
 */
static void doiconx(argv, file)
char **argv;
char *file;
   {
   char xcmd[256];

#ifdef HardWiredPaths
   static char hardpath[MaxPath];

   if ((int)(strlen(refpath) + 6) > MaxPath)
      hsyserr("path to iconx too long", "");
   strcpy(hardpath, refpath);
   strcat(hardpath, "iconx");

   if ((int)strlen(patchpath) > 18)
      strcpy(hardpath, patchpath + 18);
#endif					/* HardWiredPaths */

   argv[1] = file;

   if ((argv[0] = getenv("ICONX")) != NULL && argv[0][0] != '\0') {
      execv(argv[0], argv);		/* exec file specified by $ICONX */
      hsyserr("cannot execute $ICONX: ", argv[0]);
      }

#ifdef HardWiredPaths
   argv[0] = hardpath;			/* try predefined file */
   execv(hardpath, argv);
#endif					/* HardWiredPaths */
 
   if (findcmd(xcmd, "iconx", "PATH")) {
      argv[0] = xcmd;
      execv(xcmd, argv);	/* if no path, search path for "iconx" */
      hsyserr("cannot execute ", xcmd);
      }

#ifdef HardWiredPaths
   hsyserr("cannot execute ", hardpath);
#else					/* HardWiredPaths */
   hsyserr("cannot find iconx", "");
#endif					/* HardWiredPaths */
   }

/*
 * hsyserr(s1, s2) - print s1 and s2 on stderr, then abort.
 */
static void hsyserr(s1, s2)
char *s1, *s2;
   {
   char s[100];
   /*
    * Low-level I/O is used here to avoid sucking in a lot of stuff
    *  that would make iconx.hdr much larger.
    */
   strcpy(s, s1);
   if (s2)
      strcat(s, s2);
   strcat(s, "\n");
   write(2, s, strlen(s));
   _exit(EXIT_FAILURE);
   }

/*
 * This function searches all the directories in the path style environment
 * variable (pvar) for a command with the base name (cnam).  If the command
 * is found its full name is stored in the pointer passed (cmd) and 1 is
 * returned.  If the command isn't found 0 is returned.
 */
int findcmd(cmd, cnam, pvar)
char *cmd, *cnam, *pvar;
   {
   char *path;
   char pbuf[1024], *getdir(), *getenv();

   /*
    * If the environment variable isn't defined, give up.
    */
   if (! (path = getenv(pvar)))
      return 0;

   /*
    *Copy the path to a temporary path buffer and point at the buffer.
    * (This is necessary because we can diddle its contents in getdir.)
    */
   #if CYGWIN
      cygwin_win32_to_posix_path_list(path, pbuf);
   #else				/* CYGWIN */
      strcpy(pbuf, path);
   #endif				/* CYGWIN */
   path = pbuf;

   /*
    * Loop through all the path variable directories and check
    * for the command in each.
    */
   while (path = getdir(cmd, path)) {
      strcat(cmd, cnam);
      if (chkcmd(cmd))
         return 1;
      }

   return 0;
   }

/*
 * This function returns the next directory in the directory string passed
 * in path.  This directory string is similar to the PATH environment
 * variable.  A pointer to the remaining directory string is returned so
 * that the entire string can be scanned by the calling function.
 */

char *getdir(dir, path)
char *dir;	/* the command directory buffer */
char *path;	/* the string of directories */
   {
   char *dp;	/* the original directory pointer */

   /* if there is nothing left, return null */
   if (! *path)
      return NULL;

   /* save the original directory pointer */
   dp = dir;

   /* copy up to the next separator or the end of path */
   while (*path && *path != ':')
      *dir++ = *path++;

   /* if dir is still empty, use a dot (the current directory */
   if (dp == dir)
      *dir++ = '.';

   /* if the directory isn't terminated with a slash, tack one on */
   if (*(dir-1) != '/')
      *dir++ = '/';

   /* null terminate the directory */
   *dir = 0;

   /* if there's still a colon in path */
   if (*path) {

      /* if there's something after the colon, skip the colon */
      if (*(path+1))
         path++;

      /* otherwise, terminate path with a dot (the current directory) */
      else
         *path = '.';
      }

   return path;
   }

/*
 * This function checks to see if the file name passed exists and is
 * executable.
 */
int chkcmd(file)
char *file;
   {
   unsigned short gid, uid;
   struct stat s;

   /* if the file can't be "stat"ed fail */
   if (stat(file, &s) < 0) return 0;

   /* get the effective group and user ids for this user */
   gid = getegid();
   uid = geteuid();

   /* if this is a "regular" file AND */
   if ((s.st_mode & 0100000) &&
      /* the execute bit is set for "other" and the group id and user id
       * don't match OR */
      (((s.st_mode & 0000001) && s.st_gid != gid && s.st_uid != uid) ||
      /* the execute bit is set for "group" and the group id matches and
       * the user id doesn't OR */
      ((s.st_mode & 0000010) && s.st_gid == gid && s.st_uid != uid) ||
      /* the execute bit is set for "user" and the user id matches THEN */
      ((s.st_mode & 0000100) && s.st_uid == uid)))
      /*  succeed */
      return 1;

   /* otherwise fail */
   return 0;
   }
