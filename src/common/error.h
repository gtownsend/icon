/*
 * error.h -- routines for producing error messages.
 *
 * This source file contains the routines for issuing error messages.
 * It is built by inclusion in ../icont/tlex.c and ../iconc/clex.c,
 * with slight variations depending on whether "Iconc" is defined.
 */

/*
 * Prototype.
 */

static	char	*mapterm	(int typ,struct node *val);

#if AMIGA && __SASC
   extern void PostClip(char *file, int line, int number, char *text);
#endif				/* AMIGA && __SASC */

/*
 * yyerror produces syntax error messages.  tok is the offending token
 *  (yychar), lval is yylval, and state is the parser's state.
 *
 * errtab is searched for the state, if it is found, the associated
 *  message is produced; if the state isn't found, "syntax error"
 *  is produced.
 */
void yyerror(tok, lval, state)
int tok, state;
nodeptr lval;
   {
   register struct errmsg *p;
   int line;
#ifdef ConsoleWindow
   extern int silent;
#endif					/* ConsoleWindow */

   if (lval == NULL)
      line = 0;
   else
      line = Line(lval);

#ifdef ConsoleWindow
   if (!silent) {
#endif					/* ConsoleWindow */
   if (tok_loc.n_file)
      fprintf(stderr, "File %s; ", tok_loc.n_file);
   if (tok == EOFX)   /* special case end of file */
      fprintf(stderr, "unexpected end of file\n");
   else {
      fprintf(stderr, "Line %d # ", line);
      if (Col(lval))
         fprintf(stderr, "\"%s\": ", mapterm(tok,lval));
      for (p = errtab; p->e_state != state && p->e_state >= 0; p++) ;
      fprintf(stderr, "%s\n", p->e_mesg);
#if AMIGA && __SASC
      if (tok_loc.n_file) PostClip(tok_loc.n_file, line, 0, p->e_mesg);
#endif				/* AMIGA && __SASC */
      }
#ifdef ConsoleWindow
      }
   else if (flog != NULL) {
   if (tok_loc.n_file)
      fprintf(flog, "File %s; ", tok_loc.n_file);
   if (tok == EOFX)   /* special case end of file */
      fprintf(flog, "unexpected end of file\n");
   else {
      fprintf(flog, "Line %d # ", line);
      if (Col(lval))
         fprintf(flog, "\"%s\": ", mapterm(tok,lval));
      for (p = errtab; p->e_state != state && p->e_state >= 0; p++) ;
      fprintf(flog, "%s\n", p->e_mesg);
      }
      }
#endif					/* ConsoleWindow */
   tfatals++;
   nocode++;
   }

/*
 * mapterm finds a printable string for the given token type
 *  and value.
 */
static char *mapterm(typ,val)
int typ;
nodeptr val;
   {
   register struct toktab *t;
   register struct optab *ot;
   register int i;

   i = typ;
   if (i == IDENT || i == INTLIT || i == REALLIT || i == STRINGLIT ||
      i == CSETLIT)
         return Str0(val);
   for (t = toktab; t->t_type != 0; t++)
      if (t->t_type == i)
         return t->t_word;
   for (ot = optab; ot->tok.t_type != 0; ot++)
      if (ot->tok.t_type == i)
         return ot->tok.t_word;
   return "???";
   }

/*
 * tfatal produces the translator error messages s1 and s2 (if nonnull).  The
 *  location of the error is found in tok_loc.
 */
void tfatal(s1, s2)
char *s1, *s2;
   {

   if (tok_loc.n_file)
      fprintf(stderr, "File %s; ", tok_loc.n_file);
   fprintf(stderr, "Line %d # ", tok_loc.n_line);
   if (s2)
      fprintf(stderr, "\"%s\": ", s2);
   fprintf(stderr, "%s\n", s1);

#if AMIGA && __SASC
   if (tok_loc.n_file) {
      char text[512];
      if (s2)
         sprintf(text, "\"%s\": ", s2);
      strcat(text, s1);
      PostClip(tok_loc.n_file, tok_loc.n_line, 0, text);
      }
#endif				/* AMIGA && __SASC */

   tfatals++;
   nocode++;
   }

/*
 * nfatal produces the error messages s1 and s2 (if nonnull), and associates
 *  it with source location of node.
 */
void nfatal(n, s1, s2)
nodeptr n;
char *s1, *s2;
   {

   if (n != NULL) {
      fprintf(stderr, "File %s; ", File(n));
      fprintf(stderr, "Line %d # ", Line(n));
      }
   if (s2)
      fprintf(stderr, "\"%s\": ", s2);
   fprintf(stderr, "%s\n", s1);

#if AMIGA && __SASC
   if (n != NULL) {
      char text[512];
      if (s2)
         sprintf(text, "\"%s\": ", s2);
      strcat(text, s1);
      PostClip(File(n), Line(n), 0, text);
      }
#endif				/* AMIGA && __SASC */

   tfatals++;
   nocode++;
   }

#ifdef Iconc
/*
 * twarn produces s1 and s2 (if nonnull) as translator warning messages.
 *  The location of the error is found in tok_loc.
 */
void twarn(s1, s2)
char *s1, *s2;
   {

   if (tok_loc.n_file)
      fprintf(stderr, "File %s; ", tok_loc.n_file);
   fprintf(stderr, "Line %d # ", tok_loc.n_line);
   if (s2)
      fprintf(stderr, "\"%s\": ", s2);
   fprintf(stderr, "%s\n", s1);
   twarns++;
   }
#endif					/* Iconc */

/*
 * tsyserr is called for fatal errors.  The message s is produced and the
 *  translator exits.
 */
void tsyserr(s)
char *s;
   {


   if (tok_loc.n_file)
      fprintf(stderr, "File %s; ", tok_loc.n_file);
   fprintf(stderr, "Line %d # %s\n", in_line, s);

   exit(EXIT_FAILURE);
   }

/*
 * quit - immediate exit with error message
 */

void quit(msg)
char *msg;
   {
   quitf(msg,"");
   }

/*
 * quitf - immediate exit with message format and argument
 */
void quitf(msg,arg)
char *msg, *arg;
   {


   extern char *progname;
   fprintf(stderr,"%s: ",progname);
   fprintf(stderr,msg,arg);
   fprintf(stderr,"\n");

#if !defined(VarTran) && !defined(Iconc)
   {
      extern char *ofile;
      if (ofile)
	 remove(ofile);			/* remove bad icode file */
   }
#endif					/* !VarTran && !Iconc */

   exit(EXIT_FAILURE);
   }
