/*
 * File: rwinsys.r
 *  Window-system-specific window support routines.
 *  This file simply includes an appropriate r*win.ri file.
 */

#ifdef Graphics

   #ifdef XWindows
      #include "rxwin.ri"
   #endif				/* XWindows */

   #ifdef MSWindows
      #include "rmswin.ri"
   #endif				/* MSWindows */

#endif					/* Graphics */
