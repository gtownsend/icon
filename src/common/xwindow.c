/*
 * xwindow.c - X Window System-specific routines
 */
#include "../h/define.h"
#include "../h/config.h"
#ifdef XWindows

typedef struct {
  char *s;
  int i;
} stringint, *siptr;

#ifdef XpmFormat
   #include "../xpm/xpm.h"
#else					/* XpmFormat */
   #include <X11/Xlib.h>
   #include <X11/Xutil.h>
#endif					/* XpmFormat */

#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

int GraphicsHome  = XK_Home;
int GraphicsLeft  = XK_Left;
int GraphicsUp    = XK_Up;
int GraphicsRight = XK_Right;
int GraphicsDown  = XK_Down;
int GraphicsPrior = XK_Prior;
int GraphicsNext  = XK_Next;
int GraphicsEnd   = XK_End;

/*
 * Translate a key event.  Put ascii result if any in s.
 * Return number of ascii (>0) if the key was "normal" and s is filled in.
 * Return 0 if the key was strange and keysym should be returned.
 * Return -1 if the key was a modifier key and should be dropped.
 */
int translate_key_event(event, s, k)
XKeyEvent *event;
char *s;
KeySym *k;
{
   int i = XLookupString(event, s, 10, k, NULL);

   if (i > 0)
      return i;			/* "normal" key */
   else if (IsModifierKey(*k))
      return -1;		/* modifier key */
   else
      return 0;			/* other (e.g. function key) */
}

stringint drawops[] = {
   { 0, 16},
   {"and",		GXand},
   {"andInverted",	GXandInverted},
   {"andReverse",	GXandReverse},
   {"clear",		GXclear},
   {"copy",		GXcopy},
   {"copyInverted",	GXcopyInverted},
   {"equiv",		GXequiv},
   {"invert",		GXinvert},
   {"nand",		GXnand},
   {"noop",		GXnoop},
   {"nor",		GXnor},
   {"or",		GXor},
   {"orInverted",	GXorInverted},
   {"orReverse",	GXorReverse},
   {"set",		GXset},
   {"xor",		GXxor},
};

#define NUMCURSORSYMS	78

stringint cursorsyms[] = {
  { 0, NUMCURSORSYMS},
  {"X cursor",		XC_X_cursor},
  {"arrow",		XC_arrow},
  {"based arrow down",	XC_based_arrow_down},
  {"based arrow up",	XC_based_arrow_up},
  {"boat",		XC_boat},
  {"bogosity",		XC_bogosity},
  {"bottom left corner",XC_bottom_left_corner},
  {"bottom right corner",XC_bottom_right_corner},
  {"bottom side",	XC_bottom_side},
  {"bottom tee",	XC_bottom_tee},
  {"box spiral",	XC_box_spiral},
  {"center ptr",	XC_center_ptr},
  {"circle",		XC_circle},
  {"clock",		XC_clock},
  {"coffee mug",	XC_coffee_mug},
  {"cross",		XC_cross},
  {"cross reverse",	XC_cross_reverse},
  {"crosshair",		XC_crosshair},
  {"diamond cross",	XC_diamond_cross},
  {"dot",		XC_dot},
  {"dotbox",		XC_dotbox},
  {"double arrow",	XC_double_arrow},
  {"draft large",	XC_draft_large},
  {"draft small",	XC_draft_small},
  {"draped box",	XC_draped_box},
  {"exchange",		XC_exchange},
  {"fleur",		XC_fleur},
  {"gobbler",		XC_gobbler},
  {"gumby",		XC_gumby},
  {"hand1",		XC_hand1},
  {"hand2",		XC_hand2},
  {"heart",		XC_heart},
  {"icon",		XC_icon},
  {"iron cross",	XC_iron_cross},
  {"left ptr",		XC_left_ptr},
  {"left side",		XC_left_side},
  {"left tee",		XC_left_tee},
  {"leftbutton",	XC_leftbutton},
  {"ll angle",		XC_ll_angle},
  {"lr angle",		XC_lr_angle},
  {"man",		XC_man},
  {"middlebutton",	XC_middlebutton},
  {"mouse",		XC_mouse},
  {"pencil",		XC_pencil},
  {"pirate",		XC_pirate},
  {"plus",		XC_plus},
  {"question arrow",	XC_question_arrow},
  {"right ptr",		XC_right_ptr},
  {"right side",	XC_right_side},
  {"right tee",		XC_right_tee},
  {"rightbutton",	XC_rightbutton},
  {"rtl logo",		XC_rtl_logo},
  {"sailboat",		XC_sailboat},
  {"sb down arrow",	XC_sb_down_arrow},
  {"sb h double arrow",	XC_sb_h_double_arrow},
  {"sb left arrow",	XC_sb_left_arrow},
  {"sb right arrow",	XC_sb_right_arrow},
  {"sb up arrow",	XC_sb_up_arrow},
  {"sb v double arrow",	XC_sb_v_double_arrow},
  {"shuttle",		XC_shuttle},
  {"sizing",		XC_sizing},
  {"spider",		XC_spider},
  {"spraycan",		XC_spraycan},
  {"star",		XC_star},
  {"target",		XC_target},
  {"tcross",		XC_tcross},
  {"top left arrow",	XC_top_left_arrow},
  {"top left corner",	XC_top_left_corner},
  {"top right corner",	XC_top_right_corner},
  {"top side",		XC_top_side},
  {"top tee",		XC_top_tee},
  {"trek",		XC_trek},
  {"ul angle",		XC_ul_angle},
  {"umbrella",		XC_umbrella},
  {"ur angle",		XC_ur_angle},
  {"watch",		XC_watch},
  {"xterm",		XC_xterm},
  {"num glyphs",	XC_num_glyphs},
};

#endif					/* XWindows */
