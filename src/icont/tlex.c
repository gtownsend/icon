/*
 * tlex.c -- the lexical analyzer for icont.
 */

#include "../h/gsupport.h"
#undef T_Real
#undef T_String
#undef T_Cset
#include "../h/lexdef.h"
#include "ttoken.h"
#include "tree.h"
#include "tproto.h"
#include "../h/parserr.h"
#include "../common/lextab.h"
#include "../common/yylex.h"
#include "../common/error.h"
