/* xpmtoppm.c - read an X11 pixmap file and produce a portable pixmap
**
** Copyright (C) 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Upgraded to support XPM version 3 by
**   Arnaud Le Hors (lehors@mirsa.inria.fr)
**   Tue Apr 9 1991
**
** Rainer Sinkwitz sinkwitz@ifi.unizh.ch - 21 Nov 91:
**  - Bug fix, no advance of read ptr, would not read 
**    colors like "ac c black" because it would find 
**    the "c" of "ac" and then had problems with "c"
**    as color.
**    
**  - Now understands multword X11 color names
**  
**  - Now reads multiple color keys. Takes the color
**    of the hightest available key. Lines no longer need
**    to begin with key 'c'.
**    
**  - expanded line buffer to from 500 to 2048 for bigger files
*/

#include "ppm.h"

void ReadXPMFile();
static void getline();

/* number of xpmColorKeys */
#define NKEYS 5

char *xpmColorKeys[] =
{
 "s",					/* key #1: symbol */
 "m",					/* key #2: mono visual */
 "g4",					/* key #3: 4 grays visual */
 "g",					/* key #4: gray visual */
 "c",					/* key #5: color visual */
};

#ifdef NEED_STRSTR
/* for systems which do not provide it */
static char *
strstr(s1, s2)
    char *s1, *s2;
{
    int ls2 = strlen(s2);

    if (ls2 == 0)
	return (s1);
    while (strlen(s1) >= ls2) {
	if (strncmp(s1, s2, ls2) == 0)
	    return (s1);
	s1++;
    }
    return (0);
}

#endif

void
main(argc, argv)
    int argc;
    char *argv[];

{
    FILE *ifp;
    pixel *pixrow, *colors;
    register pixel *pP;
    int rows, cols, ncolors, chars_per_pixel, row;
    register int col;
    int *data;
    register int *ptr;

    ppm_init(&argc, argv);

    if (argc > 2)
	pm_usage("[xpmfile]");

    if (argc == 2)
	ifp = pm_openr(argv[1]);
    else
	ifp = stdin;

    ReadXPMFile(
	    ifp, &cols, &rows, &ncolors, &chars_per_pixel, &colors, &data);

    pm_close(ifp);

    ppm_writeppminit(stdout, cols, rows, (pixval) PPM_MAXMAXVAL, 0);
    pixrow = ppm_allocrow(cols);

    for (row = 0, ptr = data; row < rows; ++row) {
	for (col = 0, pP = pixrow; col < cols; ++col, ++pP, ++ptr)
	    *pP = colors[*ptr];
	ppm_writeppmrow(stdout, pixrow, cols, (pixval) PPM_MAXMAXVAL, 0);
    }

    exit(0);
}

#define MAX_LINE 2048

void
ReadXPMFile(stream, widthP, heightP, ncolorsP,
	    chars_per_pixelP, colorsP, dataP)
    FILE *stream;
    int *widthP;
    int *heightP;
    int *ncolorsP;
    int *chars_per_pixelP;
    pixel **colorsP;
    int **dataP;
{
    char line[MAX_LINE], str1[MAX_LINE], str2[MAX_LINE];
    char *t1;
    char *t2;
    int format, v, datasize;
    int *ptr;
    int *ptab;
    register int i, j;
    int flag;

    unsigned int curkey, key, highkey;	/* current color key */
    unsigned int lastwaskey;		/* key read */
    char curbuf[BUFSIZ];		/* current buffer */

    *widthP = *heightP = *ncolorsP = *chars_per_pixelP = format = -1;
    flag = 0;				/* to avoid getting twice a line */

    /* First try to read as an XPM version 3 file */

    /* Read the header line */
    getline(line, sizeof(line), stream);
    if (sscanf(line, "/* %s */", str1) == 1
	&& !strncmp(str1, "XPM", 3)) {

	/* Read the assignment line */
	getline(line, sizeof(line), stream);
	if (strncmp(line, "static char", 11))
	    pm_error("error scanning assignment line", 0, 0, 0, 0, 0);

	/* Read the hints line */
	getline(line, sizeof(line), stream);
	/* skip the comment line if any */
	if (!strncmp(line, "/*", 2)) {
	    while (!strstr(line, "*/"))
		getline(line, sizeof(line), stream);
	    getline(line, sizeof(line), stream);
	}
	if (sscanf(line, "\"%d %d %d %d\",", widthP, heightP,
		   ncolorsP, chars_per_pixelP) != 4)
	    pm_error("error scanning hints line", 0, 0, 0, 0, 0);

	/* Allocate space for color table. */
	if (*chars_per_pixelP <= 2) {
	    /* Up to two chars per pixel, we can use an indexed table. */
	    v = 1;
	    for (i = 0; i < *chars_per_pixelP; ++i)
		v *= 256;
	    *colorsP = ppm_allocrow(v);
	} else {
	    /* Over two chars per pixel, we fall back on linear search. */
	    *colorsP = ppm_allocrow(*ncolorsP);
	    ptab = (int *) malloc(*ncolorsP * sizeof(int));
	}

	/* Read the color table */
	for (i = 0; i < *ncolorsP; i++) {
	    getline(line, sizeof(line), stream);
	    /* skip the comment line if any */
	    if (!strncmp(line, "/*", 2))
		getline(line, sizeof(line), stream);

	    /* read the chars */
	    if ((t1 = index(line, '"')) == NULL)
		pm_error("error scanning color table", 0, 0, 0, 0, 0);
	    else
		t1++;
	    strncpy(str1, t1, *chars_per_pixelP);
	    str1[*chars_per_pixelP] = '\0';
	    t1++; t1++;

	    v = 0;
	    for (j = 0; j < *chars_per_pixelP; ++j)
		v = (v << 8) + str1[j];
	    /*
	     * read color keys and values 
	     */
	    curkey = 0; 
	    highkey = 1;
	    lastwaskey = 0;
	    t2 = t1;
	    while ( 1 ) {
		for (t1=t2 ;; t1++)
		    if (*t1 != ' ' && *t1 != '	')
			break;
		for (t2 = t1;; t2++)
		    if (*t2 == ' ' || *t2 == '	' || *t2 == '"')
			break;
		if (t2 == t1) break;
		strncpy(str2, t1, t2 - t1);
		str2[t2 - t1] = '\0';
	        
		if (!lastwaskey) {
		    for (key = 1; key < NKEYS + 1; key++)
			if (!strcmp(xpmColorKeys[key - 1], str2))
			    break;
		} else 
		    key = NKEYS + 1;
		if (key > NKEYS) {			/* append name */
		    if (!curkey) 
			pm_error("error scanning color table", 0, 0, 0, 0, 0);
		    if (!lastwaskey) 
			strcat(curbuf, " ");		/* append space */
		    strcat(curbuf, str2);		/* append buf */
		    lastwaskey = 0;
		}
		if (key <= NKEYS) { 			/* new key */
		    if (curkey > highkey) {	/* flush string */
			if (*chars_per_pixelP <= 2)
			    /* Index into table. */
			    (*colorsP)[v] = ppm_parsecolor(curbuf,
						(pixval) PPM_MAXMAXVAL);
			else {
			    /* Set up linear search table. */
			    (*colorsP)[i] = ppm_parsecolor(curbuf,
						(pixval) PPM_MAXMAXVAL);
			    ptab[i] = v;
			}
			highkey = curkey;
		    }
		    curkey = key;			/* set new key  */
		    curbuf[0] = '\0';		/* reset curbuf */
		    lastwaskey = 1;
		}
	    if (*t2 == '"') break;
	    }
	    if (curkey > highkey) {
		if (*chars_per_pixelP <= 2)
		    /* Index into table. */
		    (*colorsP)[v] = ppm_parsecolor(curbuf,
					(pixval) PPM_MAXMAXVAL);
		else {
		    /* Set up linear search table. */
		    (*colorsP)[i] = ppm_parsecolor(curbuf,
					(pixval) PPM_MAXMAXVAL);
		    ptab[i] = v;
		}
		highkey = curkey;
	    }
	    if (highkey == 1) 
		pm_error("error scanning color table", 0, 0, 0, 0, 0);
	}
	/* Read pixels. */
	getline(line, sizeof(line), stream);
	/* skip the comment line if any */
	if (!strncmp(line, "/*", 2))
	    getline(line, sizeof(line), stream);

    } else {				/* try as an XPM version 1 file */

	/* Read the initial defines. */
	for (;;) {
	    if (flag)
		getline(line, sizeof(line), stream);
	    else
		flag++;

	    if (sscanf(line, "#define %s %d", str1, &v) == 2) {
		if ((t1 = rindex(str1, '_')) == NULL)
		    t1 = str1;
		else
		    ++t1;
		if (!strcmp(t1, "format"))
		    format = v;
		else if (!strcmp(t1, "width"))
		    *widthP = v;
		else if (!strcmp(t1, "height"))
		    *heightP = v;
		else if (!strcmp(t1, "ncolors"))
		    *ncolorsP = v;
		else if (!strcmp(t1, "pixel"))
		    *chars_per_pixelP = v;
	    } else if (!strncmp(line, "static char", 11)) {
		if ((t1 = rindex(line, '_')) == NULL)
		    t1 = line;
		else
		    ++t1;
		break;
	    }
	}
	if (format == -1)
	    pm_error("missing or invalid format", 0, 0, 0, 0, 0);
	if (format != 1)
	    pm_error("can't handle XPM version %d", format, 0, 0, 0, 0);
	if (*widthP == -1)
	    pm_error("missing or invalid width", 0, 0, 0, 0, 0);
	if (*heightP == -1)
	    pm_error("missing or invalid height", 0, 0, 0, 0, 0);
	if (*ncolorsP == -1)
	    pm_error("missing or invalid ncolors", 0, 0, 0, 0, 0);
	if (*chars_per_pixelP == -1)
	    pm_error("missing or invalid chars_per_pixel", 0, 0, 0, 0, 0);
	if (*chars_per_pixelP > 2)
	    pm_message("warning, chars_per_pixel > 2 uses a lot of memory"
		       ,0, 0, 0, 0, 0);

	/* If there's a monochrome color table, skip it. */
	if (!strncmp(t1, "mono", 4)) {
	    for (;;) {
		getline(line, sizeof(line), stream);
		if (!strncmp(line, "static char", 11))
		    break;
	    }
	}
	/* Allocate space for color table. */
	if (*chars_per_pixelP <= 2) {
	    /* Up to two chars per pixel, we can use an indexed table. */
	    v = 1;
	    for (i = 0; i < *chars_per_pixelP; ++i)
		v *= 256;
	    *colorsP = ppm_allocrow(v);
	} else {
	    /* Over two chars per pixel, we fall back on linear search. */
	    *colorsP = ppm_allocrow(*ncolorsP);
	    ptab = (int *) malloc(*ncolorsP * sizeof(int));
	}

	/* Read color table. */
	for (i = 0; i < *ncolorsP; ++i) {
	    getline(line, sizeof(line), stream);

	    if ((t1 = index(line, '"')) == NULL)
		pm_error("error scanning color table", 0, 0, 0, 0, 0);
	    if ((t2 = index(t1 + 1, '"')) == NULL)
		pm_error("error scanning color table", 0, 0, 0, 0, 0);
	    if (t2 - t1 - 1 != *chars_per_pixelP)
		pm_error("wrong number of chars per pixel in color table",
			 0, 0, 0, 0, 0);
	    strncpy(str1, t1 + 1, t2 - t1 - 1);
	    str1[t2 - t1 - 1] = '\0';

	    if ((t1 = index(t2 + 1, '"')) == NULL)
		pm_error("error scanning color table", 0, 0, 0, 0, 0);
	    if ((t2 = index(t1 + 1, '"')) == NULL)
		pm_error("error scanning color table", 0, 0, 0, 0, 0);
	    strncpy(str2, t1 + 1, t2 - t1 - 1);
	    str2[t2 - t1 - 1] = '\0';

	    v = 0;
	    for (j = 0; j < *chars_per_pixelP; ++j)
		v = (v << 8) + str1[j];
	    if (*chars_per_pixelP <= 2)
		/* Index into table. */
		(*colorsP)[v] = ppm_parsecolor(str2,
					       (pixval) PPM_MAXMAXVAL);
	    else {
		/* Set up linear search table. */
		(*colorsP)[i] = ppm_parsecolor(str2,
					       (pixval) PPM_MAXMAXVAL);
		ptab[i] = v;
	    }
	}

	/* Read pixels. */
	for (;;) {
	    getline(line, sizeof(line), stream);
	    if (!strncmp(line, "static char", 11))
		break;
	}
    }
    datasize = *widthP * *heightP;
    *dataP = (int *) malloc(datasize * sizeof(int));
    if (*dataP == 0)
	pm_error("out of memory", 0, 0, 0, 0, 0);
    i = 0;
    ptr = *dataP;
    for (;;) {
	if (flag)
	    getline(line, sizeof(line), stream);
	else
	    flag++;

	/* Find the open quote. */
	if ((t1 = index(line, '"')) == NULL)
	    pm_error("error scanning pixels", 0, 0, 0, 0, 0);
	++t1;

	/* Handle pixels until a close quote or the end of the image. */
	while (*t1 != '"') {
	    v = 0;
	    for (j = 0; j < *chars_per_pixelP; ++j)
		v = (v << 8) + *t1++;
	    if (*chars_per_pixelP <= 2)
		/* Index into table. */
		*ptr++ = v;
	    else {
		/* Linear search into table. */
		for (j = 0; j < *ncolorsP; ++j)
		    if (ptab[j] == v)
			goto gotit;
		pm_error("unrecognized pixel in line \"%s\"", line,
			 0, 0, 0, 0);
	gotit:
		*ptr++ = j;
	    }
	    ++i;
	    if (i >= datasize)
		return;
	}
    }
}


static void
getline(line, size, stream)
    char *line;
    int size;
    FILE *stream;
{
    if (fgets(line, MAX_LINE, stream) == NULL)
	pm_error("EOF / read error", 0, 0, 0, 0, 0);
    if (strlen(line) == MAX_LINE - 1)
	pm_error("line too long", 0, 0, 0, 0, 0);
}
