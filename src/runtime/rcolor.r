/*
 * File: rcolor.r
 *  graphics tables and functions related to color
 */

#ifdef Graphics

static	int	colorphrase	(char *buf, long *r, long *g, long *b);
static	double	rgbval		(double n1, double n2, double hue);

/*
 * Structures and tables used for color parsing.
 *  Tables must be kept lexically sorted.
 */

typedef struct {	/* color name entry */
   char name[8];	/* basic color name */
   char ish[12];	/* -ish form */
   short hue;		/* hue, in degrees */
   char lgt;		/* lightness, as percentage */
   char sat;		/* saturation, as percentage */
} colrname;

typedef struct {	/* arbitrary lookup entry */
   char word[10];	/* word */
   char val;		/* value, as percentage */
} colrmod;

static colrname colortable[] = {		/* known colors */
   /* color       ish-form     hue  lgt  sat */
   { "black",    "blackish",     0,   0,   0 },
   { "blue",     "bluish",     240,  50, 100 },
   { "brown",    "brownish",    30,  25, 100 },
   { "cyan",     "cyanish",    180,  50, 100 },
   { "gray",     "grayish",      0,  50,   0 },
   { "green",    "greenish",   120,  50, 100 },
   { "grey",     "greyish",      0,  50,   0 },
   { "magenta",  "magentaish", 300,  50, 100 },
   { "orange",   "orangish",    15,  50, 100 },
   { "pink",     "pinkish",    345,  75, 100 },
   { "purple",   "purplish",   270,  50, 100 },
   { "red",      "reddish",      0,  50, 100 },
   { "violet",   "violetish",  270,  75, 100 },
   { "white",    "whitish",      0, 100,   0 },
   { "yellow",   "yellowish",   60,  50, 100 },
   };

static colrmod lighttable[] = {			/* lightness modifiers */
   { "dark",       0 },
   { "deep",       0 },		/* = very dark (see code) */
   { "light",    100 },
   { "medium",    50 },
   { "pale",     100 },		/* = very light (see code) */
   };

static colrmod sattable[] = {			/* saturation levels */
   { "moderate",  50 },
   { "strong",    75 },
   { "vivid",    100 },
   { "weak",      25 },
   };

/*
 *  parsecolor(w, s, &r, &g, &b) - parse a color specification
 *
 *  parsecolor interprets a color specification and produces r/g/b values
 *  scaled linearly from 0 to 65535.  parsecolor returns Succeeded or Failed.
 *
 *  An Icon color specification can be any of the forms
 *
 *     #rgb			(hexadecimal digits)
 *     #rrggbb
 *     #rrrgggbbb
 *     #rrrrggggbbbb
 *     nnnnn,nnnnn,nnnnn	(integers 0 - 65535)
 *     <Icon color phrase>
 *     <native color spec>
 */

int parsecolor(w, buf, r, g, b)
wbp w;
char *buf;
long *r, *g, *b;
   {
   int len, mul;
   char *fmt, c;
   double dr, dg, db;

   *r = *g = *b = 0L;

   /* trim leading spaces */
   while (isspace(*buf))
      buf++;

   /* try interpreting as three comma-separated integers */
   if (sscanf(buf, "%lf,%lf,%lf%c", &dr, &dg, &db, &c) == 3) {
      *r = dr;
      *g = dg;
      *b = db;
      if (*r>=0 && *r<=65535 && *g>=0 && *g<=65535 && *b>=0 && *b<=65535)
         return Succeeded;
      else
         return Failed;
      }

   /* try interpreting as a hexadecimal value */
   if (*buf == '#') {
      buf++;
      for (len = 0; isalnum(buf[len]); len++);
      switch (len) {
         case  3:  fmt = "%1x%1x%1x%c";  mul = 0x1111;  break;
         case  6:  fmt = "%2x%2x%2x%c";  mul = 0x0101;  break;
         case  9:  fmt = "%3x%3x%3x%c";  mul = 0x0010;  break;
         case 12:  fmt = "%4x%4x%4x%c";  mul = 0x0001;  break;
         default:  return Failed;
      }
      if (sscanf(buf, fmt, r, g, b, &c) != 3)
         return Failed;
      *r *= mul;
      *g *= mul;
      *b *= mul;
      return Succeeded;
      }

   /* try interpreting as a color phrase or as a native color spec */
   if (colorphrase(buf, r, g, b) || nativecolor(w, buf, r, g, b))
      return Succeeded;
   else
      return Failed;
   }

/*
 *  colorphrase(s, &r, &g, &b) -- parse Icon color phrase.
 *
 *  An Icon color phrase matches the pattern
 *
 *                               weak
 *                  pale         moderate
 *                  light        strong
 *          [[very] medium ]   [ vivid    ]   [color[ish]]   color
 *                  dark
 *                  deep
 *
 *  where "color" is any of:
 *
 *          black gray grey white pink violet brown
 *          red orange yellow green cyan blue purple magenta
 *
 *  A single space or hyphen separates each word from its neighbor.  The
 *  default lightness is "medium", and the default saturation is "vivid".
 *
 *  "pale" means "very light"; "deep" means "very dark".
 *
 *  This naming scheme is based loosely on
 *	A New Color-Naming System for Graphics Languages
 *	Toby Berk, Lee Brownston, and Arie Kaufman
 *	IEEE Computer Graphics & Applications, May 1982
 */

static int colorphrase(buf, r, g, b)
char *buf;
long *r, *g, *b;
   {
   int len, very;
   char c, *p, *ebuf, cbuffer[MAXCOLORNAME];
   float lgt, sat, blend, bl2, m1, m2;
   float h1, l1, s1, h2, l2, s2, r2, g2, b2;

   lgt = -1.0;				/* default no lightness mod */
   sat =  1.0;				/* default vivid saturation */
   len = strlen(buf);
   while (isspace(buf[len-1]))
      len--;				/* trim trailing spaces */

   if (len >= sizeof(cbuffer))
      return 0;				/* if too long for valid Icon spec */

   /*
    * copy spec, lowering case and replacing spaces and hyphens with NULs
    */
   for(p = cbuffer; (c = *buf) != 0; p++, buf++) {
      if (isupper(c)) *p = tolower(c);
      else if (c == ' ' || c == '-') *p = '\0';
      else *p = c;
      }
   *p = '\0';

   buf = cbuffer;
   ebuf = buf + len;
   /* check for "very" */
   if (strcmp(buf, "very") == 0) {
      very = 1;
      buf += strlen(buf) + 1;
      if (buf >= ebuf)
         return 0;
      }
   else
      very = 0;

   /* check for lightness adjective */
   p = qsearch(buf, (char *)lighttable,
      ElemCount(lighttable), ElemSize(lighttable), strcmp);
   if (p) {
      /* set the "very" flag for "pale" or "deep" */
      if (strcmp(buf, "pale") == 0)
         very = 1;			/* pale = very light */
      else if (strcmp(buf, "deep") == 0)
         very = 1;			/* deep = very dark */
      /* skip past word */
      buf += strlen(buf) + 1;
      if (buf >= ebuf)
         return 0;
      /* save lightness value, but ignore "medium" */
      if ((((colrmod *)p) -> val) != 50)
         lgt = ((colrmod *)p) -> val / 100.0;
      }
   else if (very)
      return 0;

   /* check for saturation adjective */
   p = qsearch(buf, (char *)sattable,
      ElemCount(sattable), ElemSize(sattable), strcmp);
   if (p) {
      sat = ((colrmod *)p) -> val / 100.0;
      buf += strlen(buf) + 1;
      if (buf >= ebuf)
         return 0;
      }

   if (buf + strlen(buf) >= ebuf)
      blend = h1 = l1 = s1 = 0.0;	/* only one word left */
   else {
      /* we have two (or more) name words; get the first */
      if ((p = qsearch(buf, colortable[0].name,
            ElemCount(colortable), ElemSize(colortable), strcmp)) != NULL) {
         blend = 0.5;
         }
      else if ((p = qsearch(buf, colortable[0].ish,
            ElemCount(colortable), ElemSize(colortable), strcmp)) != NULL) {
         p -= sizeof(colortable[0].name);
         blend = 0.25;
         }
      else
         return 0;

      h1 = ((colrname *)p) -> hue;
      l1 = ((colrname *)p) -> lgt / 100.0;
      s1 = ((colrname *)p) -> sat / 100.0;
      buf += strlen(buf) + 1;
      }

   /* process second (or only) name word */
   p = qsearch(buf, colortable[0].name,
      ElemCount(colortable), ElemSize(colortable), strcmp);
   if (!p || buf + strlen(buf) < ebuf)
      return 0;
   h2 = ((colrname *)p) -> hue;
   l2 = ((colrname *)p) -> lgt / 100.0;
   s2 = ((colrname *)p) -> sat / 100.0;

   /* at this point we know we have a valid spec */

   /* interpolate hls specs */
   if (blend > 0) {
      bl2 = 1.0 - blend;

      if (s1 == 0.0)
         ; /* use h2 unchanged */
      else if (s2 == 0.0)
         h2 = h1;
      else if (h2 - h1 > 180)
         h2 = blend * h1 + bl2 * (h2 - 360);
      else if (h1 - h2 > 180)
         h2 = blend * (h1 - 360) + bl2 * h2;
      else
         h2 = blend * h1 + bl2 * h2;
      if (h2 < 0)
         h2 += 360;

      l2 = blend * l1 + bl2 * l2;
      s2 = blend * s1 + bl2 * s2;
      }

   /* apply saturation and lightness modifiers */
   if (lgt >= 0.0) {
      if (very)
         l2 = (2 * lgt + l2) / 3.0;
      else
         l2 = (lgt + 2 * l2) / 3.0;
      }
   s2 *= sat;

   /* convert h2,l2,s2 to r2,g2,b2 */
   /* from Foley & Van Dam, 1st edition, p. 619 */
   /* beware of dangerous typos in 2nd edition */
   if (s2 == 0)
      r2 = g2 = b2 = l2;
   else {
      if (l2 < 0.5)
         m2 = l2 * (1 + s2);
      else
         m2 = l2 + s2 - l2 * s2;
      m1 = 2 * l2 - m2;
      r2 = rgbval(m1, m2, h2 + 120);
      g2 = rgbval(m1, m2, h2);
      b2 = rgbval(m1, m2, h2 - 120);
      }

   /* scale and convert the calculated result */
   *r = 65535 * r2;
   *g = 65535 * g2;
   *b = 65535 * b2;

   return 1;
   }

/*
 * rgbval(n1, n2, hue) - helper function for HLS to RGB conversion
 */
static double rgbval(n1, n2, hue)
double n1, n2, hue;
   {
   if (hue > 360)
      hue -= 360;
   else if (hue < 0)
      hue += 360;

   if (hue < 60)
      return n1 + (n2 - n1) * hue / 60.0;
   else if (hue < 180)
      return n2;
   else if (hue < 240)
      return n1 + (n2 - n1) * (240 - hue) / 60.0;
   else
      return n1;
   }

/*
 * Static data for XDrawImage and XPalette functions
 */

/*
 * c<n>list - the characters of the palettes that are not contiguous ASCII
 */
char c1list[] = "0123456789?!nNAa#@oOBb$%pPCc&|\
qQDd,.rREe;:sSFf+-tTGg*/uUHh`'vVIi<>wWJj()xXKk[]yYLl{}zZMm^=";
char c2list[] = "kbgcrmywx";
char c3list[] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZabcd";
char c4list[] =
   "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz{}$%&*+-/?@";

/*
 * cgrays -- lists of grayscales contained within color palettes
 */
static char *cgrays[] = { "0123456", "kxw", "@abMcdZ", "0$%&L*+-g/?@}",
"\0}~\177\200\37\201\202\203\204>\205\206\207\210]\211\212\213\214|",
"\0\330\331\332\333\334+\335\336\337\340\341V\342\343\344\345\346\201\
\347\350\351\352\353\254\354\355\356\357\360\327" };

/*
 * c1cube - a precomputed mapping from a color cube to chars in c1 palette
 *
 * This is 10x10x10 cube (A Thousand Points of Light).
 */
#define C1Side 10			/* length of one side of C1 cube */
static char c1cube[] = {
   '0', '0', 'w', 'w', 'w', 'W', 'W', 'W', 'J', 'J', '0', '0', 'v', 'v', 'v',
   'W', 'W', 'W', 'J', 'J', 's', 't', 't', 'v', 'v', 'V', 'V', 'V', 'V', 'J',
   's', 't', 't', 'u', 'u', 'V', 'V', 'V', 'V', 'I', 's', 't', 't', 'u', 'u',
   'V', 'V', 'V', 'I', 'I', 'S', 'S', 'T', 'T', 'T', 'U', 'U', 'U', 'I', 'I',
   'S', 'S', 'T', 'T', 'T', 'U', 'U', 'U', 'U', 'I', 'S', 'S', 'T', 'T', 'T',
   'U', 'U', 'U', 'U', 'H', 'F', 'F', 'T', 'T', 'G', 'G', 'U', 'U', 'H', 'H',
   'F', 'F', 'F', 'G', 'G', 'G', 'G', 'H', 'H', 'H', '0', '0', 'x', 'x', 'x',
   'W', 'W', 'W', 'J', 'J', '!', '1', '1', 'v', 'v', 'W', 'W', 'W', 'J', 'J',
   'r', '1', '1', 'v', 'v', 'V', 'V', 'V', 'j', 'j', 'r', 'r', 't', 'u', 'u',
   'V', 'V', 'V', 'j', 'j', 'r', 'r', 't', 'u', 'u', 'V', 'V', 'V', 'I', 'I',
   'S', 'S', 'T', 'T', 'T', 'U', 'U', 'U', 'I', 'I', 'S', 'S', 'T', 'T', 'T',
   'U', 'U', 'U', 'i', 'i', 'S', 'S', 'T', 'T', 'T', 'U', 'U', 'U', 'i', 'i',
   'F', 'F', 'f', 'f', 'G', 'G', 'g', 'g', 'H', 'H', 'F', 'F', 'f', 'f', 'G',
   'G', 'g', 'g', 'H', 'H', 'n', 'z', 'x', 'x', 'x', 'X', 'X', 'X', 'X', 'J',
   '!', '1', '1', 'x', 'x', 'X', 'X', 'X', 'j', 'j', 'p', '1', '1', '2', '2',
   ')', 'V', 'j', 'j', 'j', 'r', 'r', '2', '2', '2', ')', 'V', 'j', 'j', 'j',
   'r', 'r', '2', '2', '2', '>', '>', '>', 'j', 'j', 'R', 'R', '-', '-', '/',
   '/', '>', '>', 'i', 'i', 'R', 'R', 'R', 'T', '/', '/', '\'','i', 'i', 'i',
   'R', 'R', 'f', 'f', '/', '/', 'g', 'g', 'i', 'i', 'R', 'f', 'f', 'f', 'f',
   'g', 'g', 'g', 'h', 'h', 'F', 'f', 'f', 'f', 'f', 'g', 'g', 'g', 'h', 'h',
   'n', 'z', 'z', 'y', 'y', 'X', 'X', 'X', 'X', 'K', 'o', 'o', 'z', 'y', 'y',
   'X', 'X', 'X', 'j', 'j', 'p', 'p', '2', '2', '2', ')', 'X', 'j', 'j', 'j',
   'q', 'q', '2', '2', '2', ')', ')', 'j', 'j', 'j', 'q', 'q', '2', '2', '2',
   '>', '>', '>', 'j', 'j', 'R', 'R', '-', '-', '/', '/', '>', '>', 'i', 'i',
   'R', 'R', 'R', '-', '/', '/', '\'','\'','i', 'i', 'R', 'R', 'f', 'f', '/',
   '/', '\'','g', 'i', 'i', 'R', 'f', 'f', 'f', 'f', 'g', 'g', 'g', 'h', 'h',
   'E', 'f', 'f', 'f', 'f', 'g', 'g', 'g', 'h', 'h', 'n', 'z', 'z', 'y', 'y',
   'X', 'X', 'X', 'K', 'K', 'o', 'o', 'z', 'y', 'y', 'X', 'X', 'X', 'K', 'K',
   '?', '?', '?', '2', '2', ']', ']', ']', 'j', 'j', 'q', 'q', '2', '2', '2',
   ']', ']', ']', 'j', 'j', 'q', 'q', '2', '2', '3', '3', '>', '>', 'j', 'j',
   'R', 'R', ':', ':', '3', '3', '>', '>', 'i', 'i', 'R', 'R', ':', ':', ':',
   '/', '\'','\'','i', 'i', 'R', 'R', ':', ':', ':', '/', '\'','\'','i', 'i',
   'E', 'E', 'f', 'f', 'f', 'g', 'g', 'g', 'h', 'h', 'E', 'E', 'f', 'f', 'f',
   'g', 'g', 'g', 'h', 'h', 'N', 'N', 'Z', 'Z', 'Z', 'Y', 'Y', 'Y', 'K', 'K',
   'O', 'O', 'Z', 'Z', 'Z', 'Y', 'Y', 'Y', 'K', 'K', '?', '?', '?', '@', '=',
   ']', ']', ']', 'k', 'k', 'P', 'P', '@', '@', '=', ']', ']', ']', 'k', 'k',
   'P', 'P', '%', '%', '%', '3', ']', ']', 'k', 'k', 'Q', 'Q', '|', '|', '3',
   '3', '4', '4', '(', '(', 'Q', 'Q', ':', ':', ':', '4', '4', '4', '(', '(',
   'Q', 'Q', ':', ':', ':', '4', '4', '4', '<', '<', 'E', 'E', 'e', 'e', 'e',
   '+', '+', '*', '*', '<', 'E', 'E', 'e', 'e', 'e', '+', '+', '*', '*', '`',
   'N', 'N', 'Z', 'Z', 'Z', 'Y', 'Y', 'Y', 'Y', 'K', 'O', 'O', 'Z', 'Z', 'Z',
   'Y', 'Y', 'Y', 'k', 'k', 'O', 'O', 'O', 'Z', '=', '=', '}', 'k', 'k', 'k',
   'P', 'P', 'P', '@', '=', '=', '}', '}', 'k', 'k', 'P', 'P', '%', '%', '%',
   '=', '}', '}', 'k', 'k', 'Q', 'Q', '|', '|', '|', '4', '4', '4', '(', '(',
   'Q', 'Q', '.', '.', '.', '4', '4', '4', '(', '(', 'Q', 'Q', 'e', '.', '.',
   '4', '4', '4', '<', '<', 'Q', 'e', 'e', 'e', 'e', '+', '+', '*', '*', '<',
   'E', 'e', 'e', 'e', 'e', '+', '+', '*', '*', '`', 'N', 'N', 'Z', 'Z', 'Z',
   'Y', 'Y', 'Y', 'Y', 'L', 'O', 'O', 'Z', 'Z', 'Z', 'Y', 'Y', 'Y', 'k', 'k',
   'O', 'O', 'O', 'a', '=', '=', 'm', 'k', 'k', 'k', 'P', 'P', 'a', 'a', '=',
   '=', '}', 'k', 'k', 'k', 'P', 'P', '%', '%', '%', '=', '}', '8', '8', '8',
   'Q', 'Q', '|', '|', '|', '4', '4', '8', '8', '8', 'Q', 'Q', 'c', '.', '.',
   '4', '4', '4', '[', '[', 'Q', 'Q', 'c', 'c', '9', '9', '4', '5', '5', '<',
   'Q', 'e', 'e', 'e', 'e', ';', ';', '5', '5', '<', 'D', 'e', 'e', 'e', 'e',
   ';', ';', ';', '*', '`', 'A', 'A', 'Z', 'Z', 'M', 'M', 'Y', 'Y', 'L', 'L',
   'A', 'A', 'a', 'a', 'M', 'M', 'm', 'm', 'L', 'L', 'B', 'B', 'a', 'a', 'a',
   'm', 'm', 'm', 'l', 'l', 'B', 'B', 'a', 'a', 'a', 'm', 'm', 'm', 'l', 'l',
   'C', 'C', 'b', 'b', 'b', '7', '7', '7', '8', '8', 'C', 'C', 'b', 'b', 'b',
   '7', '7', '^', '[', '[', 'Q', 'c', 'c', 'c', 'c', '#', '#', '^', '[', '[',
   'Q', 'c', 'c', 'c', '9', '9', '$', '5', '5', '[', 'D', 'D', 'd', 'd', '9',
   '&', '&', '5', '5', '6', 'D', 'D', 'd', 'd', 'd', ';', ';', ';', '6', '6',
   'A', 'A', 'A', 'M', 'M', 'M', 'M', 'L', 'L', 'L', 'A', 'A', 'a', 'a', 'M',
   'M', 'm', 'm', 'L', 'L', 'B', 'B', 'a', 'a', 'a', 'm', 'm', 'm', 'l', 'l',
   'B', 'B', 'a', 'a', 'a', 'm', 'm', 'm', 'l', 'l', 'C', 'C', 'b', 'b', 'b',
   '7', '7', '7', 'l', 'l', 'C', 'C', 'b', 'b', 'b', '7', '7', '^', '^', '{',
   'C', 'c', 'c', 'c', 'c', '#', '#', '^', '^', '{', 'D', 'c', 'c', 'c', '9',
   '9', '$', '$', '^', '{', 'D', 'D', 'd', 'd', '9', '&', '&', '&', '6', '6',
   'D', 'D', 'd', 'd', 'd', ',', ',', ',', '6', '6'
};

/*
 * c1rgb - RGB values for c1 palette entries
 *
 * Entry order corresponds to c1list (above).
 * Each entry gives r,g,b in linear range 0 to 48.
 */
static unsigned char c1rgb[] = {
   0, 0, 0,		/*  0             black		*/
   8, 8, 8,		/*  1   very dark gray		*/
   16, 16, 16,		/*  2        dark gray		*/
   24, 24, 24,		/*  3             gray		*/
   32, 32, 32,		/*  4       light gray		*/
   40, 40, 40,		/*  5  very light gray		*/
   48, 48, 48,		/*  6             white		*/
   48, 24, 30,		/*  7             pink		*/
   36, 24, 48,		/*  8             violet	*/
   48, 36, 24,		/*  9  very light brown		*/
   24, 12, 0,		/*  ?             brown		*/
   8, 4, 0,		/*  !   very dark brown		*/
   16, 0, 0,		/*  n   very dark red		*/
   32, 0, 0,		/*  N        dark red		*/
   48, 0, 0,		/*  A             red		*/
   48, 16, 16,		/*  a       light red		*/
   48, 32, 32,		/*  #  very light red		*/
   30, 18, 18,		/*  @        weak red		*/
   16, 4, 0,		/*  o   very dark orange	*/
   32, 8, 0,		/*  O        dark orange	*/
   48, 12, 0,		/*  B             orange	*/
   48, 24, 16,		/*  b       light orange	*/
   48, 36, 32,		/*  $  very light orange	*/
   30, 21, 18,		/*  %        weak orange	*/
   16, 8, 0,		/*  p   very dark red-yellow	*/
   32, 16, 0,		/*  P        dark red-yellow	*/
   48, 24, 0,		/*  C             red-yellow	*/
   48, 32, 16,		/*  c       light red-yellow	*/
   48, 40, 32,		/*  &  very light red-yellow	*/
   30, 24, 18,		/*  |        weak red-yellow	*/
   16, 16, 0,		/*  q   very dark yellow	*/
   32, 32, 0,		/*  Q        dark yellow	*/
   48, 48, 0,		/*  D             yellow	*/
   48, 48, 16,		/*  d       light yellow	*/
   48, 48, 32,		/*  ,  very light yellow	*/
   30, 30, 18,		/*  .        weak yellow	*/
   8, 16, 0,		/*  r   very dark yellow-green	*/
   16, 32, 0,		/*  R        dark yellow-green	*/
   24, 48, 0,		/*  E             yellow-green	*/
   32, 48, 16,		/*  e       light yellow-green	*/
   40, 48, 32,		/*  ;  very light yellow-green	*/
   24, 30, 18,		/*  :        weak yellow-green	*/
   0, 16, 0,		/*  s   very dark green		*/
   0, 32, 0,		/*  S        dark green		*/
   0, 48, 0,		/*  F             green		*/
   16, 48, 16,		/*  f       light green		*/
   32, 48, 32,		/*  +  very light green		*/
   18, 30, 18,		/*  -        weak green		*/
   0, 16, 8,		/*  t   very dark cyan-green	*/
   0, 32, 16,		/*  T        dark cyan-green	*/
   0, 48, 24,		/*  G             cyan-green	*/
   16, 48, 32,		/*  g       light cyan-green	*/
   32, 48, 40,		/*  *  very light cyan-green	*/
   18, 30, 24,		/*  /        weak cyan-green	*/
   0, 16, 16,		/*  u   very dark cyan		*/
   0, 32, 32,		/*  U        dark cyan		*/
   0, 48, 48,		/*  H             cyan		*/
   16, 48, 48,		/*  h       light cyan		*/
   32, 48, 48,		/*  `  very light cyan		*/
   18, 30, 30,		/*  '        weak cyan		*/
   0, 8, 16,		/*  v   very dark blue-cyan	*/
   0, 16, 32,		/*  V        dark blue-cyan	*/
   0, 24, 48,		/*  I             blue-cyan	*/
   16, 32, 48,		/*  i       light blue-cyan	*/
   32, 40, 48,		/*  <  very light blue-cyan	*/
   18, 24, 30,		/*  >        weak blue-cyan	*/
   0, 0, 16,		/*  w   very dark blue		*/
   0, 0, 32,		/*  W        dark blue		*/
   0, 0, 48,		/*  J             blue		*/
   16, 16, 48,		/*  j       light blue		*/
   32, 32, 48,		/*  (  very light blue		*/
   18, 18, 30,		/*  )        weak blue		*/
   8, 0, 16,		/*  x   very dark purple	*/
   16, 0, 32,		/*  X        dark purple	*/
   24, 0, 48,		/*  K             purple	*/
   32, 16, 48,		/*  k       light purple	*/
   40, 32, 48,		/*  [  very light purple	*/
   24, 18, 30,		/*  ]        weak purple	*/
   16, 0, 16,		/*  y   very dark magenta	*/
   32, 0, 32,		/*  Y        dark magenta	*/
   48, 0, 48,		/*  L             magenta	*/
   48, 16, 48,		/*  l       light magenta	*/
   48, 32, 48,		/*  {  very light magenta	*/
   30, 18, 30,		/*  }        weak magenta	*/
   16, 0, 8,		/*  z   very dark magenta-red	*/
   32, 0, 16,		/*  Z        dark magenta-red	*/
   48, 0, 24,		/*  M             magenta-red	*/
   48, 16, 32,		/*  m       light magenta-red	*/
   48, 32, 40,		/*  ^  very light magenta-red	*/
   30, 18, 24,		/*  =        weak magenta-red	*/
   };

/*
 * palnum(d) - return palette number, or 0 if unrecognized.
 *
 *    returns +1 ... +6 for "c1" through "c6"
 *    returns +1 for &null
 *    returns -2 ... -256 for "g2" through "g256"
 *    returns 0 for unrecognized palette name
 *    returns -1 for non-string argument
 */
int palnum(d)
dptr d;
   {
   tended char *s;
   char c, x;
   int n;

   if (is:null(*d))
      return 1;
   if (!cnv:C_string(*d, s))
      return -1;
   if (sscanf(s, "%c%d%c", &c, &n, &x) != 2)
      return 0;
   if (c == 'c' && n >= 1 && n <= 6)
      return n;
   if (c == 'g' && n >= 2 && n <= 256)
      return -n;
   return 0;
   }


struct palentry *palsetup_palette;	/* current palette */

/*
 * palsetup(p) - set up palette for specified palette.
 */
struct palentry *palsetup(p)
int p;
   {
   int r, g, b, i, n, c;
   unsigned int rr, gg, bb;
   unsigned char *s = NULL, *t;
   double m;
   struct palentry *e;
   static int palnumber;		/* current palette number */

   if (palnumber == p)
      return palsetup_palette;
   if (palsetup_palette == NULL) {
      palsetup_palette =
         (struct palentry *)malloc(256 * sizeof(struct palentry));
      if (palsetup_palette == NULL)
         return NULL;
      }
   palnumber = p;

   for (i = 0; i < 256; i++)
      palsetup_palette[i].valid = palsetup_palette[i].transpt = 0;
   palsetup_palette[TCH1].transpt = 1;
   palsetup_palette[TCH2].transpt = 1;

   if (p < 0) {				/* grayscale palette */
      n = -p;
      if (n <= 64)
         s = (unsigned char *)c4list;
      else
         s = allchars;
      m = 1.0 / (n - 1);

      for (i = 0; i < n; i++) {
         e = &palsetup_palette[*s++];
         gg = 65535 * m * i;
         e->clr.red = e->clr.green = e->clr.blue = gg;
         e->valid = 1;
	 e->transpt = 0;
         }
      return palsetup_palette;
      }

   if (p == 1) {			/* special c1 palette */
      s = (unsigned char *)c1list;
      t = c1rgb;
      while ((c = *s++) != 0) {
         e = &palsetup_palette[c];
         e->clr.red   = 65535 * (((int)*t++) / 48.0);
         e->clr.green = 65535 * (((int)*t++) / 48.0);
         e->clr.blue  = 65535 * (((int)*t++) / 48.0);
         e->valid = 1;
	 e->transpt = 0;
         }
      return palsetup_palette;
      }

   switch (p) {				/* color cube plus extra grays */
      case  2:  s = (unsigned char *)c2list;	break;	/* c2 */
      case  3:  s = (unsigned char *)c3list;	break;	/* c3 */
      case  4:  s = (unsigned char *)c4list;	break;	/* c4 */
      case  5:  s = allchars;			break;	/* c5 */
      case  6:  s = allchars;			break;	/* c6 */
      }
   m = 1.0 / (p - 1);
   for (r = 0; r < p; r++) {
      rr = 65535 * m * r;
      for (g = 0; g < p; g++) {
         gg = 65535 * m * g;
         for (b = 0; b < p; b++) {
            bb = 65535 * m * b;
            e = &palsetup_palette[*s++];
            e->clr.red = rr;
            e->clr.green = gg;
            e->clr.blue = bb;
            e->valid = 1;
	    e->transpt = 0;
            }
         }
      }
   m = 1.0 / (p * (p - 1));
   for (g = 0; g < p * (p - 1); g++)
      if (g % p != 0) {
         gg = 65535 * m * g;
         e = &palsetup_palette[*s++];
         e->clr.red = e->clr.green = e->clr.blue = gg;
         e->valid = 1;
	 e->transpt = 0;
         }
   return palsetup_palette;
   }

/*
 * rgbkey(p,r,g,b) - return pointer to key of closest color in palette number p.
 *
 * In color cubes, finds "extra" grays only if r == g == b.
 */
char *rgbkey(p, r, g, b)
int p;
double r, g, b;
   {
   int n, i;
   double m;
   char *s;

   if (p > 0) {				/* color */
      if (r == g && g == b) {
         if (p == 1)
            m = 6;
         else
            m = p * (p - 1);
         return cgrays[p - 1] + (int)(0.501 + m * g);
         }
      else {
         if (p == 1)
            n = C1Side;
         else
            n = p;
         m = n - 1;
         i = (int)(0.501 + m * r);
         i = n * i + (int)(0.501 + m * g);
         i = n * i + (int)(0.501 + m * b);
         switch(p) {
            case  1:  return c1cube + i;		/* c1 */
            case  2:  return c2list + i;		/* c2 */
            case  3:  return c3list + i;		/* c3 */
            case  4:  return c4list + i;		/* c4 */
            case  5:  return (char *)allchars + i;	/* c5 */
            case  6:  return (char *)allchars + i;	/* c6 */
            }
         }
      }
   else {				/* grayscale */
      if (p < -64)
         s = (char *)allchars;
      else
         s = c4list;
      return s + (int)(0.5 + (0.299 * r + 0.587 * g + 0.114 * b) * (-p - 1));
      }

   /*NOTREACHED*/
   return 0;  /* avoid gcc warning */
   }

#else					/* Graphics */

/*
 * Stubs to prevent dynamic loader from rejecting cfunc library of IPL.
 */
int palnum(dptr *d)					{ return 0; }
char *rgbkey(int p, double r, double g, double b)	{ return 0; }

#endif					/* Graphics */
