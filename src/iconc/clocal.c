/*
 *  clocal.c -- compiler functions needed for different systems.
 */
#include "../h/gsupport.h"

/*
 * The following code is operating-system dependent [@tlocal.01].
 *  Routines needed by different systems.
 */

#if PORT
/* place to put anything system specific */
Deliberate Syntax Error
#endif					/* PORT */

#if ATARI_ST

unsigned long _STACK = 10240;   /*   MNEED ALSO, PLEASE */

char *getenv();

char iconpath[80];		/* the path returned by findpath() */
int pathset = 0;		/* not yet setup */

char *findpath();
char *getpathenv();

/*
 Pass a command to a shell.
 Find what/where is the shell by the environment variable SHELL
 */
int system(cmd)
char *cmd;
{
	char *shell, tail[128];
	int rc;
	
	if ((shell = getenv("SHELL")) == NULL) {
		fprintf(stderr,"SHELL not found.\n");
		return -1;
	}

	strcpy(tail + 1, cmd);
	*tail = strlen(tail) - 1;

	rc = Pexec(shell, tail, NULL);
	return (rc >= 0) ? rc : -1;
}

/*
 Spawn a process,
 with: variable arg list, and path found by environment.
 */
int Forkvp(cmd, argv)
char *cmd, **argv;
{
	char file[80];
	int n;
	extern char *patharg;

	if (filefound(NULL, cmd) == 0) {
		if (!pathset) {
			if (patharg) {
				strcpy(iconpath, patharg);
				if ((n = strlen(iconpath)) > 0
						&& iconpath[n-1] != '\\')
					strcat(iconpath, "\\");
			} else {
				if (findpath(cmd) == NULL)
					return -1;
			}
			pathset = 1;
		}
		strcpy(file, iconpath);
		strcat(file, cmd);
	} else
		strcpy(file, cmd);

	return Forkv(file, argv);	
}

/*
 Spawn a process with variable arg list.
 */
int Forkv(cmd, argv)
char *cmd, **argv;
{
	char tail[128];
	int i;

	*tail = tail[1] = '\0';
	for (i = 1; argv[i]; i++) {
		strcat(tail," ");
		strcat(tail,argv[i]);
	}
	*tail = strlen(tail) - 1;

	return Pexec(cmd, tail, NULL);	
}

/*
 findpath() assumes that the name passed to it is complete.
 (i.e. with the proper extension...)
 */  
char *findpath(prog)
char *prog;
{
	char *ppath, *fpath;
	int error, drvnum, drvmap, drvmask;
/*	
 First : check if ICON environment variable exist...
 (because it may be a lot faster than check the whole PATH...)
 */
	if ((ppath = getenv("ICON")) != NULL) {
		strcpy(iconpath, ppath);
		if ((error = strlen(iconpath)) > 0) {
			if (iconpath[error - 1] != '\\')
				strcat(iconpath, "\\");
			if (filefound(iconpath, prog) != 0)
				return iconpath;
		}
	}
/*	
 Second : check with PATH...
 */

	if ((ppath = getenv("PATH")) != NULL) {
		error = 0;
		while ((fpath = getpathenv(ppath, error)) != NULL) {
			if (filefound(fpath, prog) != 0) {
				strcpy(iconpath, fpath);
				return iconpath;
			}
			error++;
		}
	}

/*
 Not found here or with the ICON or PATH environment variable.
 Check all the mounted disk drives root directory, in alphabetical order.
 */
	strcpy(iconpath, "a:\\");
	drvmap = bios(10);	/* Drvmap() */ 
	for (drvnum = 0,	drvmask = 1;
	     drvnum < 16;
	     drvnum++,		drvmask <<= 1, iconpath[0]++)
		if ((drvmask & drvmap) && (filefound(iconpath, prog) != 0))
				return iconpath;

/*
 Not found...
 */
	return NULL;
}

int Pexec(file, tail, env) char *file, *tail, *env;
{
	int rc;

	if ((rc = gemdos(0x4b,0,file,tail, env)) != 0) {
		switch (rc) {
			case -33:
				fprintf(stderr,
				"Fork failed: file <%s> not found.\n", file);
				break;
			case -39:
				fprintf(stderr,
				"Fork failed: <%s> Not enough memory.\n", file);
				break;
			case -66:
				fprintf(stderr,
				"Fork failed: <%s> Bad load format.\n", file);
				break;
			default:
		}
	}
	return rc;
}

/*
 getpathenv() find the nth path entry in the env var PATH.
 append a backslash to it, if necessary; strcat ready.
 */
 
char *getpathenv(path, n) char *path; int n;
{
	char *Path2;
	char *sep = " ,;";		/* path separator in PATH */
	static char local[128];
	int i, j;

	for (i = 0; i < n; i++) {
		Path2 = strpbrk(path, sep);
		if (Path2 == NULL)
			break;
		path = Path2 + 1;	/* Past the sep */
		if ((j = strspn(path, sep)) > 0)
			path += j;
		
	}
	if (n == i) {
		strcpy(local, path);
		Path2 = strpbrk(local, sep);
		if (Path2 != NULL) {
			*Path2++ = '\\';
			*Path2 = '\0';
		} else
			strcat(local, "\\");
		if (strlen(local) == 1)
			local[0] = '\0';
		return &local[0];
	}
	return NULL;
}
	
/*
 test if a file exists.
 */
int filefound(path, file) char *path, *file;
{
	char filepath[80];

	if ((path != NULL) && (path[0] != '\0')) {
		strcpy(filepath, path);
		strcat(filepath, file);
	} else
		strcpy(filepath, file);

	if (gemdos(0x43, filepath, 0, 0) >= 0)
		return 1;
	return 0;
}

/*
 * This function returns a pointer to a static character string containing
 * the environment value assigned to the variable passed. (Fonorow/Nowlin)
 */

#define  BASEPAGE   _basepage
#define  SETLEN     256

char	*getenv(var)
char	*var;
{
	extern
	long	*BASEPAGE;

	register
	int	varlen;

	register
	char	*lenv;

	static
	char	val[SETLEN];

/*	calculate the variable length */
	varlen = strlen(var);

/*	check all the variables in the environment */
	for (lenv = (char *) BASEPAGE[11]; *lenv; lenv += strlen(lenv) + 1) {

/*		if this variable matches the variable name passed exactly */
		if (*(lenv+varlen) == '=' &&
		    strncmp(var,lenv,varlen) == 0) {

/*			save the value and return a pointer to it */
			strcpy(val,lenv+varlen+1);

			return val;
		}
	}

/*	the variable wasn't found so return null */
	return (char *) 0;
}
#endif					/* ATARI_ST */

#if MACINTOSH
#if MPW
/*
 * These are stubs for several routines defined in the runtine
 *  library that aren't necessary in MPW tools.  These routines are 
 *  referenced by the Standard C Library I/O functions, but are never called.
 *  Because they are referenced, the linker can't remove them.  The stubs in
 *  this file provide dummy routines which are never called, but reduce the
 *  size of the tool.
 */


/* Console Driver

   These drivers provide I/O to the screen (or a specified port) in
   applications.  They aren't necessary in tools.  
*/

_coFAccess() {}
_coClose() {}
_coRead() {}
_coWrite() {}
_coIoctl() {}
_coExit() {}


/* File System Driver

   Tools use the file system drivers linked with the MPW Shell.
*/

_fsFAccess() {}
_fsClose() {}
_fsRead() {}
_fsWrite() {}
_fsIoctl() {}
_fsExit() {}


/* System Driver

   Tools use the system drivers linked with the MPW Shell.
*/

_syFAccess() {}
_syClose() {}
_syRead() {}
_syWrite() {}
_syIoctl() {}
_syExit() {}


/* Floating Point Conversion Routines

   These routines, called by printf, are only necessary if floating point
   formatting is used.
*/

#endif					/* MPW */
#endif					/* MACINTOSH */

#if MSDOS

#if MICROSOFT

pointer xmalloc(n)
   long n;
   {
   return calloc((size_t)n,sizeof(char));
   }
#endif					/* MICROSOFT */

#if MICROSOFT
int _stack = (8 * 1024);
#endif					/* MICROSOFT */

#if TURBO
extern unsigned _stklen = 8192;
#endif					/* TURBO */

#if ZTC_386
#ifndef DOS386
int _stack = (8 * 1024);
#endif					/* DOS386 */
#endif					/* ZTC_386 */

#endif					/* MSDOS */

#if MVS || VM
#endif					/* MVS || VM */

#if OS2
#endif					/* OS2 */

#if UNIX
#endif					/* UNIX */

#if VMS
#endif					/* VMS */

/*
 * End of operating-system specific code.
 */

char *tjunk;			/* avoid empty module */
