/*
############################################################################
#
#      File:     ilists.c
#
#      Subject:  Icon-to-C interface for simple Icon lists
#
#      Author:   Kostas Oikonomou
#
#      Date:     April 26, 2002
#
############################################################################
#
#   This file is in the public domain.
#
############################################################################
#
#  This file provides three procedures for translating homogeneous
#  lists of integers, reals, or strings to C arrays:
#
#      IListVal(d) returns an array of C ints.
#      RListVal(d) returns an array of C doubles.
#      SListVal(d) returns an array of C char pointers (char *).
#
############################################################################
#
#  Here is an example of using this interface:
#
#  1. gcc -I/opt/icon/ipl/cfuncs -shared -fPIC -o llib.so l.c
#  where "l.c" is the C fragment below.
#
#  #include "ilists.c"
#  int example(int argc, descriptor argv[])
#  {
#    int *ia;
#    double *ra;
#    char *(*sa);
#    int n;  int i;
#    ArgList(1);  n = ListLen(argv[1]);
#    ia = IListVal(argv[1]);
#    for (i=0; i<n; i++) printf("%i ", ia[i]); printf("\n");
#    ArgList(2);  n = ListLen(argv[2]);
#    ra = RListVal(argv[2]);
#    for (i=0; i<n; i++) printf("%f ", ra[i]); printf("\n");
#    ArgList(3);  n = ListLen(argv[3]);
#    printf("n = %i\n", n);
#    sa = SListVal(argv[3]);
#    for (i=0; i<n; i++) printf("%s ", sa[i]); printf("\n");
#    Return;
#  }
#
#  2. The Icon program that loads "example" from the library llib.so:
#
#  procedure main()
#     local e, L1, L2, L3
#     e := loadfunc("./llib.so", "example")
#     L1 := []
#     every i := 1 to 5 do put(L1,10*i)
#     L3 := ["abcd","/a/b/c","%&*()","","|"]
#     e(L1,[1.1,2.2,-3.3,5.5555],L3)
#     end
#
############################################################################
*/

#include "icall.h"

void cpslots(descriptor *, descriptor *, word, word);

/*
 * Given a descriptor of an Icon list of integers, this function returns
 * a C array containing the integers.
 *
 * "cpslots" is defined in src/runtime/rstruct.r.  Using cpslots() shortens the
 * necessary code, and takes care of lists that have been constructed or
 * modified by put() and get(), etc.
 * The reference to "cpslots" is satisfied in iconx.
 */

int *IListVal(descriptor d)		/*: make int[] array from list */
   {
   int *a;
   int n = ListLen(d);
   descriptor slot[n];
   int i;

   cpslots(&d,&slot[0],1,n+1);
   a = (int *) calloc(n, sizeof(int));
   if (!a) return NULL;
   for (i=0; i<n; i++) a[i] = IntegerVal(slot[i]);
   return &a[0];
   }

double *RListVal(descriptor d)		/*: make double[] array from list */
   {
   double *a;
   int n = ListLen(d);
   descriptor slot[n];
   int i;

   cpslots(&d,&slot[0],1,n+1);
   a = (double *) calloc(n, sizeof(double));
   if (!a) return NULL;
   for (i=0; i<n; i++) a[i] = RealVal(slot[i]);
   return &a[0];
   }

char **SListVal(descriptor d)		/*: make char*[] array from list */
   {
   char *(*a);
   int n = ListLen(d);
   descriptor slot[n];
   int i;

   cpslots(&d,&slot[0],1,n+1);
   /* array of n pointers to chars */
   a = (char **) calloc(n, sizeof(char *));
   if (!a) return NULL;
   for (i=0; i<n; i++) a[i] = StringVal(slot[i]);
   return &a[0];
   }
