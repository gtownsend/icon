/*
 * yylex.h -- the lexical analyzer.
 *
 * This source file contains the lexical analyzer, yylex(), and its
 * support routines. It is built by inclusion in ../icont/tlex.c and
 * ../iconc/clex.c, with slight variations depending on whether "Iconc"
 * is defined.
 */

#if !defined(Iconc)
   #include "../h/esctab.h"
#endif					/* !Iconc */

/*
 * Prototypes.
 */

static  int             bufcmp          (char *s);
static	struct toktab   *findres	(void);
static	struct toktab   *getident	(int ac,int *cc);
static	struct toktab   *getnum		(int ac,int *cc);
static	struct toktab   *getstring	(int ac,int *cc);
static	int		setfilenm	(int c);
static	int		setlineno	(void);

#if !defined(Iconc)
   static	int	ctlesc		(void);
   static	int	hexesc		(void);
   static	int	octesc		(int ac);
#endif					/* !Iconc */

#define isletter(s)	(isupper(c) | islower(c))
#define tonum(c)        (isdigit(c) ? (c - '0') : ((c & 037) + 9))

struct node tok_loc =
   {0, NULL, 0, 0};	/* "model" node containing location of current token */

struct str_buf lex_sbuf;	/* string buffer for lexical analyzer */

/*
 * yylex - find the next token in the input stream, and return its token
 *  type and value to the parser.
 *
 * Variables of interest:
 *
 *  cc - character following last token.
 *  nlflag - set if a newline was between the last token and the current token
 *  lastend - set if the last token was an Ender.
 *  lastval - when a semicolon is inserted and returned, lastval gets the
 *   token value that would have been returned if the semicolon hadn't
 *   been inserted.
 */

static struct toktab *lasttok = NULL;
static int lastend = 0;
static int eofflag = 0;
static int cc = '\n';

int yylex()
   {
   register struct toktab *t;
   register int c;
   int n;
   int nlflag;
   static nodeptr lastval;
   static struct node semi_loc;

   if (lasttok != NULL) {
      /*
       * A semicolon was inserted and returned on the last call to yylex,
       *  instead of going to the input, return lasttok and set the
       *  appropriate variables.
       */

      yylval = lastval;
      tok_loc = *lastval;
      t = lasttok;
      goto ret;
      }
   nlflag = 0;
loop:
   c = cc;
   /*
    * Remember where a semicolon will go if we insert one.
    */
   semi_loc.n_file = tok_loc.n_file;
   semi_loc.n_line = in_line;
   if (cc == '\n')
      --semi_loc.n_line;
   semi_loc.n_col = incol;
   /*
    * Skip whitespace and comments and process #line directives.
    */
   while (c == Comment || isspace(c)) {
      if (c == '\n') {
         nlflag++;
         c = NextChar;
	 if (c == Comment) {
            /*
	     * Check for #line directive at start of line.
             */
            if (('l' == (c = NextChar)) &&
                ('i' == (c = NextChar)) &&
                ('n' == (c = NextChar)) &&
                ('e' == (c = NextChar))) {
               c = setlineno();
	       while ((c == ' ') || (c == '\t'))
		  c = NextChar;
               if (c != EOF && c != '\n')
                  c = setfilenm(c);
	       }
	    while (c != EOF && c != '\n')
               c = NextChar;
	    }
         }
      else {
	 if (c == Comment) {
	    while (c != EOF && c != '\n')
               c = NextChar;
	    }
         else {
            c = NextChar;
            }
         }
      }
   /*
    * A token is the next thing in the input.  Set token location to
    *  the current line and column.
    */
   tok_loc.n_line = in_line;
   tok_loc.n_col = incol;

   if (c == EOF) {
      /*
       * End of file has been reached.	Set eofflag, return T_Eof, and
       *  set cc to EOF so that any subsequent scans also return T_Eof.
       */
      if (eofflag++) {
	 eofflag = 0;
	 cc = '\n';
	 yylval = NULL;
	 return 0;
	 }
      cc = EOF;
      t = T_Eof;
      yylval = NULL;
      goto ret;
      }

   /*
    * Look at current input character to determine what class of token
    *  is next and take the appropriate action.  Note that the various
    *  token gathering routines write a value into cc.
    */
   if (isalpha(c) || (c == '_')) {   /* gather ident or reserved word */
      if ((t = getident(c, &cc)) == NULL)
	 goto loop;
      }
   else if (isdigit(c) || (c == '.')) {	/* gather numeric literal or "." */
      if ((t = getnum(c, &cc)) == NULL)
	 goto loop;
      }
   else if (c == '"' || c == '\'') {    /* gather string or cset literal */
      if ((t = getstring(c, &cc)) == NULL)
	 goto loop;
      }
   else {			/* gather longest legal operator */
      if ((n = getopr(c, &cc)) == -1)
	 goto loop;
      t = &(optab[n].tok);
      yylval = OpNode(n);
      }
   if (nlflag && lastend && (t->t_flags & Beginner)) {
      /*
       * A newline was encountered between the current token and the last,
       *  the last token was an Ender, and the current token is a Beginner.
       *  Return a semicolon and save the current token in lastval.
       */
      lastval = yylval;
      lasttok = t;
      tok_loc = semi_loc;
      yylval = OpNode(semicol_loc);
      return SEMICOL;
      }
ret:
   /*
    * Clear lasttok, set lastend if the token being returned is an
    *  Ender, and return the token.
    */
   lasttok = 0;
   lastend = t->t_flags & Ender;
   return (t->t_type);
   }

/*
 * getident - gather an identifier beginning with ac.  The character
 *  following identifier goes in cc.
 */

static struct toktab *getident(ac, cc)
int ac;
int *cc;
   {
   register int c;
   register struct toktab *t;

   c = ac;
   /*
    * Copy characters into string space until a non-alphanumeric character
    *  is found.
    */
   do {
      AppChar(lex_sbuf, c);
      c = NextChar;
      } while (isalnum(c) || (c == '_'));
   *cc = c;
   /*
    * If the identifier is a reserved word, make a ResNode for it and return
    *  the token value.  Otherwise, install it with putid, make an
    *  IdNode for it, and return.
    */
   if ((t = findres()) != NULL) {
      lex_sbuf.endimage = lex_sbuf.strtimage;
      yylval = ResNode(t->t_type);
      return t;
      }
   else {
      yylval = IdNode(str_install(&lex_sbuf));
      return (struct toktab *)T_Ident;
      }
   }

/*
 * findres - if the string just copied into the string space by getident
 *  is a reserved word, return a pointer to its entry in the token table.
 *  Return NULL if the string isn't a reserved word.
 */

static struct toktab *findres()
   {
   register struct toktab *t;
   register char c;

   c = *lex_sbuf.strtimage;
   if (!islower(c))
      return NULL;
   /*
    * Point t at first reserved word that starts with c (if any).
    */
   if ((t = restab[c - 'a']) == NULL)
      return NULL;
   /*
    * Search through reserved words, stopping when a match is found
    *  or when the current reserved word doesn't start with c.
    */
   while (t->t_word[0] == c) {
      if (bufcmp(t->t_word))
	 return t;
      t++;
      }
   return NULL;
   }

/*
 * bufcmp - compare a null terminated string to what is in the string buffer.
 */
static int bufcmp(s)
char *s;
   {
   register char *s1;
   s1 = lex_sbuf.strtimage;
   while (s != '\0' && s1 < lex_sbuf.endimage && *s == *s1) {
      ++s;
      ++s1;
      }
   if (*s == '\0' && s1 == lex_sbuf.endimage)
      return 1;
   else
      return 0;
   }

/*
 * getnum - gather a numeric literal starting with ac and put the
 *  character following the literal into *cc.
 *
 * getnum also handles the "." operator, which is distinguished from
 *  a numeric literal by what follows it.
 */

static struct toktab *getnum(ac, cc)
int ac;
int *cc;
   {
   register int c, r, state;
   int realflag, n, dummy;

   c = ac;
   if (c == '.') {
      r = 0;
      state = 7;
      realflag = 1;
      }
   else {
      r = tonum(c);
      state = 0;
      realflag = 0;
      }
   for (;;) {
      AppChar(lex_sbuf, c);
      c = NextChar;
      switch (state) {
	 case 0:		/* integer part */
	    if (isdigit(c))	    { r = r * 10 + tonum(c); continue; }
	    if (c == '.')           { state = 1; realflag++; continue; }
	    if (c == 'e' || c == 'E')  { state = 2; realflag++; continue; }
	    if (c == 'r' || c == 'R')  {
	       state = 5;
	       if (r < 2 || r > 36)
		  tfatal("invalid radix for integer literal", (char *)NULL);
	       continue;
	       }
	    break;
	 case 1:		/* fractional part */
	    if (isdigit(c))   continue;
	    if (c == 'e' || c == 'E')   { state = 2; continue; }
	    break;
	 case 2:		/* optional exponent sign */
	    if (c == '+' || c == '-') { state = 3; continue; }
	 case 3:		/* first digit after e, e+, or e- */
	    if (isdigit(c)) { state = 4; continue; }
	    tfatal("invalid real literal", (char *)NULL);
	    break;
	 case 4:		/* remaining digits after e */
	    if (isdigit(c))   continue;
	    break;
	 case 5:		/* first digit after r */
	    if ((isdigit(c) || isletter(c)) && tonum(c) < r)
	       { state = 6; continue; }
	    tfatal("invalid integer literal", (char *)NULL);
	    break;
	 case 6:		/* remaining digits after r */
	    if (isdigit(c) || isletter(c)) {
	       if (tonum(c) >= r) {	/* illegal digit for radix r */
		  tfatal("invalid digit in integer literal", (char *)NULL);
		  r = tonum('z');       /* prevent more messages */
		  }
	       continue;
	       }
	    break;
         case 7:		/* token began with "." */
            if (isdigit(c)) {
               state = 1;		/* followed by digit is a real const */
	       realflag = 1;
               continue;
               }
            *cc = c;			/* anything else is just a dot */
	    lex_sbuf.endimage--;	/* remove dot (undo AppChar) */
	    n = getopr((int)'.', &dummy);
	    yylval = OpNode(n);
	    return &(optab[n].tok);
         }
      break;
      }
   *cc = c;
   if (realflag) {
      yylval = RealNode(str_install(&lex_sbuf));
      return T_Real;
      }
   yylval = IntNode(str_install(&lex_sbuf));
   return T_Int;
   }

/*
 * getstring - gather a string literal starting with ac and place the
 *  character following the literal in *cc.
 */
static struct toktab *getstring(ac, cc)
int ac;
int *cc;
   {
   register int c, sc;
   int sav_indx;
   int len;

   sc = ac;
   sav_indx = -1;
   c = NextChar;
   while (c != sc && c != '\n' && c != EOF) {
      /*
       * If a '_' is the last non-white space before a new-line,
       *  we must remember where it is.
       */
      if (c == '_')
         sav_indx = lex_sbuf.endimage - lex_sbuf.strtimage;
      else if (!isspace(c))
         sav_indx = -1;

      if (c == Escape) {
         c = NextChar;
         if (c == EOF)
            break;

#if defined(Iconc)
         AppChar(lex_sbuf, Escape);
         if (c == '^') {
            c = NextChar;
            if (c == EOF)
               break;
            AppChar(lex_sbuf, '^');
            }
#else					/* Iconc */
	 if (isoctal(c))
	    c = octesc(c);
	 else if (c == 'x')
	    c = hexesc();
	 else if (c == '^')
	    c = ctlesc();
	 else
	    c = esctab[c];
#endif					/* Iconc */

	 }
      AppChar(lex_sbuf, c);
      c = NextChar;

      /*
       * If a '_' is the last non-white space before a new-line, the
       *  string continues at the first non-white space on the next line
       *  and everything from the '_' to the end of this line is ignored.
       */
      if (c == '\n' && sav_indx >= 0) {
         lex_sbuf.endimage = lex_sbuf.strtimage + sav_indx;
         while ((c = NextChar) != EOF && isspace(c))
            ;
         }
      }
   if (c == sc)
      *cc = ' ';
   else {
      tfatal("unclosed quote", (char *)NULL);
      *cc = c;
      }
   len = lex_sbuf.endimage - lex_sbuf.strtimage;
   if (ac == '"') {     /* a string literal */
      yylval = StrNode(str_install(&lex_sbuf), len);
      return T_String;
      }
   else {		/* a cset literal */
      yylval = CsetNode(str_install(&lex_sbuf), len);
      return T_Cset;
      }
   }

#if !defined(Iconc)

/*
 * ctlesc - translate a control escape -- backslash followed by
 *  caret and one character.
 */

static int ctlesc()
   {
   register int c;

   c = NextChar;
   if (c == EOF)
      return EOF;

   return (c & 037);
   }

/*
 * octesc - translate an octal escape -- backslash followed by
 *  one, two, or three octal digits.
 */

static int octesc(ac)
int ac;
   {
   register int c, nc, i;

   c = 0;
   nc = ac;
   i = 1;
   do {
      c = (c << 3) | (nc - '0');
      nc = NextChar;
      if (nc == EOF)
	 return EOF;
      } while (isoctal(nc) && i++ < 3);
   PushChar(nc);

   return (c & 0377);
   }

/*
 * hexesc - translate a hexadecimal escape -- backslash-x
 *  followed by one or two hexadecimal digits.
 */

static int hexesc()
   {
   register int c, nc, i;

   c = 0;
   i = 0;
   while (i++ < 2) {
      nc = NextChar;
      if (nc == EOF)
	 return EOF;
      if (nc >= 'a' && nc <= 'f')
	 nc -= 'a' - 10;
      else if (nc >= 'A' && nc <= 'F')
	 nc -= 'A' - 10;
      else if (isdigit(nc))
	 nc -= '0';
      else {
	 PushChar(nc);
	 break;
	 }
      c = (c << 4) | nc;
      }

   return c;
   }

#endif					/* !Iconc */

/*
 * setlineno - set line number from #line comment, return following char.
 */

static int setlineno()
   {
   register int c;

   while ((c = NextChar) == ' ' || c == '\t')
      ;
   if (c < '0' || c > '9') {
      tfatal("no line number in #line directive", "");
      while (c != EOF && c != '\n')
	 c = NextChar;
      return c;
      }
   in_line = 0;
   while (c >= '0' && c <= '9') {
      in_line = in_line * 10 + (c - '0');
      c = NextChar;
      }
   return c;
   }

/*
 * setfilenm -	set file name from #line comment, return following char.
 */

static int setfilenm(c)
register int c;
   {
   while (c == ' ' || c == '\t')
      c = NextChar;
   if (c != '"') {
      tfatal("'\"' missing from file name in #line directive", "");
      while (c != EOF && c != '\n')
	 c = NextChar;
      return c;
      }
   while ((c = NextChar) != '"' && c != EOF && c != '\n')
      AppChar(lex_sbuf, c);
   if (c == '"') {
      tok_loc.n_file = str_install(&lex_sbuf);
      return NextChar;
      }
   else {
      tfatal("'\"' missing from file name in #line directive", "");
      return c;
      }
   }

/*
 * nextchar - return the next character in the input.
 *
 *  Called from the lexical analyzer; interfaces it to the preprocessor.
 */

int nextchar()
   {
   register int c;

   if ((c = peekc) != 0) {
      peekc = 0;
      return c;
      }
   c = ppch();
   switch (c) {
      case EOF:
	 if (incol) {
	    c = '\n';
	    in_line++;
	    incol = 0;
	    peekc = EOF;
	    break;
	    }
	 else {
	    in_line = 0;
	    incol = 0;
	    break;
	    }
      case '\n':
	 in_line++;
	 incol = 0;
	 break;
      case '\t':
	 incol = (incol | 7) + 1;
	 break;
      case '\b':
	 if (incol)
	    incol--;
	 break;
      default:
	 incol++;
      }
   return c;
   }
