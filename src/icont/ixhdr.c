/*
 * ixhdr.c -- bootstrap header for icode files
 *
 * (used when BinaryHeader is defined)
 */

#include "../h/gsupport.h"
#include "tproto.h"

#include "../h/header.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

static	void	doiconx		(char *argv[]);
static	void	hsyserr		(char *av, char *file);

int main(int argc, char *argv[]) {
   char fullpath[256];
   char *argvx[1000];

   /*
    * Abort if we've been invoked with setuid or setgid privileges.
    *  Allowing such usage would open a huge security hole, because
    *  there is no way to ensure that the right iconx will interpret
    *  the right user program.
    */
   if (getuid() != geteuid() || getgid() != getegid())
      hsyserr(argv[0], ": cannot run an Icon program setuid/setgid");

   /*
    * Shift the argument list to make room for iconx in argv[0].
    */
   do 
      argvx[argc+1] = argv[argc];
   while (argc--);

   doiconx(argvx);
}

/*
 * doiconx(argv) - execute iconx, passing argument list.
 *
 *  To find the interpreter, first check the environment variable ICONX.
 *  If it defines a path, it had better work, else we abort.
 *
 *  If there's no $ICONX, but there's a hardwired path, try that.
 *  If THAT doesn't work, try searching $PATH for an "iconx" program.
 *  If nothing works, abort.
 */
static void doiconx(char *argv[]) {
   char xcmd[256];

   if ((argv[0] = getenv("ICONX")) != NULL && argv[0][0] != '\0') {
      execv(argv[0], argv);		/* exec file specified by $ICONX */
      hsyserr("cannot execute $ICONX: ", argv[0]);
      }

   if (findonpath("iconx", xcmd, sizeof(xcmd))) {    /* if iconx on $PATH */
      argv[0] = xcmd;
      execv(xcmd, argv);
      hsyserr("cannot execute ", xcmd);
      }

   hsyserr(argv[1], ": cannot find iconx");
   }

/*
 * hsyserr(s1, s2) - print s1 and s2 on stderr, then abort.
 */
static void hsyserr(char *s1, char *s2) {
   fprintf(stderr, "%s%s\n", s1, s2);
   exit(EXIT_FAILURE);
   }
