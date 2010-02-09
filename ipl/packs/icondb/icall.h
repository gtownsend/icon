/*
############################################################################
#
#	File:     icall.h
#
#	Subject:  Definitions for external C functions
#
#	Author:   Gregg M. Townsend
#
#	Date:     November 17, 2004
#
############################################################################
#
#   This file is in the public domain.
#
############################################################################
#
#   Contributor: Kostas Oikonomou
#
############################################################################
#
#   These definitions assist in writing external C functions for use with
#   Version 9 of Icon.
#
############################################################################
#
#   From Icon, loadfunc(libfile, funcname) loads a C function of the form
#	int func(int argc, descriptor argv[])
#   where "descriptor" is the structure type defined here.  The C
#   function returns -1 to fail, 0 to succeed, or a positive integer
#   to report an error.  Argv[1] through argv[argc] are the incoming
#   arguments; the return value on success (or the offending value
#   in case of error) is stored in argv[0].
#
#   In the macro descriptions below, d is a descriptor value, typically
#   a member of the argv array.  IMPORTANT: many macros assume that the
#   C function's parameters are named "argc" and "argv" as noted above.
#
############################################################################
#
#   IconType(d) returns one of the characters {cfinprsCILRST} indicating
#   the type of a value according to the key on page 247 of the Red Book
#   or page 273 of the Blue Book (The Icon Programming Language).
#   The character I indicates a large (multiprecision) integer.
#
#   Only a few of these types (i, r, f, s) are easily manipulated in C.
#   Given that the type has been verified, the following macros return
#   the value of a descriptor in C terms:
#
#	IntegerVal(d)	value of a integer (type 'i') as a C long
#	RealVal(d)	value of a real (type 'r') as a C double
#	FileVal(d)	value of a file (type 'f') as a C FILE pointer
#	FileStat(d)	status field of a file
#	StringVal(d)	value of a string (type 's') as a C char pointer
#			(copied if necessary to add \0 for termination)
#
#	StringAddr(d)	address of possibly unterminated string
#	StringLen(d)	length of string
#
#	ListLen(d)	length of list
#
#   These macros check the type of an argument, converting if necessary,
#   and returning an error code if the argument is wrong:
#
#	ArgInteger(i)		check that argv[i] is an integer
#	ArgReal(i)		check that argv[i] is a real number
#	ArgString(i)		check that argv[i] is a string
#	ArgList(i)		check that argv[i] is a list
#
#   Caveats:
#      Allocation failure is not detected.
#
############################################################################
#
#   These macros return from the C function back to Icon code:
#
#	Return			return argv[0] (initially &null)
#	RetArg(i)		return argv[i]
#	RetNull()		return &null
#	RetInteger(i)		return integer value i
#	RetReal(v)		return real value v
#	RetFile(fp,status,name)	return (newly opened) file
#	RetString(s)		return null-terminated string s
#	RetStringN(s, n)	return string s whose length is n
#	RetAlcString(s, n)	return already-allocated string
#	RetConstString(s)	return constant string s
#	RetConstStringN(s, n)	return constant string s of length n
#	Fail			return failure status
#	Error(n)		return error code n
#	ArgError(i,n)		return argv[i] as offending value for error n
#
############################################################################
 */

#include <stdio.h>
#include <limits.h>

#if INT_MAX == 32767
#define WordSize 16
#elif LONG_MAX == 2147483647L
#define WordSize 32
#else
#define WordSize 64
#endif

#if WordSize <= 32
#define F_Nqual    0x80000000		/* set if NOT string qualifier */
#define F_Var      0x40000000		/* set if variable */
#define F_Ptr      0x10000000		/* set if value field is pointer */
#define F_Typecode 0x20000000		/* set if dword includes type code */
#else
#define F_Nqual    0x8000000000000000	/* set if NOT string qualifier */
#define F_Var      0x4000000000000000	/* set if variable */
#define F_Ptr      0x1000000000000000	/* set if value field is pointer */
#define F_Typecode 0x2000000000000000	/* set if dword includes type code */
#endif

#define D_Typecode	(F_Nqual | F_Typecode)

#define T_Null		 0		/* null value */
#define T_Integer	 1		/* integer */
#define T_Real		 3		/* real number */
#define T_File		 5		/* file, including window */

#define D_Null		(T_Null     | D_Typecode)
#define D_Integer	(T_Integer  | D_Typecode)
#define D_Real		(T_Real     | D_Typecode | F_Ptr)
#define D_File		(T_File     | D_Typecode | F_Ptr)

#define Fs_Read		0001		/* file open for reading */
#define Fs_Write	0002		/* file open for writing */
#define Fs_Pipe		0020		/* file is a [popen] pipe */
#define Fs_Window	0400		/* file is a window */


typedef long word;
typedef struct { word dword, vword; } descriptor;
typedef struct { word title; double rval; } realblock;
typedef struct { word title; FILE *fp; word stat; descriptor fname; } fileblock;
typedef struct { word title, size, id; void *head, *tail; } listblock;


char *alcstr(char *s, word len);
realblock *alcreal(double v);
fileblock *alcfile(FILE *fp, int stat, descriptor *name);
int cnv_c_str(descriptor *s, descriptor *d);
int cnv_int(descriptor *s, descriptor *d);
int cnv_real(descriptor *s, descriptor *d);
int cnv_str(descriptor *s, descriptor *d);
double getdbl(descriptor *d);

extern descriptor nulldesc;		/* null descriptor */


#define IconType(d) ((d).dword>=0 ? 's' : "niIrcfpRL.S.T.....C"[(d).dword&31])


#define IntegerVal(d) ((d).vword)

#define RealVal(d) getdbl(&(d))

#define FileVal(d) (((fileblock *)((d).vword))->fp)
#define FileStat(d) (((fileblock *)((d).vword))->stat)

#define StringAddr(d) ((char *)(d).vword)
#define StringLen(d) ((d).dword)

#define StringVal(d) \
(*(char*)((d).vword+(d).dword) ? cnv_c_str(&(d),&(d)) : 0, (char*)((d).vword))

#define ListLen(d) (((listblock *)((d).vword))->size)


#define ArgInteger(i) do { if (argc < (i)) Error(101); \
if (!cnv_int(&argv[i],&argv[i])) ArgError(i,101); } while (0)

#define ArgReal(i) do { if (argc < (i)) Error(102); \
if (!cnv_real(&argv[i],&argv[i])) ArgError(i,102); } while (0)

#define ArgString(i) do { if (argc < (i)) Error(103); \
if (!cnv_str(&argv[i],&argv[i])) ArgError(i,103); } while (0)

#define ArgList(i) \
do {if (argc < (i)) Error(108); \
if (IconType(argv[i]) != 'L') ArgError(i,108); } while(0)


#define RetArg(i) return (argv[0] = argv[i], 0)

#define RetNull() return (argv->dword = D_Null, argv->vword = 0)

#define RetInteger(i) return (argv->dword = D_Integer, argv->vword = i, 0)

#define RetReal(v) return (argv->dword=D_Real, argv->vword=(word)alcreal(v), 0)

#define RetFile(fp,stat,name) \
do { descriptor dd; dd.vword = (word)alcstr(name, dd.dword = strlen(name)); \
   argv->dword = D_File; argv->vword = (word)alcfile(fp, stat, &dd); \
   return 0; } while (0)

#define RetString(s) \
do { word n = strlen(s); \
argv->dword = n; argv->vword = (word)alcstr(s,n); return 0; } while (0)

#define RetStringN(s,n) \
do { argv->dword = n; argv->vword = (word)alcstr(s,n); return 0; } while (0)

#define RetConstString(s) return (argv->dword=strlen(s), argv->vword=(word)s, 0)

#define RetConstStringN(s,n) return (argv->dword=n, argv->vword=(word)s, 0)

#define RetAlcString(s,n) return (argv->dword=n, argv->vword=(word)s, 0)


#define Fail return -1
#define Return return 0
#define Error(n) return n
#define ArgError(i,n) return (argv[0] = argv[i], n)
