/*
 * File: rwinsys.r
 *  Window-system-specific window support routines.
 *  This file simply includes an appropriate r*win.ri file.
 */

#passthru #pragma GCC diagnostic ignored "-Wunused-variable"

#ifdef Graphics

   #ifdef XWindows
      #include "rxwin.ri"
   #endif				/* XWindows */

   #ifdef WinGraphics
      #include "rmswin.ri"
   #endif				/* WinGraphics */

#endif					/* Graphics */
