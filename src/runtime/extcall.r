/*
 * extcall.r
 */

/*  NOTE:  This file is considerably different for OS/2 than for other
 *  systems.  In the future, differences for other systems may appear.
 *  At present, all versions are being kept together in one file with
 *  conditionals.
 */

#if OS2
#if !COMPILER
#ifdef ExternalFunctions
/*
 */

struct os2xfunc {
    struct os2xfunc *next;
    unsigned long modhandle;
    char *funcname;
    char *modname;
    void (*func)();
};

static struct os2xfunc *xfunchead = NULL, *curxfunc;


#define compare(a,b) (StrLen(a) != sizeof(b) - 1 || \
		      memcmp(StrLoc(a), (b), sizeof(b) - 1))

#define comparev(a,b) (StrLen(a) != strlen((b)) || \
		       memcmp(StrLoc(a), (b), StrLen(a)))

static dptr loadfnc(dptr dargv, int argc, int *ip);
static dptr unloadfunc(dptr dargv, int argc, int *ip);

dptr extcall(dptr dargv, int argc, int *ip)
{
#passthru dptr (* _System func)(dptr dargv, int argc, int *ip);
    *ip = -1;

    if (!cnv_str(dargv, dargv) ) {  /* 1st argument must be a string */
	*ip = 103;		    /* "String expected" error */
	return dargv;
    }

    if (!compare(*dargv, "loadfunc"))   return loadfnc(dargv, argc,ip);
    if (!compare(*dargv, "unloadfunc")) return unloadfunc(dargv,argc,ip);

    for( curxfunc = xfunchead; curxfunc; curxfunc = curxfunc->next) {

	if (!comparev(*dargv,curxfunc->funcname)) {
	    func = curxfunc->func;
	    return (*func)(dargv, argc, ip);
	}

    }

    *ip = 216;			   /* external function not found */
    return NULL;
}

static dptr loadfnc(dptr dargv, int argc, int *ip)
{

    char modname[MaxCvtLen];
    char funcname[MaxCvtLen];

    unsigned long modhandle;
    struct os2xfunc *modptr = NULL;
    int rc;
#passthru dptr (* _System funcaddr)(dptr dargv, int argc, int *ip);


    if( argc < 3) {
	*ip = 103;
	return NULL;
    }

    ++dargv;
    if (!cnv_str(dargv,dargv)) {
	*ip = 103;
	return dargv;
    }
    if (!cnv_str(dargv+1,dargv+1)) {
	*ip = 103;
	return dargv+1;
    }
    strncpy(modname,StrLoc(*dargv),StrLen(*dargv));
    strncpy(funcname,StrLoc(*(dargv+1)),StrLen(*(dargv+1)));

    for( curxfunc = xfunchead; curxfunc; curxfunc->next ) {
	if( !comparev(*dargv,curxfunc->modname) ) {
	    modptr = curxfunc;
	    if ( !comparev(*(dargv+1),curxfunc->funcname )) break;
	}
    }
    if (curxfunc) {
	return &nulldesc;   /* Already loaded... */
    }
    if (!modptr) {
	rc = _loadmod(modname,&modhandle);
	if (rc) {
	    *ip = 216;
	    return dargv;
	}
    }
    else modhandle = modptr->modhandle;

    rc = DosQueryProcAddr( modhandle, 0, funcname, &funcaddr );
    if( rc ) {
	if( !modptr )
	    _freemod(modhandle);
	*ip = 216;
	return dargv+1;
    }

    modptr = malloc( sizeof(struct os2xfunc) );

    modptr->next = NULL;
    modptr->modhandle = modhandle;
    modptr->func = funcaddr;
    modptr->modname = strdup(modname);
    modptr->funcname = strdup(funcname);
    if( !xfunchead ) xfunchead = modptr;
    else {
	for(curxfunc=xfunchead; curxfunc->next; curxfunc=curxfunc->next);
	curxfunc->next = modptr;
    }
    return &nulldesc;
}

static dptr unloadfunc(dptr dargv, int argc, int *ip)
{
    char modname[MaxCvtLen];
    char funcname[MaxCvtLen];

    unsigned long modhandle;
    struct os2xfunc *modptr = NULL;
    int rc;


    if( argc < 2) {
	*ip = 103;
	return NULL;
    }

    ++dargv;
    if (!cnv_str(dargv,dargv)) {
	*ip = 103;
	return dargv;
    }

    for( curxfunc = xfunchead; curxfunc; curxfunc = curxfunc->next) {

	if (!comparev(*dargv,curxfunc->funcname)) break;

    }

    if (!curxfunc) return &nulldesc;	/* Just ignore not found */

    modptr = curxfunc;

    if( xfunchead == modptr )
	xfunchead = modptr->next;
    else {
	for( curxfunc = xfunchead; curxfunc; curxfunc = curxfunc->next) {
	    if(curxfunc->next = modptr) {
		curxfunc->next = modptr->next;
		break;
	    }
	}
	if(!curxfunc) return &nulldesc; /* ?? didn't find it 2nd time?? */
    }

    /* At this point the function has been removed from the chain */
    /* Run the chain one last time to see if we can free the module */

    for( curxfunc = xfunchead; curxfunc; curxfunc = curxfunc->next) {
	if(modptr->modhandle == curxfunc->modhandle) break;
    }
    if (curxfunc)
	_freemod(modptr->modhandle);

    free(modptr->modname);
    free(modptr->funcname);
    free(modptr);
    return &nulldesc;

}
#else					/* ExternalFunctions */
static char x;			/* prevent empty module */
#endif					/* ExternalFunctions */
#endif					/* !COMPILER */

#else					/* OS2 */

#if !COMPILER
#ifdef ExternalFunctions

/*
 * extcall - stub procedure for external call interface.
 */
dptr extcall(dargv, argc, ip)
dptr dargv;
int argc;
int *ip;
   {
   *ip = 216;			/* no external function to find */
   return (dptr)NULL;
   }

#else					/* ExternalFunctions */
static char x;			/* prevent empty module */
#endif 					/* ExternalFunctions */
#endif					/* !COMPILER */
#endif					/* OS2 */
