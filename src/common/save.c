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

/*
 * save(s) -- for Sun Workstations.
 */

#ifdef SUN
#include <a.out.h>
wrtexec(ef)
int ef;
{
   struct exec *hdrp, hdr;
   extern environ, etext;
   int tsize, dsize;

   hdrp = (struct exec *)PAGSIZ;

   /*
    * This code only handles the ZMAGIC format...
    */
   if (hdrp->a_magic != ZMAGIC)
      syserr("executable is not ZMAGIC format");
   /*
    * Construct the header by copying in the header in core and fixing
    *  up values as necessary.
    */
   hdr = *hdrp;
   tsize = (char *)&etext - (char *)hdrp;
   hdr.a_text = (tsize + PAGSIZ) & ~(PAGSIZ-1);
   dsize = sbrk(0) - (int)&environ;
   hdr.a_data = (dsize + PAGSIZ) & ~(PAGSIZ-1);
   hdr.a_bss = 0;
   hdr.a_syms = 0;
   hdr.a_trsize = 0;
   hdr.a_drsize = 0;

   /*
    * Write the text.
    */
   write(ef, hdrp, tsize);
   lseek(ef, hdr.a_text, 0);

   /*
    * Write the data.
    */
   write(ef, &environ, dsize);
   lseek(ef, hdr.a_data - dsize, 1);

   /*
    * Write the header.
    */
   lseek(ef, 0, 0);
   write(ef, &hdr, sizeof(hdr));

   close(ef);
   return hdr.a_data;
}
#endif					/* SUN */

#else					/* ExecImages */
static char junk;
#endif					/* ExecImages */
