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
            backpatch(lab);
            break;

         case Op_Line:
            lineno = getdec();
            newline();
            if (profile)
               lemit(op, name);
            break;

         case Op_Colm:
            colmno = getdec();
            if (lnfree >= &lntable[nsize])
               lntable  = (struct ipc_line *)trealloc(lntable,&lnfree,&nsize,
                  sizeof(struct ipc_line), 1, "line number table");
            lnfree->ipc = pc;
            lnfree->line = lineno + (colmno << 16);
            lnfree++;
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
   outop(op);
   }

static void lemitl(op, lab, name)
int op, lab;
char *name;
   {
   misalign();

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
   outop(op);
   outword(loc);
   }

static void lemitin(op, offset, n, name)
int op, n;
word offset;
char *name;
   {
   misalign();
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

      outword(T_Real);

      #ifdef Double
         #if WordBits != 64
            /* fill out real block with an empty word */
            outword(0);
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
      outword(T_Cset);
      outword(j);		   /* cset size */
      outblock((char *)csbuf,sizeof(csbuf));
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
    * Initialize header with new (post 9.5.0) magic word and
    * ensure unused areas are now zeroed to allow future use.
    */
   memset(&hdr, 0, sizeof(struct header));
   strcpy((char *)hdr.iversion, IVersion);
   hdr.magic = IHEADER_MAGIC;
   if (profile)
      hdr.flags |= IHEADER_PROFILING;

   /*
    * Output record constructor procedure blocks.
    */
   align();
   hdr.Records = pc;
   outword(nrecords);
   for (gp = lgfirst; gp != NULL; gp = gp->g_next) {
      if ((gp->g_flag & F_Record) && gp->g_procid > 0) {
         s = &lsspace[gp->g_name];
         gp->g_pc = pc;
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

      /*
       * Output record/field table (not compressed).
       */
      hdr.Ftab = pc;
      for (fp = lffirst; fp != NULL; fp = fp->f_nextentry) {
         rp = fp->f_rlist;
         for (i = 1; i <= nrecords; i++) {
            while (rp != NULL && rp->r_gp->g_procid < 0)
	       rp = rp->r_link;		/* skip unreferenced constructor */
            if (rp != NULL && rp->r_gp->g_procid == i) {
               outop(rp->r_fnum);
               rp = rp->r_link;
	       }
            else {
               outop(-1);
               }
            }
         }

   /*
    * Output descriptors for field names.
    */
   align();
   hdr.Fnames = pc;
   for (fp = lffirst; fp != NULL; fp = fp->f_nextentry) {
      s = &lsspace[fp->f_name];
      outword(strlen(s));      /* name of field: length & offset */
      outword(fp->f_name);
   }

   /*
    * Output global variable descriptors.
    */
   hdr.Globals = pc;
   for (gp = lgfirst; gp != NULL; gp = gp->g_next) {
      if (gp->g_flag & F_Builtin) {		/* function */
         outword(D_Proc);
         outword(-gp->g_procid);
         }
      else if (gp->g_flag & F_Proc) {		/* Icon procedure */
         outword(D_Proc);
         outword(gp->g_pc);
         }
      else if (gp->g_flag & F_Record) {		/* record constructor */
         outword(D_Proc);
         outword(gp->g_pc);
         }
      else {				/* simple global variable */
         outword(D_Null);
         outword(0);
         }
      }

   /*
    * Output descriptors for global variable names.
    */
   hdr.Gnames = pc;
   for (gp = lgfirst; gp != NULL; gp = gp->g_next) {
      outword(strlen(&lsspace[gp->g_name]));
      outword(gp->g_name);
      }

   /*
    * Output a null descriptor for each static variable.
    */
   hdr.Statics = pc;
   for (i = lstatics; i > 0; i--) {
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

   pc += (char *)fnmfree - (char *)fnmtbl;

   hdr.linenums = pc;
   if (longwrite((char *)lntable, (long)((char *)lnfree - (char *)lntable),
      outfile) < 0)
         quit("cannot write icode file");

   pc += (char *)lnfree - (char *)lntable;
   hdr.Strcons = pc;

   if (longwrite(lsspace, (long)lsfree, outfile) < 0)
         quit("cannot write icode file");

   pc += lsfree;

   /*
    * Output icode file header.
    */
   hdr.hsize = pc;
   hdr.trace = trace;

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
