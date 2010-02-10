
/*-----------------3/27/2007 11:23AM-----------------
 * loadable C function mysqldb for icon access to
 * a mySQL database from linux, by Carl Sturtivant.
 * (This also built on solaris.)
 *
 * This should be Garbage Collection safe except
 * under very extreme memory shortages.
 *
 * Requires a mySQL installation to build.
 * I used the following from bash:

CFG=/usr/bin/mysql_config
sh -c "gcc -shared -o mysqldb.so -fPIC `$CFG --cflags` mysqldb.c `$CFG --libs`"

 * for details about calling mysqldb, see below.
 * --------------------------------------------------*/

#include <stdio.h>
#include <string.h>

/* http://dev.mysql.com/doc/refman/5.0/en/c.html */
/* #include "/usr/include/mysql/mysql.h" */
#include <mysql.h>


#include "icall.h"


/* macros obtained by modifying some from icall.h */

#define Mkinteger(i, dp) \
do { (dp)->dword = D_Integer; (dp)->vword = (i); } while(0)

#define Mkstring(s, dp) \
do { word n = strlen(s); \
(dp)->dword = n; (dp)->vword = (word)alcstr(s,n); } while(0)

/* ensure that return to icon removes our tended descriptors from the list */
#define ReturnDescriptor(d) do { gcu_aware_pop(); return ( argv[0] = (d), 0 ); } while(0)
#define ReturnError(d, n) do { gcu_aware_pop(); return ( argv[0] = (d), n ); } while(0)


/****************start of Garbage Collection Utilities****************/

/* Structure for chaining descriptors to be tended properly by GC (rstructs.h) */
struct tend_desc {
   struct tend_desc *previous;
   int num;
   descriptor d[1]; /* actual size is in num */
};
typedef struct tend_desc gcu_tended;

/* global chain of such structures used by iconx (rinit.r) */
extern gcu_tended *tend;

/* int parameter to pass to gcu_initialize */
#define gcu_max(vars) ( (sizeof(vars) - sizeof(gcu_tended) )/sizeof(descriptor) )

/* initialize all descriptors to &null and assign the number */
static void gcu_initialize(int maxindex, void *descriptors) {
    int i;
    gcu_tended *desc = (gcu_tended *)descriptors;
    desc->num = maxindex+1;
    for( i = 0; i <= maxindex; ++i ) (desc->d)[i] = nulldesc;
}

/* add descriptors in a gcu_tended structure to the tended list */
static void gcu_aware_push(void *descriptors) {
    gcu_tended *desc = (gcu_tended *)descriptors;
    desc->previous = tend;
    tend = descriptors;
}

/* remove descriptors in a gcu_tended structure from the tended list */
static void gcu_aware_pop() {
    tend = tend->previous;
}

/****************end of Garbage Collection utilities****************/


/****************start of list utilities****************/

int Zlist(descriptor argv[]);   /* resolved in iconx: icon function list(i,X):L  */
int Osubsc(descriptor argv[]);  /* resolved in iconx: icon operator L[i]:v       */
int Oasgn(descriptor argv[]);   /* resolved in iconx: icon operator v:=X         */

typedef int (*iconfunction)(descriptor argv[]);

/* safely call an icon built-in function or operator with two arguments from C. */
static descriptor iconcall2(iconfunction F, descriptor x1, descriptor x2) {
    struct { /* structure like struct tend_desc with extra descriptors at the bottom */
        gcu_tended other; /* vital: used to chain onto the tend list */
        descriptor stack[3]; /* GC is aware of these once this struct is pushed onto the tend list */
    } tended;
    gcu_initialize( gcu_max(tended), &tended ); /* vital: call before icon may be made aware of this */
    gcu_aware_push( &tended ); /* GC is now aware of tended.stack */
    tended.stack[0] = nulldesc;
    tended.stack[1] = x1;
    tended.stack[2] = x2;
    F(tended.stack); /* No error handling for the uses below */
    gcu_aware_pop(); /* vital: GC is now unaware of tended.stack */
    return tended.stack[0];
}

/* returns list(n, &null) --- allocates memory */
static descriptor newlist(int length) {
    descriptor len;
    Mkinteger(length, &len);
    return iconcall2( &Zlist, len, nulldesc );
}

/* returns list[index] := value */
static descriptor assign(descriptor list, int index, descriptor value) {
    descriptor i;
    Mkinteger(index, &i);
    return iconcall2( &Oasgn, iconcall2(&Osubsc, list, i), value );
}

/* returns .list[index] */
static descriptor subscript(descriptor list, int index) {
    descriptor i, result;
    Mkinteger(index, &i);
    result = iconcall2(&Osubsc, list, i);
    /* result of an icon subscripting operation is a variable */
    deref(&result, &result); /* deref resolved in iconx */
    return result;
}

/****************end of list utilities****************/


/* make icon list of mysql error information */
static descriptor error_info(int mysqlNumber, const char * mysqlError) {
    descriptor number;
    struct {
        gcu_tended other;
        descriptor text, ls;
    } tended;
    gcu_initialize( gcu_max(tended), &tended );
    gcu_aware_push( &tended );
    tended.ls = newlist(2);
    Mkinteger(mysqlNumber, &number);
    Mkstring((char *)mysqlError, &tended.text);
    assign( tended.ls, 1, number );
    assign( tended.ls, 2, tended.text );
    gcu_aware_pop();
    return tended.ls;
}

/* make mySQL row retrieved from query results into icon list */
static descriptor convertrow(MYSQL_ROW row, int numfields) {
    int i;
    struct {
        gcu_tended other;
        descriptor x, ls;
    } tended;
    gcu_initialize( gcu_max(tended), &tended );
    gcu_aware_push( &tended );
    tended.ls = newlist(numfields);
    for( i = 1; i <= numfields; ++i ) {
        if( row[i-1] ) Mkstring( row[i-1], &tended.x );
        else tended.x = nulldesc;
        assign( tended.ls, i, tended.x );
    }
    gcu_aware_pop();
    return tended.ls;
}

/*--------------------------------------------------
 * Called with a list, mysqldb attempts to connect.
 * Only one database can be connected to at a time.
 * Needs the database name, username, password,
 * and optionally the host, and if so optionally
 * the port number, all passed in a list. The host
 * defaults to localhost, and the port number to
 * the default port number for mySQL.
 *
 * Called with a string, mysqldb attempts to
 * execute that string as a mySQL query.
 *
 * Called with no parameters, mysqldb closes
 * the connection if it is open.
 *
 * Returns a list of lists for a SELECT query, or
 * the number of rows affected for other queries.
 * Otherwise, fails if everything works, returns
 * error information if not, except if incorrect
 * argument types are supplied, in which case the
 * result is an error.
 * --------------------------------------------------*/
int mysqldb(int argc, descriptor argv[]) {
    static MYSQL dbh; /* connection sticks around between calls */
    static int connected = 0;

    MYSQL_RES *result;
    MYSQL_ROW row;
    char *querystring, *hoststring,
        *databasestring, *userstring, *passwordstring;
    int i, len, rowsize, portnum;
    struct {
        gcu_tended other;
        descriptor ls, host, port, database, user, password, answer;
    } tended;
    gcu_initialize( gcu_max(tended), &tended );
    gcu_aware_push( &tended );


    if( argc == 0 ) {  /* close connection */
        if( connected ) mysql_close(&dbh);
        connected = 0;
        gcu_aware_pop();
        Fail;
    } /* end close connection */

    if( argc >= 1 && IconType(argv[1]) == 'L' ) { /* connect to MySQL */
        if( connected )
            ReturnDescriptor( error_info(-1, "mysqldb: already connected") );
        if( !mysql_init(&dbh) )
            ReturnDescriptor( error_info(-1, "mysqldb: cannot initialize mySQL!") );

        tended.ls = argv[1];
        hoststring = "localhost"; /* host defaults to localhost */
        portnum = 0; /* port defaults to 0 giving the mySQL default */

        switch( ListLen(tended.ls) ) {
            default:
                ReturnDescriptor( error_info(-1, "mysqldb: list of dbname, user, pwd, [host, [port]] expected") );
            case 5 :
                tended.port = subscript(tended.ls, 5);
                if( !cnv_int(&tended.port,&tended.port) ) ReturnError(tended.port,101);
                portnum = IntegerVal(tended.port);
            case 4 :
                tended.host = subscript(tended.ls, 4);
                if ( !cnv_str(&tended.host,&tended.host) ) ReturnError(tended.host,103);
                hoststring = StringVal(tended.host);
            case 3 :
                tended.password = subscript(tended.ls, 3);
                if ( !cnv_str(&tended.password,&tended.password) ) ReturnError(tended.password,103);
                passwordstring = StringVal(tended.password);
                tended.user = subscript(tended.ls, 2);
                if ( !cnv_str(&tended.user,&tended.user) ) ReturnError(tended.user,103);
                userstring = StringVal(tended.user);
                tended.database = subscript(tended.ls, 1);
                if ( !cnv_str(&tended.database,&tended.database) ) ReturnError(tended.database,103);
                databasestring = StringVal(tended.database);
        }

        if( mysql_real_connect(&dbh, hoststring, userstring,
                passwordstring, databasestring, portnum, NULL, 0) ) {
            connected = 1;
            gcu_aware_pop();
            Fail;
        }
        else ReturnDescriptor( error_info(mysql_errno(&dbh), mysql_error(&dbh))  );
    } /* end connect to MySQL */

    if( argc >= 1 && IconType(argv[1]) == 's' )  { /* execute a query */
        if( !connected )
            ReturnDescriptor( error_info(-1, "mysqldb: not connected") );
        querystring = StringVal(argv[1]);

        if( mysql_query(&dbh, querystring) )
            ReturnDescriptor( error_info(mysql_errno(&dbh), mysql_error(&dbh))  );

        result = mysql_store_result(&dbh);

        if( !result ) /* not a SELECT query or some sort of error */
            if( mysql_field_count(&dbh) != 0 )
                ReturnDescriptor( error_info(mysql_errno(&dbh), mysql_error(&dbh))  );
            else { /* not a SELECT query */
                gcu_aware_pop();
                RetInteger( mysql_affected_rows(&dbh) );
            }

        /* SELECT query */
        tended.answer = newlist( mysql_num_rows(result) );
        rowsize = mysql_num_fields(result);
        i = 0;
        while( row = mysql_fetch_row(result) )
            assign( tended.answer, ++i, convertrow(row, rowsize) );
        mysql_free_result(result);
        ReturnDescriptor(tended.answer);
    } /* end execute a query */

    /* wrong argument type to mysqldb */
    ReturnError(argv[1], 110); /* list or string expected */
}
