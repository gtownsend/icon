/*
 * Coswitch for Windows using Visual C++.
 *
 * Written by Frank J. Lhota, based on an assembly version
 * authored by Robert Goldberg and modified for OS/2 2.0 by Mark
 * Emmer.
 */

#include <sys/asm_linkage.h>
#include <sys/trap.h>

/*
 * The Windows co-expression context consists of 5 words. The
 * following constants define the byte offsets for each of the
 * registers stored in the context.
 */

#define SP_OFF "0"
#define BP_OFF "4"
#define SI_OFF "8"
#define DI_OFF "12"
#define BX_OFF "16"

int	coswitch(old, new, first)
int	*old;
int	*new;
int	first;
{

   /* Save current context to *old */
   __asm__ __volatile__ (
      "movl %%esp," SP_OFF "(%0)\n\t"
      "movl %%ebp," BP_OFF "(%0)\n\t"
      "movl %%esi," SI_OFF "(%0)\n\t"
      "movl %%edi," DI_OFF "(%0)\n\t"
      "movl %%ebx," BX_OFF "(%0)"
      : : "a"( old )
	  );

   if ( first )
      {
      /* first != 0 => restore context in *new. */
	  __asm__ __volatile__ (
	     "movl " SP_OFF "(%0),%%esp\n\t"
	     "movl " BP_OFF "(%0),%%ebp\n\t"
	     "movl " SI_OFF "(%0),%%esi\n\t"
	     "movl " DI_OFF "(%0),%%edi\n\t"
	     "movl " BX_OFF "(%0),%%ebx"
	     : : "a"( new )
	     );
      }
   else
      {
      /*
       * first == 0 => Set things up for first activation of this
       *	       coexpression. Load stack pointer from first
       *	       word of *new and call new_context, which
       *	       should never return.
       */
	  __asm__ __volatile__ (
	     "movl " SP_OFF "(%0),%%esp\n\t"
	     "movl %%esp,%%ebp"
	     : : "a"( new )
	     );
      new_context( 0, NULL );
      syserr( "interp() returned in coswitch" );
      }

   return 0;
}

