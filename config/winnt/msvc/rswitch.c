/*
 * Coswitch for NT using Visual C++.
 *
 * Written by Frank J. Lhota, based on an assembly version
 * authored by Robert Goldberg and modified for OS/2 2.0 by Mark
 * Emmer.
 */

#include "../h/rt.h"

/*
 * The NT co-expression context consists of 5 words. The
 * following constants define the byte offsets for each of the
 * registers stored in the context.
 */

#define SP_OFF  0
#define BP_OFF  4
#define SI_OFF  8
#define DI_OFF 12
#define BX_OFF 16

int	coswitch(old, new, first)
word	*old;
word	*new;
int	first;
{

   /* Save current context to *old */
   __asm {
      mov	eax, old
      mov	SP_OFF [ eax ], esp
      mov	BP_OFF [ eax ], ebp
      mov       SI_OFF [ eax ], esi
      mov	DI_OFF [ eax ], edi
      mov	BX_OFF [ eax ], ebx
      }

   if ( first )
      /* first != 0 => restore context in *new. */
      __asm {
	 mov	eax, new
	 mov	esp, SP_OFF [ eax ]
	 mov	ebp, BP_OFF [ eax ]
	 mov	esi, SI_OFF [ eax ]
	 mov	edi, DI_OFF [ eax ]
	 mov	ebx, BX_OFF [ eax ]
	 }
   else
      {
      /*
       * first == 0 => Set things up for first activation of this
       *	       coexpression. Load stack pointer from first
       *	       word of *new and call new_context, which
       *	       should never return.
       */
      __asm {
	 mov	eax, new
	 mov	esp, SP_OFF [ eax ]
	 mov	ebp, esp
	 }
      new_context( 0, NULL );
      syserr( "interp() returned in coswitch" );
      }

   return 0;
}

