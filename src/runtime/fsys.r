/*
 * File: fsys.r
 *  Contents: close, chdir, exit, getenv, open, read, reads, remove, rename,
 *  seek, stop, system, where, write, writes, [getch, getche, kbhit]
 */

"close(f) - close file f."

function{1} close(f)

   if !is:file(f) then
      runerr(105, f)

   abstract {
      return file ++ integer
      }

   body {
      FILE *fp;
      int status;

      fp = BlkLoc(f)->file.fd;
      status = BlkLoc(f)->file.status;
      if ((status & (Fs_Read | Fs_Write)) == 0)
	 return f;			/* if already closed */

      #ifdef Graphics
         pollctr >>= 1;
         pollctr++;
         if (BlkLoc(f)->file.status & Fs_Window) {
            /*
             * Close a window.
             */
   	    BlkLoc(f)->file.status = Fs_Window;	/* clears read and write */
   	    SETCLOSED((wbp) fp);
   	    wclose((wbp) fp);
   	    return f;
   	 }
      #endif				/* Graphics */

      #ifdef ReadDirectory
         if (BlkLoc(f)->file.status & Fs_Directory) {
	    /*
	     * Close a directory.
	     */
            closedir((DIR*) fp);
	    BlkLoc(f)->file.status = 0;
	    return f;
	    }
      #endif				/* ReadDirectory */
      
      #ifdef Pipes
         if (BlkLoc(f)->file.status & Fs_Pipe) {
            /*
             * Close a pipe.  (Returns pclose status, contrary to doc.)
             */
	    BlkLoc(f)->file.status = 0;
	    return C_integer((pclose(fp) >> 8) & 0377);
	    }
      #endif				/* Pipes */

      /*
       * Close a simple file.
       */
      fclose(fp);
      BlkLoc(f)->file.status = 0;
      return f;
      }
end

#undef exit
#passthru #undef exit

"exit(i) - exit process with status i, which defaults to 0."

function{} exit(status)
   if !def:C_integer(status, EXIT_SUCCESS) then
      runerr(101, status)
   inline {
      c_exit((int)status);
      }
end


"getenv(s) - return contents of environment variable s."

function{0,1} getenv(s)

   /*
    * Make a C-style string out of s
    */
   if !cnv:C_string(s) then
      runerr(103,s)
   abstract {
      return string
      }

   inline {
      register char *p;
      long l;

      if ((p = getenv(s)) != NULL) {	/* get environment variable */
	 l = strlen(p);
	 Protect(p = alcstr(p,l),runerr(0));
	 return string(l,p);
	 }
      else				/* fail if not in environment */
	 fail;

      }
end


#ifdef Graphics
"open(s1, s2, ...) - open file named s1 with options s2"
" and attributes given in trailing arguments."
function{0,1} open(fname, spec, attr[n])
#else					/* Graphics */
"open(fname, spec) - open file fname with specification spec."
function{0,1} open(fname, spec)
#endif					/* Graphics */
   declare {
      tended struct descrip filename;
      }

   /*
    * fopen and popen require a C string, but it looks terrible in
    *  error messages, so convert it to a string here and use a local
    *  variable (fnamestr) to store the C string.
    */
   if !cnv:string(fname) then
      runerr(103, fname)

   /*
    * spec defaults to "r".
    */
   if !def:tmp_string(spec, letr) then
      runerr(103, spec)

   abstract {
      return file
      }

   body {
      tended char *fnamestr;
      register word slen;
      register int i;
      register char *s;
      int status;
      char mode[4];
      extern FILE *fopen();
      FILE *f;
      struct b_file *fl;

#ifdef Graphics
      int j, err_index = -1;
      tended struct b_list *hp;
      tended struct b_lelem *bp;
#endif					/* Graphics */

      /*
       * get a C string for the file name
       */
      if (!cnv:C_string(fname, fnamestr))
	 runerr(103,fname);

      status = 0;

      /*
       * Scan spec, setting appropriate bits in status.  Produce a
       *  run-time error if an unknown character is encountered.
       */
      s = StrLoc(spec);
      slen = StrLen(spec);
      for (i = 0; i < slen; i++) {
	 switch (*s++) {
	    case 'a':
	    case 'A':
	       status |= Fs_Write|Fs_Append;
	       continue;
	    case 'b':
	    case 'B':
	       status |= Fs_Read|Fs_Write;
	       continue;
	    case 'c':
	    case 'C':
	       status |= Fs_Create|Fs_Write;
	       continue;
	    case 'r':
	    case 'R':
	       status |= Fs_Read;
	       continue;
	    case 'w':
	    case 'W':
	       status |= Fs_Write;
	       continue;
	    case 't':
	    case 'T':
	       status &= ~Fs_Untrans;
	       continue;
	    case 'u':
	    case 'U':
	       status |= Fs_Untrans;
	       continue;

            #ifdef Pipes
	    case 'p':
	    case 'P':
	       status |= Fs_Pipe;
	       continue;
            #endif			/* Pipes */

	    case 'x':
	    case 'X':
	    case 'g':
	    case 'G':
#ifdef Graphics
	       status |= Fs_Window | Fs_Read | Fs_Write;
	       continue;
#else					/* Graphics */
	       fail;
#endif					/* Graphics */

	    default:
	       runerr(209, spec);
	    }
	 }

      /*
       * Construct a mode field for fopen/popen.
       */
      mode[0] = '\0';
      mode[1] = '\0';
      mode[2] = '\0';
      mode[3] = '\0';

      if ((status & (Fs_Read|Fs_Write)) == 0)	/* default: read only */
	 status |= Fs_Read;
      if (status & Fs_Create)
	 mode[0] = 'w';
      else if (status & Fs_Append)
	 mode[0] = 'a';
      else if (status & Fs_Read)
	 mode[0] = 'r';
      else
	 mode[0] = 'w';

      if ((status & (Fs_Read|Fs_Write)) == (Fs_Read|Fs_Write))
	 mode[1] = '+';
      if ((status & Fs_Untrans) != 0)
         strcat(mode, "b");

      /*
       * Open the file with fopen or popen.
       */

#ifdef Graphics
      if (status & Fs_Window) {
	 /*
	  * allocate an empty event queue for the window
	  */
	 Protect(hp = alclist(0), runerr(0));
	 Protect(bp = alclstb(MinListSlots, (word)0, 0), runerr(0));
	 hp->listhead = hp->listtail = (union block *) bp;
#ifdef ListFix
	 bp->listprev = bp->listnext = (union block *) hp;
#endif					/* ListFix */

	 /*
	  * loop through attributes, checking validity
	  */
	 for (j = 0; j < n; j++) {
	    if (is:null(attr[j]))
	       attr[j] = emptystr;
	    if (!is:string(attr[j]))
	       runerr(109, attr[j]);
	    }

	 f = (FILE *)wopen(fnamestr, hp, attr, n, &err_index);
	 if (f == NULL) {
	    if (err_index >= 0) runerr(145, attr[err_index]);
	    else if (err_index == -1) fail;
	    else runerr(305);
	    }
	 } else
#endif					/* Graphics */

#ifdef Pipes
      if (status & Fs_Pipe) {
	 if (status != (Fs_Read|Fs_Pipe) && status != (Fs_Write|Fs_Pipe))
	    runerr(209, spec);
	 f = popen(fnamestr, mode);
	 }
      else
#endif					/* Pipes */

         {
#ifdef ReadDirectory
         struct stat sbuf;
         if ((status & Fs_Write) == 0
         && stat(fnamestr, &sbuf) == 0
         && S_ISDIR(sbuf.st_mode)) {
            status |= Fs_Directory;
            f = (FILE*) opendir(fnamestr);
            }
         else
#endif					/* ReadDirectory */
            f = fopen(fnamestr, mode);
         }

      /*
       * Fail if the file cannot be opened.
       */
      if (f == NULL) {
	 fail;
	 }

      /*
       * Return the resulting file value.
       */
      StrLen(filename) = strlen(fnamestr);
      StrLoc(filename) = fnamestr;

      Protect(fl = alcfile(f, status, &filename), runerr(0));
#ifdef Graphics
      /*
       * link in the Icon file value so this window can find it
       */
      if (status & Fs_Window) {
	 ((wbp)f)->window->filep.dword = D_File;
	 BlkLoc(((wbp)f)->window->filep) = (union block *)fl;
	 if (is:null(lastEventWin)) {
	    lastEventWin = ((wbp)f)->window->filep;
            lastEvFWidth = FWIDTH((wbp)f);
            lastEvLeading = LEADING((wbp)f);
            lastEvAscent = ASCENT((wbp)f);
            }
	 }
#endif					/* Graphics */
      return file(fl);
      }
end


"read(f) - read line on file f."

function{0,1} read(f)
   /*
    * Default f to &input.
    */
   if is:null(f) then
      inline {
	 f.dword = D_File;
	 BlkLoc(f) = (union block *)&k_input;
	 }
   else if !is:file(f) then
      runerr(105, f)

   abstract {
      return string
      }

   body {
      register word slen, rlen;
      register char *sp;
      int status;
      static char sbuf[MaxReadStr];
      tended struct descrip s;
      FILE *fp;

      /*
       * Get a pointer to the file and be sure that it is open for reading.
       */
      fp = BlkLoc(f)->file.fd;
      status = BlkLoc(f)->file.status;
      if ((status & Fs_Read) == 0)
	 runerr(212, f);

      if (status & Fs_Writing) {
	 fseek(fp, 0L, SEEK_CUR);
	 BlkLoc(f)->file.status &= ~Fs_Writing;
	 }
      BlkLoc(f)->file.status |= Fs_Reading;

#ifdef ReadDirectory
      if ((BlkLoc(f)->file.status & Fs_Directory) != 0) {
         struct dirent *de = readdir((DIR*) fp);
         if (de == NULL)
            fail;
         slen = strlen(de->d_name);
         Protect(sp = alcstr(de->d_name, slen), runerr(0));
         return string(slen, sp);
         }
#endif					/* ReadDirectory */

      /*
       * Use getstrg to read a line from the file, failing if getstrg
       *  encounters end of file. [[ What about -2?]]
       */
      StrLen(s) = 0;
      do {
#ifdef Graphics
	 pollctr >>= 1;
	 pollctr++;
	 if (status & Fs_Window) {
	    slen = wgetstrg(sbuf,MaxReadStr,fp);
	    if (slen == -1)
	       runerr(141);
	    if (slen == -2)
	       runerr(143);
	    if (slen == -3)
               fail;
	    }
	 else
#endif					/* Graphics */

	 if ((slen = getstrg(sbuf, MaxReadStr, &BlkLoc(f)->file)) == -1)
	    fail;

	 /*
	  * Allocate the string read and make s a descriptor for it.
	  */
	 rlen = slen < 0 ? (word)MaxReadStr : slen;

	 Protect(reserve(Strings, rlen), runerr(0));
	 if (StrLen(s) > 0 && !InRange(strbase,StrLoc(s),strfree)) {
	    Protect(reserve(Strings, StrLen(s)+rlen), runerr(0));
	    Protect((StrLoc(s) = alcstr(StrLoc(s),StrLen(s))), runerr(0));
	    }

	 Protect(sp = alcstr(sbuf,rlen), runerr(0));
	 if (StrLen(s) == 0)
	    StrLoc(s) = sp;
	 StrLen(s) += rlen;
	 } while (slen < 0);
      return s;
      }
end


"reads(f,i) - read i characters on file f."

function{0,1} reads(f,i)
   /*
    * Default f to &input.
    */
   if is:null(f) then
      inline {
	 f.dword = D_File;
	 BlkLoc(f) = (union block *)&k_input;
	 }
   else if !is:file(f) then
      runerr(105, f)

   /*
    * i defaults to 1 (read a single character)
    */
   if !def:C_integer(i,1L) then
      runerr(101, i)

   abstract {
      return string
      }

   body {
      long tally, nbytes;
      int status;
      FILE *fp;
      tended struct descrip s;

      /*
       * Get a pointer to the file and be sure that it is open for reading.
       */
      fp = BlkLoc(f)->file.fd;
      status = BlkLoc(f)->file.status;
      if ((status & Fs_Read) == 0)
	 runerr(212, f);

      if (status & Fs_Writing) {
	 fseek(fp, 0L, SEEK_CUR);
	 BlkLoc(f)->file.status &= ~Fs_Writing;
	 }
      BlkLoc(f)->file.status |= Fs_Reading;

      /*
       * Be sure that a positive number of bytes is to be read.
       */
      if (i <= 0) {
	 irunerr(205, i);

	 errorfail;
	 }

#ifdef ReadDirectory
      /*
       *  If reading a directory, return up to i bytes of next entry.
       */
      if ((BlkLoc(f)->file.status & Fs_Directory) != 0) {
         char *sp;
         struct dirent *de = readdir((DIR*) fp);
         if (de == NULL)
            fail;
         nbytes = strlen(de->d_name);
         if (nbytes > i)
            nbytes = i;
         Protect(sp = alcstr(de->d_name, nbytes), runerr(0));
         return string(nbytes, sp);
         }
#endif					/* ReadDirectory */

      /*
       * For now, assume we can read the full number of bytes.
       */
      Protect(StrLoc(s) = alcstr(NULL, i), runerr(0));
      StrLen(s) = 0;

#ifdef Graphics
      pollctr >>= 1;
      pollctr++;
      if (status & Fs_Window) {
	 tally = wlongread(StrLoc(s),sizeof(char),i,fp);
	 if (tally == -1)
	    runerr(141);
	 else if (tally == -2)
	    runerr(143);
	 else if (tally == -3)
            fail;
	 }
      else
#endif					/* Graphics */
      tally = longread(StrLoc(s),sizeof(char),i,fp);

      if (tally == 0)
	 fail;
      StrLen(s) = tally;
      /*
       * We may not have used the entire amount of storage we reserved.
       */
      nbytes = DiffPtrs(StrLoc(s) + tally, strfree);
      if (nbytes < 0)
         EVVal(-nbytes, E_StrDeAlc);
      else
         EVVal(nbytes, E_String);
      strtotal += nbytes;
      strfree = StrLoc(s) + tally;
      return s;
      }
end


"remove(s) - remove the file named s."

function{0,1} remove(s)

   /*
    * Make a C-style string out of s
    */
   if !cnv:C_string(s) then
      runerr(103,s)
   abstract {
      return null
      }

   inline {
      if (remove(s) != 0)
	 fail;
      return nulldesc;
      }
end


"rename(s1,s2) - rename the file named s1 to have the name s2."

function{0,1} rename(s1,s2)

   /*
    * Make C-style strings out of s1 and s2
    */
   if !cnv:C_string(s1) then
      runerr(103,s1)
   if !cnv:C_string(s2) then
      runerr(103,s2)

   abstract {
      return null
      }

   body {
      if (rename(s1,s2) != 0)
	 fail;
      return nulldesc;
      }
end


"seek(f,i) - seek to offset i in file f."
" [[ What about seek error ? ]] "

function{0,1} seek(f,o)

   /*
    * f must be a file
    */
   if !is:file(f) then
      runerr(105,f)

   /*
    * o must be an integer and defaults to 1.
    */
   if !def:C_integer(o,1L) then
      runerr(0)

   abstract {
      return file
      }

   body {
      FILE *fd;

      fd = BlkLoc(f)->file.fd;
      if (BlkLoc(f)->file.status == 0)
	 fail;
#ifdef ReadDirectory
      if ((BlkLoc(f)->file.status & Fs_Directory) != 0)
         fail;
#endif					/* ReadDirectory */

#ifdef Graphics
      pollctr >>= 1;
      pollctr++;
      if (BlkLoc(f)->file.status & Fs_Window)
	 fail;
#endif					/* Graphics */

      if (o > 0) {
	 if (fseek(fd, o - 1, SEEK_SET) != 0)
	    fail;
	 }
      else {
	 if (fseek(fd, o, SEEK_END) != 0)
	    fail;
	 }
      BlkLoc(f)->file.status &= ~(Fs_Reading | Fs_Writing);
      return f;
      }
end


"system(s) - execute string s as a system command."

function{1} system(s)
   /*
    * Make a C-style string out of s
    */
   if !cnv:C_string(s) then
      runerr(103,s)

   abstract {
      return integer
      }

   inline {
      /*
       * Pass the C string to the system() function and return
       * the exit code of the command as the result of system().
       * Note, the expression on a "return" may not have side effects,
       * so the exit code must be returned via a variable.
       */
      C_integer i;

#ifdef Graphics
      pollctr >>= 1;
      pollctr++;
#endif					/* Graphics */

      i = (C_integer)system(s);
      return C_integer i;
      }
end



"where(f) - return current offset position in file f."

function{0,1} where(f)

   if !is:file(f) then
      runerr(105,f)

   abstract {
      return integer
      }

   body {
      FILE *fd;
      long ftell();
      long pos;

      fd = BlkLoc(f)->file.fd;

      if ((BlkLoc(f)->file.status == 0))
	 fail;
#ifdef ReadDirectory
      if ((BlkLoc(f)->file.status & Fs_Directory) != 0)
         fail;
#endif					/* ReadDirectory */

#ifdef Graphics
      pollctr >>= 1;
      pollctr++;
      if (BlkLoc(f)->file.status & Fs_Window)
	 fail;
#endif					/* Graphics */

      pos = ftell(fd) + 1;
      if (pos == 0)
	 fail;	/* may only be effective on ANSI systems */

      return C_integer pos;
      }
end

/*
 * stop(), write(), and writes() differ in whether they stop the program
 *  and whether they output newlines. The macro GenWrite is used to
 *  produce all three functions.
 */
#define False 0
#define True 1

#begdef DefaultFile(error_out)
   inline {
#if error_out
      if ((k_errout.status & Fs_Write) == 0)
	 runerr(213);
      else {
	 f = k_errout.fd;
	 }
#else					/* error_out */
      if ((k_output.status & Fs_Write) == 0)
	 runerr(213);
      else {
	 f = k_output.fd;
	 }
#endif					/* error_out */
      }
#enddef					/* DefaultFile */

#begdef Finish(retvalue, nl, terminate)
#if nl
   /*
    * Append a newline to the file.
    */
#ifdef Graphics
   pollctr >>= 1;
   pollctr++;
   if (status & Fs_Window)
      wputc('\n',(wbp)f);
   else
#endif					/* Graphics */
      putc('\n', f);
#endif					/* nl */

   /*
    * Flush the file.
    */
#ifdef Graphics
   if (!(status & Fs_Window)) {
#endif					/* Graphics */
      if (ferror(f))
	 runerr(214);
      fflush(f);

#ifdef Graphics
      }
#endif					/* Graphics */


#if terminate
	    c_exit(EXIT_FAILURE);
#else					/* terminate */
	    return retvalue;
#endif					/* terminate */
#enddef					/* Finish */

#begdef GenWrite(name, nl, terminate)

#name "(a,b,...) - write arguments"
#if !nl
   " without newline terminator"
#endif					/* nl */
#if terminate
   " (starting on error output) and stop"
#endif					/* terminate */
"."

#if terminate
function {} name(x[nargs])
#else					/* terminate */
function {1} name(x[nargs])
#endif					/* terminate */

   declare {
      FILE *f = NULL;
      word status = k_errout.status;
      }

#if terminate
   abstract {
      return empty_type
      }
#endif					/* terminate */

   len_case nargs of {
      0: {
#if !terminate
	 abstract {
	    return null
	    }
#endif					/* terminate */
	 DefaultFile(terminate)
	 body {
	    Finish(nulldesc, nl, terminate)
	    }
	 }

      default: {
#if !terminate
	 abstract {
	    return type(x)
	    }
#endif					/* terminate */
	 /*
	  * See if we need to start with the default file.
	  */
	 if !is:file(x[0]) then
	    DefaultFile(terminate)

	 body {
	    tended struct descrip t;
	    register word n;

	    /*
	     * Loop through the arguments.
	     */
	    for (n = 0; n < nargs; n++) {
	       if (is:file(x[n])) {	/* Current argument is a file */
#if nl
		  /*
		   * If this is not the first argument, output a newline to the
		   * current file and flush it.
		   */
		  if (n > 0) {

		     /*
		      * Append a newline to the file and flush it.
		      */
#ifdef Graphics
		     pollctr >>= 1;
		     pollctr++;
		     if (status & Fs_Window) {
			wputc('\n',(wbp)f);
			wflush((wbp)f);
			  }
		     else {
#endif					/* Graphics */
			putc('\n', f);
			if (ferror(f))
			   runerr(214);
			fflush(f);
#ifdef Graphics
			}
#endif					/* Graphics */
		     }
#endif					/* nl */

		  /*
		   * Switch the current file to the file named by the current
		   * argument providing it is a file.
		   */
		  status = BlkLoc(x[n])->file.status;
		  if ((status & Fs_Write) == 0)
		     runerr(213, x[n]);
		  f = BlkLoc(x[n])->file.fd;
		  }
	       else {
		  /*
		   * Convert the argument to a string, defaulting to a empty
		   *  string.
		   */
		  if (!def:tmp_string(x[n],emptystr,t))
		     runerr(109, x[n]);

		  /*
		   * Output the string.
		   */
#ifdef Graphics
		  if (status & Fs_Window)
		     wputstr((wbp)f, StrLoc(t), StrLen(t));
		  else
#endif					/* Graphics */
		  if (putstr(f, &t) == Failed) {
		     runerr(214, x[n]);
		     }
		  }
	       }

	    Finish(x[n-1], nl, terminate)
	    }
	 }
      }
end
#enddef					/* GenWrite */

GenWrite(stop,	 True,	True)  /* stop(s, ...) - write message and stop */
GenWrite(write,  True,	False) /* write(s, ...) - write with new-line */
GenWrite(writes, False, False) /* writes(s, ...) - write with no new-line */

#ifdef KeyboardFncs

"getch() - return a character from console."

function{0,1} getch()
   abstract {
      return string;
      }
   body {
      int i;
      i = getch();
      if (i<0 || i>255)
	 fail;
      return string(1, (char *)&allchars[i & 0xFF]);
      }
end

"getche() -- return a character from console with echo."

function{0,1} getche()
   abstract {
      return string;
      }
   body {
      int i;
      i = getche();
      if (i<0 || i>255)
	 fail;
      return string(1, (char *)&allchars[i & 0xFF]);
      }
end


"kbhit() -- Check to see if there is a keyboard character waiting to be read."

function{0,1} kbhit()
   abstract {
      return null
      }
   inline {
      if (kbhit())
	 return nulldesc;
      else
         fail;
      }
end
#endif					/* KeyboardFncs */

"chdir(s) - change working directory to s."
function{0,1} chdir(s)

   if !cnv:C_string(s) then
      runerr(103,s)
   abstract {
      return null
      }
   inline {
      if (chdir(s) != 0)
         fail;
      return nulldesc;
   }
end

"delay(i) - delay for i milliseconds."

function{1} delay(n)

   if !cnv:C_integer(n) then
      runerr(101,n)
   abstract {
      return null
      }

   inline {
      if (idelay(n) == Failed)
        fail;
#ifdef Graphics
      pollctr >>= 1;
      pollctr++;
#endif					/* Graphics */
      return nulldesc;
      }
end

"flush(f) - flush file f."

function{1} flush(f)
   if !is:file(f) then
      runerr(105, f)
   abstract {
      return type(f)
      }

   body {
      FILE *fp;
      int status;

      fp = BlkLoc(f)->file.fd;
      status = BlkLoc(f)->file.status;
      if ((status & (Fs_Read | Fs_Write)) == 0)
	 return f;			/* if already closed */

#ifdef ReadDirectory
      if ((BlkLoc(f)->file.status & Fs_Directory) != 0)
         return f;
#endif					/* ReadDirectory */

#ifdef Graphics
      pollctr >>= 1;
      pollctr++;
      if (!(BlkLoc(f)->file.status & Fs_Window))
	 fflush(fp);
#else					/* Graphics */
      fflush(fp);
#endif					/* Graphics */

      /*
       * Return the flushed file.
       */
      return f;
      }
end

#ifdef FAttrib

"fattrib(str, att) - get the attribute of a file "

function{*} fattrib (fname, att[argc])

   if !cnv:C_string(fname) then
        runerr(103, fname)

   abstract {
      return string ++ integer
      }

   body {
      tended char *s;
      struct stat fs;
      int fd, i;
      char *retval;
      char *temp;
      long l;

         if ( stat(fname, &fs) == -1 )
            fail;
	 for(i=0; i<argc; i++) {
	    if (!cnv:C_string(att[i], s)) {
	       runerr(103, att[i]);
	       }
         if ( !strcasecmp("size", s) ) {
            suspend C_integer(fs.st_size);
            }
         else if ( !strcasecmp("status", s) ) {
            temp = make_mode (fs.st_mode);
	    l = strlen(temp);
            Protect(retval = alcstr(temp,l), runerr(0));
	    free(temp);
            suspend string(l, retval);
            }
         else if ( !strcasecmp("m_time", s) ) {
	    temp = ctime(&(fs.st_mtime));
	    l = strlen(temp);
	    if (temp[l-1] == '\n') l--;
	    Protect(temp = alcstr(temp, l), runerr(0));
            suspend string(l, temp);
            }
         else if ( !strcasecmp("a_time", s) ) {
	    temp = ctime(&(fs.st_atime));
	    l = strlen(temp);
	    if (temp[l-1] == '\n') l--;
	    Protect(temp = alcstr(temp, l), runerr(0));
            suspend string(l, temp);
            }
         else if ( !strcasecmp("c_time", s) ) {
	    temp = ctime(&(fs.st_ctime));
	    l = strlen(temp);
	    if (temp[l-1] == '\n') l--;
	    Protect(temp = alcstr(temp, l), runerr(0));
            suspend string(l, temp);
            }
         else {
            runerr(205, att[i]);
            }
      }
      fail;
   }
end
#endif					/* FAttrib */
