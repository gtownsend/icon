/*
 * File: winmain.r
 * Special initialization under MS Windows.
 *
 * THIS FILE IS UNTESTED.
 * It preserves some old code but needs work to become functional.
 */

#ifdef MSWindows

FILE *finredir, *fouredir, *ferredir;

void detectRedirection()
{
   struct stat sb;
   /*
    * Look at the standard file handles and attempt to detect
    * redirection.
    */
   if (fstat(stdin->_file, &sb) == 0) {
      if (sb.st_mode & S_IFCHR) {		/* stdin is a device */
	 }
      if (sb.st_mode & S_IFREG) {		/* stdin is a regular file */
	 }
      /* stdin is of size sb.st_size */
      if (sb.st_size > 0) {
         ConsoleFlags |= StdInRedirect;
	 }
      }
   else {					/* unable to identify stdin */
      }

   if (fstat(stdout->_file, &sb) == 0) {
      if (sb.st_mode & S_IFCHR) {		/* stdout is a device */
	 }
      if (sb.st_mode & S_IFREG) {		/* stdout is a regular file */
	 }
      /* stdout is of size sb.st_size */
      if (sb.st_size == 0)
         ConsoleFlags |= StdOutRedirect;
      }
   else {					/* unable to identify stdout */
     }
}

int CmdParamToArgv(char *s, char ***avp)
   {
   char tmp[MaxPath], dir[MaxPath];
   char *t, *t2;
   int rv=0, i;
   FILE *f;

   t = salloc(s);
   t2 = t;

   *avp = malloc(2 * sizeof(char *));
   (*avp)[rv] = NULL;

   detectRedirection();

   while (*t2) {
      while (*t2 && isspace(*t2)) t2++;
      switch (*t2) {
	 case '\0': break;
	 case '<': case '>': {
	    /*
	     * perform file redirection; this is for Windows 3.1
	     * and other situations where Wiconx is launched from
	     * a shell that does not process < and > characters.
	     */
	    char c = *t2++, buf[128], *t3;
	    FILE *f;
	    while (*t2 && isspace(*t2)) t2++;
	    t3 = buf;
	    while (*t2 && !isspace(*t2)) *t3++ = *t2++;
	    *t3 = '\0';
	    if (c == '<')
	       f = fopen(buf, "r");
	    else
	       f = fopen(buf, "w");
	    if (f == NULL) {
	       MessageBox(0, "unable to redirect i/o", "system error",
			  MB_ICONHAND);
	       c_exit(-1);
	       }
	    if (c == '<') {
	       finredir = f;
	       ConsoleFlags |= StdInRedirect;
	       }
	    else {
	       fouredir = f;
	       ConsoleFlags |= StdOutRedirect;
	       }
	    break;
	    }
	 case '"': {
	    char *t3 = ++t2;			/* skip " */

            while (*t2 && (*t2 != '"')) t2++;
            if (*t2)
	       *t2++ = '\0';
	    *avp = realloc(*avp, (rv + 2) * sizeof (char *));
	    (*avp)[rv++] = salloc(t3);
            (*avp)[rv] = NULL;

	    break;
	    }
         default: {
            FINDDATA_T fd;
	    char *t3 = t2;
            while (*t2 && !isspace(*t2)) t2++;
	    if (*t2)
	       *t2++ = '\0';
            strcpy(tmp, t3);
	    if (!FINDFIRST(tmp, &fd)) {
	       *avp = realloc(*avp, (rv + 2) * sizeof (char *));
	       (*avp)[rv++] = salloc(t3);
               (*avp)[rv] = NULL;
               }
	    else {
               int end;
               strcpy(dir, t3);
	       do {
	          end = strlen(dir)-1;
	          while (end >= 0 && dir[end] != '\\' && dir[end] != '/' &&
			dir[end] != ':') {
                     dir[end] = '\0';
		     end--;
	             }
		  strcat(dir, FILENAME(&fd));
	          *avp = realloc(*avp, (rv + 2) * sizeof (char *));
	          (*avp)[rv++] = salloc(dir);
                  (*avp)[rv] = NULL;
	          } while (FINDNEXT(&fd));
	       FINDCLOSE(&fd);
	       }
            break;
	    }
         }
      }

   free(t);
   return rv;
   }

char *lognam;
char tmplognam[128];

void MSStartup(HINSTANCE hInstance, HINSTANCE hPrevInstance)
   {
   WNDCLASS wc;
   #ifdef ConsoleWindow
      extern FILE *flog;

      /*
       * Select log file name.  Might make this a command-line option.
       * Default to "WICON.LOG".  The log file is used by Wi to report
       * translation errors and jump to the offending source code line.
       */
      if ((lognam = getenv("WICONLOG")) == NULL)
         lognam = "WICON.LOG";
      remove(lognam);
      lognam = strdup(lognam);
      flog = fopen(tmpnam(tmplognam), "w");

      if (flog == NULL) {
         syserr("unable to open logfile");
         }
   #endif				/* ConsoleWindow */
   if (!hPrevInstance) {
      #if NT
         wc.style = CS_HREDRAW | CS_VREDRAW;
      #else				/* NT */
         wc.style = 0;
      #endif				/* NT */
      wc.lpfnWndProc = WndProc;
      wc.cbClsExtra = 0;
      wc.cbWndExtra = 0;
      wc.hInstance  = hInstance;
      wc.hIcon      = NULL;
      wc.hCursor    = NULL;
      wc.hbrBackground = GetStockObject(WHITE_BRUSH);
      wc.lpszMenuName = NULL;
      wc.lpszClassName = "iconx";
      RegisterClass(&wc);
      }
   }

void iconx(int argc, char **argv);

jmp_buf mark_sj;

int_PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int nCmdShow)
   {
   int argc;
   char **argv;

   mswinInstance = hInstance;
   ncmdShow = nCmdShow;

   argc = CmdParamToArgv(GetCommandLine(), &argv);
   MSStartup(hInstance, hPrevInstance);
   if (setjmp(mark_sj) == 0)
      iconx(argc,argv);
   while (--argc>=0)
      free(argv[argc]);
   free(argv);
   wfreersc();
   xmfree();
   return 0;
}

/*
 * Free malloc-ed memory; the main regions then co-expressions.  Note:
 *  this is only correct if all allocation is done by routines that are
 *  compatible with free() -- which may not be the case if Allocreg()
 *  in rmemfix.c is defined to be other than malloc().
 */

void xmfree()
   {
   register struct b_coexpr **ep, *xep;
   register struct astkblk *abp, *xabp;

   if (mainhead != (struct b_coexpr *)NULL)
      free((pointer)mainhead->es_actstk);	/* activation block for &main */
   free((pointer)code);			/* icode */
   code = NULL;
   free((pointer)stack);		/* interpreter stack */
   stack = NULL;
   /*
    * more is needed to free chains of heaps, also a multithread version
    * of this function may be needed someday.
    */
   if (strbase)
      free((pointer)strbase);		/* allocated string region */
   strbase = NULL;
   if (blkbase)
      free((pointer)blkbase);		/* allocated block region */
   blkbase = NULL;
   if (curstring != &rootstring)
      free((pointer)curstring);		/* string region */
   curstring = NULL;
   if (curblock != &rootblock)
      free((pointer)curblock);		/* allocated block region */
   curblock = NULL;
   if (quallist)
      free((pointer)quallist);		/* qualifier list */
   quallist = NULL;

   /*
    * The co-expression blocks are linked together through their
    *  nextstk fields, with stklist pointing to the head of the list.
    *  The list is traversed and each stack is freeing.
    */
   ep = &stklist;
   while (*ep != NULL) {
      xep = *ep;
      *ep = (*ep)->nextstk;
       /*
        * Free the astkblks.  There should always be one and it seems that
        *  it's not possible to have more than one, but nonetheless, the
        *  code provides for more than one.
        */
      for (abp = xep->es_actstk; abp; ) {
         xabp = abp;
         abp = abp->astk_nxt;
         free((pointer)xabp);
         }
      free((pointer)xep);
      stklist = NULL;
      }

   }

#endif					/* MSWindows */
