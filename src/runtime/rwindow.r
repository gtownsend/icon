/*
 * File: rwindow.r
 *  non window-system-specific window support routines
 */

#ifdef Graphics

static	int	setpos          (wbp w, char *s);
static	int	sicmp		(siptr sip1, siptr sip2);

int canvas_serial, context_serial;

#ifndef MultiThread
struct descrip amperX = {D_Integer};
struct descrip amperY = {D_Integer};
struct descrip amperCol = {D_Integer};
struct descrip amperRow = {D_Integer};
struct descrip amperInterval = {D_Integer};
struct descrip lastEventWin = {D_Null};
int lastEvFWidth = 0, lastEvLeading = 0, lastEvAscent = 0;
uword xmod_control, xmod_shift, xmod_meta;
#endif					/* MultiThread */


/*
 * subscript the already-processed-events "queue" to index i.
 * used in "cooked mode" I/O to determine, e.g. how far to backspace.
 */
char *evquesub(w,i)
wbp w;
int i;
   {
   wsp ws = w->window;
   int j = ws->eQback+i;

   if (i < 0) {
      if (j < 0) j+= EQUEUELEN;
      else if (j > EQUEUELEN) j -= EQUEUELEN;
      return &(ws->eventQueue[j]);
      }
   else {
      /* "this isn't getting called in the forwards direction!\n" */
      return NULL;
      }
   }


/*
 * get event from window, assigning to &x, &y, and &interval
 *
 * returns 0 for success, -1 if window died or EOF, -2 for malformed queue
 */
int wgetevent(w,res)
wbp w;
dptr res;
   {
   struct descrip xdesc, ydesc;
   uword i;

   if (wstates != NULL && wstates->next != NULL		/* if multiple windows*/
   && (BlkLoc(w->window->listp)->list.size == 0)) {	/* & queue is empty */
      while (BlkLoc(w->window->listp)->list.size == 0) {
         #ifdef WinGraphics
	    if (ISCURSORON(w) && w->window->hasCaret == 0) {
	       wsp ws = w->window;
	       CreateCaret(ws->iconwin, NULL, FWIDTH(w), FHEIGHT(w));
	       SetCaretBlinkTime(500);
	       SetCaretPos(ws->x, ws->y - ASCENT(w));
	       ShowCaret(ws->iconwin);
	       ws->hasCaret = 1;
	       }
         #endif				/* WinGraphics */
	 if (pollevent() < 0)				/* poll all windows */
	    break;					/* break on error */
         idelay(POLLSLEEP);
	 }
      }

   if (wgetq(w,res) == -1)
      return -1;					/* window died */

   if (BlkLoc(w->window->listp)->list.size < 2)
      return -2;					/* malformed queue */

   wgetq(w,&xdesc);
   wgetq(w,&ydesc);

   if (xdesc.dword != D_Integer || ydesc.dword != D_Integer)
      return -2;			/* bad values on queue */

   IntVal(amperX) = IntVal(xdesc) & 0xFFFF;		/* &x */
   if (IntVal(amperX) >= 0x8000)
      IntVal(amperX) -= 0x10000;
   IntVal(amperY) = IntVal(ydesc) & 0xFFFF;		/* &y */
   if (IntVal(amperY) >= 0x8000)
      IntVal(amperY) -= 0x10000;
   IntVal(amperX) -= w->context->dx;
   IntVal(amperY) -= w->context->dy;
   MakeInt(1 + XTOCOL(w,IntVal(amperX)), &(amperCol));	/* &col */
   MakeInt(YTOROW(w,IntVal(amperY)) , &(amperRow));	/* &row */

   xmod_control = IntVal(xdesc) & EQ_MOD_CONTROL;	/* &control */
   xmod_meta = IntVal(xdesc) & EQ_MOD_META;		/* &meta */
   xmod_shift = IntVal(xdesc) & EQ_MOD_SHIFT;		/* &shift */

   i = (((uword) IntVal(ydesc)) >> 16) & 0xFFF;		/* mantissa */
   i <<= 4 * ((((uword) IntVal(ydesc)) >> 28) & 0x7);	/* scale it */
   IntVal(amperInterval) = i;				/* &interval */
   return 0;
   }

/*
 * get event from window (drop mouse events), no echo
 *
 * return: 1 = success, -1 = window died, -2 = malformed queue, -3 = EOF
 */
int wgetchne(w,res)
wbp w;
dptr res;
   {
   int i;

   while (1) {
      i = wgetevent(w,res);
      if (i != 0)
	 return i;
      if (is:string(*res)) {
#ifdef WinGraphics
         if (*StrLoc(*res) == '\032') return -3; /* control-Z gives EOF */
#endif					/* WinGraphics */
         return 1;
	 }
      }
   }

/*
 * get event from window (drop mouse events), with echo
 *
 * returns 1 for success, -1 if window died, -2 for malformed queue, -3 for EOF
 */
int wgetche(w,res)
wbp w;
dptr res;
   {
   int i;
   i = wgetchne(w,res);
   if (i != 1)
      return i;
   i = *StrLoc(*res);
   if ((0 <= i) && (i <= 127) && (ISECHOON(w))) {
      wputc(i, w);
      if (i == '\r') wputc((int)'\n', w); /* CR -> CR/LF */
      }
   return 1;
   }

/*
 * Get a window that has an event pending (queued)
 */
wsp getactivewindow()
   {
   static LONG next = 0;
   LONG i, j, nwindows = 0;
   wsp ptr, ws, stdws = NULL;
   extern FILE *ConsoleBinding;

   if (wstates == NULL) return NULL;
   for(ws = wstates; ws; ws=ws->next) nwindows++;
   if (ConsoleBinding) stdws = ((wbp)ConsoleBinding)->window;
   /*
    * make sure we are still in bounds
    */
   next %= nwindows;
   /*
    * position ptr on the next window to get events from
    */
   for (ptr = wstates, i = 0; i < next; i++, ptr = ptr->next);
   /*
    * Infinite loop, checking for an event somewhere, sleeping awhile
    * each iteration.
    */
   for (;;) {
      /*
       * Check for any new pending events.
       */
      switch (pollevent()) {
      case -1: ReturnErrNum(141, NULL);
      case 0: return NULL;
	 }
      /*
       * go through windows, looking for one with an event pending
       */
      for (ws = ptr, i = 0, j = next + 1; i < nwindows;
	   (ws = (ws->next) ? ws->next : wstates), i++, j++)
	 if (ws != stdws && BlkLoc(ws->listp)->list.size > 0) {
	    next = j;
	    return ws;
	    }
      /*
       * couldn't find a pending event - wait awhile
       */
      idelay(POLLSLEEP);
      }
   }

/*
 * wlongread(s,elsize,nelem,f) -- read string from window for reads(w)
 *
 * returns length(>=0) for success, -1 if window died, -2 for malformed queue
 *  -3 on EOF
 */
int wlongread(s, elsize, nelem, f)
char *s;
int elsize, nelem;
FILE *f;
   {
   int c;
   tended char *ts = s;
   struct descrip foo;
   long l = 0, bytes = elsize * nelem;

   while (l < bytes) {
     c = wgetche((wbp)f, &foo);
     if (c == -3 && l > 0)
	return l;
     if (c < 0)
	return c;
     c = *StrLoc(foo);
     switch(c) {
       case '\177':
       case '\010':
         if (l > 0) { ts--; l--; }
         break;
       default:
         *ts++ = c; l++;
         break;
       }
     }
   return l;
   }

/*
 * wgetstrg(s,maxlen,f) -- get string from window for read(w) or !w
 *
 * returns length(>=0) for success, -1 if window died, -2 for malformed queue
 *  -3 for EOF, -4 if length was limited by maxi
 */
int wgetstrg(s, maxlen, f)
char *s;
long  maxlen;
FILE *f;
   {
   int c;
   tended char *ts = s;
   long l = 0;
   struct descrip foo;

   while (l < maxlen) {
      c = wgetche((wbp)f,&foo);
      if (c == -3 && l > 0)
	 return l;
      if (c < 0)
	 return c;
      c = *StrLoc(foo);
      switch(c) {
        case '\177':
        case '\010':
          if (l > 0) { ts--; l--; }
          break;
        case '\r':
        case '\n':
          return l;
        default:
          *ts++ = c; l++;
          break;
        }
      }
   return -4;
   }


/*
 * Assignment side-effects for &x,&y,&row,&col
 */
int xyrowcol(dx)
dptr dx;
{
   if (VarLoc(*dx) == &amperX) { /* update &col too */
      wbp w;
      if (!is:file(lastEventWin) ||
          ((BlkLoc(lastEventWin)->file.status & Fs_Window) == 0) ||
          ((BlkLoc(lastEventWin)->file.status & (Fs_Read|Fs_Write)) == 0)) {
         MakeInt(1 + IntVal(amperX)/lastEvFWidth, &amperCol);
	 }
      else {
         w = (wbp)BlkLoc(lastEventWin)->file.fd;
         MakeInt(1 + XTOCOL(w, IntVal(amperX)), &amperCol);
         }
      }
   else if (VarLoc(*dx) == &amperY) { /* update &row too */
      wbp w;
      if (!is:file(lastEventWin) ||
          ((BlkLoc(lastEventWin)->file.status & Fs_Window) == 0) ||
          ((BlkLoc(lastEventWin)->file.status & (Fs_Read|Fs_Write)) == 0)) {
         MakeInt(IntVal(amperY) / lastEvLeading + 1, &amperRow);
         }
      else {
         w = (wbp)BlkLoc(lastEventWin)->file.fd;
         MakeInt(YTOROW(w, IntVal(amperY)), &amperRow);
         }
      }
   else if (VarLoc(*dx) == &amperCol) { /* update &x too */
      wbp w;
      if (!is:file(lastEventWin) ||
          ((BlkLoc(lastEventWin)->file.status & Fs_Window) == 0) ||
          ((BlkLoc(lastEventWin)->file.status & (Fs_Read|Fs_Write)) == 0)) {
         MakeInt((IntVal(amperCol) - 1) * lastEvFWidth, &amperX);
         }
      else {
         w = (wbp)BlkLoc(lastEventWin)->file.fd;
         MakeInt(COLTOX(w, IntVal(amperCol)), &amperX);
         }
      }
   else if (VarLoc(*dx) == &amperRow) { /* update &y too */
      wbp w;
      if (!is:file(lastEventWin) ||
          ((BlkLoc(lastEventWin)->file.status & Fs_Window) == 0) ||
          ((BlkLoc(lastEventWin)->file.status & (Fs_Read|Fs_Write)) == 0)) {
         MakeInt((IntVal(amperRow)-1) * lastEvLeading + lastEvAscent, &amperY);
         }
      else {
         w = (wbp)BlkLoc(lastEventWin)->file.fd;
         MakeInt(ROWTOY(w, IntVal(amperRow)), &amperY);
         }
      }
   return 0;
   }


/*
 * Enqueue an event, encoding time interval and key state with x and y values.
 */
void qevent(ws,e,x,y,t,f)
wsp ws;		/* canvas */
dptr e;		/* event code (descriptor pointer) */
int x, y;	/* x and y values */
uword t;	/* ms clock value */
long f;		/* modifier key flags */
   {
   dptr q = &(ws->listp);	/* a window's event queue (Icon list value) */
   struct descrip d;
   uword ivl, mod;
   int expo;

   mod = 0;				/* set modifier key bits */
   if (f & ControlMask) mod |= EQ_MOD_CONTROL;
   if (f & Mod1Mask)    mod |= EQ_MOD_META;
   if (f & ShiftMask)   mod |= EQ_MOD_SHIFT;

   if (t != ~(uword)0) {		/* if clock value supplied */
      if (ws->timestamp == 0)		/* if first time */
	 ws->timestamp = t;
      if (t < ws->timestamp)		/* if clock went backwards */
	 t = ws->timestamp;
      ivl = t - ws->timestamp;		/* calc interval in milliseconds */
      ws->timestamp = t;		/* save new clock value */
      expo = 0;
      while (ivl >= 0x1000) {		/* if too big */
	 ivl >>= 4;			/* reduce significance */
	 expo += 0x1000;		/* bump exponent */
	 }
      ivl += expo;			/* combine exponent with mantissa */
      }
   else
      ivl = 0;				/* report 0 if interval unknown */

   c_put(q, e);
   d.dword = D_Integer;
   IntVal(d) = mod | (x & 0xFFFF);
   c_put(q, &d);
   IntVal(d) = (ivl << 16) | (y & 0xFFFF);
   c_put(q, &d);
   }

/*
 * setpos() - set (move) canvas position on the screen
 */
static int setpos(w,s)
wbp w;
char *s;
   {
   char *s2, tmp[32];
   int posx, posy;

   s2 = s;
   while (isspace(*s2)) s2++;
   if (!isdigit(*s2) && (*s2 != '-')) return Error;
   posx = atol(s2);
   if (*s2 == '-') s2++;
   while (isdigit(*s2)) s2++;
   if (*s2 == '.') {
      s2++;
      while (isdigit(*s2)) s2++;
      }
   if (*s2++ != ',') return Error;
   if (!isdigit(*s2) && (*s2 != '-')) return Error;
   posy = atol(s2);
   if (*s2 == '-') s2++;
   while (isdigit(*s2)) s2++;
   if (*s2 == '.') {
      s2++;
      while (isdigit(*s2)) s2++;
      }
   if (*s2) return Error;
   if (posx < 0) {
      if (posy < 0) sprintf(tmp,"%d%d",posx,posy);
      else sprintf(tmp,"%d+%d",posx,posy);
      }
   else {
      if (posy < 0) sprintf(tmp,"+%d%d",posx,posy);
      else sprintf(tmp,"+%d+%d",posx,posy);
      }
   return setgeometry(w,tmp);
   }

/*
 * setsize() - set canvas size
 */
int setsize(w,s)
wbp w;
char *s;
   {
   char *s2, tmp[32];
   int width, height;

   s2 = s;
   while (isspace(*s2)) s2++;
   if (!isdigit(*s2) && (*s2 != '-')) return Error;
   width = atol(s2);
   if (*s2 == '-') s2++;
   while (isdigit(*s2)) s2++;
   if (*s2 == '.') {
      s2++;
      while (isdigit(*s2)) s2++;
      }
   if (*s2++ != ',') return Error;
   height = atol(s2);
   if (*s2 == '-') s2++;
   while (isdigit(*s2)) s2++;
   if (*s2 == '.') {
      s2++;
      while (isdigit(*s2)) s2++;
      }
   if (*s2) return Error;
   sprintf(tmp,"%dx%d",width,height);
   return setgeometry(w,tmp);
   }



/*
 * put a string out to a window using the current attributes
 */
void wputstr(w,s,len)
wbp w;
char *s;
int len;
   {
   char *s2 = s;
   wstate *ws = w->window;
   /* turn off the cursor */
   hidecrsr(ws);

   while (len > 0) {
      /*
       * find a chunk of printable text
       */
#ifdef WinGraphics
      while (len > 0) {
	 if (IsDBCSLeadByte(*s2)) {
	    s2++; s2++; len--; len--;
	    }
	 else if (isprint(*s2)) {
	    s2++; len--;
	    }
	 else break;
	 }
#else					/* WinGraphics */
      while (isprint(*s2) && len > 0) {
	 s2++; len--;
	 }
#endif					/* WinGraphics */
      /*
       * if a chunk was parsed, write it out
       */
      if (s2 != s)
         xdis(w, s, s2 - s);
      /*
       * put the 'unprintable' character, if didn't just hit the end
       */
      if (len-- > 0) {
         wputc(*s2++, w);
         }
    s = s2;
    }

  /* show the cursor again */
  UpdateCursorPos(ws, w->context);
  showcrsr(ws);
  return;
}

/*
 * mapping from recognized style attributes to flag values
 */
stringint fontwords[] = {
   { 0,			17 },		/* number of entries */
   { "bold",		FONTATT_WEIGHT	| FONTFLAG_BOLD },
   { "condensed",	FONTATT_WIDTH	| FONTFLAG_CONDENSED },
   { "demi",		FONTATT_WEIGHT	| FONTFLAG_DEMI },
   { "demibold",	FONTATT_WEIGHT	| FONTFLAG_DEMI | FONTFLAG_BOLD },
   { "extended",	FONTATT_WIDTH	| FONTFLAG_EXTENDED },
   { "italic",		FONTATT_SLANT	| FONTFLAG_ITALIC },
   { "light",		FONTATT_WEIGHT	| FONTFLAG_LIGHT },
   { "medium",		FONTATT_WEIGHT	| FONTFLAG_MEDIUM },
   { "mono",		FONTATT_SPACING	| FONTFLAG_MONO },
   { "narrow",		FONTATT_WIDTH	| FONTFLAG_NARROW },
   { "normal",		FONTATT_WIDTH	| FONTFLAG_NORMAL },
   { "oblique",		FONTATT_SLANT	| FONTFLAG_OBLIQUE },
   { "proportional",	FONTATT_SPACING	| FONTFLAG_PROPORTIONAL },
   { "roman",		FONTATT_SLANT	| FONTFLAG_ROMAN },
   { "sans",		FONTATT_SERIF	| FONTFLAG_SANS },
   { "serif",		FONTATT_SERIF	| FONTFLAG_SERIF },
   { "wide",		FONTATT_WIDTH	| FONTFLAG_WIDE },
};

/*
 * parsefont - extract font family name, style attributes, and size
 *
 * these are window system independent values, so they require
 *  further translation into window system dependent values.
 *
 * returns 1 on an OK font name
 * returns 0 on a "malformed" font (might be a window-system fontname)
 */
int parsefont(s, family, style, size)
char *s;
char family[MAXFONTWORD+1];
int *style;
int *size;
   {
   char c, *a, attr[MAXFONTWORD+1];
   int tmp;

   /*
    * set up the defaults
    */
   *family = '\0';
   *style = 0;
   *size = -1;

   /*
    * now, scan through the raw and break out pieces
    */
   for (;;) {

      /*
       * find start of next comma-separated attribute word
       */
      while (isspace(*s) || *s == ',')	/* trim leading spaces & empty words */
         s++;
      if (*s == '\0')			/* stop at end of string */
         break;

      /*
       * copy word, converting to lower case to implement case insensitivity
       */
      for (a = attr; (c = *s) != '\0' && c != ','; s++) {
         if (isupper(c))
            c = tolower(c);
         *a++ = c;
         if (a - attr >= MAXFONTWORD)
            return 0;			/* too long */
         }

      /*
       * trim trailing spaces and terminate word
       */
      while (isspace(a[-1]))
         a--;
      *a = '\0';

      /*
       * interpret word as family name, size, or style characteristic
       */
      if (*family == '\0')
         strcpy(family, attr);		/* first word is the family name */

      else if (sscanf(attr, "%d%c", &tmp, &c) == 1 && tmp > 0) {
         if (*size != -1 && *size != tmp)
            return 0;			/* if conflicting sizes given */
         *size = tmp;			/* integer value is a size */
         }

      else {				/* otherwise it's a style attribute */
         tmp = si_s2i(fontwords, attr);	/* look up in table */
         if (tmp != -1) {		/* if recognized */
            if ((tmp & *style) != 0 && (tmp & *style) != tmp)
               return 0;		/* conflicting attribute */
            *style |= tmp;
            }
         }
      }

   /* got to end of string; it's OK if it had at least a font family */
   return (*family != '\0');
   }

/*
 * parsepattern() - parse an encoded numeric stipple pattern
 */
int parsepattern(s, len, width, nbits, bits)
char *s;
int len;
int *width, *nbits;
C_integer *bits;
   {
   C_integer v;
   int i, j, hexdigits_per_row, maxbits = *nbits;

   /*
    * Get the width
    */
   if (sscanf(s, "%d,", width) != 1) return Error;
   if (*width < 1) return Failed;

   /*
    * skip over width
    */
   while ((len > 0) && isdigit(*s)) {
      len--; s++;
      }
   if ((len <= 1) || (*s != ',')) return Error;
   len--; s++;					/* skip over ',' */

   if (*s == '#') {
      /*
       * get remaining bits as hex constant
       */
      s++; len--;
      if (len == 0) return Error;
      hexdigits_per_row = *width / 4;
      if (*width % 4) hexdigits_per_row++;
      *nbits = len / hexdigits_per_row;
      if (len % hexdigits_per_row) (*nbits)++;
      if (*nbits > maxbits) return Failed;
      for (i = 0; i < *nbits; i++) {
         v = 0;
	 for (j = 0; j < hexdigits_per_row; j++, len--, s++) {
	    if (len == 0) break;
	    v <<= 4;
	    if (isdigit(*s)) v += *s - '0';
	    else switch (*s) {
	    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	       v += *s - 'a' + 10; break;
	    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	       v += *s - 'A' + 10; break;
	    default: return Error;
	       }
	    }
	 *bits++ = v;
	 }
      }
   else {
      if (*width > 32) return Failed;
      /*
       * get remaining bits as comma-separated decimals
       */
      v = 0;
      *nbits = 0;
      while (len > 0) {
	 while ((len > 0) && isdigit(*s)) {
	    v = v * 10 + *s - '0';
	    len--; s++;
	    }
	 (*nbits)++;
	 if (*nbits > maxbits) return Failed;
	 *bits++ = v;
	 v = 0;

	 if (len > 0) {
	    if (*s == ',') { len--; s++; }
	    else {
	       ReturnErrNum(205, Error);
	       }
	    }
	 }
      }
   return Succeeded;
   }

/*
 * parsegeometry - parse a string of the form: intxint[+-]int[+-]int
 * Returns:
 *  0 on bad value, 1 if size is set, 2 if position is set, 3 if both are set
 */
int parsegeometry(buf, x, y, width, height)
char *buf;
SHORT *x, *y, *width, *height;
   {
   int retval = 0;
   if (isdigit(*buf)) {
      retval++;
      if ((*width = atoi(buf)) <= 0) return 0;
      while (isdigit(*++buf));
      if (*buf++ != 'x') return 0;
      if ((*height = atoi(buf)) <= 0) return 0;
      while (isdigit(*++buf));
      }

   if (*buf == '+' || *buf == '-') {
      retval += 2;
      *x = atoi(buf);
      buf++; /* skip over +/- */
      while (isdigit(*buf)) buf++;

      if (*buf != '+' && *buf != '-') return 0;
      *y = atoi(buf);
      buf++; /* skip over +/- */
      while (isdigit(*buf)) buf++;
      if (*buf) return 0;
      }
   return retval;
   }


/* return failure if operation returns either failure or error */
#define AttemptAttr(operation) if ((operation) != Succeeded) return Failed;

/* does string (already checked for "on" or "off") say "on"? */
#define ATOBOOL(s) (s[1]=='n')

/*
 * Attribute manipulation
 *
 * wattrib() - get/set a single attribute in a window, return the result attr
 *  string.
 */
int wattrib(w, s, len, answer, abuf)
wbp w;
char *s;
long len;
dptr answer;
char * abuf;
   {
   char val[128], *valptr;
   struct descrip d;
   char *mid, *midend, c;
   int r, a;
   C_integer tmp;
   long lenattr, lenval;
   double gamma;
   SHORT new_height, new_width;
   wsp ws = w->window;
   wcp wc = w->context;

   valptr = val;
   /*
    * catch up on any events pending - mainly to update pointerx, pointery
    */
   if (pollevent() == -1)
      fatalerr(141,NULL);

   midend = s + len;
   for (mid = s; mid < midend; mid++)
      if (*mid == '=') break;

   if (mid < midend) {
      /*
       * set an attribute
       */
      lenattr = mid - s;
      lenval  = len - lenattr - 1;
      mid++;

      strncpy(abuf, s, lenattr);
      abuf[lenattr] = '\0';
      strncpy(val, mid, lenval);
      val[lenval] = '\0';
      StrLen(d) = strlen(val);
      StrLoc(d) = val;

      switch (a = si_s2i(attribs, abuf)) {
      case A_LINES: case A_ROWS: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 if ((new_height = tmp) < 1)
	    return Failed;
	 new_height = ROWTOY(w, new_height);
	 new_height += MAXDESCENDER(w);
	 if (setheight(w, new_height) == Failed) return Failed;
	 break;
         }
      case A_COLUMNS: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 if ((new_width = tmp) < 1)
	    return Failed;
	 new_width = COLTOX(w, new_width + 1);
	 if (setwidth(w, new_width) == Failed) return Failed;
	 break;
         }
      case A_HEIGHT: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
         if ((new_height = tmp) < 1) return Failed;
	 if (setheight(w, new_height) == Failed) return Failed;
	 break;
         }
      case A_WIDTH: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
         if ((new_width = tmp) < 1) return Failed;
	 if (setwidth(w, new_width) == Failed) return Failed;
	 break;
         }
      case A_SIZE: {
	 AttemptAttr(setsize(w, val));
	 break;
	 }
      case A_GEOMETRY: {
	 AttemptAttr(setgeometry(w, val));
	 break;
         }
      case A_RESIZE: {
	 if (strcmp(val, "on") & strcmp(val, "off"))
	    return Failed;
         allowresize(w, ATOBOOL(val));
	 break;
         }
      case A_ROW: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 ws->y = ROWTOY(w, tmp) + wc->dy;
	 break;
	 }
      case A_COL: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 ws->x = COLTOX(w, tmp) + wc->dx;
	 break;
	 }
      case A_CANVAS: {
	 AttemptAttr(setcanvas(w,val));
	 break;
	 }
      case A_ICONIC: {
	 AttemptAttr(seticonicstate(w,val));
	 break;
	 }
      case A_ICONIMAGE: {
	 if (!val[0]) return Failed;
	 AttemptAttr(seticonimage(w, &d));
         break;
	 }
      case A_ICONLABEL: {
	 AttemptAttr(seticonlabel(w, val));
	 break;
	 }
      case A_ICONPOS: {
	 AttemptAttr(seticonpos(w,val));
	 break;
	 }
      case A_LABEL:
      case A_WINDOWLABEL: {
	 AttemptAttr(setwindowlabel(w, val));
	 break;
         }
      case A_CURSOR: {
	 int on_off;
	 if (strcmp(val, "on") & strcmp(val, "off"))
	    return Failed;
	 on_off = ATOBOOL(val);
         setcursor(w, on_off);
	 break;
         }
      case A_FONT: {
	 AttemptAttr(setfont(w, &valptr));
	 break;
         }
      case A_PATTERN: {
	 AttemptAttr(SetPattern(w, val, strlen(val)));
         break;
	 }
      case A_POS: {
	 AttemptAttr(setpos(w, val));
	 break;
	 }
      case A_POSX: {
	 char tmp[20];
	 sprintf(tmp,"%s,%d",val,ws->posy);
	 AttemptAttr(setpos(w, tmp));
	 break;
	 }
      case A_POSY: {
	 char tmp[20];
	 sprintf(tmp,"%d,%s",ws->posx,val);
	 AttemptAttr(setpos(w, tmp));
         break;
	 }
      case A_FG: {
	 if (cnv:C_integer(d, tmp) && tmp < 0) {
	    if (isetfg(w, tmp) != Succeeded) return Failed;
	    }
	 else {
	    if (setfg(w, val) != Succeeded) return Failed;
	    }
	 break;
         }
      case A_BG: {
	 if (cnv:C_integer(d, tmp) && tmp < 0) {
	    if (isetbg(w, tmp) != Succeeded) return Failed;
	    }
	 else {
	    if (setbg(w, val) != Succeeded) return Failed;
	    }
	 break;
         }
      case A_GAMMA: {
         if (sscanf(val, "%lf%c", &gamma, &c) != 1 || gamma <= 0.0)
            return Failed;
         if (setgamma(w, gamma) != Succeeded)
            return Failed;
         break;
         }
      case A_FILLSTYLE: {
	 AttemptAttr(setfillstyle(w, val));
	 break;
	 }
      case A_LINESTYLE: {
	 AttemptAttr(setlinestyle(w, val));
	 break;
	 }
      case A_LINEWIDTH: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 if (setlinewidth(w, tmp) == Error)
	    return Failed;
	 break;
	 }
      case A_POINTER: {
	 AttemptAttr(setpointer(w, val));
	 break;
         }
      case A_DRAWOP: {
	 AttemptAttr(setdrawop(w, val));
	 break;
         }
      case A_DISPLAY: {
	 AttemptAttr(setdisplay(w,val));
	 break;
         }
      case A_X: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 ws->x = tmp + wc->dx;
         UpdateCursorPos(ws, wc);	/* tell system where to blink it */
	 break;
	 }
      case A_Y: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 ws->y = tmp + wc->dy;
         UpdateCursorPos(ws, wc);	/* tell system where to blink it */
	 break;
	 }
      case A_DX: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 wc->dx = tmp;
         UpdateCursorPos(ws, wc);	/* tell system where to blink it */
	 break;
	 }
      case A_DY: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 wc->dy = tmp;
         UpdateCursorPos(ws, wc);	/* tell system where to blink it */
	 break;
	 }
      case A_LEADING: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 setleading(w, tmp);
         break;
         }
      case A_IMAGE: {
         /* first try GIF; then try platform-dependent format */
         r = readGIF(val, 0, &ws->initimage);
         if (r == Succeeded) {
            setwidth(w, ws->initimage.width);
            setheight(w, ws->initimage.height);
            }
         else
            r = setimage(w, val);
	 AttemptAttr(r);
         break;
         }
      case A_ECHO: {
	 if (strcmp(val, "on") & strcmp(val, "off"))
	    return Failed;
	 if (ATOBOOL(val)) SETECHOON(w);
	 else CLRECHOON(w);
	 break;
         }
      case A_CLIPX:
      case A_CLIPY:
      case A_CLIPW:
      case A_CLIPH: {
	 if (!*val) {
	    wc->clipx = wc->clipy = 0;
	    wc->clipw = wc->cliph = -1;
	    unsetclip(w);
	    }
	 else {
	    if (!cnv:C_integer(d, tmp))
	       return Failed;
	    if (wc->clipw < 0) {
	       wc->clipx = wc->clipy = 0;
	       wc->clipw = ws->width;
	       wc->cliph = ws->height;
	       }
	    switch (a) {
	       case A_CLIPX:  wc->clipx = tmp;  break;
	       case A_CLIPY:  wc->clipy = tmp;  break;
	       case A_CLIPW:  wc->clipw = tmp;  break;
	       case A_CLIPH:  wc->cliph = tmp;  break;
	       }
	    setclip(w);
	    }
	 break;
	 }
      case A_REVERSE: {
	 if (strcmp(val, "on") && strcmp(val, "off"))
	    return Failed;
	 if ((!ATOBOOL(val) && ISREVERSE(w)) ||
	     (ATOBOOL(val) && !ISREVERSE(w))) {
	    toggle_fgbg(w);
	    ISREVERSE(w) ? CLRREVERSE(w) : SETREVERSE(w);
	    }
	 break;
         }
      case A_POINTERX: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 ws->pointerx = tmp + wc->dx;
	 warpPointer(w, ws->pointerx, ws->pointery);
	 break;
	 }
      case A_POINTERY: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 ws->pointery = tmp + wc->dy;
	 warpPointer(w, ws->pointerx, ws->pointery);
	 break;
	 }
      case A_POINTERCOL: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 ws->pointerx = COLTOX(w, tmp) + wc->dx;
	 warpPointer(w, ws->pointerx, ws->pointery);
	 break;
	 }
      case A_POINTERROW: {
	 if (!cnv:C_integer(d, tmp))
	    return Failed;
	 ws->pointery = ROWTOY(w, tmp) + wc->dy;
	 warpPointer(w, ws->pointerx, ws->pointery);
	 break;
	 }
      /*
       * remaining valid attributes are error #147
       */
      case A_DEPTH:
      case A_DISPLAYHEIGHT:
      case A_DISPLAYWIDTH:
      case A_FHEIGHT:
      case A_FWIDTH:
      case A_ASCENT:
      case A_DESCENT:
         ReturnErrNum(147, Error);
      /*
       * invalid attribute
       */
      default:
	 ReturnErrNum(145, Error);
	 }
      strncpy(abuf, s, len);
      abuf[len] = '\0';
      }
   else {
      int a;
      /*
       * get an attribute
       */
      strncpy(abuf, s, len);
      abuf[len] = '\0';
      switch (a=si_s2i(attribs, abuf)) {
      case A_IMAGE:
         ReturnErrNum(147, Error);
      case A_VISUAL:
	 if (getvisual(w, abuf) == Failed) return Failed;
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_DEPTH:
	 MakeInt(SCREENDEPTH(w), answer);
	 break;
      case A_DISPLAY:
	 getdisplay(w, abuf);
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_ASCENT:
	 MakeInt(ASCENT(w), answer);
	 break;
      case A_DESCENT:
	 MakeInt(DESCENT(w), answer);
	 break;
      case A_FHEIGHT:
	 MakeInt(FHEIGHT(w), answer);
	 break;
      case A_FWIDTH:
	 MakeInt(FWIDTH(w), answer);
	 break;
      case A_ROW:
	 MakeInt(YTOROW(w, ws->y - wc->dy), answer);
	 break;
      case A_COL:
	 MakeInt(1 + XTOCOL(w, ws->x - wc->dx), answer);
	 break;
      case A_POINTERROW: {
	 XPoint xp;
	 query_pointer(w, &xp);
	 MakeInt(YTOROW(w, xp.y - wc->dy), answer);
	 break;
	 }
      case A_POINTERCOL: {
	 XPoint xp;
	 query_pointer(w, &xp);
	 MakeInt(1 + XTOCOL(w, xp.x - wc->dx), answer);
	 break;
	 }
      case A_LINES:
      case A_ROWS:
	 MakeInt(YTOROW(w,ws->height - DESCENT(w)), answer);
	 break;
      case A_COLUMNS:
	 MakeInt(XTOCOL(w,ws->width), answer);
	 break;
      case A_POS: case A_POSX: case A_POSY:
	 if (getpos(w) == Failed)
	    return Failed;
	 switch (a) {
	 case A_POS:
	    sprintf(abuf, "%d,%d", ws->posx, ws->posy);
	    MakeStr(abuf, strlen(abuf), answer);
	    break;
	 case A_POSX:
	    MakeInt(ws->posx, answer);
	    break;
	 case A_POSY:
	    MakeInt(ws->posy, answer);
	    break;
	    }
	 break;
      case A_FG:
	 getfg(w, abuf);
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_BG:
	 getbg(w, abuf);
	 MakeStr(abuf, strlen(abuf), answer);
         break;
      case A_GAMMA:
         Protect(BlkLoc(*answer) = (union block *)alcreal(wc->gamma),
            return Error);
         answer->dword = D_Real;
         break;
      case A_FILLSTYLE:
         sprintf(abuf, "%s",
            (wc->fillstyle == FS_SOLID) ? "solid" :
            (wc->fillstyle == FS_STIPPLE) ? "masked" : "textured");
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_LINESTYLE:
	 getlinestyle(w, abuf);
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_LINEWIDTH:
	 MakeInt(LINEWIDTH(w), answer);
	 break;
      case A_HEIGHT: { MakeInt(ws->height, answer); break; }
      case A_WIDTH: { MakeInt(ws->width, answer); break; }
      case A_SIZE:
	 sprintf(abuf, "%d,%d", ws->width, ws->height);
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_RESIZE:
	 sprintf(abuf,"%s",(ISRESIZABLE(w)?"on":"off"));
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_DISPLAYHEIGHT:
	 MakeInt(DISPLAYHEIGHT(w), answer);
	 break;
      case A_DISPLAYWIDTH:
	 MakeInt(DISPLAYWIDTH(w), answer);
	 break;
      case A_CURSOR:
	 sprintf(abuf,"%s",(ISCURSORON(w)?"on":"off"));
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_ECHO:
	 sprintf(abuf,"%s",(ISECHOON(w)?"on":"off"));
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_REVERSE:
	 sprintf(abuf,"%s",(ISREVERSE(w)?"on":"off"));
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_FONT:
	 getfntnam(w, abuf);
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_X:  MakeInt(ws->x - wc->dx, answer); break;
      case A_Y:  MakeInt(ws->y - wc->dy, answer); break;
      case A_DX: MakeInt(wc->dx, answer); break;
      case A_DY: MakeInt(wc->dy, answer); break;
      case A_LEADING: MakeInt(LEADING(w), answer); break;
      case A_POINTERX: {
	 XPoint xp;
	 query_pointer(w, &xp);
	 MakeInt(xp.x - wc->dx, answer);
	 break;
	 }
      case A_POINTERY: {
	 XPoint xp;
	 query_pointer(w, &xp);
	 MakeInt(xp.y - wc->dy, answer);
	 break;
	 }
      case A_POINTER:
	 getpointername(w, abuf);
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_DRAWOP:
	 getdrawop(w, abuf);
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_GEOMETRY:
	 if (getpos(w) == Failed) return Failed;
         if (ws->win)
           sprintf(abuf, "%dx%d+%d+%d",
		   ws->width, ws->height, ws->posx, ws->posy);
         else
           sprintf(abuf, "%dx%d", ws->pixwidth, ws->pixheight);
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_CANVAS:
	 getcanvas(w, abuf);
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_ICONIC:
	 geticonic(w, abuf);
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_ICONIMAGE:
	 if (ICONFILENAME(w) != NULL)
	    sprintf(abuf, "%s", ICONFILENAME(w));
	 else *abuf = '\0';
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_ICONLABEL:
	 if (ICONLABEL(w) != NULL)
	    sprintf(abuf, "%s", ICONLABEL(w));
	 else return Failed;
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_LABEL:
      case A_WINDOWLABEL:
	 if (WINDOWLABEL(w) != NULL)
	    sprintf(abuf,"%s", WINDOWLABEL(w));
	 else return Failed;
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
      case A_ICONPOS: {
	 switch (geticonpos(w,abuf)) {
	    case Failed: return Failed;
	    case Error:  return Failed;
	    }
	 MakeStr(abuf, strlen(abuf), answer);
	 break;
	 }
      case A_PATTERN: {
	 s = w->context->patternname;
	 if (s != NULL)
	    MakeStr(s, strlen(s), answer);
	 else
	    MakeStr("black", 5, answer);
	 break;
	 }
      case A_CLIPX:
	 if (wc->clipw >= 0)
	    MakeInt(wc->clipx, answer);
	 else
	    *answer = nulldesc;
	 break;
      case A_CLIPY:
	 if (wc->clipw >= 0)
	    MakeInt(wc->clipy, answer);
	 else
	    *answer = nulldesc;
	 break;
      case A_CLIPW:
	 if (wc->clipw >= 0)
	    MakeInt(wc->clipw, answer);
	 else
	    *answer = nulldesc;
	 break;
      case A_CLIPH:
	 if (wc->clipw >= 0)
	    MakeInt(wc->cliph, answer);
	 else
	    *answer = nulldesc;
	 break;
      default:
	 ReturnErrNum(145, Error);
	 }
   }
   wflush(w);
   return Succeeded;
   }

/*
 * rectargs -- interpret rectangle arguments uniformly
 *
 *  Given an arglist and the index of the next x value, rectargs sets
 *  x/y/width/height to explicit or defaulted values.  These result values
 *  are in canonical form:  Width and height are nonnegative and x and y
 *  have been corrected by dx and dy.
 *
 *  Returns index of bad argument, if any, or -1 for success.
 */
int rectargs(w, argc, argv, i, px, py, pw, ph)
wbp w;
int argc;
dptr argv;
int i;
C_integer *px, *py, *pw, *ph;
   {
   int defw, defh;
   wcp wc = w->context;
   wsp ws = w->window;

   /*
    * Get x and y, defaulting to -dx and -dy.
    */
   if (i >= argc)
      *px = -wc->dx;
   else if (!def:C_integer(argv[i], -wc->dx, *px))
      return i;

   if (++i >= argc)
      *py = -wc->dy;
   else if (!def:C_integer(argv[i], -wc->dy, *py))
      return i;

   *px += wc->dx;
   *py += wc->dy;

   /*
    * Get w and h, defaulting to extend to the edge
    */
   defw = ws->width - *px;
   defh = ws->height - *py;

   if (++i >= argc)
      *pw = defw;
   else if (!def:C_integer(argv[i], defw, *pw))
      return i;

   if (++i >= argc)
      *ph = defh;
   else if (!def:C_integer(argv[i], defh, *ph))
      return i;

   /*
    * Correct negative w/h values.
    */
   if (*pw < 0)
      *px -= (*pw = -*pw);
   if (*ph < 0)
      *py -= (*ph = -*ph);

   return -1;
   }

/*
 * docircles -- draw or file circles.
 *
 *  Helper for DrawCircle and FillCircle.
 *  Returns index of bad argument, or -1 for success.
 */
int docircles(w, argc, argv, fill)
wbp w;
int argc;
dptr argv;
int fill;
   {
   XArc arc;
   int i, dx, dy;
   double x, y, r, theta, alpha;

   dx = w->context->dx;
   dy = w->context->dy;

   for (i = 0; i < argc; i += 5) {	/* for each set of five args */

      /*
       * Collect arguments.
       */
      if (i + 2 >= argc)
         return i + 2;			/* missing y or r */
      if (!cnv:C_double(argv[i], x))
         return i;
      if (!cnv:C_double(argv[i + 1], y))
         return i + 1;
      if (!cnv:C_double(argv[i + 2], r))
         return i + 2;
      if (i + 3 >= argc)
         theta = 0.0;
      else if (!def:C_double(argv[i + 3], 0.0, theta))
         return i + 3;
      if (i + 4 >= argc)
         alpha = 2 * Pi;
      else if (!def:C_double(argv[i + 4], 2 * Pi, alpha))
         return i + 4;

      /*
       * Put in canonical form: r >= 0, -2*pi <= theta < 0, alpha >= 0.
       */
      if (r < 0) {			/* ensure positive radius */
         r = -r;
         theta += Pi;
         }
      if (alpha < 0) {			/* ensure positive extent */
         theta += alpha;
         alpha = -alpha;
         }

      theta = fmod(theta, 2 * Pi);
      if (theta > 0)			/* normalize initial angle */
         theta -= 2 * Pi;

      /*
       * Build the Arc descriptor.
       */
      arc.x = x + dx - r;
      arc.y = y + dy - r;
      ARCWIDTH(arc) = 2 * r;
      ARCHEIGHT(arc) = 2 * r;

      arc.angle1 = ANGLE(theta);
      if (alpha >= 2 * Pi)
         arc.angle2 = EXTENT(2 * Pi);
      else
         arc.angle2 = EXTENT(alpha);

      /*
       * Draw or fill the arc.
       */
      if (fill) {			/* {} required due to form of macros */
         fillarcs(w, &arc, 1);
         }
      else {
         drawarcs(w, &arc, 1);
         }
      }
   return -1;
   }


/*
 * genCurve - draw a smooth curve through a set of points.  Algorithm from
 *  Barry, Phillip J., and Goldman, Ronald N. (1988).
 *  A Recursive Evaluation Algorithm for a class of Catmull-Rom Splines.
 *  Computer Graphics 22(4), 199-204.
 */
void genCurve(w, p, n, helper)
wbp w;
XPoint *p;
int n;
void (*helper)	(wbp, XPoint [], int);
   {
   int    i, j, steps;
   float  ax, ay, bx, by, stepsize, stepsize2, stepsize3;
   float  x, dx, d2x, d3x, y, dy, d2y, d3y;
   XPoint *thepoints = NULL;
   long npoints = 0;

   for (i = 3; i < n; i++) {
      /*
       * build the coefficients ax, ay, bx and by, using:
       *                             _              _   _    _
       *   i                 i    1 | -1   3  -3   1 | | Pi-3 |
       *  Q (t) = T * M   * G   = - |  2  -5   4  -1 | | Pi-2 |
       *               CR    Bs   2 | -1   0   1   0 | | Pi-1 |
       *                            |_ 0   2   0   0_| |_Pi  _|
       */

      ax = p[i].x - 3 * p[i-1].x + 3 * p[i-2].x - p[i-3].x;
      ay = p[i].y - 3 * p[i-1].y + 3 * p[i-2].y - p[i-3].y;
      bx = 2 * p[i-3].x - 5 * p[i-2].x + 4 * p[i-1].x - p[i].x;
      by = 2 * p[i-3].y - 5 * p[i-2].y + 4 * p[i-1].y - p[i].y;

      /*
       * calculate the forward differences for the function using
       * intervals of size 0.1
       */
#ifndef abs
#define abs(x) ((x)<0?-(x):(x))
#endif
#ifndef max
#define max(x,y) ((x>y)?x:y)
#endif

      steps = max(abs(p[i-1].x - p[i-2].x), abs(p[i-1].y - p[i-2].y)) + 10;
      if (steps+4 > npoints) {
         if (thepoints != NULL) free(thepoints);
	 thepoints = malloc((steps+4) * sizeof(XPoint));
	 npoints = steps+4;
         }

      stepsize = 1.0/steps;
      stepsize2 = stepsize * stepsize;
      stepsize3 = stepsize * stepsize2;

      x = thepoints[0].x = p[i-2].x;
      y = thepoints[0].y = p[i-2].y;
      dx = (stepsize3*0.5)*ax + (stepsize2*0.5)*bx + (stepsize*0.5)*(p[i-1].x-p[i-3].x);
      dy = (stepsize3*0.5)*ay + (stepsize2*0.5)*by + (stepsize*0.5)*(p[i-1].y-p[i-3].y);
      d2x = (stepsize3*3) * ax + stepsize2 * bx;
      d2y = (stepsize3*3) * ay + stepsize2 * by;
      d3x = (stepsize3*3) * ax;
      d3y = (stepsize3*3) * ay;

      /* calculate the points for drawing the curve */

      for (j = 0; j < steps; j++) {
	 x = x + dx;
	 y = y + dy;
	 dx = dx + d2x;
	 dy = dy + d2y;
	 d2x = d2x + d3x;
	 d2y = d2y + d3y;
         thepoints[j + 1].x = (int)x;
         thepoints[j + 1].y = (int)y;
         }
      helper(w, thepoints, steps + 1);
      }
   if (thepoints != NULL) {
      free(thepoints);
      thepoints = NULL;
      }
   }

static void curveHelper(wbp w, XPoint *thepoints, int n)
   {
   /*
    * Could use drawpoints(w, thepoints, n)
    *  but that ignores the linewidth and linestyle attributes...
    * Might make linestyle work a little better by "compressing" straight
    *  sections produced by genCurve into single drawline points.
    */
   drawlines(w, thepoints, n);
   }

/*
 * draw a smooth curve through the array of points
 */
void drawCurve(w, p, n)
wbp w;
XPoint *p;
int n;
   {
   genCurve(w, p, n, curveHelper);
   }

/*
 * Compare two unsigned long values for qsort or qsearch.
 */
int ulcmp(p1, p2)
pointer p1, p2;
   {
   register unsigned long u1 = *(unsigned int *)p1;
   register unsigned long u2 = *(unsigned int *)p2;

   if (u1 < u2)
      return -1;
   else
      return (u1 > u2);
   }

/*
 * the next section consists of code to deal with string-integer
 * (stringint) symbols.  See graphics.h.
 */

/*
 * string-integer comparison, for qsearch()
 */
static int sicmp(sip1,sip2)
siptr sip1, sip2;
{
  return strcmp(sip1->s, sip2->s);
}

/*
 * string-integer lookup function: given a string, return its integer
 */
int si_s2i(sip,s)
siptr sip;
char *s;
{
  stringint key;
  siptr p;
  key.s = s;

  p = (siptr)qsearch((char *)&key,(char *)(sip+1),sip[0].i,sizeof(key),sicmp);
  if (p) return p->i;
  return -1;
}

/*
 * string-integer inverse function: given an integer, return its string
 */
char *si_i2s(sip,i)
siptr sip;
int i;
{
  register siptr sip2 = sip+1;
  for(;sip2<=sip+sip[0].i;sip2++) if (sip2->i == i) return sip2->s;
  return NULL;
}


/*
 * And now, the stringint data.
 * Convention: the 0'th element of a stringint array contains the
 * NULL string, and an integer count of the # of elements in the array.
 */

stringint attribs[] = {
   { 0,			NUMATTRIBS},
   {"ascent",		A_ASCENT},
   {"bg",		A_BG},
   {"canvas",		A_CANVAS},
   {"ceol",		A_CEOL},
   {"cliph",		A_CLIPH},
   {"clipw",		A_CLIPW},
   {"clipx",		A_CLIPX},
   {"clipy",		A_CLIPY},
   {"col",		A_COL},
   {"columns",		A_COLUMNS},
   {"cursor",		A_CURSOR},
   {"depth",		A_DEPTH},
   {"descent",		A_DESCENT},
   {"display",		A_DISPLAY},
   {"displayheight",	A_DISPLAYHEIGHT},
   {"displaywidth",	A_DISPLAYWIDTH},
   {"drawop",		A_DRAWOP},
   {"dx",		A_DX},
   {"dy",		A_DY},
   {"echo",		A_ECHO},
   {"fg",		A_FG},
   {"fheight",		A_FHEIGHT},
   {"fillstyle",	A_FILLSTYLE},
   {"font",		A_FONT},
   {"fwidth",		A_FWIDTH},
   {"gamma",		A_GAMMA},
   {"geometry",		A_GEOMETRY},
   {"height",		A_HEIGHT},
   {"iconic",		A_ICONIC},
   {"iconimage",	A_ICONIMAGE},
   {"iconlabel",	A_ICONLABEL},
   {"iconpos",		A_ICONPOS},
   {"image",		A_IMAGE},
   {"label",		A_LABEL},
   {"leading",		A_LEADING},
   {"lines",		A_LINES},
   {"linestyle",	A_LINESTYLE},
   {"linewidth",	A_LINEWIDTH},
   {"pattern",		A_PATTERN},
   {"pointer",		A_POINTER},
   {"pointercol",	A_POINTERCOL},
   {"pointerrow",	A_POINTERROW},
   {"pointerx",		A_POINTERX},
   {"pointery",		A_POINTERY},
   {"pos",		A_POS},
   {"posx",		A_POSX},
   {"posy",		A_POSY},
   {"resize",		A_RESIZE},
   {"reverse",		A_REVERSE},
   {"row",		A_ROW},
   {"rows",		A_ROWS},
   {"size",		A_SIZE},
   {"visual",		A_VISUAL},
   {"width",		A_WIDTH},
   {"windowlabel",	A_WINDOWLABEL},
   {"x",		A_X},
   {"y",		A_Y},
};


/*
 * There are more, X-specific stringint arrays in ../common/xwindow.c
 */

#endif					/* Graphics */
