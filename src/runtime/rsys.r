/*
 * File: rsys.r
 *  Contents: [flushrec], [getrec], getstrg, host, longread, [putrec], putstr
 */

#ifdef RecordIO
/*
 * flushrec - force buffered output to be written with a record break.
 *  Applies only to files with mode "s".
 */

void flushrec(fd)
FILE *fd;
{
#if SASC
   afwrite("", 1, 0, fd);
#endif					/* SASC */
}

/*
 * getrec - read a record into buf from file fd. At most maxi characters
 *  are read.  getrec returns the length of the record.
 *  Returns -1 if EOF and -2 if length was limited by
 *  maxi. [[ Needs ferror() check. ]]
 *  This function is meaningful only for files opened with mode "s".
 */

int getrec(buf, maxi, fd)
register char *buf;
int maxi;
FILE *fd;
   {
#ifdef SASC
   register int l;

   l = afreadh(buf, 1, maxi+1, fd);     /* read record or maxi+1 chars */
   if (l == 0) return -1;
   if (l <= maxi) return l;
   ungetc(buf[maxi], fd);               /* if record not used up, push
                                           back last char read */
   return -2;
#endif					/* SASC */
   }
#endif					/* RecordIO */

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

#if AMIGA
#if LATTICE
   /* This code is special for Lattice 4.0.  It was different for
    *  Lattice 3.10 and probably won't work for other C compilers.
    */
   extern struct UFB _ufbs[];

   if (IsInteractive(_ufbs[fileno(fd)].ufbfh))
      return read(fileno(fd),buf,maxi);
#endif					/* LATTICE */
#endif					/* AMIGA */

#ifdef XWindows
   if (isatty(fileno(fd))) wflushall();
#endif					/* XWindows */

   l = 0;
   while (1) {

#ifdef Graphics
      /* insert non-blocking read/code to service windows here */
#endif					/* Graphics */

      if ((c = fgetc(fd)) == '\n')	/* \n terminates line */
	 break;
      if (c == '\r' && (fbp->status & Fs_Untrans) == 0) {
	 /* \r terminates line in translated mode */
	 if ((c = fgetc(fd)) != '\n')	/* consume following \n */
	     ungetc(c, fd);		/* (put back if not \n) */
	 break;
	 }
      if (c == EOF)
	 if (l > 0) return l;
	 else return -1;
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

#ifdef HostStr
   /*
    * The string constant HostStr contains the host name.
    */
   strcpy(hostname,HostStr);
#elif VMS				/* HostStr */
   /*
    * VMS has its own special logic.
    */
   char *h;
   if (!(h = getenv("ICON_HOST")) && !(h = getenv("SYS$NODE")))
      h = "VAX/VMS";
   strcpy(hostname,h);
#else					/* HostStr */
   {
   /*
    * Use the uname system call.  (POSIX)
    */
   struct utsname utsn;
   uname(&utsn);
   strcpy(hostname,utsn.nodename);
   }
#endif					/* HostStr */

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

#if NT
   /*
    * Under NT/MSVC++, ftell() used in Icon where() returns bad answers
    * after a wlongread().  We work around it here by fseeking after fread.
    */
   long pos = ftell(fd);
#endif					/* NT */

#ifdef XWindows
   if (isatty(fileno(fd))) wflushall();
#endif					/* XWindows */

   while (len > 0) {
      n = fread(ts, width, (int)((len < MaxIn) ? len : MaxIn), fd);
      if (n <= 0) {
#if NT
         fseek(fd, pos + tally, SEEK_SET);
#endif					/* NT */
         return tally;
	 }
      tally += n;
      ts += n;
      len -= n;
      }
#if NT
   fseek(fd, pos + tally, SEEK_SET);
#endif					/* NT */
   return tally;
   }

#ifdef RecordIO
/*
 * Write string referenced by descriptor d, avoiding a record break.
 *  Applies only to files openend with mode "s".
 */

int putrec(f, d)
register FILE *f;
dptr d;
   {
#if SASC
   register char *s;
   register word l;

   l = StrLen(*d);
   if (l == 0)
      return Succeeded;
   s = StrLoc(*d);

   if (afwriteh(s,1,l,f) < l)
      return Failed;
   else
      return Succeeded;
   /*
    * Note:  Because RecordIO depends on SASC, and because SASC
    *  uses its own malloc rather than the Icon malloc, file usage
    *  cannot cause a garbage collection.  This may require
    *  reevaluation if RecordIO is supported for any other compiler.
    */
#endif					/* SASC */
   }
#endif					/* RecordIO */

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

#ifdef MSWindows
#ifdef ConsoleWindow
   if ((f == stdout && !(ConsoleFlags & StdOutRedirect)) ||
	(f == stderr && !(ConsoleFlags & StdErrRedirect))) {
      if (ConsoleBinding == NULL)
         ConsoleBinding = OpenConsole();
#if BORLAND_286
      goto Lab1;
#else
      { int i; for(i=0;i<l;i++) Consoleputc(s[i], f); }
#endif
      return Succeeded;
      }
Lab1:
#endif					/* ConsoleWindow */
#endif					/* MSWindows */
#ifdef PresentationManager
   if (ConsoleFlags & OutputToBuf) {
      /* check for overflow */
      if (MaxReadStr * 4 - ((int)ConsoleStringBufPtr - (int)ConsoleStringBuf) < l + 1)
	 return Failed;
      /* big enough */
      memcpy(ConsoleStringBufPtr, s, l);
      ConsoleStringBufPtr += l;
      *ConsoleStringBufPtr = '\0';
      } /* End of if - push to buffer */
   else if ((f == stdout && !(ConsoleFlags & StdOutRedirect)) ||
	    (f == stderr && !(ConsoleFlags & StdErrRedirect)))
      wputstr((wbinding *)PMOpenConsole(), s, l);
   return Succeeded;
#endif					/* PresentationManager */
#if VMS
   /*
    * This is to get around a bug in VMS C's fwrite routine.
    */
   {
      int i;
      for (i = 0; i < l; i++)
         if (putc(s[i], f) == EOF)
            break;
      if (i == l)
         return Succeeded;
      else
         return Failed;
   }
#else					/* VMS */
   if (longwrite(s,l,f) < 0)
      return Failed;
   else
      return Succeeded;
#endif					/* VMS */
   }

/*
 * idelay(n) - delay for n milliseconds
 */
int idelay(n)
int n;
   {

/*
 * The following code is operating-system dependent [@fsys.01].
 */
#if OS2
#if OS2_32
   DosSleep(n);
   return Succeeded;
#else					/* OS2_32 */
   return Failed;
#endif					/* OS2_32 */
#endif					/* OS2 */

#if VMS
   delay_vms(n);
   return Succeeded;
#endif					/* VMS */

#if SASC
   sleepd(0.001*n);
   return Succeeded;
#endif                                   /* SASC */

#if UNIX
   struct timeval t;
   t.tv_sec = n / 1000;
   t.tv_usec = (n % 1000) * 1000;
   select(1, NULL, NULL, NULL, &t);
   return Succeeded;
#endif					/* UNIX */

#if MSDOS
#if SCCX_MX
   msleep(n);
   return Succeeded;
#else					/* SCCX_MX */
#if NT
#ifdef MSWindows
   Sleep(n);
#else					/* MSWindows */
   /* ? should be a way for NT console apps to sleep... */
   return Failed;
#endif					/* MSWindows */
   return Succeeded;
#else					/* NT */
#if BORLAND_286
   /* evil busy wait */
    clock_t start = clock();
    while ((double)((clock() - start) / CLK_TCK) / 1000 < n);
    return Succeeded;
#else					/* BORLAND_286 */
   return Failed;
#endif					/* BORLAND_286 */
#endif					/* NT */
#endif					/* SCCX_MX */
#endif					/* MSDOS */

#if MACINTOSH
   void MacDelay(int n);
   MacDelay(n);
   return Succeeded;
#endif					/* MACINTOSH */


#if AMIGA
#if __SASC
   Delay(n/20);
   return Succeeded;
#else					/* __SASC */
   return Failed
#endif                                  /* __SASC */
#endif					/* AMIGA */

#if PORT || ARM || ATARI_ST || MVS || VM
   return Failed;
#endif					/* PORT || ARM || ATARI_ST ... */

   /*
    * End of operating-system dependent code.
    */
   }

#ifdef MSWindows
#ifdef FAttrib
/*
 * make_mode takes mode_t type (an integer) input and returns the file permission
 * in the format of a string.
*/
#if UNIX
char *make_mode (mode_t st_mode)
#endif					/* UNIX */
#if MSDOS
char *make_mode (unsigned short st_mode)
#endif					/* MSDOS */
{
   char *buf;

   if ( (buf = (char *) malloc(sizeof(char)*11)) == NULL ) {
      fprintf(stderr,"fatal malloc error\n");
      return NULL;
   }

#if UNIX
   if ( st_mode & S_IFIFO )      buf[0] = 'f';
   else if ( st_mode & S_IFCHR ) buf[0] = 'c';
   else if ( st_mode & S_IFDIR ) buf[0] = 'd';
   else if ( st_mode & S_IFBLK ) buf[0] = 'b';
   else if ( st_mode & S_IFREG ) buf[0] = '-';
   else			         buf[0] = '\?';

   if (st_mode & S_IRUSR) buf[1] = 'r';
   else    buf[1] = '-';
   if (st_mode & S_IWUSR) buf[2] = 'w';
   else    buf[2] = '-';
   if (st_mode & S_IXUSR) buf[3] = 'x';
   else    buf[3] = '-';
   if (st_mode & S_IRGRP) buf[4] = 'r';
   else    buf[4] = '-';
   if (st_mode & S_IWGRP) buf[5] = 'w';
   else    buf[5] = '-';
   if (st_mode & S_IXGRP) buf[6] = 'x';
   else    buf[6] = '-';
   if (st_mode & S_IROTH) buf[7] = 'r';
   else    buf[7] = '-';
   if (st_mode & S_IWOTH) buf[8] = 'w';
   else    buf[8] = '-';
   if (st_mode & S_IXOTH) buf[9] = 'x';
   else    buf[9] = '-';
#endif					/* UNIX */
#if MSDOS
   if ( st_mode & _S_IFIFO )      buf[0] = 'f';
   else if ( st_mode & _S_IFCHR ) buf[0] = 'c';
   else if ( st_mode & _S_IFDIR ) buf[0] = 'd';
   else if ( st_mode & _S_IFREG ) buf[0] = '-';
   else			         buf[0] = '\?';

   if (st_mode & S_IREAD) buf[1] = 'r';
   else    buf[1] = '-';
   if (st_mode & S_IWRITE) buf[2] = 'w';
   else    buf[2] = '-';
   if (st_mode & S_IEXEC) buf[3] = 'x';
   else    buf[3] = '-';
   if (st_mode & S_IREAD) buf[4] = 'r';
   else    buf[4] = '-';
   if (st_mode & S_IWRITE) buf[5] = 'w';
   else    buf[5] = '-';
   if (st_mode & S_IEXEC) buf[6] = 'x';
   else    buf[6] = '-';
   if (st_mode & S_IREAD) buf[7] = 'r';
   else    buf[7] = '-';
   if (st_mode & S_IWRITE) buf[8] = 'w';
   else    buf[8] = '-';
   if (st_mode & S_IEXEC) buf[9] = 'x';
   else    buf[9] = '-';
#endif					/* MSDOS */
 
   buf[10] = '\0';

   return buf;     
}

#endif					/* FAttrib */
#endif					/* MSWindows */
