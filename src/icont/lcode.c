/*
 * lcode.c -- linker routines to parse .u1 files and produce icode.
 */

#include "link.h"
#include "tproto.h"
#include "tglobals.h"
#include "opcode.h"
#include "keyword.h"
#include "../h/version.h"
#include "../h/header.h"

/*
 *  This needs fixing ...
 */
#undef CsetPtr
#define CsetPtr(b,c)	((c) + (((b)&0377) >> LogIntBits))

/*
 * Prototypes.
 */

static void	align		(void);
static void	backpatch	(int lab);
static void	clearlab	(void);
static void	flushcode	(void);
static void	intout		(int oint);
static void	lemit		(int op,char *name);
static void	lemitcon	(int k);
static void	lemitin		(int op,word offset,int n,char *name);
static void	lemitint	(int op,long i,char *name);
static void	lemitl		(int op,int lab,char *name);
static void	lemitn		(int op,word n,char *name);
static void	lemitproc    (word name,int nargs,int ndyn,int nstat,int fstat);
static void	lemitr		(int op,word loc,char *name);
static void	misalign	(void);
static void	outblock	(char *addr,int count);
static void	setfile		(void);
static void	wordout		(word oword);

#ifdef FieldTableCompression
   static void	charout		(unsigned char oint);
   static void	shortout	(short oint);
#endif					/* FieldTableCompression */

#ifdef DeBugLinker
   static void	dumpblock	(char *addr,int count);
#endif					/* DeBugLinker */

word pc = 0;		/* simulated program counter */

#define outword(n)	wordout((word)(n))
#define outop(n)	intout((int)(n))
#define outchar(n)	charout((unsigned char)(n))
#define outshort(n)	shortout((short)(n))
#define CodeCheck(n) if ((long)codep + (n) > (long)((long)codeb + maxcode))\
                     codeb = (char *) trealloc(codeb, &codep, &maxcode, 1,\
                       (n), "code buffer");

#define ByteBits 8

/*
 * gencode - read .u1 file, resolve variable references, and generate icode.
 *  Basic process is to read each line in the file and take some action
 *  as dictated by the opcode.	This action sometimes involves parsing
 *  of arguments and usually culminates in the call of the appropriate
 *  lemit* routine.
 */
void gencode()
   {
   register int op, k, lab;
   int j, nargs, flags, implicit;
   char *name;
   word id, procname;
   struct centry *cp;
   struct gentry *gp;
   struct fentry *fp;
   union xval gg;

   while ((op = getopc(&name)) != EOF) {
      switch (op) {

         /* Ternary operators. */

         case Op_Toby:
         case Op_Sect:

         /* Binary operators. */

         case Op_Asgn:
         case Op_Cat:
         case Op_Diff:
         case Op_Div:
         case Op_Eqv:
         case Op_Inter:
         case Op_Lconcat:
         case Op_Lexeq:
         case Op_Lexge:
         case Op_Lexgt:
         case Op_Lexle:
         case Op_Lexlt:
         case Op_Lexne:
         case Op_Minus:
         case Op_Mod:
         case Op_Mult:
         case Op_Neqv:
         case Op_Numeq:
         case Op_Numge:
         case Op_Numgt:
         case Op_Numle:
         case Op_Numlt:
         case Op_Numne:
         case Op_Plus:
         case Op_Power:
         case Op_Rasgn:
         case Op_Rswap:
         case Op_Subsc:
         case Op_Swap:
         case Op_Unions:

         /* Unary operators. */

         case Op_Bang:
         case Op_Compl:
         case Op_Neg:
         case Op_Nonnull:
         case Op_Null:
         case Op_Number:
         case Op_Random:
         case Op_Refresh:
         case Op_Size:
         case Op_Tabmat:
         case Op_Value:

         /* Instructions. */

         case Op_Bscan:
         case Op_Ccase:
         case Op_Coact:
         case Op_Cofail:
         case Op_Coret:
         case Op_Dup:
         case Op_Efail:
         case Op_Eret:
         case Op_Escan:
         case Op_Esusp:
         case Op_Limit:
         case Op_Lsusp:
         case Op_Pfail:
         case Op_Pnull:
         case Op_Pop:
         case Op_Pret:
         case Op_Psusp:
         case Op_Push1:
         case Op_Pushn1:
         case Op_Sdup:
            newline();
            lemit(op, name);
            break;

         case Op_Chfail:
         case Op_Create:
         case Op_Goto:
         case Op_Init:
            lab = getlab();
            newline();
            lemitl(op, lab, name);
            break;

         case Op_Cset:
         case Op_Real:
            k = getdec();
            newline();
            lemitr(op, lctable[k].c_pc, name);
            break;

         case Op_Field:
            id = getid();
            newline();
            fp = flocate(id);
            if (fp != NULL)
               lemitn(op, (word)(fp->f_fid-1), name);
	    else
               lemitn(op, (word)-1, name);	/* no warning any more */
            break;


         case Op_Int: {
            long i;
            k = getdec();
            newline();
            cp = &lctable[k];
            /*
             * Check to see if a large integers has been converted to a string.
             *  If so, generate the code for +s.
             */
            if (cp->c_flag & F_StrLit) {
               lemit(Op_Pnull,"pnull");
               lemitin(Op_Str, cp->c_val.sval, cp->c_length, "str");
               lemit(Op_Number,"number");
               break;
               }
            i = (long)cp->c_val.ival;
            lemitint(op, i, name);
            break;
            }


         case Op_Invoke:
            k = getdec();
            newline();
            if (k == -1)
               lemit(Op_Apply,"apply");
            else
               lemitn(op, (word)k, name);
            break;

         case Op_Keywd:
            id = getstr();
            newline();
            k = klookup(&lsspace[id]);
            switch (k) {
               case 0:
                  lfatal(&lsspace[id],"invalid keyword");
                  break;
               case K_FAIL:
                  lemit(Op_Efail,"efail");
                  break;
               case K_NULL:
                  lemit(Op_Pnull,"pnull");
                  break;
               default:
               lemitn(op, (word)k, name);
            }
            break;

         case Op_Llist:
            k = getdec();
            newline();
            lemitn(op, (word)k, name);
            break;

         case Op_Lab:
            lab = getlab();
            newline();
            #ifdef DeBugLinker
               if (Dflag)
                  fprintf(dbgfile, "L%d:\n", lab);
            #endif			/* DeBugLinker */
            backpatch(lab);
            break;

         case Op_Line:
            /*
             * Line number change.
             *  All the interesting stuff happens in Op_Colm now.
             */
            lineno = getdec();

            #ifndef SrcColumnInfo
               /*
                * Enter the value in the line number table
                *  that is stored in the icode file and used during error
                *  handling and execution monitoring.  One can generate a VM
                *  instruction for these changes, but since the numbers are not
                *  saved and restored during backtracking, it is more accurate
                *  to check for line number changes in-line in the interpreter.
                *  Fortunately, the in-line check is about as fast as executing
                *  Op_Line instructions.  All of this is complicated by the use
                *  of Op_Line to generate Noop instructions when enabled by the
                *  LineCodes #define.
                *
                * If SrcColumnInfo is required, this code is duplicated,
                *  with changes, in the Op_Colm case below.
                */
               if (lnfree >= &lntable[nsize])
                  lntable  = (struct ipc_line *)trealloc(lntable,&lnfree,&nsize,
                     sizeof(struct ipc_line), 1, "line number table");
               lnfree->ipc = pc;
               lnfree->line = lineno;
               lnfree++;
            #endif			/* SrcColumnInfo */

            /*
             * Could generate an Op_Line for monitoring, but don't anymore:
             *
             * lemitn(op, (word)lineno, name);
             */

            newline();

            #ifdef LineCodes
               #ifndef EventMon
                  lemit(Op_Noop,"noop");
               #endif			/* EventMon */
            #endif			/* LineCodes */

            break;

         case Op_Colm:			/* always recognize, maybe ignore */

            colmno = getdec();
            #ifdef SrcColumnInfo
               if (lnfree >= &lntable[nsize])
                  lntable  = (struct ipc_line *)trealloc(lntable,&lnfree,&nsize,
                     sizeof(struct ipc_line), 1, "line number table");
               lnfree->ipc = pc;
               lnfree->line = lineno + (colmno << 16);
               lnfree++;
            #endif			/* SrcColumnInfo */
            break;

         case Op_Mark:
            lab = getlab();
            newline();
            lemitl(op, lab, name);
            break;

         case Op_Mark0:
            lemit(op, name);
            break;

         case Op_Str:
            k = getdec();
            newline();
            cp = &lctable[k];
            lemitin(op, cp->c_val.sval, cp->c_length, name);
            break;

         case Op_Tally:
            k = getdec();
            newline();
            lemitn(op, (word)k, name);
            break;

         case Op_Unmark:
            lemit(Op_Unmark, name);
            break;

         case Op_Var:
            k = getdec();
            newline();
            flags = lltable[k].l_flag;
            if (flags & F_Global)
               lemitn(Op_Global, (word)(lltable[k].l_val.global->g_index),
                  "global");
            else if (flags & F_Static)
               lemitn(Op_Static, (word)(lltable[k].l_val.staticid-1), "static");
            else if (flags & F_Argument)
               lemitn(Op_Arg, (word)(lltable[k].l_val.offset-1), "arg");
            else
               lemitn(Op_Local, (word)(lltable[k].l_val.offset-1), "local");
            break;

         /* Declarations. */

         case Op_Proc:
            getstr();
            newline();
            procname = putident(strlen(&lsspace[lsfree]) + 1, 0);
            if (procname >= 0 && (gp = glocate(procname)) != NULL) {
               /*
                * Initialize for wanted procedure.
                */
               locinit();
               clearlab();
               lineno = 0;
               implicit = gp->g_flag & F_ImpError;
               nargs = gp->g_nargs;
	       align();
               #ifdef DeBugLinker
                  if (Dflag)
                     fprintf(dbgfile, "\n# procedure %s\n", &lsspace[lsfree]);
               #endif			/* DeBugLinker */
               }
            else {
               /*
                * Skip unreferenced procedure.
                */
               while ((op = getopc(&name)) != EOF && op != Op_End)
                  if (op == Op_Filen)
                     setfile();		/* handle filename op while skipping */
                  else
                     newline();		/* ignore everything else */
               }
            break;

         case Op_Local:
            k = getdec();
            flags = getoct();
            id = getid();
            putlocal(k, id, flags, implicit, procname);
            break;

         case Op_Con:
            k = getdec();
            flags = getoct();
            if (flags & F_IntLit) {
               {
               long m;
               word s_indx;

               j = getdec();		/* number of characters in integer */
               m = getint(j,&s_indx);	/* convert if possible */
               if (m < 0) {		/* negative indicates integer too big */
                  gg.sval = s_indx;	/* convert to a string */
                  putconst(k, F_StrLit, j, pc, &gg);
                  }
               else {			/* integers is small enough */
                  gg.ival = m;
                  putconst(k, flags, 0, pc, &gg);
                  }
               }
               }
            else if (flags & F_RealLit) {
               gg.rval = getreal();
               putconst(k, flags, 0, pc, &gg);
               }
            else if (flags & F_StrLit) {
               j = getdec();
               gg.sval = getstrlit(j);
               putconst(k, flags, j, pc, &gg);
               }
            else if (flags & F_CsetLit) {
               j = getdec();
               gg.sval = getstrlit(j);
               putconst(k, flags, j, pc, &gg);
               }
            else
               fprintf(stderr, "gencode: illegal constant\n");
            newline();
            lemitcon(k);
            break;

         case Op_Filen:
            setfile();
            break;

         case Op_Declend:
            newline();
            gp->g_pc = pc;
            lemitproc(procname, nargs, dynoff, lstatics-static1, static1);
            break;

         case Op_End:
            newline();
            flushcode();
            break;

         default:
            fprintf(stderr, "gencode: illegal opcode(%d): %s\n", op, name);
            newline();
         }
      }
   }

/*
 * setfile - handle Op_Filen.
 */
static void setfile()
   {
   if (fnmfree >= &fnmtbl[fnmsize])
      fnmtbl = (struct ipc_fname *) trealloc(fnmtbl, &fnmfree,
         &fnmsize, sizeof(struct ipc_fname), 1, "file name table");
   fnmfree->ipc = pc;
   fnmfree->fname = getrest();
   strcpy(icnname, &lsspace[fnmfree->fname]);
   fnmfree++;
   newline();
   }

/*
 *  lemit - emit opcode.
 *  lemitl - emit opcode with reference to program label.
 *	for a description of the chaining and backpatching for labels.
 *  lemitn - emit opcode with integer argument.
 *  lemitr - emit opcode with pc-relative reference.
 *  lemitin - emit opcode with reference to identifier table & integer argument.
 *  lemitint - emit word opcode with integer argument.
 *  lemitcon - emit constant table entry.
 *  lemitproc - emit procedure block.
 *
 * The lemit* routines call out* routines to effect the "outputting" of icode.
 *  Note that the majority of the code for the lemit* routines is for debugging
 *  purposes.
 */
static void lemit(op, name)
int op;
char *name;
   {

   #ifdef DeBugLinker
      if (Dflag)
         fprintf(dbgfile, "%ld:\t%d\t\t\t\t# %s\n", (long)pc, op, name);
   #endif				/* DeBugLinker */

   outop(op);
   }

static void lemitl(op, lab, name)
int op, lab;
char *name;
   {
   misalign();

   #ifdef DeBugLinker
      if (Dflag)
         fprintf(dbgfile, "%ld:\t%d\tL%d\t\t\t# %s\n", (long)pc, op, lab, name);
   #endif				/* DeBugLinker */

   if (lab >= maxlabels)
      labels  = (word *) trealloc(labels, NULL, &maxlabels, sizeof(word),
         lab - maxlabels + 1, "labels");
   outop(op);
   if (labels[lab] <= 0) {		/* forward reference */
      outword(labels[lab]);
      labels[lab] = WordSize - pc;	/* add to front of reference chain */
      }
   else					/* output relative offset */
      outword(labels[lab] - (pc + WordSize));
   }

static void lemitn(op, n, name)
int op;
word n;
char *name;
   {
   misalign();

   #ifdef DeBugLinker
      if (Dflag)
         fprintf(dbgfile, "%ld:\t%d\t%ld\t\t\t# %s\n", (long)pc, op, (long)n,
            name);
   #endif				/* DeBugLinker */

   outop(op);
   outword(n);
   }


static void lemitr(op, loc, name)
int op;
word loc;
char *name;
   {
   misalign();

   loc -= pc + ((IntBits/ByteBits) + WordSize);

   #ifdef DeBugLinker
      if (Dflag) {
         if (loc >= 0)
            fprintf(dbgfile, "%ld:\t%d\t*+%ld\t\t\t# %s\n",(long) pc, op,
               (long)loc, name);
         else
            fprintf(dbgfile, "%ld:\t%d\t*-%ld\t\t\t# %s\n",(long) pc, op,
               (long)-loc, name);
         }
   #endif				/* DeBugLinker */

   outop(op);
   outword(loc);
   }

static void lemitin(op, offset, n, name)
int op, n;
word offset;
char *name;
   {
   misalign();

   #ifdef DeBugLinker
      if (Dflag)
         fprintf(dbgfile, "%ld:\t%d\t%d,S+%ld\t\t\t# %s\n", (long)pc, op, n,
            (long)offset, name);
   #endif				/* DeBugLinker */

   outop(op);
   outword(n);
   outword(offset);
   }

/*
 * lemitint can have some pitfalls.  outword is used to output the
 *  integer and this is picked up in the interpreter as the second
 *  word of a short integer.  The integer value output must be
 *  the same size as what the interpreter expects.  See op_int and op_intx
 *  in interp.s
 */
static void lemitint(op, i, name)
int op;
long i;
char *name;
   {
   misalign();

   #ifdef DeBugLinker
      if (Dflag)
         fprintf(dbgfile,"%ld:\t%d\t%ld\t\t\t# %s\n",(long)pc,op,(long)i,name);
   #endif				/* DeBugLinker */

   outop(op);
   outword(i);
   }

static void lemitcon(k)
register int k;
   {
   register int i, j;
   register char *s;
   int csbuf[CsetSize];
   union {
      char ovly[1];  /* Array used to overlay l and f on a bytewise basis. */
      long l;
      double f;
      } x;

   if (lctable[k].c_flag & F_RealLit) {

      #ifdef Double
         /* access real values one word at a time */
         int *rp, *rq;
         rp = (int *) &(x.f);
         rq = (int *) &(lctable[k].c_val.rval);
         *rp++ = *rq++;
         *rp = *rq;
      #else				/* Double */
         x.f = lctable[k].c_val.rval;
      #endif				/* Double */

      #ifdef DeBugLinker
         if (Dflag) {
            fprintf(dbgfile,"%ld:\t%d\t\t\t\t# real(%g)",(long)pc,T_Real, x.f);
            dumpblock(x.ovly,sizeof(double));
            }
      #endif				/* DeBugLinker */

      outword(T_Real);

      #ifdef Double
         #if WordBits != 64
            /* fill out real block with an empty word */
            outword(0);
            #ifdef DeBugLinker
               if (Dflag)
	          fprintf(dbgfile,"\t0\t\t\t\t\t# padding\n");
            #endif			/* DeBugLinker */
         #endif				/* WordBits != 64 */
      #endif				/* Double */

      outblock(x.ovly,sizeof(double));
      }
   else if (lctable[k].c_flag & F_CsetLit) {
      for (i = 0; i < CsetSize; i++)
         csbuf[i] = 0;
      s = &lsspace[lctable[k].c_val.sval];
      i = lctable[k].c_length;
      while (i--) {
         Setb(*s, csbuf);
         s++;
         }
      j = 0;
      for (i = 0; i < 256; i++) {
         if (Testb(i, csbuf))
           j++;
         }

      #ifdef DeBugLinker
         if (Dflag) {
            fprintf(dbgfile, "%ld:\t%d\n",(long) pc, T_Cset);
            fprintf(dbgfile, "\t%d\n",j);
            }
      #endif				/* DeBugLinker */

      outword(T_Cset);
      outword(j);		   /* cset size */
      outblock((char *)csbuf,sizeof(csbuf));

      #ifdef DeBugLinker
         if (Dflag)
            dumpblock((char *)csbuf,CsetSize);
      #endif				/* DeBugLinker */

      }
   }

static void lemitproc(name, nargs, ndyn, nstat, fstat)
word name;
int nargs, ndyn, nstat, fstat;
   {
   register int i;
   register char *p;
   word s_indx;
   int size;
   /*
    * FncBlockSize = sizeof(BasicFncBlock) +
    *  sizeof(descrip)*(# of args + # of dynamics + # of statics).
    */
   size = (9*WordSize) + (2*WordSize) * (abs(nargs)+ndyn+nstat);

   p = &lsspace[name];
   #ifdef DeBugLinker
      if (Dflag) {
         fprintf(dbgfile, "%ld:\t%d\n", (long)pc, T_Proc); /* type code */
         fprintf(dbgfile, "\t%d\n", size);		/* size of block */
         fprintf(dbgfile, "\tZ+%ld\n",(long)(pc+size));	/* entry point */
         fprintf(dbgfile, "\t%d\n", nargs);		/* # arguments */
         fprintf(dbgfile, "\t%d\n", ndyn);		/* # dynamic locals */
         fprintf(dbgfile, "\t%d\n", nstat);		/* # static locals */
         fprintf(dbgfile, "\t%d\n", fstat);		/* first static */
         fprintf(dbgfile, "\t%d\tS+%ld\t\t\t# %s\n",	/* name of procedure */
            (int)strlen(p), (long)(name), p);
      }
   #endif				/* DeBugLinker */

   outword(T_Proc);
   outword(size);
   outword(pc + size - 2*WordSize); /* Have to allow for the two words
					that we've already output. */
   outword(nargs);
   outword(ndyn);
   outword(nstat);
   outword(fstat);
   outword(strlen(p));          /* procedure name: length & offset */
   outword(name);

   /*
    * Output string descriptors for argument names by looping through
    *  all locals, and picking out those with F_Argument set.
    */
   for (i = 0; i <= nlocal; i++) {
      if (lltable[i].l_flag & F_Argument) {
         s_indx = lltable[i].l_name;
         p = &lsspace[s_indx];

         #ifdef DeBugLinker
            if (Dflag)
               fprintf(dbgfile, "\t%d\tS+%ld\t\t\t# %s\n", (int)strlen(p),
                  (long)s_indx, p);
         #endif				/* DeBugLinker */

         outword(strlen(p));
         outword(s_indx);
         }
      }

   /*
    * Output string descriptors for local variable names.
    */
   for (i = 0; i <= nlocal; i++) {
      if (lltable[i].l_flag & F_Dynamic) {
         s_indx = lltable[i].l_name;
         p = &lsspace[s_indx];

         #ifdef DeBugLinker
            if (Dflag)
               fprintf(dbgfile, "\t%d\tS+%ld\t\t\t# %s\n", (int)strlen(p),
                  (long)s_indx, p);
         #endif				/* DeBugLinker */

         outword(strlen(p));
         outword(s_indx);
         }
      }

   /*
    * Output string descriptors for static variable names.
    */
   for (i = 0; i <= nlocal; i++) {
      if (lltable[i].l_flag & F_Static) {
         s_indx = lltable[i].l_name;
         p = &lsspace[s_indx];

         #ifdef DeBugLinker
            if (Dflag)
               fprintf(dbgfile, "\t%d\tS+%ld\t\t\t# %s\n", (int)strlen(p),
                  (long)s_indx, p);
         #endif				/* DeBugLinker */

         outword(strlen(p));
         outword(s_indx);
         }
      }
   }

/*
 * gentables - generate interpreter code for global, static,
 *  identifier, and record tables, and built-in procedure blocks.
 */

void gentables()
   {
   register int i;
   register char *s;
   register struct gentry *gp;
   struct fentry *fp;
   struct rentry *rp;
   struct header hdr;

   /*
    * Output record constructor procedure blocks.
    */
   align();
   hdr.Records = pc;

   #ifdef DeBugLinker
      if (Dflag) {
         fprintf(dbgfile, "\n\n# global tables\n");
         fprintf(dbgfile, "\n%ld:\t%d\t\t\t\t# record blocks\n",
	    (long)pc, nrecords);
         }
   #endif				/* DeBugLinker */

   outword(nrecords);
   for (gp = lgfirst; gp != NULL; gp = gp->g_next) {
      if ((gp->g_flag & F_Record) && gp->g_procid > 0) {
         s = &lsspace[gp->g_name];
         gp->g_pc = pc;

         #ifdef DeBugLinker
            if (Dflag) {
               fprintf(dbgfile, "%ld:\n", pc);
               fprintf(dbgfile, "\t%d\n", T_Proc);
               fprintf(dbgfile, "\t%d\n", RkBlkSize(gp));
               fprintf(dbgfile, "\t_mkrec\n");
               fprintf(dbgfile, "\t%d\n", gp->g_nargs);
               fprintf(dbgfile, "\t-2\n");
               fprintf(dbgfile, "\t%d\n", gp->g_procid);
               fprintf(dbgfile, "\t1\n");
               fprintf(dbgfile, "\t%d\tS+%ld\t\t\t# %s\n", (int)strlen(s),
                  (long)gp->g_name, s);
               }
         #endif				/* DeBugLinker */

         outword(T_Proc);		/* type code */
         outword(RkBlkSize(gp));
         outword(0);			/* entry point (filled in by interp)*/
         outword(gp->g_nargs);		/* number of fields */
         outword(-2);			/* record constructor indicator */
         outword(gp->g_procid);		/* record id */
         outword(1);			/* serial number */
         outword(strlen(s));		/* name of record: size and offset */
         outword(gp->g_name);

         for (i=0;i<gp->g_nargs;i++) {	/* field names (filled in by interp) */
            int foundit = 0;
            /*
             * Find the field list entry corresponding to field i in
             * record gp, then write out a descriptor for it.
             */
            for (fp = lffirst; fp != NULL; fp = fp->f_nextentry) {
               for (rp = fp->f_rlist; rp!= NULL; rp=rp->r_link) {
                  if (rp->r_gp == gp && rp->r_fnum == i) {
                     if (foundit) {
                        /*
                         * This internal error should never occur
                         */
                        fprintf(stderr,"found rec %d field %d already!!\n",
                           gp->g_procid, i);
                        fflush(stderr);
                        exit(1);
                        }
                     #ifdef DeBugLinker
                        if (Dflag)
                           fprintf(dbgfile, "\t%d\tS+%ld\t\t\t# %s\n",
                              (int)strlen(&lsspace[fp->f_name]),
                              fp->f_name, &lsspace[fp->f_name]);
                     #endif		/* DeBugLinker */
                     outword(strlen(&lsspace[fp->f_name]));
                     outword(fp->f_name);
                     foundit++;
                     }
                  }
               }
            if (!foundit) {
               /*
                * This internal error should never occur
                */
               fprintf(stderr,"never found rec %d field %d!!\n",
                       gp->g_procid,i);
               fflush(stderr);
               exit(1);
               }
            }
         }
      }

   #ifndef FieldTableCompression

      /*
       * Output record/field table (not compressed).
       */
      hdr.Ftab = pc;
      #ifdef DeBugLinker
         if (Dflag)
            fprintf(dbgfile,"\n%ld:\t\t\t\t\t# record/field table\n",(long)pc);
      #endif				/* DeBugLinker */

      for (fp = lffirst; fp != NULL; fp = fp->f_nextentry) {
         #ifdef DeBugLinker
            if (Dflag)
               fprintf(dbgfile, "%ld:\t\t\t\t\t# %s\n", (long)pc,
                  &lsspace[fp->f_name]);
         #endif				/* DeBugLinker */
         rp = fp->f_rlist;
         for (i = 1; i <= nrecords; i++) {
            while (rp != NULL && rp->r_gp->g_procid < 0)
	       rp = rp->r_link;		/* skip unreferenced constructor */
            if (rp != NULL && rp->r_gp->g_procid == i) {
               #ifdef DeBugLinker
                  if (Dflag)
                      fprintf(dbgfile, "\t%d\n", rp->r_fnum);
               #endif			/* DeBugLinker */
               outop(rp->r_fnum);
               rp = rp->r_link;
	       }
            else {
               #ifdef DeBugLinker
                  if (Dflag)
                      fprintf(dbgfile, "\t-1\n");
               #endif			/* DeBugLinker */
               outop(-1);
               }
            #ifdef DeBugLinker
               if (Dflag && (i == nrecords || (i & 03) == 0))
                  putc('\n', dbgfile);
            #endif			/* DeBugLinker */
            }
         }

   #else				/* FieldTableCompression */

      /*
       * Output record/field table (compressed).
       * This code has not been tested recently.
       */
      {
      int counter = 0, f_num, first, begin, end, entries;
      int *f_fo, *f_row, *f_tabp;
      char *f_bm;
      int pointer, first_avail = 0, inserted, bytes;
      hdr.Fo = pc;

      /*
       * Compute the field width required for this binary;
       * it is determined by the maximum # of fields in any one record.
       */
      long ct = 0;
      for (gp = lgfirst; gp != NULL; gp = gp->g_next)
         if ((gp->g_flag & F_Record) && gp->g_procid > 0)
            if (gp->g_nargs > ct) ct=gp->g_nargs;
      if (ct > 65535L) hdr.FtabWidth = 4;
      else if (ct > 254) hdr.FtabWidth = 2; /* 255 is (not present) */
      else hdr.FtabWidth = 1;

      /* Find out how many field names there are. */
      hdr.Nfields = 0;
      for (fp = lffirst; fp != NULL; fp = fp->f_nextentry)
         hdr.Nfields++;

      entries = hdr.Nfields * nrecords / 4 + 1;
      f_tabp = malloc (entries * sizeof (int));
      for (i = 0; i < entries; i++)
         f_tabp[i] = -1;
      f_fo = malloc (hdr.Nfields * sizeof (int));
   
      bytes = nrecords / 8;
      if (nrecords % 8 != 0)
         bytes++;
      f_bm = calloc (hdr.Nfields, bytes);
      f_row = malloc (nrecords * sizeof (int));
      f_num = 0;

      for (fp = lffirst; fp != NULL; fp = fp->f_nextentry) {
         rp = fp->f_rlist;
         first = 1;
         for (i = 0; i < nrecords; i++) {
            while (rp != NULL && rp->r_gp->g_procid < 0)
   	    rp = rp->r_link;		/* skip unreferenced constructor */
            if (rp != NULL && rp->r_gp->g_procid == i + 1) {
               if (first) {
                  first = 0;
                  begin = end = i;
                  }
               else
                  end = i;
               f_row[i] = rp->r_fnum;
               rp = rp->r_link;
   	       }
            else {
               f_row[i] = -1;
               }
            }

         inserted = 0;
         pointer = first_avail;
         while (!inserted) {
            inserted = 1;
            for (i = begin; i <= end; i++) {
               if (pointer + (end - begin) >= entries) {
                  int j;
                  int old_entries = entries;
                  entries *= 2;
                  f_tabp = realloc (f_tabp, entries * sizeof (int));
                  for (j = old_entries; j < entries; j++)
                     f_tabp[j] = -1;
                  }
               if (f_row[i] != -1)
                  if (f_tabp[pointer + (i - begin)] != -1) {
                     inserted = 0;
                     break;
                     }
               }
            pointer++;
            }
         pointer--;

         /* Create bitmap */
         for (i = 0; i < nrecords; i++) {
            int index = f_num * bytes + i / 8;
   				/* Picks out byte within bitmap row */
            if (f_row[i] != -1) {
               f_bm[index] |= 01;
   	       }
            if (i % 8 != 7)
               f_bm [index] <<= 1;
   	    }

         if (nrecords%8)
            f_bm[(f_num + 1) * bytes - 1] <<= 7 - (nrecords % 8);

         f_fo[f_num++] = pointer - begin;
         /* So that f_fo[] points to the first bit */

         for (i = begin; i <= end; i++)
            if (f_row[i] != -1)
               f_tabp[pointer + (i - begin)] = f_row[i];
         if (pointer + (end - begin) >= counter)
            counter = pointer + (end - begin + 1);
         while ((f_tabp[first_avail] != -1) && (first_avail <= counter))
            first_avail++;
         }

         /* Write out the arrays. */
         #ifdef DeBugLinker
            if (Dflag)
            fprintf (dbgfile, "\n%ld:\t\t\t\t\t# field offset array\n",
	       (long)pc);
         #endif				/* DeBugLinker */

         /*
          * Compute largest value stored in fo array
          */
         {
	 word maxfo = 0;
         for (i = 0; i < hdr.Nfields; i++) {
            if (f_fo[i] > maxfo) maxfo = f_fo[i];
            }
         if (maxfo < 254)
            hdr.FoffWidth = 1;
         else if (maxfo < 65535L)
            hdr.FoffWidth = 2;
         else
            hdr.FoffWidth = 4;
         }

         for (i = 0; i < hdr.Nfields; i++) {
            #ifdef DeBugLinker
               if (Dflag)
                  fprintf (dbgfile, "\t%d\n", f_fo[i]);
            #endif			/* DeBugLinker */
            if (hdr.FoffWidth == 1) {
	       outchar(f_fo[i]);
	       }
            else if (hdr.FoffWidth == 2)
	       outshort(f_fo[i]);
            else
            outop (f_fo[i]);
            }

         #ifdef DeBugLinker
            if (Dflag)
               fprintf (dbgfile, "\n%ld:\t\t\t\t\t# Bit maps array\n",
	          (long)pc);
         #endif				/* DeBugLinker */

         for (i = 0; i < hdr.Nfields; i++) {
            #ifdef DeBugLinker
               if (Dflag) {
                  int ct, index = i * bytes;
                  unsigned char this_bit = 0200;

                  fprintf (dbgfile, "\t");
                  for (ct = 0; ct < nrecords; ct++) {
                     if ((f_bm[index] | this_bit) == f_bm[index])
                        fprintf (dbgfile, "1");
                     else
                        fprintf (dbgfile, "0");

                     if (ct % 8 == 7) {
                        fprintf (dbgfile, " ");
                        index++;
                        this_bit = 0200;
                        }
                     else
                        this_bit >>= 1;
                     }
                  fprintf (dbgfile, "\n");
                  }
            #endif			/* DeBugLinker */
            for (pointer = i * bytes; pointer < (i + 1) * bytes; pointer++) {
               outchar (f_bm[pointer]);
	       }
            }

         align();

         #ifdef DeBugLinker
            if (Dflag)
               fprintf (dbgfile, "\n%ld:\t\t\t\t\t# record/field array\n",
	          (long)pc);
         #endif				/* DeBugLinker */

         hdr.Ftab = pc;
         for (i = 0; i < counter; i++) {
            #ifdef DeBugLinker
               if (Dflag)
                  fprintf (dbgfile, "\t%d\t%d\n", i, f_tabp[i]);
            #endif			/* DeBugLinker */
            if (hdr.FtabWidth == 1)
               outchar(f_tabp[i]);
            else if (hdr.FtabWidth == 2)
               outshort(f_tabp[i]);
            else
               outop (f_tabp[i]);
            }

         /* Free memory allocated by Jigsaw. */
         free (f_fo);
         free (f_bm);
         free (f_tabp);
         free (f_row);
         }

      #endif				/* FieldTableCompression */

   /*
    * Output descriptors for field names.
    */
   align();
   hdr.Fnames = pc;
   for (fp = lffirst; fp != NULL; fp = fp->f_nextentry) {
      s = &lsspace[fp->f_name];

      #ifdef DeBugLinker
         if (Dflag)
            fprintf(dbgfile, "%ld:\t%d\tS+%ld\t\t\t# %s\n",
               (long)pc, (int)strlen(s), (long)fp->f_name, s);
      #endif				/* DeBugLinker */

      outword(strlen(s));      /* name of field: length & offset */
      outword(fp->f_name);
   }

   /*
    * Output global variable descriptors.
    */
   hdr.Globals = pc;
   for (gp = lgfirst; gp != NULL; gp = gp->g_next) {
      if (gp->g_flag & F_Builtin) {		/* function */
         #ifdef DeBugLinker
            if (Dflag)
               fprintf(dbgfile, "%ld:\t%06lo\t%d\t\t\t# %s\n",
                   (long)pc, (long)D_Proc, -gp->g_procid, &lsspace[gp->g_name]);
         #endif				/* DeBugLinker */
         outword(D_Proc);
         outword(-gp->g_procid);
         }
      else if (gp->g_flag & F_Proc) {		/* Icon procedure */
         #ifdef DeBugLinker
            if (Dflag)
               fprintf(dbgfile, "%ld:\t%06lo\tZ+%ld\t\t\t# %s\n",
                   (long)pc,(long)D_Proc, (long)gp->g_pc, &lsspace[gp->g_name]);
         #endif				/* DeBugLinker */
         outword(D_Proc);
         outword(gp->g_pc);
         }
      else if (gp->g_flag & F_Record) {		/* record constructor */
         #ifdef DeBugLinker
            if (Dflag)
               fprintf(dbgfile, "%ld:\t%06lo\tZ+%ld\t\t\t# %s\n", (long) pc,
                   (long)D_Proc, (long)gp->g_pc, &lsspace[gp->g_name]);
         #endif				/* DeBugLinker */
         outword(D_Proc);
         outword(gp->g_pc);
         }
      else {				/* simple global variable */
         #ifdef DeBugLinker
            if (Dflag)
               fprintf(dbgfile, "%ld:\t%06lo\t0\t\t\t# %s\n",(long)pc,
                  (long)D_Null, &lsspace[gp->g_name]);
         #endif				/* DeBugLinker */
         outword(D_Null);
         outword(0);
         }
      }

   /*
    * Output descriptors for global variable names.
    */
   hdr.Gnames = pc;
   for (gp = lgfirst; gp != NULL; gp = gp->g_next) {

      #ifdef DeBugLinker
         if (Dflag)
            fprintf(dbgfile, "%ld:\t%d\tS+%ld\t\t\t# %s\n",
               (long)pc, (int)strlen(&lsspace[gp->g_name]), (long)(gp->g_name),
                  &lsspace[gp->g_name]);
      #endif				/* DeBugLinker */

      outword(strlen(&lsspace[gp->g_name]));
      outword(gp->g_name);
      }

   /*
    * Output a null descriptor for each static variable.
    */
   hdr.Statics = pc;
   for (i = lstatics; i > 0; i--) {

      #ifdef DeBugLinker
         if (Dflag)
            fprintf(dbgfile, "%ld:\t0\t0\n", (long)pc);
      #endif				/* DeBugLinker */

      outword(D_Null);
      outword(0);
      }
   flushcode();

   /*
    * Output the string constant table and the two tables associating icode
    *  locations with source program locations.  Note that the calls to write
    *  really do all the work.
    */

   hdr.Filenms = pc;
   if (longwrite((char *)fnmtbl, (long)((char *)fnmfree - (char *)fnmtbl),
      outfile) < 0)
         quit("cannot write icode file");

   #ifdef DeBugLinker
      if (Dflag) {
         int k = 0;
         struct ipc_fname *ptr;
         for (ptr = fnmtbl; ptr < fnmfree; ptr++) {
            fprintf(dbgfile, "%ld:\t%03d\tS+%03d\t\t\t# %s\n",
               (long)(pc + k), ptr->ipc, ptr->fname, &lsspace[ptr->fname]);
            k = k + 8;
            }
         putc('\n', dbgfile);
         }

   #endif				/* DeBugLinker */

   pc += (char *)fnmfree - (char *)fnmtbl;

   hdr.linenums = pc;
   if (longwrite((char *)lntable, (long)((char *)lnfree - (char *)lntable),
      outfile) < 0)
         quit("cannot write icode file");

   #ifdef DeBugLinker
      if (Dflag) {
         int k = 0;
         struct ipc_line *ptr;
         for (ptr = lntable; ptr < lnfree; ptr++) {
            fprintf(dbgfile, "%ld:\t%03d\t%03d\n", (long)(pc + k),
               ptr->ipc, ptr->line);
            k = k + 8;
            }
         putc('\n', dbgfile);
         }

   #endif				/* DeBugLinker */

   pc += (char *)lnfree - (char *)lntable;

   hdr.Strcons = pc;
   #ifdef DeBugLinker
      if (Dflag) {
         int c, j, k;
         j = k = 0;
         for (s = lsspace; s < &lsspace[lsfree]; ) {
            fprintf(dbgfile, "%ld:\t%03o", (long)(pc + k), *s++ & 0377);
            k = k + 8;
            for (i = 7; i > 0; i--) {
               if (s >= &lsspace[lsfree])
                  fprintf(dbgfile,"    ");
               else
                  fprintf(dbgfile, " %03o", *s++ & 0377);
               }
            fprintf(dbgfile, "   ");
            for (i = 0; i < 8; i++)
               if (j < lsfree) {
                  c = lsspace[j++];
                  putc(isprint(c & 0377) ? c : ' ', dbgfile);
   	       }
            putc('\n', dbgfile);
            }
         }

   #endif				/* DeBugLinker */

   if (longwrite(lsspace, (long)lsfree, outfile) < 0)
         quit("cannot write icode file");

   pc += lsfree;

   /*
    * Output icode file header.
    */
   hdr.hsize = pc;
   strcpy((char *)hdr.config,IVersion);
   hdr.trace = trace;


   #ifdef DeBugLinker
      if (Dflag) {
         fprintf(dbgfile, "\n");
         fprintf(dbgfile, "size:	 %ld\n", (long)hdr.hsize);
         fprintf(dbgfile, "trace:	 %ld\n", (long)hdr.trace);
         fprintf(dbgfile, "records: %ld\n", (long)hdr.Records);
         fprintf(dbgfile, "ftab:	 %ld\n", (long)hdr.Ftab);
         fprintf(dbgfile, "fnames:  %ld\n", (long)hdr.Fnames);
         fprintf(dbgfile, "globals: %ld\n", (long)hdr.Globals);
         fprintf(dbgfile, "gnames:  %ld\n", (long)hdr.Gnames);
         fprintf(dbgfile, "statics: %ld\n", (long)hdr.Statics);
         fprintf(dbgfile, "strcons:   %ld\n", (long)hdr.Strcons);
         fprintf(dbgfile, "filenms:   %ld\n", (long)hdr.Filenms);
         fprintf(dbgfile, "linenums:   %ld\n", (long)hdr.linenums);
         fprintf(dbgfile, "config:   %s\n", hdr.config);
         }
   #endif				/* DeBugLinker */

   fseek(outfile, hdrsize, 0);
   if (longwrite((char *)&hdr, (long)sizeof(hdr), outfile) < 0)
      quit("cannot write icode file");

   if (verbose >= 2) {
      word tsize = sizeof(hdr) + hdr.hsize;
      fprintf(stderr, "  bootstrap  %7ld\n", hdrsize);
      tsize += hdrsize;
      fprintf(stderr, "  header     %7ld\n", (long)sizeof(hdr));
      fprintf(stderr, "  procedures %7ld\n", (long)hdr.Records);
      fprintf(stderr, "  records    %7ld\n", (long)(hdr.Ftab - hdr.Records));
      fprintf(stderr, "  fields     %7ld\n", (long)(hdr.Globals - hdr.Ftab));
      fprintf(stderr, "  globals    %7ld\n", (long)(hdr.Statics - hdr.Globals));
      fprintf(stderr, "  statics    %7ld\n", (long)(hdr.Filenms - hdr.Statics));
      fprintf(stderr, "  linenums   %7ld\n", (long)(hdr.Strcons - hdr.Filenms));
      fprintf(stderr, "  strings    %7ld\n", (long)(hdr.hsize - hdr.Strcons));
      fprintf(stderr, "  total      %7ld\n", (long)tsize);
      }
   }

/*
 * align() outputs zeroes as padding until pc is a multiple of WordSize.
 */
static void align()
   {
   static word x = 0;

   if (pc % WordSize != 0)
      outblock((char *)&x, (int)(WordSize - (pc % WordSize)));
   }

/*
 * misalign() outputs a Noop instruction for padding if pc + sizeof(int)
 *  is not a multiple of WordSize.  This is for operations that output
 *  an int opcode followed by an operand that needs to be word-aligned.
 */
static void misalign()
   {
   if ((pc + IntBits/ByteBits) % WordSize != 0)
      lemit(Op_Noop, "noop [pad]");
   }

/*
 * intout(i) outputs i as an int that is used by the runtime system
 *  IntBits/ByteBits bytes must be moved from &word[0] to &codep[0].
 */
static void intout(oint)
int oint;
   {
   int i;
   union {
      int i;
      char c[IntBits/ByteBits];
      } u;

   CodeCheck(IntBits/ByteBits);
   u.i = oint;

   for (i = 0; i < IntBits/ByteBits; i++)
      codep[i] = u.c[i];

   codep += IntBits/ByteBits;
   pc += IntBits/ByteBits;
   }

#ifdef FieldTableCompression
/*
 * charout(i) outputs i as an unsigned char that is used by the runtime system
 */
static void charout(unsigned char ochar)
   {
   CodeCheck(1);
   *codep++ = (unsigned char)ochar;
   pc++;
   }
/*
 * shortout(i) outputs i as a short that is used by the runtime system
 *  IntBits/ByteBits bytes must be moved from &word[0] to &codep[0].
 */
static void shortout(short oint)
   {
   int i;
   union {
      short i;
      char c[2];
      } u;

   CodeCheck(2);
   u.i = oint;

   for (i = 0; i < 2; i++)
      codep[i] = u.c[i];

   codep += 2;
   pc += 2;
   }
#endif					/* FieldTableCompression */


/*
 * wordout(i) outputs i as a word that is used by the runtime system
 *  WordSize bytes must be moved from &oword[0] to &codep[0].
 */
static void wordout(oword)
word oword;
   {
   int i;
   union {
        word i;
        char c[WordSize];
        } u;

   CodeCheck(WordSize);
   u.i = oword;

   for (i = 0; i < WordSize; i++)
      codep[i] = u.c[i];

   codep += WordSize;
   pc += WordSize;
   }

/*
 * outblock(a,i) output i bytes starting at address a.
 */
static void outblock(addr,count)
char *addr;
int count;
   {
   CodeCheck(count);
   pc += count;
   while (count--)
      *codep++ = *addr++;
   }

#ifdef DeBugLinker
   /*
    * dumpblock(a,i) dump contents of i bytes at address a, used only
    *  in conjunction with -L.
    */
   static void dumpblock(addr, count)
   char *addr;
   int count;
      {
      int i;
      for (i = 0; i < count; i++) {
         if ((i & 7) == 0)
            fprintf(dbgfile,"\n\t");
         fprintf(dbgfile," %03o",(0377 & (unsigned)addr[i]));
         }
      putc('\n',dbgfile);
      }
   #endif				/* DeBugLinker */

/*
 * flushcode - write buffered code to the output file.
 */
static void flushcode()
   {
   if (codep > codeb)
      if (longwrite(codeb, DiffPtrs(codep,codeb), outfile) < 0)
         quit("cannot write icode file");
   codep = codeb;
   }

/*
 * clearlab - clear label table to all zeroes.
 */
static void clearlab()
   {
   register int i;

   for (i = 0; i < maxlabels; i++)
      labels[i] = 0;
   }

/*
 * backpatch - fill in all forward references to lab.
 */
static void backpatch(lab)
int lab;
   {
   word p, r;
   char *q;
   char *cp, *cr;
   register int j;

   if (lab >= maxlabels)
      labels  = (word *) trealloc(labels, NULL, &maxlabels, sizeof(word),
         lab - maxlabels + 1, "labels");

   p = labels[lab];
   if (p > 0)
      quit("multiply defined label in ucode");
   while (p < 0) {		/* follow reference chain */
      r = pc - (WordSize - p);	/* compute relative offset */
      q = codep - (pc + p);	/* point to word with address */
      cp = (char *) &p;		/* address of integer p       */
      cr = (char *) &r;		/* address of integer r       */
      for (j = 0; j < WordSize; j++) {	  /* move bytes from word pointed to */
         *cp++ = *q;			  /* by q to p, and move bytes from */
         *q++ = *cr++;			  /* r to word pointed to by q */
         }			/* moves integers at arbitrary addresses */
      }
   labels[lab] = pc;
   }

#ifdef DeBugLinker
   void idump(s)		/* dump code region */
      char *s;
      {
      int *c;

      fprintf(stderr,"\ndump of code region %s:\n",s);
      for (c = (int *)codeb; c < (int *)codep; c++)
          fprintf(stderr,"%ld: %d\n",(long)c, (int)*c);
      fflush(stderr);
      }
   #endif				/* DeBugLinker */
