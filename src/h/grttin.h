/*
 * Group of include files for input to rtt.
 *   rtt reads these files for preprocessor directives and typedefs, but
 *   does not output any code from them.
 */
#include "../h/define.h"
#include "../h/arch.h"
#include "../h/config.h"
#include "../h/version.h"

#ifndef NoTypeDefs
   #include "../h/typedefs.h"
#endif					/* NoTypeDefs */

/*
 * Macros that must be expanded by rtt.
 */

/*
 * Declaration for library routine.
 */
#begdef LibDcl(nm,n,pn)
   #passthru OpBlock(nm,n,pn,0)

   int O##nm(nargs,cargp)
   int nargs;
   register dptr cargp;
#enddef					/* LibDcl */

/*
 * Error exit from non top-level routines. Set tentative values for
 *   error number and error value; these errors will but put in
 *   effect if the run-time error routine is called.
 */
#begdef ReturnErrVal(err_num, offending_val, ret_val)
   do {
   t_errornumber = err_num;
   t_errorvalue = offending_val;
   t_have_val = 1;
   return ret_val;
   } while (0)
#enddef					/* ReturnErrVal */

#begdef ReturnErrNum(err_num, ret_val)
   do {
   t_errornumber = err_num;
   t_errorvalue = nulldesc;
   t_have_val = 0;
   return ret_val;
   } while (0)
#enddef					/* ReturnErrNum */

/*
 * Code expansions for exits from C code for top-level routines.
 */
#define Fail		return A_Resume
#define Return		return A_Continue

/*
 * RunErr encapsulates a call to the function err_msg, followed
 *  by Fail.  The idea is to avoid the problem of calling
 *  runerr directly and forgetting that it may actually return.
 */

#define RunErr(n,dp) do {\
   err_msg((int)n,dp);\
   Fail;\
   } while (0)

/*
 * Protection macro.
 */
#define Protect(notnull,orelse) do {if ((notnull)==NULL) orelse;} while(0)

#ifdef EventMon
/*
 * perform what amounts to "function inlining" of EVVal
 */
#begdef EVVal(value,event)
   do {
      if (is:null(curpstate->eventmask)) break;
      else if (!Testb((word)event, curpstate->eventmask)) break;
      MakeInt(value, &(curpstate->parent->eventval));
      actparent(event);
   } while (0)
#enddef					/* EVVal */
#begdef EVValD(dp,event)
   do {
      if (is:null(curpstate->eventmask)) break;
      else if (!Testb((word)event, curpstate->eventmask)) break;
      curpstate->parent->eventval = *(dp);
      actparent(event);
   } while (0)
#enddef					/* EVValD */
#begdef EVValX(bp,event)
   do {
      struct progstate *parent = curpstate->parent;
      if (is:null(curpstate->eventmask)) break;
      else if (!Testb((word)event, curpstate->eventmask)) break;
      parent->eventval.dword = D_Coexpr;
      BlkLoc(parent->eventval) = (union block *)(bp);
      actparent(event);
   } while (0)
#enddef					/* EVValX */

#define InterpEVVal(arg1,arg2)  { ExInterp; EVVal(arg1,arg2); EntInterp; }
#define InterpEVValD(arg1,arg2) { ExInterp; EVValD(arg1,arg2); EntInterp; }
#define InterpEVValX(arg1,arg2) { ExInterp; EVValX(arg1,arg2); EntInterp; }

/*
 * Macro with construction of event descriptor.
 */

#begdef Desc_EVValD(bp, code, type)
   do {
   eventdesc.dword = type;
   eventdesc.vword.bptr = (union block *)(bp);
   EVValD(&eventdesc, code);
   } while (0)
#enddef					/* Desc_EVValD */

#else					/* EventMon */
   #define EVVal(arg1,arg2)
   #define EVValD(arg1,arg2)
   #define EVValX(arg1,arg2)
   #define InterpEVVal(arg1,arg2)
   #define InterpEVValD(arg1,arg2)
   #define InterpEVValX(arg1,arg2)
   #define Desc_EVValD(bp, code, type)
#endif					/* EventMon */

/*
 * dummy typedefs for things defined in #include files
 */
typedef int clock_t, time_t, fd_set;

#ifdef ReadDirectory
   typedef int DIR;
#endif					/* ReadDirectory */

/*
 * graphics
 */
#ifdef Graphics
   typedef int wbp, wsp, wcp, wdp, wclrp, wfp;
   typedef int wbinding, wstate, wcontext, wfont;
   typedef int siptr, stringint;
   typedef int XRectangle, XPoint, XSegment, XArc, SysColor, LinearColor;
   typedef int LONG, SHORT;

   #ifdef XWindows
      typedef int Atom, Time, XSelectionEvent, XErrorEvent, XErrorHandler;
      typedef int XGCValues, XColor, XFontStruct, XWindowAttributes, XEvent;
      typedef int XExposeEvent, XKeyEvent, XButtonEvent, XConfigureEvent;
      typedef int XSizeHints, XWMHints, XClassHint, XTextProperty;
      typedef int Colormap, XVisualInfo, va_list;
      typedef int *Display, Cursor, GC, Window, Pixmap, Visual, KeySym;
      typedef int WidgetClass, XImage, XpmAttributes;
   #endif				/* XWindows */

   #ifdef WinGraphics
      typedef int clock_t, jmp_buf, MINMAXINFO, OSVERSIONINFO, BOOL_CALLBACK;
      typedef int int_PASCAL, LRESULT_CALLBACK, MSG, BYTE, WORD, DWORD;
      typedef int HINSTANCE, LPSTR, HBITMAP, WNDCLASS, PAINTSTRUCT, POINT, RECT;
      typedef int HWND, HDC, UINT, WPARAM, LPARAM, HANDLE, HPEN, HBRUSH, SIZE;
      typedef int COLORREF, HFONT, LOGFONT, TEXTMETRIC, FONTENUMPROC, FARPROC;
      typedef int LOGPALETTE, HPALETTE, PALETTEENTRY, HCURSOR, BITMAP, HDIB;
      typedef int va_list, LOGPEN, LOGBRUSH, LPVOID, MCI_PLAY_PARMS;
      typedef int MCI_OPEN_PARMS, MCI_STATUS_PARMS, MCI_SEQ_SET_PARMS;
      typedef int CHOOSEFONT, CHOOSECOLOR, OPENFILENAME, HMENU, LPBITMAPINFO;
      typedef int childcontrol, CPINFO, BITMAPINFO, BITMAPINFOHEADER, RGBQUAD;
      typedef int BOOL, LPMSG, STARTUPINFO;
   #endif				/* WinGraphics */

   /*
    * Convenience macros to make up for RTL's long-windedness.
    */
   #begdef CnvCShort(desc, s)
      {
      C_integer tmp;
      if (!cnv:C_integer(desc,tmp) || tmp > 0x7FFF || tmp < -0x8000)
         runerr(101,desc);
      s = (short) tmp;
      }
   #enddef				/* CnvCShort */

   #define CnvCInteger(d,i) \
      if (!cnv:C_integer(d,i)) runerr(101,d);

   #define DefCInteger(d,default,i) \
      if (!def:C_integer(d,default,i)) runerr(101,d);

   #define CnvString(din,dout) \
      if (!cnv:string(din,dout)) runerr(103,din);

   #define CnvTmpString(din,dout) \
      if (!cnv:tmp_string(din,dout)) runerr(103,din);

   /*
    * conventions supporting optional initial window arguments:
    *
    * All routines declare argv[argc] as their parameters
    * Macro OptWindow checks argv[0] and assigns _w_ and warg if it is a window
    * warg serves as a base index and is added everywhere argv is indexed
    * n is used to denote the actual number of "objects" in the call
    * Macro ReturnWindow returns either the initial window argument, or &window
    */
   #begdef OptWindow(w)
      if (argc>warg && is:file(argv[warg])) {
         if ((BlkLoc(argv[warg])->file.status & Fs_Window) == 0)
            runerr(140,argv[warg]);
         if ((BlkLoc(argv[warg])->file.status & (Fs_Read|Fs_Write)) == 0)
            runerr(142,argv[warg]);
         (w) = (wbp)BlkLoc(argv[warg])->file.fd;
         if (ISCLOSED(w))
            runerr(142,argv[warg]);
         warg++;
         }
      else {
         if (!(is:file(kywd_xwin[XKey_Window]) &&
            (BlkLoc(kywd_xwin[XKey_Window])->file.status & Fs_Window)))
               runerr(140,kywd_xwin[XKey_Window]);
         if (!(BlkLoc(kywd_xwin[XKey_Window])->file.status&(Fs_Read|Fs_Write)))
            runerr(142,kywd_xwin[XKey_Window]);
         (w) = (wbp)BlkLoc(kywd_xwin[XKey_Window])->file.fd;
         if (ISCLOSED(w))
            runerr(142,kywd_xwin[XKey_Window]);
         }
   #enddef				/* OptWindow */

   #begdef ReturnWindow
         if (!warg) return kywd_xwin[XKey_Window];
         else return argv[0]
   #enddef				/* ReturnWindow */

   #begdef CheckArgMultiple(mult)
      {
      if ((argc-warg) % (mult)) runerr(146);
      n = (argc-warg)/mult;
      if (!n) runerr(146);
      }
   #enddef				/* CheckArgMultiple */

   /*
    * calloc to make sure uninit'd entries are zeroed.
    */
   #begdef GRFX_ALLOC(var,type)
      do {
         var = (struct type *)calloc(1, sizeof(struct type));
         if (var == NULL) ReturnErrNum(305, NULL);
         var->refcount = 1;
      } while(0)
   #enddef				/* GRFX_ALLOC */

   #begdef GRFX_LINK(var, chain)
      do {
         var->next = chain;
         var->previous = NULL;
         if (chain) chain->previous = var;
         chain = var;
      } while(0)
   #enddef				/* GRFX_LINK */

   #begdef GRFX_UNLINK(var, chain)
      do {
         if (var->previous) var->previous->next = var->next;
         else chain = var->next;
         if (var->next) var->next->previous = var->previous;
         free(var);
      } while(0)
   #enddef				/* GRFX_UNLINK */

#endif					/* Graphics */

#ifdef FAttrib
   typedef unsigned long mode_t;
   typedef int HFILE, OFSTRUCT, FILETIME, SYSTEMTIME;
#endif					/* FAttrib */
