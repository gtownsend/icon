/*
#################################################################################
#
#      File:     ilists.c
#
#      Subject:  Icon-to-C interface for simple Icon lists
#
#      Author:   Kostas Oikonomou
#
#      Date:     April 2002
#
#################################################################################
#
#  A simple Icon list is one all of whose elements are of the same type.  This
#  can be integer, real, or string.  This code uses Gregg Townsend's "icall.h".
#  In addition, it uses the declaration of "b_list", a list-header block.
#
#  Provides the macros
#
#      ArgList(i)
#      ListLen(d)
#
#  and the C routines
#
#      IListVal(d)
#      RListVal(d)
#      SListVal(d)
#
#################################################################################
#
# 
# Here is an example of using this interface:
# 
# 1. gcc -I/opt/icon/ipl/cfuncs -G -fpic -o llib.so l.c
# where "l.c" is the C fragment below.
#
# #include "icall.h"
# #include "ilists.c"
# int example(int argc, descriptor argv[])
# {
#   int *ia;
#   double *ra;
#   char *(*sa);
#   int n;  int i;
#   ArgList(1);  n = ListLen(argv[1]);
#   ia = IListVal(argv[1]); for (i=0; i<n; i++) printf("%i ", ia[i]); printf("\n");
#   ArgList(2);  n = ListLen(argv[2]);
#   ra = RListVal(argv[2]);  for (i=0; i<n; i++) printf("%f ", ra[i]); printf("\n");
#   ArgList(3);  n = ListLen(argv[3]);
#   printf("n = %i\n", n);
#   sa = SListVal(argv[3]);  for (i=0; i<n; i++) printf("%s ", sa[i]); printf("\n");
#   Return;
# }
# 
# 2. The Icon program that loads "example" from the library llib.so:
# 
# procedure main()
#   local e, L1, L2, L3
#   e := loadfunc("./llib.so", "example")
#   L1 := []
#   every i := 1 to 5 do put(L1,10*i)
#   L3 := ["abcd","/a/b/c","%&*()","","|"]
#   e(L1,[1.1,2.2,-3.3,5.5555],L3)
# end
#
#################################################################################
*/



#include "icall.h"

#define ArgList(i) \
do {if (argc < (i)) Error(101); \
    if (IconType(argv[i]) != 'L') ArgError(i,108); \
   } while(0);

#define ListLen(d) (((struct b_list *) d.vword)->size)

/* List-header block, see src/h/rstructs.h */
struct b_list {
  word title;                 /* T_List */
  word size;                  /* current list size */
  word id;		      /* identification number */
  word listhead, listtail;    /* Hack! Assumes size of pointer = size of word */
};


/*  
    Given a descriptor of an Icon list of integers, this function returns
    a  C array containing the integers.
    "cpslots" is defined in src/runtime/rstruct.r.  Using cpslots() shortens the
    necessary code, and takes care of lists that have been constructed or
    modified by put() and get(), etc.
    The reference to "cpslots" is satisfied in iconx.
*/

int *IListVal(descriptor d)
{  
  int *a;
  int n = ListLen(d);
  {
    descriptor slot[n];
    int i;
    cpslots(d,&slot[0],1,n+1);
    a = (int *) calloc(n, sizeof(int));
    for (i=0; i<n; i++) a[i] = IntegerVal(slot[i]);
  }
  return &a[0];
}

double *RListVal(descriptor d)
{
  double *a;
  int n = ListLen(d);
  {
    descriptor slot[n];
    int i;
    cpslots(d,&slot[0],1,n+1);
    a = (double *) calloc(n, sizeof(double));
    for (i=0; i<n; i++) a[i] = RealVal(slot[i]);
  }
  return &a[0];
}

char **SListVal(descriptor d)
{
  char *(*a);
  int n = ListLen(d);
  {
    descriptor slot[n];
    int i;
    cpslots(d,&slot[0],1,n+1);
    /* array of n pointers to chars */
    a = (char **) calloc(n, sizeof(char *));
    for (i=0; i<n; i++) a[i] = StringVal(slot[i]);
  }
  return &a[0];
}
