/*
 * File: rlocal.r
 * Routines needed for different systems.
 */

/*  IMPORTANT NOTE:  Because of the way RTL works, this file should not
 *  contain any includes of system files, as in
 *
 *	include <foo>
 *
 *  Instead, such includes should be placed in h/sys.h.
 */

/*
 * The following code is operating-system dependent [@rlocal.01].
 *  Routines needed by different systems.
 */

#if PORT
   /* place for anything system-specific */
Deliberate Syntax Error
#endif					/* PORT */


/*********************************** AMIGA ***********************************/

#if AMIGA
#if LATTICE
long _STACK = 20000;
long _MNEED = 200000;	/* reserve space for allocation (may be too large) */
#endif					/* LATTICE */
#if __SASC
long __stack = 20000;
#endif					/* __SASC */
#if AZTEC_C
/*
 * abs
 */
abs(i)
int i;
{
	return ((i<0)? (-i) : i);
}

/*
 * ldexp
 */
double ldexp(value,exp)
double value;
{
  double retval = 1.0;
  if(exp>0) {
    while(exp-->0) retval *= 2.0;
  } else if (exp<0) {
    while(exp++<0) retval = retval / 2.0;
  }
  return value * retval;
}

/*
 *  abort()
 */
void abort()
{
  fprintf(stderr,"icon error with ICONCORE set\n");
  fflush(stderr);
  exit(1);
}

#endif					/* AZTEC_C */

/* Implement popen and pclose using Per Bojsen's APipe-Handler */

FILE *popen(const char *pname, const char *mode) {

   char *exname;
   char device[] = "APIPE:";
   FILE *fp;

   exname = malloc(strlen(pname) + strlen(device) + 1);
   strcpy(exname, device);
   strcat(exname, pname);
   fp = fopen(exname, mode);
   free(exname);
   return fp;
   }

int pclose(FILE *pipe) {
   return fclose(pipe);
   }

#endif					/* AMIGA */

/*********************************** ARM ***********************************/

#if ARM
#include "kernel.h"

char *mktemp (const char *);

static char *strdup (const char *);
static int os_cmd (char *);
static int cmp_cmd (char *, char *);

#define MAX_PIPE 20

typedef enum
{
   unopened = 0,
   reading,
   writing
}
pipemode;

static struct pipe
{
   char *command;   /* The command being executed      */
   char *name;   /* The name of the pipe file      */
   FILE *fd;   /* The file used as a pipe      */
   pipemode pmode;   /* The open mode of the pipe      */
   int retval;   /* The return value of the command   */
}
pipes[MAX_PIPE];

FILE *popen (char *command, char *mode)
{
   FILE *current;
   char *name;
   int i;
   pipemode curmode;
   int rval = -1;
   char tmp[11];

   /* decide on mode */
   if ( mode[1] != 0 )
      return NULL;
   else if ( *mode == 'r' )
      curmode = reading;
   else if ( *mode == 'w' )
      curmode = writing;
   else
      return NULL;

   /* Get a slot in the pipes structure */
   for ( i = 0; i < MAX_PIPE; ++i )
   {
      if ( pipes[i].pmode == unopened )
         break;
   }

   if ( i >= MAX_PIPE )
      return NULL;

   /* Get a file name to use */
   sprintf(tmp, "Pipe%.2d", i);
   name = mktemp(tmp);

   if ( name == NULL )
      return NULL;

   /*
    * If we're reading, just call system() to get a file filled
    * with output.
    */

   if ( curmode == reading )
   {
      char *tmpname;
      int oscmd = os_cmd(command);
      char cmd[256];
      int n;

      if (*command == '%')
      {
         oscmd = 1;
         ++command;
      }

      if (!oscmd)
      {
         char *s;

         while (*command && isspace(*command))
            ++command;

         s = command;

         while (*s && !isspace(*s))
            ++s;

         n = sprintf(cmd, "%.*s > %s%s",
            s - command, command, name, s);
      }
      else
      {
         tmpname = mktemp("PipeTmp");

         if (tmpname == NULL)
         {
            free(name);
            return NULL;
         }

         n = sprintf(cmd, "%s{ > %s }", command, tmpname);
      }

      /* Emergency! Overflow in command buffer! */
      if (n > 255)
      {
         if (oscmd)
         {
            remove(tmpname);
            free(tmpname);
         }
         free(name);
         return NULL;
      }

      _kernel_setenv("Sys$ReturnCode", "0");
      rval = system(cmd);

      if (rval == _kernel_ERROR)
      {
         remove(tmpname);
         free(tmpname);
         free(name);
         return NULL;
      }

      if (oscmd)
      {
         int ch;
         FILE *in = fopen(tmpname, "r");
         FILE *out = fopen(name, "w");

         if (in == NULL || out == NULL)
         {
            remove(tmpname);
            free(tmpname);
            free(name);
            return NULL;
         }

         /* Strip out CRs from the output */
         while ((ch = getc(in)) != EOF && !ferror(out))
         {
            if (ch != '\r')
               putc(ch, out);
         }

         /* Did we succeed? */
         ch = (ferror(in) || ferror(out));

         /* Tidy up */
         fclose(in);
         fclose(out);
         remove(tmpname);
         free(tmpname);

         if (ch)
         {
            free(name);
            return NULL;
         }
      }

      if ( (current = fopen(name,"r")) == NULL )
      {
         free(name);
         return NULL;
      }
   }
   else
   {
      if ( (current = fopen(name,"w")) == NULL )
      {
         free(name);
         return NULL;
      }
   }

   pipes[i].command = strdup(command);
   pipes[i].name = name;
   pipes[i].fd = current;
   pipes[i].pmode = curmode;
   pipes[i].retval = rval;
   return current;
}

#define ReadCat	5

/* Create a temporary file name by adding a directory prefix to file.
 * If the external variable temp_dir is not zero, this directory will be
 * used. Otherwise, the following are used, in order.
 *   1. <Tmp$Dir>
 *   2. &.Tmp
 *   3. The current directory.
 * The function returns zero on an error (temp_dir is not a directory, or
 * malloc() failed), otherwise it returns a malloc-ed string containing
 * the required name.
 */

static char *concat (const char *dir, const char *file);

char *temp_dir = 0;

char *mktemp (const char *file)
{
   char *dir;
   char *name;
   char buf[11];
   int len = strlen(file);
   _kernel_osfile_block blk;
   _kernel_swi_regs regs;

   /* Is the supplied filename a pure file name? */
   if (len > 10)
      return 0;

   /* Pad out the supplied filename on the left with a unique ID
    * (Based on the program start time)
    */
   if (len < 10 && _kernel_swi(OS_GetEnv,&regs,&regs) == NULL)
   {
      int i;
      char *time = (char *)regs.r[2];

      strcpy(buf,file);

      for (i = len; i < 10; ++i)
      {
         char c = time[(9 - i) >> 1];

         if (i & 1)
            c >>= 4;

         c &= 0x0F;
         buf[i] = "abcdefghijklmnop"[c];
      }

      buf[10] = 0;

      file = buf;
   }

   /* First, try the supplied directory */
   if ( temp_dir )
   {
      if ( _kernel_osfile(ReadCat,temp_dir,&blk) == 2 )
         return concat(temp_dir,file);
      else
      {
         /* Is it a filing system name only? */
         len = strlen(temp_dir);

         if (temp_dir[len-1] != ':')
            return 0;

         /* One extra, just in case file == "", for the '@' */
         name = malloc(len + strlen(file) + 2);

         if (name == 0)
            return 0;

         strcpy(name,temp_dir);
         name[len] = '@';
         name[len+1] = '\0';

         if (_kernel_osfile(ReadCat,name,&blk) != 2)
         {
            free(name);
            return 0;
         }

         strcpy(&name[len],file);
         return name;
      }
   }

   /* Otherwise, go through the list... */

   /* First of all, try <Tmp$Dir> */
   if ((dir = getenv("Tmp$Dir")) != 0)
   {
      if (_kernel_osfile(ReadCat,dir,&blk) == 2)
         return concat(dir,file);
      else
      {
         /* Is it a filing system name only? */
         len = strlen(dir);

         if (dir[len-1] != ':')
            goto no_go;

         /* One extra, just in case file == "", for the '@' */
         name = malloc(len + strlen(file) + 2);

         if (name == 0)
            goto no_go;

         strcpy(name,dir);
         name[len] = '@';
         name[len+1] = '\0';

         if (_kernel_osfile(ReadCat,name,&blk) != 2)
         {
            free(name);
            goto no_go;
         }

         strcpy(&name[len],file);
         return name;
      }
   }

no_go:
   /* No <Tmp$Dir>, so try &.Tmp, if it exists */
   if (_kernel_osfile(ReadCat,"&.Tmp",&blk) == 2)
      return concat("&.Tmp",file);

   /* Out of luck - use the current directory */
   name = malloc(strlen(file)+1);
   if ( name )
      strcpy(name,file);

   return name;
}

static char *concat (const char *dir, const char *file)
{
	char *result = malloc(strlen(dir)+strlen(file)+2);
	char *p = result;

	if ( result == 0 )
		return 0;

	while ( *dir )
		*p++ = *dir++;

	*p++ = '.';
	while ( *file )
		*p++ = *file++;

	*p = '\0';

	return result;
}

/* ----------------------------------------------------------------- */

#ifdef test

int main (int argc, char *argv[])
{
	char *tmp;

	if ( argc != 2 && argc != 3 )
	{
		fprintf(stderr,"Usage: %s file [dir]\n",argv[0]);
		return 1;
	}

	if ( argc == 3 )
		temp_dir = argv[2];

	tmp = mktemp (argv[1]);

	printf("Temp file = %s\n", tmp ? tmp : "<Not possible>");

	return 0;
}

#endif

int pclose (FILE *current)
{
	int rval;
	int i;

	/* Get the appropriate slot in thbe pipes structure */
	for ( i = 0; i < MAX_PIPE; ++i )
	{
		if ( pipes[i].fd == current )
			break;
	}

	if ( i >= MAX_PIPE )
		return -1;

	if ( pipes[i].pmode == reading )
	{
		/* Input pipes are just files we're done with */
		rval = pipes[i].retval;
		fclose(current);
		remove(pipes[i].name);
	}
	else
	{
		/*
		 * Output pipes are temporary files we have
		 * to cram down the throats of programs.
		 */
		char *command = pipes[i].command;
		int oscmd = os_cmd(command);
		int n;
		char cmd[256];

		if (*command == '%')
		{
			oscmd = 1;
			++command;
		}

		/* Close the pipe file */
		fclose(current);

		/* Create the required command string */
		if (oscmd)
			n = sprintf(cmd, "%s{ < %s }", command, pipes[i].name);
		else
		{
			char *s;

			while (*command && isspace(*command))
				++command;

			s = command;

			while (*s && !isspace(*s))
				++s;

			n = sprintf(cmd, "%.*s < %s%s",
				s - command, command, pipes[i].name, s);
		}

		/* Check for overflow in command buffer */
		if (n > 255)
			rval = -1;
		else
		{
			_kernel_setenv("Sys$ReturnCode", "0");
			rval = system(cmd);
		}

		remove(pipes[i].name);
	}

	/* clean up current pipe */
	pipes[i].pmode = unopened;
	free(pipes[i].name);
	free(pipes[i].command);
	return rval;
}

/* save a string on the heap; return pointer to it */

static char *strdup (const char *str)
{
	char *p = malloc(strlen(str)+1);

	if (p == NULL)
	{
		fprintf(stderr,"Not enough memory to save string\n");
		exit(1);
	}

	return (strcpy(p,str));
}

/* Check whether a command is an OS command (binary search on the table
 * os_commands of valid OS commands).
 */

static char *os_commands[] =
{
	"access",	"adfs",		"alphabet",	"alphabets",
	"append",	"audio",	"basic",	"breakclr",
	"breaklist",	"breakset",	"build",	"cat",
	"cdir",		"channelvoice",	"close",	"configure",
	"continue",	"copy",		"count",	"countries",
	"country",	"create",	"debug",	"delete",
	"deskfs",	"dir",		"dump",		"echo",
	"enumdir",	"error",	"eval",		"ex",
	"exec",		"fileinfo",	"fontcat",	"fontlist",
	"fx",		"go",		"gos",		"help",
	"iconsprites",	"if",		"ignore",	"info",
	"initstore",	"key",		"keyboard",	"lcat",
	"lex",		"lib",		"list",		"load",
	"memory",	"memorya",	"memoryi",	"modules",
	"obey",		"opt",		"poduleload",	"podules",
	"podulesave",	"pointer",	"print",	"qsound",
	"quit",		"ram",		"remove",	"rename",
	"rmclear",	"rmensure",	"rmfaster",	"rmkill",
	"rmload",	"rmreinit",	"rmrun",	"rmtidy",
	"rommodules",	"run",		"save",		"schoose",
	"scopy",	"screenload",	"screensave",	"sdelete",
	"set",		"seteval",	"setmacro",	"settype",
	"sflipx",	"sflipy",	"sget",		"shadow",
	"shellcli",	"show",		"showregs",	"shut",
	"shutdown",	"sinfo",	"slist",	"sload",
	"smerge",	"snew",		"sound",	"speaker",
	"spool",	"spoolon",	"srename",	"ssave",
	"stamp",	"status",	"stereo",	"tempo",
	"time",		"tuning",	"tv",		"type",
	"unplug",	"unset",	"up",		"voices",
	"volume",	"wimppalette",	"wimpslot",	"wimptask",
	"wipe"
};

#define NUM_CMDS (sizeof(os_commands) / sizeof(char *))

static int os_cmd (char *cmd)
{
	int low = 0;
	int high = NUM_CMDS - 1;

	while (low <= high)
	{
		int mid = (high + low) / 2;
		int i = cmp_cmd(cmd,os_commands[mid]);

		if (i == 0)
			return 1;
		else if (i < 0)
			high = mid - 1;
		else
			low = mid + 1;
	}

	return 0;
}

static int cmp_cmd (char *cmd, char *name)
{
	while (*name && tolower(*cmd) == *name)
		++name, ++cmd;

	if (*name)
		return (tolower(*cmd) - *name);

	return (*cmd != '\0' && !isspace(*cmd));
}

#ifdef test
int main (int argc, char *argv[])
{
	FILE *fp;
	char *cmd;

	if (argc <= 1)
	{
		printf("Usage Popen [cmd or Popen ]cmd\n");
		return 0;
	}

	cmd = argv[1];

	if (*cmd++ == ']')
	{
		fp = popen(cmd,"w");
		fprintf(fp,"hello\nworld\nhow\nare\nyou\n");
		pclose(fp);
	}
	else
	{
		char buf[500];
		fp = popen(cmd,"r");
		while (!feof(fp))
		{
			if (!fgets(buf,499,fp))
			{
				printf("Read error!\n");
				return 1;
			}
			buf[strlen(buf)-1] = 0;
			printf(">%s<\n", buf);
		}
		pclose(fp);
	}

	return 0;
}
#endif

int getch (void)
{
	return _kernel_osrdch();
}

int getche (void)
{
	int ch = _kernel_osrdch();

	_kernel_oswrch(ch);

	return ch;
}

int kbhit (void)
{
	return ((_kernel_osbyte(152,0,0) & 0x00FF0000) != 0x00010000);
}

#endif					/* ARM */

/*********************************** MAC ***********************************/

#if MACINTOSH
#if MPW
/*
 * Special routines for Macintosh Programmer's Workshop (MPW) implementation
 *  of the Icon Programming Language
 */

#include <Types.h>
#include <Events.h>
#include <Files.h>
#include <FCntl.h>
#include <IOCtl.h>
#include <SANE.h>
#include <OSUtils.h>
#include <Memory.h>
#include <Errors.h>
#include "time.h"
#include <QuickDraw.h>
#include <ToolUtils.h>
#include <CursorCtl.h>

#define isatty(fd) (!ioctl((fd), FIOINTERACTIVE))

   void
SetFileToMPWText(const char *fname) {
   FInfo info;
   int needToSet = 0;

   if (getfinfo(fname,0,&info) == 0) {
      if (info.fdType == 0) {
	 info.fdType = 'TEXT';
	 needToSet = 1;
	 }
      if (info.fdCreator == 0) {
	 info.fdCreator = 'MPS ';
	 needToSet = 1;
	 }
      if (needToSet) {
	 setfinfo(fname,0,&info);
	 }
      }
   return;
   }


   int
MPWFlush(FILE *f) {
   static int fetched = 0;
   static char *noLineFlush;

   if (!fetched) {
      noLineFlush = getenv("NOLINEFLUSH");
      fetched = 1;
      }
   if (!noLineFlush || noLineFlush[0] == '\0')
         fflush(f);
   return 0;
   }


   void
SetFloatTrap(void (*fpetrap)()) {
   /* This is equivalent to SIGFPE signal in the Standard Apple
      Numeric Environment (SANE) */
   environment e;

   getenvironment(&e);
      #ifdef mc68881
	 e.FPCR |= CURUNDERFLOW|CUROVERFLOW|CURDIVBYZERO;
      #else					/* mc68881 */
	 e |= UNDERFLOW|OVERFLOW|DIVBYZERO;
      #endif					/* mc68881 */
   setenvironment(e);
   #ifdef mc68881
      {
      static trapvector tv =
         {fpetrap,fpetrap,fpetrap,fpetrap,fpetrap,fpetrap,fpetrap};
      settrapvector(&tv);
      }
   #else					/* mc68881 */
      sethaltvector((haltvector)fpetrap);
   #endif					/* mc68881 */
   }


   void
SetWatchCursor(void) {
   SetCursor(*GetCursor(watchCursor));	/* Set watch cursor */
   }


#define TicksPerRotation 10 /* rotate cursor no more often than 6 times
				 per second */

   void
RotateTheCursor(void) {
   static unsigned long nextRotate = 0;
   if (TickCount() >= nextRotate) {
      RotateCursor(0);
      nextRotate = TickCount() + TicksPerRotation;
      }
   else {
      RotateCursor(1);
      }
   }

/*
 *  Initialization and Termination Routines
 */

/*
 *  MacExit -- This function is installed by an atexit() call in MacInit
 *  -- it is called automatically when the program terminates.
 */
   void
MacExit() {
   void ResetStack();
   extern Ptr MemBlock;

   ResetStack();
   /* if (MemBlock != NULL) DisposPtr(MemBlock); */
   }

/*
 *  MacInit -- This function is called near the beginning of execution of
 *  iconx.  It is called by our own brk/sbrk initialization routine.
 */
   void
MacInit() {
   atexit(MacExit);
   return;
   }

/*
 * MacDelay -- Delay n milliseconds.
 */
   void
MacDelay(int n) {
   unsigned long endTicks;
   unsigned long nextRotate;

   endTicks = TickCount() + (n * 3 + 25) / 50;
   nextRotate = 0;
   while (TickCount() < endTicks) {
      if (TickCount() >= nextRotate) {
         nextRotate = TickCount() + TicksPerRotation;
	 RotateCursor(0);
         }
      else {
         RotateCursor(1);
	 }
      }
   }
#endif					/* MPW */
#endif					/* MACINTOSH */

/*********************************** MSDOS ***********************************/

#if MSDOS
#if INTEL_386
/*  sbrk(incr) - adjust the break value by incr.
 *  Returns the new break value, or -1 if unsuccessful.
 */

pointer sbrk(incr)
msize incr;
{
   static pointer base = 0;		/* base of the sbrk region */
   static pointer endofmem, curr;
   pointer result;
   union REGS rin, rout;

   if (!base) {					/* if need to initialize				*/
      rin.w.eax = 0x80004800;	/* use DOS allocate function with max	*/
      rin.w.ebx = 0xffffffff;	/*  request to determine size of free	*/
      intdos(&rin, &rout);		/*  memory (including virtual memory.	*/
	  rin.w.ebx = rout.w.ebx;	/* DOS allocate all of memory.			*/
      intdos(&rin, &rout);
      if (rout.w.cflag)
         return (pointer)-1;
      curr = base = (pointer)rout.w.eax;
      endofmem = (pointer)((char *)base + rin.w.ebx);
      }

   if ((char *)curr + incr > (char *)endofmem)
      return (pointer)-1;
   result = curr;
   curr = (pointer)((char *)curr + incr);
   return result;

}

/*  brk(addr) - set the break address to the given value, rounded up to a page.
 *  returns 0 if successful, -1 if not.
 */

int brk(addr)
pointer addr;
{
   int result;
   result = sbrk((char *)addr - (char *)sbrk(0)) == (pointer)-1 ? -1 : 0;
   return result;
}

#endif					/* INTEL_386 */

#if TURBO
extern unsigned _stklen = 16 * 1024;
#endif					/* TURBO */

#endif					/* MSDOS */

/********************************* MVS || VM ********************************/

#if MVS || VM
#if SASC
#passthru #include <options.h>
char _linkage = _OPTIMIZE;

#if MVS
char *_style = "tso:";          /* use dsnames as file names */
#define SYS_OSVS
#else                                   /* MVS */
#define SYS_CMS
#endif                                  /* MVS */

#passthru #define RES_SIGNAL
#passthru #define RES_COPROC
#passthru #define RES_IOUTIL
#passthru #define RES_DSNAME
#passthru #define RES_FILEDEF
#passthru #define RES_UNITREC
#passthru #define RES_TSOENVVAR
#passthru #define ALLOW_TRANSIENT /* temporary */

#passthru #include <resident.h>

#endif                                  /* SASC */
#endif                                  /* MVS || VM */

/*********************************** OS/2 ***********************************/

#if OS2
void abort()
{
#ifdef DeBugIconx
    blkdump();
#endif
    fflush(stderr);
#if CSET2
    _fcloseall();
#else
    fcloseall();
#endif		CSET2
    _exit(1);
}

#ifndef OS2EMX
#if CSET2
/* 20 is the default file handle max, this can be dynamically altered */
#define _NFILE  20
#endif		/* CSET2 */

static int _pipes[_NFILE];

/*
 * popen("command",mode)
 *
 * cmd = command to be passed to shell. (CMD.EXE or comspec->)
 * mode = "r" | "w"
 */
FILE *popen(char *cmd, char *mode)
{
#if OS2_32
    HFILE whandle, rhandle;
#else
    int whandle, rhandle;
#endif		/* OS2_32 */
    int phandle, chandle, shandle;
    int rc;
    char *cmdshell;

    /* Validate */
    if(cmd == NULL || mode == NULL) return NULL;
    if(tolower(*mode) != 'r' && tolower(*mode) != 'w')
	return NULL;

    /* Create the pipe */
#if OS2_32
    if (DosCreatePipe(&rhandle, &whandle, (ULONG)BUFSIZ) < 0)
#else
    if (DosMakePipe(&rhandle, &whandle, BUFSIZ) < 0)
#endif		/* OS2_32 */
	return NULL;

    /* Dup STDIN or STDOUT to the pipe */
    if (*mode == 'r') {
	/* Dup stdout */
	phandle = rhandle;
	chandle = whandle;
	shandle = dup(1);	/* Save STDOUT */
	rc = dup2(chandle, 1);
    } else {
	/* Dup stdin */
	phandle = whandle;
	chandle = rhandle;
	shandle = dup(0);	/* Save STDIN */
	rc = dup2(chandle, 0);
    }
    if (rc < 0) {
	perror("dup2");
	return NULL;
    }
    close(chandle);

    /* Make sure that we don't pass this handle on */
    DosSetFHandState(phandle, OPEN_FLAGS_NOINHERIT);

    /* Invoke the child, remember its processid */
    cmdshell = getenv("COMSPEC");
    if (cmdshell == NULL) cmdshell = "CMD.EXE";

    _pipes[chandle] = spawnlp(P_NOWAIT, cmdshell, cmdshell,"/c",cmd, NULL);

    /* Clean up by reestablishing our STDIN/STDOUT */
    if (*mode == 'r')
	rc = dup2(shandle, 1);
    else
	rc = dup2(shandle, 0);
    if (rc < 0) {
	perror("dup2");
	return NULL;
    }
    close(shandle);

    return fdopen(phandle, mode);
}
pclose(ptr)
FILE *ptr;
{
    int status, pnum;

    pnum = fileno(ptr);
    fclose(ptr);

    /* Now wait for child to end */
    cwait(&status, _pipes[pnum], WAIT_GRANDCHILD);

    return status;
}
#endif					/* OS2EMX */

/* End of pipe support for OS/2 */
#endif					/* OS2 */

/*********************************** UNIX ***********************************/

#if UNIX

#ifdef KeyboardFncs

/*
 * Documentation notwithstanding, the Unix versions of the keyboard functions
 * read from standard input and not necessarily from the keyboard (/dev/tty).
 */
#define STDIN 0

/*
 * int getch() -- read character without echoing
 * int getche() -- read character with echoing
 *
 * Read and return a character from standard input in non-canonical
 * ("cbreak") mode.  Return -1 for EOF.
 *
 * Reading is done even if stdin is not a tty;
 * the tty get/set functions are just rejected by the system.
 */

int rchar(int with_echo);

int getch(void)		{ return rchar(0); }
int getche(void)	{ return rchar(1); }

int rchar(int with_echo)
{
   struct termios otty, tty;
   char c;
   int n;

   tcgetattr(STDIN, &otty);		/* get current tty attributes */

   tty = otty;
   tty.c_lflag &= ~ICANON;
   if (with_echo)
      tty.c_lflag |= ECHO;
   else
      tty.c_lflag &= ~ECHO;
   tcsetattr(STDIN, TCSANOW, &tty);	/* set temporary attributes */

   n = read(STDIN, &c, 1);		/* read one char from stdin */

   tcsetattr(STDIN, TCSANOW, &otty);	/* reset tty to original state */

   if (n == 1)				/* if read succeeded */
      return c & 0xFF;
   else
      return -1;
}

/*
 * kbhit() -- return nonzero if characters are available for getch/getche.
 */
int kbhit(void)
{
   struct termios otty, tty;
   fd_set fds;
   struct timeval tv;
   int rv;

   tcgetattr(STDIN, &otty);		/* get current tty attributes */

   tty = otty;
   tty.c_lflag &= ~ICANON;		/* disable input batching */
   tcsetattr(STDIN, TCSANOW, &tty);	/* set attribute temporarily */

   FD_ZERO(&fds);			/* initialize fd struct */
   FD_SET(STDIN, &fds);			/* set STDIN bit */
   tv.tv_sec = tv.tv_usec = 0;		/* set immediate return */
   rv = select(STDIN + 1, &fds, NULL, NULL, &tv);

   tcsetattr(STDIN, TCSANOW, &otty);	/* reset tty to original state */

   return rv;				/* return result */
}
#endif					/* KeyboardFncs */

#endif					/* UNIX */

/*********************************** VMS ***********************************/

#if VMS
#passthru #define LIB_GET_EF     LIB$GET_EF
#passthru #define SYS_CREMBX     SYS$CREMBX
#passthru #define LIB_FREE_EF    LIB$FREE_EF
#passthru #define DVI__DEVNAM    DVI$_DEVNAM
#passthru #define SYS_GETDVIW    SYS$GETDVIW
#passthru #define SYS_DASSGN     SYS$DASSGN
#passthru #define LIB_SPAWN      LIB$SPAWN
#passthru #define SYS_QIOW       SYS$QIOW
#passthru #define IO__WRITEOF    IO$_WRITEOF
#passthru #define SYS_WFLOR      SYS$WFLOR
#passthru #define sys_expreg     sys$expreg
#passthru #define STS_M_SUCCESS  STS$M_SUCCESS
#passthru #define sys_cretva     sys$cretva
#passthru #define SYS_ASSIGN     SYS$ASSIGN
#passthru #define SYS_QIO        SYS$QIO
#passthru #define IO__TTYREADALL IO$_TTYREADALL
#passthru #define IO__WRITEVBLK  IO$_WRITEVBLK
#passthru #define IO_M_NOECHO    IO$M_NOECHO
#passthru #define SYS_SCHDWK     SYS$SCHDWK
#passthru #define SYS_HIBER      SYS$HIBER

typedef struct _descr {
   int length;
   char *ptr;
} descriptor;

typedef struct _pipe {
   long pid;			/* process id of child */
   long status;			/* exit status of child */
   long flags;			/* LIB$SPAWN flags */
   int channel;			/* MBX channel number */
   int efn;			/* Event flag to wait for */
   char mode;			/* the open mode */
   FILE *fptr;			/* file pointer (for fun) */
   unsigned running : 1;	/* 1 if child is running */
} Pipe;

Pipe _pipes[_NFILE];		/* one for every open file */

#define NOWAIT		1
#define NOCLISYM	2
#define NOLOGNAM	4
#define NOKEYPAD	8
#define NOTIFY		16
#define NOCONTROL	32
#define SFLAGS	(NOWAIT|NOKEYPAD|NOCONTROL)

/*
 * delay_vms - delay for n milliseconds
 */

void delay_vms(n)
int n;
{
   int pid = getpid();
   int delay_time[2];

   delay_time[0] = -1000 * n;
   delay_time[1] = -1;
   SYS_SCHDWK(&pid, 0, delay_time, 0);
   SYS_HIBER();
}

/*
 * popen - open a pipe command
 * Last modified 2-Apr-86/chj
 *
 *	popen("command", mode)
 */

FILE *popen(cmd, mode)
char *cmd;
char *mode;
{
   FILE *pfile;			/* the Pfile */
   Pipe *pd;			/* _pipe database */
   descriptor mbxname;		/* name of mailbox */
   descriptor command;		/* command string descriptor */
   descriptor nl;		/* null device descriptor */
   char mname[65];		/* mailbox name string */
   int chan;			/* mailbox channel number */
   int status;			/* system service status */
   int efn;
   struct {
      short len;
      short code;
      char *address;
      char *retlen;
      int last;
   } itmlst;

   if (!cmd || !mode)
      return (0);
   LIB_GET_EF(&efn);
   if (efn == -1)
      return (0);
   if (_tolower(mode[0]) != 'r' && _tolower(mode[0]) != 'w')
      return (0);
   /* create and open the mailbox */
   status = SYS_CREMBX(0, &chan, 0, 0, 0, 0, 0);
   if (!(status & 1)) {
      LIB_FREE_EF(&efn);
      return (0);
   }
   itmlst.last = mbxname.length = 0;
   itmlst.address = mbxname.ptr = mname;
   itmlst.retlen = &mbxname.length;
   itmlst.code = DVI__DEVNAM;
   itmlst.len = 64;
   status = SYS_GETDVIW(0, chan, 0, &itmlst, 0, 0, 0, 0);
   if (!(status & 1)) {
      LIB_FREE_EF(&efn);
      return (0);
   }
   mname[mbxname.length] = '\0';
   pfile = fopen(mname, mode);
   if (!pfile) {
      LIB_FREE_EF(&efn);
      SYS_DASSGN(chan);
      return (0);
   }
   /* Save file information now */
   pd = &_pipes[fileno(pfile)];	/* get Pipe pointer */
   pd->mode = _tolower(mode[0]);
   pd->fptr = pfile;
   pd->pid = pd->status = pd->running = 0;
   pd->flags = SFLAGS;
   pd->channel = chan;
   pd->efn = efn;
   /* fork the command */
   nl.length = strlen("_NL:");
   nl.ptr = "_NL:";
   command.length = strlen(cmd);
   command.ptr = cmd;
   status = LIB_SPAWN(&command,
      (pd->mode == 'r') ? 0 : &mbxname,	/* input file */
      (pd->mode == 'r') ? &mbxname : 0,	/* output file */
      &pd->flags, 0, &pd->pid, &pd->status, &pd->efn, 0, 0, 0, 0);
   if (!(status & 1)) {
      LIB_FREE_EF(&efn);
      SYS_DASSGN(chan);
      return (0);
   } else {
      pd->running = 1;
   }
   return (pfile);
}

/*
 * pclose - close a pipe
 * Last modified 2-Apr-86/chj
 *
 */
pclose(pfile)
FILE *pfile;
{
   Pipe *pd;
   int status;
   int fstatus;

   pd = fileno(pfile) ? &_pipes[fileno(pfile)] : 0;
   if (pd == NULL)
      return (-1);
   fflush(pd->fptr);			/* flush buffers */
   fstatus = fclose(pfile);
   if (pd->mode == 'w') {
      status = SYS_QIOW(0, pd->channel, IO__WRITEOF, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      SYS_WFLOR(pd->efn, 1 << (pd->efn % 32));
   }
   SYS_DASSGN(pd->channel);
   LIB_FREE_EF(&pd->efn);
   pd->running = 0;
   return (fstatus);
}

/*
 * redirect(&argc,argv,nfargs) - redirect standard I/O
 *    int *argc		number of command arguments (from call to main)
 *    char *argv[]	command argument list (from call to main)
 *    int nfargs	number of filename arguments to process
 *
 * argc and argv will be adjusted by redirect.
 *
 * redirect processes a program's command argument list and handles redirection
 * of stdin, and stdout.  Any arguments which redirect I/O are removed from the
 * argument list, and argc is adjusted accordingly.  redirect would typically be
 * called as the first statement in the main program.
 *
 * Files are redirected based on syntax or position of command arguments.
 * Arguments of the following forms always redirect a file:
 *
 *    <file	redirects standard input to read the given file
 *    >file	redirects standard output to write to the given file
 *    >>file	redirects standard output to append to the given file
 *
 * It is often useful to allow alternate input and output files as the
 * first two command arguments without requiring the <file and >file
 * syntax.  If the nfargs argument to redirect is 2 or more then the
 * first two command arguments, if supplied, will be interpreted in this
 * manner:  the first argument replaces stdin and the second stdout.
 * A filename of "-" may be specified to occupy a position without
 * performing any redirection.
 *
 * If nfargs is 1, only the first argument will be considered and will
 * replace standard input if given.  Any arguments processed by setting
 * nfargs > 0 will be removed from the argument list, and again argc will
 * be adjusted.  Positional redirection follows syntax-specified
 * redirection and therefore overrides it.
 *
 */


redirect(argc,argv,nfargs)
int *argc, nfargs;
char *argv[];
{
   int i;

   i = 1;
   while (i < *argc)  {		/* for every command argument... */
      switch (argv[i][0])  {		/* check first character */
         case '<':			/* <file redirects stdin */
            filearg(argc,argv,i,1,stdin,"r");
            break;
         case '>':			/* >file or >>file redirects stdout */
            if (argv[i][1] == '>')
               filearg(argc,argv,i,2,stdout,"a");
            else
               filearg(argc,argv,i,1,stdout,"w");
            break;
         default:			/* not recognized, go on to next arg */
            i++;
      }
   }
   if (nfargs >= 1 && *argc > 1)	/* if positional redirection & 1 arg */
      filearg(argc,argv,1,0,stdin,"r");	/* then redirect stdin */
   if (nfargs >= 2 && *argc > 1)	/* likewise for 2nd arg if wanted */
      filearg(argc,argv,1,0,stdout,"w");/* redirect stdout */
}



/* filearg(&argc,argv,n,i,fp,mode) - redirect and remove file argument
 *    int *argc		number of command arguments (from call to main)
 *    char *argv[]	command argument list (from call to main)
 *    int n		argv entry to use as file name and then delete
 *    int i		first character of file name to use (skip '<' etc.)
 *    FILE *fp		file pointer for file to reopen (typically stdin etc.)
 *    char mode[]	file access mode (see freopen spec)
 */

filearg(argc,argv,n,i,fp,mode)
int *argc, n, i;
char *argv[], mode[];
FILE *fp;
{
   if (strcmp(argv[n]+i,"-"))		/* alter file if arg not "-" */
      fp = freopen(argv[n]+i,mode,fp);
   if (fp == NULL)  {			/* abort on error */
      fprintf(stderr,"%%can't open %s",argv[n]+i);
      exit(EXIT_FAILURE);
   }
   for ( ;  n < *argc;  n++)		/* move down following arguments */
      argv[n] = argv[n+1];
   *argc = *argc - 1;			/* decrement argument count */
}

#ifdef KeyboardFncs

short channel;
int   request_queued = 0;
int   char_available = 0;
char  char_typed;

void assign_channel_to_terminal()
{
   descriptor terminal;

   terminal.length = strlen("SYS$COMMAND");
   terminal.ptr    = "SYS$COMMAND";
   SYS_ASSIGN(&terminal, &channel, 0, 0);
}

word read_a_char(echo_on)
int echo_on;
{
   if (char_available) {
      char_available = 0;
      if (echo_on)
         SYS_QIOW(2, channel, IO__WRITEVBLK, 0, 0, 0, &char_typed, 1,
		  0, 32, 0, 0);
      goto return_char;
      }
   if (echo_on)
      SYS_QIOW(1, channel, IO__TTYREADALL, 0, 0, 0, &char_typed, 1, 0, 0, 0, 0);
   else
      SYS_QIOW(1, channel, IO__TTYREADALL | IO_M_NOECHO, 0, 0, 0,
	       &char_typed, 1, 0, 0, 0, 0);

return_char:
   if (char_typed == '\003' && kill(getpid(), SIGINT) == -1) {
      perror("kill");
      return 0;
      }
   if (char_typed == '\034' && kill(getpid(), SIGQUIT) == -1) {
      perror("kill");
      return 0;
      }
   return (word)char_typed;
}

int getch()
{
   return read_a_char(0);
}

int getche()
{
   return read_a_char(1);
}

void ast_proc()
{
   char_available = 1;
   request_queued = 0;
}

int kbhit()
{
   if (!request_queued) {
      request_queued = 1;
      SYS_QIO(1, channel, IO__TTYREADALL | IO_M_NOECHO, 0, ast_proc, 0,
              &char_typed, 1, 0, 0, 0, 0);
      }
   return char_available;
}

#endif					/* KeyboardFncs */

#endif					/* VMS */
/*
 * End of operating-system specific code.
 */

static char xjunk;			/* avoid empty module */
