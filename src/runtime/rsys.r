/*
 * File: rsys.r
 *  Contents: getstrg, host, longread, putstr
 */

/*
 * getstrg - read a line into buf from file fbp.  At most maxi characters
 *  are read.  getstrg returns the length of the line, not counting the
 *  newline.  Returns -1 if EOF and -2 if length was limited by maxi.
 *  Discards \r before \n in translated mode.  [[ Needs ferror() check. ]]
 */

int getstrg(buf, maxi, fbp)
register char *buf;
int maxi;
struct b_file *fbp;
   {
   register int c, l;
   FILE *fd;

   fd = fbp->fd;

   #ifdef XWindows
      if (isatty(fileno(fd))) wflushall();
   #endif				/* XWindows */

   l = 0;
   while (1) {

      #ifdef Graphics
         /* insert non-blocking read/code to service windows here */
      #endif				/* Graphics */

      if ((c = fgetc(fd)) == '\n')	/* \n terminates line */
	 break;
      if (c == '\r' && (fbp->status & Fs_Untrans) == 0) {
	 /* \r terminates line in translated mode */
	 if ((c = fgetc(fd)) != '\n')	/* consume following \n */
	     ungetc(c, fd);		/* (put back if not \n) */
	 break;
	 }
      if (c == EOF) {
	 if (l > 0) return l;
	 else return -1;
         }
      if (++l > maxi) {
	 ungetc(c, fd);
	 return -2;
	 }
      *buf++ = c;
      }
   return l;
   }

/*
 * iconhost - return some sort of host name into the buffer pointed at
 *  by hostname.  This code accommodates several different host name
 *  fetching schemes.
 */
void iconhost(hostname)
char *hostname;
   {
   /*
    * Use the uname system call.  (POSIX)
    */
   struct utsname utsn;
   uname(&utsn);
   strcpy(hostname,utsn.nodename);
   }

/*
 * Read a long string in shorter parts. (Standard read may not handle long
 *  strings.)
 */
word longread(s,width,len,fd)
FILE *fd;
int width;
char *s;
long len;
{
   tended char *ts = s;
   long tally = 0;
   long n = 0;

#ifdef XWindows
   if (isatty(fileno(fd))) wflushall();
#endif					/* XWindows */

   while (len > 0) {
      n = fread(ts, width, (int)((len < MaxIn) ? len : MaxIn), fd);
      if (n <= 0) {
         return tally;
	 }
      tally += n;
      ts += n;
      len -= n;
      }
   return tally;
   }

/*
 * Print string referenced by descriptor d. Note, d must not move during
 *   a garbage collection.
 */

int putstr(f, d)
register FILE *f;
dptr d;
   {
   register char *s;
   register word l;

   l = StrLen(*d);
   if (l == 0)
      return  Succeeded;
   s = StrLoc(*d);
   if (longwrite(s,l,f) < 0)
      return Failed;
   else
      return Succeeded;
   }

/*
 * idelay(n) - delay for n milliseconds
 */
int idelay(n)
int n;
   {
   #if MSWIN
      Sleep(n);
      return Succeeded;
   #else				/* MSWIN */
      struct timeval t;
      t.tv_sec = n / 1000;
      t.tv_usec = (n % 1000) * 1000;
      select(1, NULL, NULL, NULL, &t);
      return Succeeded;
   #endif				/* MSWIN */
   }

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

#ifdef FAttrib
/*
 * make_mode takes mode_t type (an integer) input and returns the
 * file permission in the format of a string.
*/
char *make_mode (mode_t st_mode)
{
   char *buf;

   if ( (buf = (char *) malloc(sizeof(char)*11)) == NULL ) {
      fprintf(stderr,"fatal malloc error\n");
      return NULL;
   }

   if ( st_mode & S_IFIFO )      buf[0] = 'f';
   else if ( st_mode & S_IFCHR ) buf[0] = 'c';
   else if ( st_mode & S_IFDIR ) buf[0] = 'd';
   else if ( st_mode & S_IFREG ) buf[0] = '-';
   else			         buf[0] = '\?';

   if (st_mode & S_IREAD)  buf[1] = 'r';  else buf[1] = '-';
   if (st_mode & S_IWRITE) buf[2] = 'w';  else buf[2] = '-';
   if (st_mode & S_IEXEC)  buf[3] = 'x';  else buf[3] = '-';
   if (st_mode & S_IREAD)  buf[4] = 'r';  else buf[4] = '-';
   if (st_mode & S_IWRITE) buf[5] = 'w';  else buf[5] = '-';
   if (st_mode & S_IEXEC)  buf[6] = 'x';  else buf[6] = '-';
   if (st_mode & S_IREAD)  buf[7] = 'r';  else buf[7] = '-';
   if (st_mode & S_IWRITE) buf[8] = 'w';  else buf[8] = '-';
   if (st_mode & S_IEXEC)  buf[9] = 'x';  else buf[9] = '-';

   buf[10] = '\0';
   return buf;
}
#endif					/* FAttrib */
