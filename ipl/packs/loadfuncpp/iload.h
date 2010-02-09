
/* C++ support for easy extensions to icon via loadfunc,
 * without garbage collection difficulties.
 * Include loadfuncpp.h and link dynamically to
 * iload.cpp, which contains the necessary glue.
 * See iexample.cpp for typical use.
 * Carl Sturtivant, 2008/3/17
 */


#include <climits>
#include <cstdlib>

#if LONG_MAX == 2147483647L //32 bit icon implementation word
#define D_Null      0xA0000000
#define D_Integer   0xA0000001
#define D_Lrgint    0xB0000002
#define D_Real		0xB0000003
#define D_File		0xB0000005
#define D_Proc		0xB0000006
#define D_External	0xB0000013
#define D_Illegal	0xA0000063
#define F_Nqual     0x80000000
#define F_Var       0x40000000
#else                       //64 bit icon implementation word
#define D_Null      0xA000000000000000
#define D_Integer   0xA000000000000001
#define D_Lrgint    0xB000000000000002
#define D_Real		0xB000000000000003
#define D_File		0xB000000000000005
#define D_Proc		0xB000000000000006
#define D_External	0xB000000000000013
#define D_Illegal	0xA000000000000063
#define F_Nqual     0x8000000000000000
#define F_Var       0x4000000000000000
#endif

#define T_Null       0  // null value
#define T_Integer    1  // integer
#define T_Lrgint     2  // long integer
#define T_Real       3  // real number
#define T_Cset       4  // cset
#define T_File       5  // file
#define T_Proc       6  // procedure
#define T_Record     7  // record
#define T_List       8  // list
#define T_Set       10  // set
#define T_Table     12  // table
#define T_Coexpr    18  // coexpression
#define T_External  19  // external value

#define TypeMask    63  // type mask

#define SUSPEND		1   // Call the interpreter suspending from a C function: G_Csusp

extern "C" { //callbacks in iconx

void deref(value*, value*); 	//dereference an icon 'variable' descriptor
char* alcstr(char*, int);   	//allocate an icon string by copying
char *alcreal(double);		//allocate double by copying
char *alcbignum(long);		//allocate Icon large integer block w/ given number of DIGITS
double getdbl(value*);			//retrieve double
char* alcfile(FILE *fp, int stat, value *name);
int anycmp(const value*, const value*);		//comparator used when sorting in Icon
//alcexternal in iconx for Icon 9.5 and above
external* alcexternal(long nbytes, external_ftable* ftable, external* ep);

void syserr(char*); //fatally terminate Icon-style with error message

int interp(int fsig, value *cargp); //the Icon interpreter, called recursively when suspending


//the prototypes of all icon functions and operators in iconx needed to do the dirty work
iconfunc Oasgn;
iconfunc Osubsc;
iconfunc Osize;
iconfunc Oneg;
iconfunc Ocompl;
iconfunc Orefresh;
iconfunc Orandom;
iconfunc Oplus;
iconfunc Ominus;
iconfunc Omult;
iconfunc Odivide;
iconfunc Omod;
iconfunc Opowr;
iconfunc Ounion;
iconfunc Ointer;
iconfunc Odiff;
iconfunc Ocater;
iconfunc Olconcat;
iconfunc Osect;
iconfunc Oswap;

iconfvbl Ollist;

iconfunc Zloadfunc;
iconfunc Zproc;
iconfunc Zvariable;

iconfunc Zlist;
iconfunc Zset;
iconfunc Ztable;

iconfunc Zstring;
iconfunc Zcset;
iconfunc Zinteger;
iconfunc Zreal;
iconfunc Znumeric;

iconfvbl Zput;
iconfvbl Zpush;

iconfvbl Zrunerr;
iconfvbl Zwrites;
iconfunc Zimage;

iconfunc Zabs;
iconfunc Zacos;
iconfunc Zargs;
iconfunc Zasin;
iconfunc Zatan;
iconfunc Zcenter;
iconfunc Zchar;
iconfunc Zchdir;
iconfunc Zclose;
iconfunc Zcollect;
iconfunc Zcopy;
iconfunc Zcos;
iconfunc Zdelay;
iconfunc Zdelete;
iconfunc Zdisplay;
iconfunc Zdtor;
iconfunc Zerrorclear;
iconfunc Zexit;
iconfunc Zexp;
iconfunc Zflush;
iconfunc Zget;
iconfunc Zgetch;
iconfunc Zgetche;
iconfunc Zgetenv;
iconfunc Ziand;
iconfunc Zicom;
iconfunc Zinsert;
iconfunc Zior;
iconfunc Zishift;
iconfunc Zixor;
iconfunc Zkbhit;
iconfunc Zleft;
iconfunc Zlog;
iconfunc Zmap;
iconfunc Zmember;
iconfunc Zname;
iconfunc Zopen;
iconfunc Zord;
iconfunc Zpop;
iconfunc Zpull;
iconfunc Zread;
iconfunc Zreads;
iconfunc Zremove;
iconfunc Zrename;
iconfunc Zrepl;
iconfunc Zreverse;
iconfunc Zright;
iconfunc Zrtod;
iconfunc Zseek;
iconfunc Zserial;
iconfunc Zsin;
iconfunc Zsort;
iconfunc Zsortf;
iconfunc Zsqrt;
iconfunc Zsystem;
iconfunc Ztan;
iconfunc Ztrim;
iconfunc Ztype;
iconfunc Zwhere;

iconfvbl Zdetab;
iconfvbl Zentab;
iconfvbl Zpush;
iconfvbl Zput;
iconfvbl Zstop;
iconfvbl Zwrite;

iconfunc Kallocated;
iconfunc Kascii;
iconfunc Kclock;
//iconfunc Kcol;
iconfunc Kcollections;
//iconfunc Kcolumn;
//iconfunc Kcontrol;
iconfunc Kcset;
iconfunc Kcurrent;
iconfunc Kdate;
iconfunc Kdateline;
iconfunc Kdigits;
iconfunc Kdump;
iconfunc Ke;
iconfunc Kerror;
iconfunc Kerrornumber;
iconfunc Kerrortext;
iconfunc Kerrorvalue;
iconfunc Kerrout;
//iconfunc Keventcode;
//iconfunc Keventsource;
//iconfunc Keventvalue;
iconfunc Kfail;
iconfunc Kfeatures;
iconfunc Kfile;
iconfunc Khost;
iconfunc Kinput;
//iconfunc Kinterval;
iconfunc Klcase;
//iconfunc Kldrag;
iconfunc Kletters;
iconfunc Klevel;
iconfunc Kline;
//iconfunc Klpress;
//iconfunc Klrelease;
iconfunc Kmain;
//iconfunc Kmdrag;
//iconfunc Kmeta;
//iconfunc Kmpress;
//iconfunc Kmrelease;
iconfunc Knull;
iconfunc Koutput;
iconfunc Kphi;
iconfunc Kpi;
iconfunc Kpos;
iconfunc Kprogname;
iconfunc Krandom;
//iconfunc Krdrag;
iconfunc Kregions;
iconfunc Kresize;
//iconfunc Krow;
//iconfunc Krpress;
//iconfunc Krrelease;
//iconfunc Kshift;
iconfunc Ksource;
iconfunc Kstorage;
iconfunc Ksubject;
iconfunc Ktime;
iconfunc Ktrace;
iconfunc Kucase;
iconfunc Kversion;
iconfunc Kwindow;
//iconfunc Kx;
//iconfunc Ky;

} //end extern "C"

struct proc_block {
   	long title;					/* T_Proc */
   	long blksize;				/* size of block */
   	iconfvbl *entryp;			/* entry point for C routine */
   	long nparam;				/* number of parameters */
   	long ndynam;				/* number of dynamic locals */
   	long nstatic;				/* number of static locals */
   	long fstatic;				/* index (in global table) of first static */
   	value pname;				/* procedure name (string qualifier) */
   	value lnames[1];			/* list of local names (qualifiers) */
  private:
   	inline void init(value procname) {
   		title = T_Proc;
   		blksize = sizeof(proc_block);
   		ndynam = -1; //treat as a built-in function
   		nstatic = 0;
   		fstatic = 0;
   		pname = procname;
   		lnames[0] = nullstring;
   	}
   	static long extra_bytes;
  public:
   	proc_block(value procname, iconfvbl *function);
   	proc_block(value procname, iconfunc *function, int arity);
   	proc_block(value procname, iconfvbl *function, int arity);
   	proc_block(proc_block*);
   	static proc_block* bind(proc_block*, const value&);
	static void* operator new(size_t); 	//allocated by iconx
	static void operator delete(void*);	//do nothing
};

struct coexp_block {
	long title;
	long size;
	long id;
	coexp_block* next;
	void* es_pfp;
	void* es_efp;
	void* es_gfp;
	safe_variable* es_tend;
	value* es_argp;
	//...
};

// name/proc-block table of built-in functions
struct pstrnm {	char* pstrep; proc_block *pblock; };
extern pstrnm pntab[]; //table of original procedure blocks (src/runtime/data.r)
extern int pnsize; //size of said table
extern "C" {
int dp_pnmcmp(struct pstrmn*, value*); //comparison function
char* qsearch(char*, char*, int, int, int (*)(struct pstrmn*, value*)); //search for a name
}

inline int safecall_0(iconfunc*,  value&);
inline int safecall_1(iconfunc*,  value&, const value&);
inline int safecall_2(iconfunc*,  value&, const value&, const value&);
inline int safecall_3(iconfunc*,  value&, const value&, const value&, const value&);
inline int safecall_4(iconfunc*,  value&, const value&, const value&, const value&, const value&);
inline int safecall_5(iconfunc*,  value&, const value&, const value&, const value&, const value&, const value&);
inline int safecall_6(iconfunc*,  value&, const value&, const value&, const value&, const value&, const value&, const value&);
inline int safecall_v0(iconfvbl*, value&);
inline int safecall_v1(iconfvbl*, value&, const value&);
inline int safecall_v2(iconfvbl*, value&, const value&, const value&);
inline int safecall_v3(iconfvbl*, value&, const value&, const value&, const value&);
inline int safecall_vbl(iconfvbl*,safe&, const variadic&);

//iconx GC tend list
extern safe_variable* tend;
//our global GC tend list
extern safe_variable*& global_tend;

extern value k_current, k_main; //descriptors for &current and &main

//useful helper functions
namespace Value {
	value list(value n = (long)0, value init = nullvalue);
	value pair(value, value);
	value set(value list=nullvalue);
	void runerr(value i, value x = nullvalue);
	value table(value init = nullvalue);
	value variable(value name);
	value proc(value name, value arity = nullvalue);
	value libproc(value name, value arity = nullvalue);
	value call(const value& proc, const value& arglist);
	value create(const value&, const value&);	// create x!y
	value reduce(const value&, const value&, const value&, const value&);
}; //end namespace Value

//raw call to the modified three argument loadfunc
static int rawloadfuncpp(value argv[]);

