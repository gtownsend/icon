/*
 * clex.c -- the lexical analyzer for iconc.
 */
#define Iconc

#include "../h/gsupport.h"
#include "../h/lexdef.h"
#include "ctrans.h"
#include "ctoken.h"
#include "ctree.h"
#include "csym.h"
#include "ccode.h"
#include "cproto.h"

#include "../h/parserr.h"
#include "../common/lextab.h"
#include "../common/yylex.h"
#include "../common/error.h"
