/*
 * ipp.c -- the Icon preprocessor.
 *
 *  All Icon source passes through here before translation or compilation.
 *  Directives recognized are:
 *	#line n [filename]
 *	$line n [filename]
 *	$include filename
 *	$define identifier text
 *	$undef identifier
 *	$ifdef identifier
 *	$ifndef identifier
 *	$else
 *	$endif
 *	$error [text]
 *
 *  Entry points are
 *	ppinit(fname,inclpath,m4flag) -- open input file
 *	ppdef(s,v) -- "$define s v", or "$undef s" if v is a null pointer
 *	ppch() -- return next preprocessed character
 *	ppecho() -- preprocess to stdout (for icont/iconc -E)
 *
 *  See ../h/features.h for the set of predefined symbols.
 */

#include "../h/gsupport.h"

#define HTBINS 256			/* number of hash bins */

typedef struct fstruct {		/* input file structure */
   struct fstruct *prev;		/* previous file */
   char *fname;				/* file name */
   long lno;				/* line number */
   FILE *fp;				/* stdio file pointer */
   int m4flag;				/* nz if preprocessed by m4 */
   int ifdepth;				/* $if nesting depth when opened */
   } infile;

typedef struct bstruct {		/* buffer pointer structure */
   struct bstruct *prev;		/* previous pointer structure */
   struct cd *defn;			/* definition being processed */
   char *ptr;				/* saved pointer value */
   char *stop;				/* saved stop value */
   char *lim;				/* saved limit value */
   } buffer;

typedef struct {			/* preprocessor token structure */
   char *addr;				/* beginning of token */
   short len;				/* length */
   } ptok;

typedef struct cd {			/* structure holding a definition */
   struct cd *next;			/* link to next defn */
   struct cd *prev;			/* link to previous defn */
   short nlen, vlen;			/* length of name & val */
   char inuse;				/* nonzero if curr being expanded */
   char s[1];				/* name then value, as needed, no \0 */
   } cdefn;

static	int	ppopen	(char *fname, int m4);
static	FILE *	m4pipe	(char *fname);
static	char *	rline	(FILE *fp);
static	void	pushdef	(cdefn *d);
static	void	pushline (char *fname, long lno);
static	void	ppdir	(char *line);
static	void	pfatal	(char *s1, char *s2);
static	void	skipcode (int doelse, int report);
static	char *	define	(char *s);
static	char *	undef	(char *s);
static	char *	ifdef	(char *s);
static	char *	ifndef	(char *s);
static	char *	ifxdef	(char *s, int f);
static	char *	elsedir	(char *s);
static	char *	endif	(char *s);
static	char *	errdir	(char *s);
static	char *	include	(char *s);
static	char *	setline	(char *s);
static	char *	wskip	(char *s);
static	char *	nskip	(char *s);
static	char *	matchq	(char *s);
static	char *	getidt	(char *dst, char *src);
static	char *	getfnm	(char *dst, char *src);
static	cdefn *	dlookup	(char *name, int len, char *val);

struct ppcmd {
   char *name;
   char *(*func)();
   }
pplist[] = {
   { "define",  define  },
   { "undef",   undef   },
   { "ifdef",   ifdef   },
   { "ifndef",  ifndef  },
   { "else",    elsedir },
   { "endif",   endif   },
   { "include", include },
   { "line",    setline },
   { "error",   errdir  },
   { 0,         0       }};

static infile nofile;			/* ancestor of all files; all zero */
static infile *curfile;			/* pointer to current entry */

static buffer *bstack;			/* stack of pending buffers */
static buffer *bfree;			/* pool of free bstructs */

static char *buf;			/* input line buffer */
static char *bnxt;			/* next character */
static char *bstop;			/* limit of preprocessed chars */
static char *blim;			/* limit of all chars */

static cdefn *cbin[HTBINS];		/* hash bins for defn table */

static char *lpath;			/* LPATH for finding source files */

static int ifdepth;			/* depth of $if nesting */

extern int tfatals, nocode;		/* provided by icont, iconc */

/*
 * ppinit(fname, inclpath, m4) -- initialize preprocessor to read from fname.
 *
 *  Returns 1 if successful, 0 if open failed.
 */
int ppinit(fname, inclpath, m4)
char *fname;
char *inclpath;
int m4;
   {
   int i;
   cdefn *d, *n;

   /*
    * clear out any existing definitions from previous files
    */
   for (i = 0; i < HTBINS; i++) {
      for (d = cbin[i]; d != NULL; d = n) {
         n = d->next;
         free((char *)d);
         }
      cbin[i] = NULL;
      }

   /*
    * install predefined symbols
    */
#define Feature(guard,symname,kwval) dlookup(symname, -1, "1");
#include "../h/features.h"

   /*
    * initialize variables and open source file
    */
   lpath = inclpath;
   curfile = &nofile;			/* init file struct pointer */
   return ppopen(fname, m4);		/* open main source file */
   }

/*
 * ppopen(fname, m4) -- open a new file for reading by the preprocessor.
 *
 *  Returns 1 if successful, 0 if open failed.
 *
 *  Open calls may be nested.  Files are closed when EOF is read.
 */
static int ppopen(fname, m4)
char *fname;
int m4;
   {
   FILE *f;
   infile *fs;

   for (fs = curfile; fs->fname != NULL; fs = fs->prev)
      if (strcmp(fname, fs->fname) == 0) {
         pfatal("circular include", fname);	/* issue error message */
         return 1;				/* treat as success */
         }
   if (m4)
      f = m4pipe(fname);
   else if (curfile == &nofile && strcmp(fname, "-") == 0) { /* 1st file only */
      f = stdin;
      fname = "stdin";
      }
   else
      f = fopen(fname, "r");
   if (f == NULL) {
      return 0;
      }
   fs = alloc(sizeof(infile));
   fs->prev = curfile;
   fs->fp = f;
   fs->fname = salloc(fname);
   fs->lno = 0;
   fs->m4flag = m4;
   fs->ifdepth = ifdepth;
   pushline(fs->fname, 0L);
   curfile = fs;
   return 1;
   }

/*
 * m4pipe -- open a pipe from m4.
 */
static FILE *m4pipe(filename)
char *filename;
   {
   FILE *f;
   char *s = alloc(4 + strlen(filename));
   sprintf(s, "m4 %s", filename);
   f = popen(s, "r");
   free(s);
   return f;
   }

/*
 * ppdef(s,v) -- define/undefine a symbol
 *
 *  If v is a null pointer, undefines symbol s.
 *  Otherwise, defines s to have the value v.
 *  No error is given for a redefinition.
 */
void ppdef(s, v)
char *s, *v;
   {
   dlookup(s, -1, (char *)NULL);
   if (v != NULL)
      dlookup(s, -1, v);
   }

/*
 * ppecho() -- run input through preprocessor and echo directly to stdout.
 */
void ppecho()
   {
   int c;

   while ((c = ppch()) != EOF)
      putchar(c);
   }

/*
 * ppch() -- get preprocessed character.
 */
int ppch()
   {
   int c, f;
   char *p;
   buffer *b;
   cdefn *d;
   infile *fs;

   for (;;) {
      if (bnxt < bstop)			/* if characters ready to go */
         return ((int)*bnxt++) & 0xFF;		/* return first one */

      if (bnxt < blim) {
         /*
          * There are characters in the buffer, but they haven't been
          *  checked for substitutions yet.  Process either one id, if
          *  that's what's next, or as much else as we can.
          */
         f = *bnxt;
         if (isalpha(f) || f == '_') {
            /*
             * This is the first character of an identifier.  It could
             *  be the name of a definition.  If so, the name will be
             *  contiguous in this buffer.  Check it.
             */
            p = bnxt + 1;
            while (p < blim && (isalnum(c = *p) || c == '_'))	/* find end */
               p++;
            bstop = p;			/* safe to consume through end */
            if (((d = dlookup(bnxt, p-bnxt, bnxt)) == 0)  || (d->inuse == 1)) {
               bnxt++;
               return f;		/* not defined; just use it */
               }
            /*
             * We got a match.  Remove the token from the input stream and
             *  push the replacement value.
             */
            bnxt = p;
            pushdef(d);			/* make defn the curr buffer */
            continue;			/* loop to preprocess */
            }
         else {
            /*
             * Not an id.  Find the end of non-id stuff and mark it as
             *  having been preprocessed.  This is where we skip over
             *  string and cset literals to avoid processing them.
             */
            p = bnxt++;
            while (p < blim) {
               c = *p;
               if (isalpha(c) || c == '_') {	/* there's an id ahead */
                  bstop = p;
                  return f;
                  }
               else if (isdigit(c)) {		/* numeric constant */
                  p = nskip(p);
                  }
               else if (c == '#') {		/* comment: skip to EOL */
                  bstop = blim;
                  return f;
                  }
               else if (c == '"' || c == '\''){	/* quoted literal */
                  p = matchq(p);		/* skip to end */
                  if (*p != '\0')
                     p++;
                  }
               else
                  p++;				/* else advance one char */
               }
            bstop = blim;			/* mark end of processed chrs */
            return f;				/* return first char */
            }
         }

      /*
       * The buffer is empty.  Revert to a previous buffer.
       */
      if (bstack != NULL) {
         b = bstack;
         b->defn->inuse = 0;
         bnxt = b->ptr;
         bstop = b->stop;
         blim = b->lim;
         bstack = b->prev;
         b->prev = bfree;
         bfree = b;
         continue;				/* loop to preprocess */
         }

      /*
       * There's nothing at all in memory.  Read a new line.
       */
      if ((buf = rline(curfile->fp)) != NULL) {
         /*
          * The read was successful.
          */
         p = bnxt = bstop = blim = buf;		/* reset buffer pointers */
         curfile->lno++;			/* bump line number */
         while (isspace(c = *p))
            p++;				/* find first nonwhite */
         if (c == '$' && (!ispunct(p[1]) || p[1]==' '))
            ppdir(p + 1);			/* handle preprocessor cmd */
         else if (buf[1]=='l' && buf[2]=='i' && buf[3]=='n' && buf[4]=='e' &&
                  buf[0]=='#' && buf[5]==' ')
            ppdir(p + 1);			/* handle #line form */
         else {
            /*
             * Not a preprocessor line; will need to scan for symbols.
             */
            bnxt = buf;
            blim = buf + strlen(buf);
            bstop = bnxt;			/* no chars scanned yet */
            }
         }

      else {
         /*
          * The read hit EOF.
          */
         if (curfile->ifdepth != ifdepth) {
            pfatal("unterminated $if", (char *)0);
            ifdepth = curfile->ifdepth;
            }

         /*
          * switch to previous file and close current file.
          */
         fs = curfile;
         curfile = fs->prev;

         if (fs->m4flag) {			/* if m4 preprocessing */
            void quit();
            if (pclose(fs->fp) != 0)		/* close pipe */
               quit("m4 terminated abnormally");
            }
         else
            fclose(fs->fp);		/* close current file */

         free((char *)fs->fname);
         free((char *)fs);
         if (curfile == &nofile)	/* if at outer level, return EOF */
            return EOF;
         else				/* else generate #line comment */
            pushline(curfile->fname, curfile->lno);
         }
      }
   }

/*
 * rline(fp) -- read arbitrarily long line and return pointer.
 *
 *  Allocates memory as needed.  Returns NULL for EOF.  Lines end with "\n\0".
 */
static char *rline(fp)
FILE *fp;
   {
#define LINE_SIZE_INIT 100
#define LINE_SIZE_INCR 100
   static char *lbuf = NULL;	/* line buffer */
   static int llen = 0;		/* current buffer length */
   register char *p;
   register int c, n;

   /* if first time, allocate buffer */
   if (!lbuf) {
      lbuf = alloc(LINE_SIZE_INIT);
      llen = LINE_SIZE_INIT;
      }

   /* first character is special; return NULL if hit EOF here */
   c = getc(fp);
   if (c == EOF)
      return NULL;
   if (c == '\n')
      return "\n";

   p = lbuf;
   n = llen - 3;
   *p++ = c;

   for (;;)  {
      /* read until buffer full; return after newline or EOF */
      while (--n >= 0 && (c = getc(fp)) != '\n' && c != EOF)
         *p++ = c;
      if (n >= 0) {
         *p++ = '\n';			/* always terminate with \n\0 */
         *p++ = '\0';
         return lbuf;
         }

      /* need to read more, so we need a bigger buffer */
      llen += LINE_SIZE_INCR;
      lbuf = realloc(lbuf, (unsigned int)llen);
      if (!lbuf) {
         fprintf(stderr, "rline(%d): out of memory\n", llen);
         exit(EXIT_FAILURE);
         }
      p = lbuf + llen - LINE_SIZE_INCR - 2;
      n = LINE_SIZE_INCR;
      }
   }

/*
 * pushdef(d) -- insert definition into the input stream.
 */
static void pushdef(d)
cdefn *d;
   {
   buffer *b;

   d->inuse = 1;
   b = bfree;
   if (b == NULL)
      b = (buffer *)alloc(sizeof(buffer));
   else
      bfree = b->prev;
   b->prev = bstack;
   b->defn = d;
   b->ptr = bnxt;
   b->stop = bstop;
   b->lim = blim;
   bstack = b;
   bnxt = bstop = d->s + d->nlen;
   blim = bnxt + d->vlen;
   }

/*
 * pushline(fname,lno) -- push #line directive into input stream.
 */
static void pushline(fname, lno)
char *fname;
long lno;
   {
   static char tbuf[200];

   sprintf(tbuf, "#line %ld \"%s\"\n", lno, fname);
   bnxt = tbuf;
   bstop = blim = tbuf + strlen(tbuf);
   }

/*
 * ppdir(s) -- handle preprocessing directive.
 *
 *  s is the portion of the line following the $.
 */
static void ppdir(s)
char *s;
   {
   char b0, *cmd, *errmsg;
   struct ppcmd *p;

   b0 = buf[0];				/* remember first char of line */
   bnxt = "\n";				/* set buffer pointers to empty line */
   bstop = blim = bnxt + 1;

   s = wskip(s);			/* skip whitespace */
   s = getidt(cmd = s - 1, s);		/* get command name */
   s = wskip(s);			/* skip whitespace */

   for (p = pplist; p->name != NULL; p++) /* find name in table */
      if (strcmp(cmd, p->name) == 0) {
         errmsg = (*p->func)(s);	/* process directive */
         if (errmsg != NULL && (p->func != setline || b0 != '#'))
            pfatal(errmsg, (char *)0);	/* issue err if not from #line form */
      return;
      }

   pfatal("invalid preprocessing directive", cmd);
   }

/*
 * pfatal(s1,s2) -- output a preprocessing error message.
 *
 *  s1 is the error message; s2 is the offending value, if any.
 *  If s2 ends in a newline, the newline is truncated in place.
 *
 *  We can't use tfatal() because we have our own line counter which may be
 *  out of sync with the lexical analyzer's.
 */
static void pfatal(s1, s2)
char *s1, *s2;
   {
   int n;

   fprintf(stderr, "File %s; Line %ld # ", curfile->fname, curfile->lno);
   if (s2 != NULL && *s2 != '\0') {
      n = strlen(s2);
      if (n > 0 && s2[n-1] == '\n')
         s2[n-1] = '\0';			/* remove newline */
      fprintf(stderr, "\"%s\": ", s2);		/* print offending value */
      }
   fprintf(stderr, "%s\n", s1);			/* print diagnostic */
   tfatals++;
   nocode++;
   }

/*
 * errdir(s) -- handle deliberate $error.
 */
static char *errdir(s)
char *s;
   {
   pfatal("explicit $error", s);		/* issue msg with text */
   return NULL;
   }

/*
 * define(s) -- handle $define directive.
 */
static char *define(s)
char *s;
   {
   char c, *name, *val;

   if (isalpha(c = *s) || c == '_')
      s = getidt(name = s - 1, s);		/* get name */
   else
      return "$define: missing name";
   if (*s == '(')
      return "$define: \"(\" after name requires preceding space";
   val = s = wskip(s);
   if (*s != '\0') {
      while ((c = *s) != '\0' && c != '#') {	/* scan value */
         if (c == '"' || c == '\'') {
            s = matchq(s);
            if (*s == '\0')
               return "$define: unterminated literal";
            }
         s++;
         }
      while (isspace(s[-1]))			/* trim trailing whitespace */
         s--;
      }
   *s = '\0';
   dlookup(name, -1, val);		/* install in table */
   return NULL;
   }

/*
 * undef(s) -- handle $undef directive.
 */
static char *undef(s)
char *s;
   {
   char c, *name;

   if (isalpha(c = *s) || c == '_')
      s = getidt(name = s - 1, s);		/* get name */
   else
      return "$undef: missing name";
   if (*wskip(s) != '\0')
      return "$undef: too many arguments";
   dlookup(name, -1, (char *)NULL);
   return NULL;
   }

/*
 * include(s) -- handle $include directive.
 */
static char *include(s)
char *s;
   {
   char *fname;
   char fullpath[MaxPath];

   s = getfnm(fname = s - 1, s);
   if (*fname == '\0')
      return "$include: invalid file name";
   if (*wskip(s) != '\0')
      return "$include: too many arguments";
   if (!pathfind(fullpath, lpath, fname, (char *)NULL) || !ppopen(fullpath, 0))
      pfatal("cannot open", fname);
   return NULL;
   }

/*
 * setline(s) -- handle $line (or #line) directive.
 */
static char *setline(s)
char *s;
   {
   long n;
   char c;
   char *fname;

   if (!isdigit(c = *s))
      return "$line: no line number";
   n = c - '0';

   while (isdigit(c = *++s))		/* extract line number */
      n = 10 * n + c - '0';
   s = wskip(s);			/* skip whitespace */

   if (isalpha (c = *s) || c == '_' || c == '"') {	/* if filename */
      s = getfnm(fname = s - 1, s);			/* extract it */
      if (*fname == '\0')
         return "$line: invalid file name";
      }
   else
      fname = NULL;

   if (*wskip(s) != '\0')
      return "$line: too many arguments";

   curfile->lno = n;			/* set line number */
   if (fname != NULL) {			/* also set filename if given */
      free(curfile->fname);
      curfile->fname = salloc(fname);
      }

   pushline(curfile->fname, curfile->lno);
   return NULL;
   }

/*
 * ifdef(s), ifndef(s) -- conditional processing if s is/isn't defined.
 */
static char *ifdef(s)
char *s;
   {
   return ifxdef(s, 1);
   }

static char *ifndef(s)
char *s;
   {
   return ifxdef(s, 0);
   }

/*
 * ifxdef(s) -- handle $ifdef (if n is 1) or $ifndef (if n is 0).
 */
static char *ifxdef(s, f)
char *s;
int f;
   {
   char c, *name;

   ifdepth++;
   if (isalpha(c = *s) || c == '_')
      s = getidt(name = s - 1, s);		/* get name */
   else
      return "$ifdef/$ifndef: missing name";
   if (*wskip(s) != '\0')
      return "$ifdef/$ifndef: too many arguments";
   if ((dlookup(name, -1, name) != NULL) ^ f)
      skipcode(1, 1);				/* skip to $else or $endif */
   return NULL;
   }

/*
 * elsedir(s) -- handle $else by skipping to $endif.
 */
static char *elsedir(s)
char *s;
   {
   if (ifdepth <= curfile->ifdepth)
      return "unexpected $else";
   if (*s != '\0')
      pfatal ("extraneous arguments on $else/$endif", s);
   skipcode(0, 1);			/* skip the $else section */
   return NULL;
   }

/*
 * endif(s) -- handle $endif.
 */
static char *endif(s)
char *s;
   {
   if (ifdepth <= curfile->ifdepth)
      return "unexpected $endif";
   if (*s != '\0')
      pfatal ("extraneous arguments on $else/$endif", s);
   ifdepth--;
   return NULL;
   }

/*
 * skipcode(doelse,report) -- skip code to $else (doelse=1) or $endif (=0).
 *
 *  If report is nonzero, generate #line directive at end of skip.
 */
static void skipcode(doelse, report)
int doelse, report;
   {
   char c, *p, *cmd;

   while ((p = buf = rline(curfile->fp)) != NULL) {
      curfile->lno++;			/* bump line number */

      /*
       * Handle #line form encountered while skipping.
       */
      if (buf[1]=='l' && buf[2]=='i' && buf[3]=='n' && buf[4]=='e' &&
            buf[0]=='#' && buf[5]==' ') {
         ppdir(buf + 1);			/* interpret #line */
         continue;
         }

      /*
       * Check for any other kind of preprocessing directive.
       */
      while (isspace(c = *p))
         p++;				/* find first nonwhite */
      if (c != '$' || (ispunct(p[1]) && p[1]!=' '))
         continue;			/* not a preprocessing directive */
      p = wskip(p+1);			/* skip whitespace */
      p = getidt(cmd = p-1, p);		/* get command name */
      p = wskip(p);			/* skip whitespace */

      /*
       * Check for a directive that needs special attention.
       *  Deliberately accept any form of $if... as valid
       *  in anticipation of possible future extensions;
       *  this allows them to appear here if commented out.
       */
      if (cmd[0] == 'i' && cmd[1] == 'f') {
         ifdepth++;
         skipcode(0, 0);		/* skip to $endif */
         }
      else if (strcmp(cmd, "line") == 0)
         setline(p);			/* process $line, ignore errors */
      else if (strcmp(cmd, "endif") == 0 ||
               (doelse == 1 && strcmp(cmd, "else") == 0)) {
         /*
          * Time to stop skipping.
          */
         if (*p != '\0')
            pfatal ("extraneous arguments on $else/$endif", p);
         if (cmd[1] == 'n')		/* if $endif */
            ifdepth--;
         if (report)
            pushline(curfile->fname, curfile->lno);
         return;
         }
      }

   /*
    *  At EOF, just return; main loop will report unterminated $if.
    */
   }

/*
 * Token scanning functions.
 */

/*
 * wskip(s) -- skip whitespace and return updated pointer
 *
 *  If '#' is encountered, skips to end of string.
 */
static char *wskip(s)
char *s;
   {
   char c;

   while (isspace(c = *s))
      s++;
   if (c == '#')
      while ((c = *++s) != 0)
         ;
   return s;
   }

/*
 * nskip(s) -- skip over numeric constant and return updated pointer.
 */
static char *nskip(s)
char *s;
   {
      char c;

      while (isdigit(c = *++s))
         ;
      if (c == 'r' || c == 'R') {
         while (isalnum(c = *++s))
            ;
         return s;
         }
      if (c == '.')
         while (isdigit (c = *++s))
            ;
      if (c == 'e' || c == 'E') {
         c = s[1];
         if (c == '+' || c == '-')
            s++;
         while (isdigit (c = *++s))
            ;
         }
      return s;
   }

/*
 * matchq(s) -- scan for matching quote character and return pointer.
 *
 *  Taking *s as the quote character, s is incremented until it points
 *  to either another occurrence of the character or the '\0' terminating
 *  the string.  Escaped quote characters do not stop the scan.  The
 *  updated pointer is returned.
 */
static char *matchq(s)
char *s;
   {
   char c, q;

   q = *s;
   if (q == '\0')
      return s;
   while ((c = *++s) != q && c != '\0') {
      if (c == '\\')
         if (*++s == '\0')
            return s;
      }
   return s;
   }

/*
 * getidt(dst,src) -- extract identifier, return updated pointer
 *
 *  The identifier (in Icon terms, "many(&letters++&digits++'_')")
 *  at src is copied to dst and '\0' is appended.  A pointer to the
 *  character following the identifier is returned.
 *
 *  dst may partially overlap src if dst has a lower address.  This
 *  is typically done to avoid the need for another arbitrarily-long
 *  buffer.  An offset of -1 allows room for insertion of the '\0'.
 */
static char *getidt(dst, src)
char *dst, *src;
   {
   char c;

   while (isalnum(c = *src) || (c == '_')) {
      *dst++ = c;
      src++;
      }
   *dst = '\0';
   return src;
   }

/*
 * getfnm(dst,src) -- extract filename, return updated pointer
 *
 *  Similarly to getidt, getfnm extracts a quoted or unquoted file name.
 *  An empty string at dst indicates a missing or unterminated file name.
 */
static char *getfnm(dst, src)
char *dst, *src;
   {
   char *lim;

   if (*src != '"')
      return getidt(dst, src);
   lim = matchq(src);
   if (*lim != '"') {
      *dst = '\0';
      return lim;
      }
   while (++src < lim)
      if ((*dst++ = *src) == '\\')
         dst[-1] = *++src;
   *dst = '\0';
   return lim + 1;
   }

/*
 * dlookup(name, len, val) look up entry in definition table.
 *
 *  If val == name, return the existing value, or NULL if undefined.
 *  If val == NULL, delete any existing value and undefine the name.
 *  If val != NULL, install a new value, and print error if different.
 *
 *  If name is null, the call is ignored.
 *  If len < 0, strlen(name) is taken.
 */
static cdefn *dlookup(name, len, val)
char *name;
int len;
char *val;
   {
   int h, i, nlen, vlen;
   unsigned int t;
   cdefn *d, **p;

   if (len < 0)
      len = strlen(name);
   if (len == 0)
      return NULL;
   for (t = i = 0; i < len; i++)
      t = 37 * t + (name[i] & 0xFF);	/* calc hash value */
   h = t % HTBINS;			/* calc bin number */
   p = &cbin[h];			/* get head of list */
   while ((d = *p) != NULL) {
      if (d->nlen == len && strncmp(name, d->s, len) == 0) {
         /*
          * We found a match in the table.
          */
         if (val == NULL) {		/* if $undef */
            *p = d->next;		/* delete from table */
            free((char *)d);
            return NULL;
            }
         if (val != name && strcmp(val, d->s + d->nlen) != 0)
            pfatal("value redefined", name);
         return d;			/* return pointer to entry */
         }
      p = &d->next;
      }
   /*
    * No match. Install a definition if that is what is wanted.
    */
   if (val == name || val == NULL)	/* if was reference or $undef */
      return NULL;
   nlen = strlen(name);
   vlen = strlen(val);
   d = (cdefn *)alloc(sizeof(*d) - sizeof(d->s) + nlen + vlen + 1);
   d->nlen = nlen;
   d->vlen = vlen;
   d->inuse = 0;
   strcpy(d->s, name);
   strcpy(d->s + nlen, val);
   d->prev = NULL;
   d->next = cbin[h];
   if (d->next != NULL)
      d->next->prev = d;
   cbin[h] = d;
   return d;
   }
