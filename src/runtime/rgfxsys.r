/*
 * File: rgfxsys.r
 *  Window-system-specific graphics support routines.
 *  This file simply includes an appropriate r*graph.ri file.
 */

#ifdef Graphics

#ifdef PresentationManager
#include "rpmgraph.ri"
#endif					/* PresentationManager */

#else					/* Graphics */
static char junk;		/* avoid empty module */
#endif					/* Graphics */
