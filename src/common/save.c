/*
 * save(s) -- for systems that support ExecImages
 */

#include "../h/gsupport.h"

#ifdef ExecImages

/*
 * save(s) -- for generic BSD systems.
 */

#ifdef GenericBSD
#include <a.out.h>
wrtexec(ef)
int ef;
{
   struct exec hdr;
   extern environ, etext;
   int tsize, dsize;

   /*
    * Construct the header.  The text and data region sizes must be multiples
    *  of 1024.
    */

#ifdef __NetBSD__
   hdr.a_midmag = ZMAGIC;
#else
   hdr.a_magic = ZMAGIC;
#endif

   tsize = (int)&etext;
   hdr.a_text = (tsize + 1024) & ~(1024-1);
   dsize = sbrk(0) - (int)&environ;
   hdr.a_data = (dsize + 1024) & ~(1024-1);
   hdr.a_bss = 0;
   hdr.a_syms = 0;
   hdr.a_entry = 0;
   hdr.a_trsize = 0;
   hdr.a_drsize = 0;

   /*
    * Write the header.
    */
   write(ef, &hdr, sizeof(hdr));

   /*
    * Write the text, starting at N_TXTOFF.
    */
   lseek(ef, N_TXTOFF(hdr), 0);
   write(ef, 0, tsize);
   lseek(ef, hdr.a_text - tsize, 1);

   /*
    * Write the data.
    */
   write(ef, &environ, dsize);
   lseek(ef, hdr.a_data - dsize, 1);
   close(ef);
   return hdr.a_data;
}
#endif					/* GenericBSD */

#endif					/* ExecImages */
