#include "../h/gsupport.h"

/*
 * redirerr - redirect error output to the named file. '-' indicates that
 *  it should be redirected to standard out.
 */
int redirerr(p)
char *p;
   {
   if ( *p == '-' ) { /* let - be stdout */

/*
 * The following code is operating-system dependent [@redirerr.01].  Redirect
 *  stderr to stdout.
 */

#if PORT
   /* may not be possible */
   Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
   #if AZTEC_C
       /*
        * If it doesn't work, try trick used for HIGH_C, below.
        */
       stderr->_unit  = stdout->_unit;
       stderr->_flags = stdout->_flags;
   #endif				/* AZTEC C */
   #if LATTICE || __SASC
      /*
       * The following code varies from compiler to compiler, or even
       *  between versions of the same compiler (e.g. lattice 3.1 vs. 4.0).
       */
      stderr->_file = stdout->_file;
      stderr->_flag = stdout->_flag;
   #endif				/* LATTICE || __SASC */
#endif					/* AMIGA */

#if ARM || MVS || VM
   /* cannot do this */
#endif					/* ARM || MVS || VM */

#if ATARI_ST || MSDOS || OS2 || VMS
   #if HIGHC_386 || NT
      /*
       * Don't like doing this, but it seems to work.
       */
      setbuf(stdout,NULL);
      setbuf(stderr,NULL);
      stderr->_file = stdout->_file;
   #else				/* HIGHC_386 || NT */
      dup2(fileno(stdout),fileno(stderr));
   #endif				/* HIGHC_386 || NT */
#endif					/* ATARI_ST || MSDOS || OS2 ... */


#if MACINTOSH
   #if LSC
      /* cannot do */
   #endif				/* LSC */
   #if MPW
      close(fileno(stderr));
      dup(fileno(stdout));
   #endif				/* MPW */
#endif					/* MACINTOSH */

#if UNIX
      /*
       * This relies on the way UNIX assigns file numbers.
       */
      close(fileno(stderr));
      dup(fileno(stdout));
#endif					/* UNIX */

/*
 * End of operating-system specific code.
 */

       }
    else    /* redirecting to named file */
       if (freopen(p, "w", stderr) == NULL)
          return 0;
   return 1;
   }
