/*
 * This file contains definitions for the various event codes and values
 * that go to make up event streams.
 */

/*
 * Note: the blank character should *not* be used as an event code.
 */

#ifdef EventMon

/*
 * Allocation events use lowercase codes.
 */
#define E_Lrgint	'\114'		/* Large integer allocation */
#define E_Real		'\144'		/* Real allocation */
#define E_Cset		'\145'		/* Cset allocation */
#define E_File		'\147'		/* File allocation */
#define E_Record	'\150'		/* Record allocation */
#define E_Tvsubs	'\151'		/* Substring tv allocation */
#define E_External	'\152'		/* External allocation */
#define E_List		'\153'		/* List allocation */
#define E_Lelem		'\155'		/* List element allocation */
#define E_Table		'\156'		/* Table allocation */
#define E_Telem		'\157'		/* Table element allocation */
#define E_Tvtbl		'\160'		/* Table-element tv allocation */
#define E_Set		'\161'		/* Set allocation */
#define E_Selem		'\164'		/* Set element allocation */
#define E_Slots		'\167'		/* Hash header allocation */
#define E_Coexpr	'\170'		/* Co-expression allocation */
#define E_Refresh	'\171'		/* Refresh allocation */
#define E_Alien		'\172'		/* Alien allocation */
#define E_Free		'\132'		/* Free region */
#define E_String	'\163'		/* String allocation */

/*
 * Some other monitoring codes.
 */
#define	E_BlkDeAlc	'\055'		/* Block deallocation */
#define	E_StrDeAlc	'\176'		/* String deallocation */

/*
 * These are not "events"; they are provided for uniformity in tools
 *  that deal with types.
 */
#define E_Integer	'\100'		/* Integer value pseudo-event */
#define E_Null		'\044'		/* Null value pseudo-event */
#define E_Proc		'\045'		/* Procedure value pseudo-event */
#define E_Kywdint	'\136'		/* Integer keyword value pseudo-event */
#define E_Kywdpos	'\046'		/* Position value pseudo-event */
#define E_Kywdsubj	'\052'		/* Subject value pseudo-event */

/*
 * Codes for main sequence events
 */

   /*
    * Timing events
    */
#define E_Tick		'\056'		/* Clock tick */

   /*
    * Code-location event
    */
#define E_Loc		'\174'		/* Location change */
#define E_Line		'\355'		/* Line change */

   /*
    * Virtual-machine instructions
    */
#define E_Opcode	'\117'		/* Virtual-machine instruction */

   /*
    * Type-conversion events
    */
#define E_Aconv		'\111'		/* Conversion attempt */
#define E_Tconv		'\113'		/* Conversion target */
#define E_Nconv		'\116'		/* Conversion not needed */
#define E_Sconv		'\121'		/* Conversion success */
#define E_Fconv		'\112'		/* Conversion failure */

   /*
    * Structure events
    */
#define	E_Lbang		'\301'		/* List generation */
#define	E_Lcreate	'\302'		/* List creation */
#define	E_Lget		'\356'		/* List get/pop -- only E_Lget used */
#define	E_Lpop		'\356'		/* List get/pop */
#define	E_Lpull		'\304'		/* List pull */
#define	E_Lpush		'\305'		/* List push */
#define	E_Lput		'\306'		/* List put */
#define	E_Lrand		'\307'		/* List random reference */
#define	E_Lref		'\310'		/* List reference */
#define E_Lsub		'\311'		/* List subscript */
#define	E_Rbang		'\312'		/* Record generation */
#define	E_Rcreate	'\313'		/* Record creation */
#define	E_Rrand		'\314'		/* Record random reference */
#define	E_Rref		'\315'		/* Record reference */
#define E_Rsub		'\316'		/* Record subscript */
#define	E_Sbang		'\317'		/* Set generation */
#define	E_Screate	'\320'		/* Set creation */
#define	E_Sdelete	'\321'		/* Set deletion */
#define	E_Sinsert	'\322'		/* Set insertion */
#define	E_Smember	'\323'		/* Set membership */
#define	E_Srand		'\336'		/* Set random reference */
#define	E_Sval		'\324'		/* Set value */
#define	E_Tbang		'\325'		/* Table generation */
#define	E_Tcreate	'\326'		/* Table creation */
#define	E_Tdelete	'\327'		/* Table deletion */
#define	E_Tinsert	'\330'		/* Table insertion */
#define	E_Tkey		'\331'		/* Table key generation */
#define	E_Tmember	'\332'		/* Table membership */
#define	E_Trand		'\337'		/* Table random reference */
#define	E_Tref		'\333'		/* Table reference */
#define	E_Tsub		'\334'		/* Table subscript */
#define	E_Tval		'\335'		/* Table value */

   /*
    * Scanning events
    */

#define E_Snew		'\340'		/* Scanning environment creation */
#define E_Sfail		'\341'		/* Scanning failure */
#define E_Ssusp		'\342'		/* Scanning suspension */
#define E_Sresum	'\343'		/* Scanning resumption */
#define E_Srem		'\344'		/* Scanning environment removal */
#define E_Spos		'\346'		/* Scanning position */

   /*
    * Assignment
    */

#define E_Assign	'\347'		/* Assignment */
#define	E_Value		'\350'		/* Value assigned */

   /*
    * Sub-string assignment
    */

#define E_Ssasgn	'\354'		/* Sub-string assignment */
   /*
    * Interpreter stack events
    */

#define E_Intcall	'\351'		/* interpreter call */
#define E_Intret	'\352'		/* interpreter return */
#define E_Stack		'\353'		/* stack depth */

   /*
    * Expression events
    */
#define E_Ecall		'\143'		/* Call of operation */
#define E_Efail		'\146'		/* Failure from expression */
#define E_Bsusp		'\142'		/* Suspension from operation */
#define E_Esusp		'\141'		/* Suspension from alternation */
#define E_Lsusp		'\154'		/* Suspension from limitation */
#define E_Eresum	'\165'		/* Resumption of expression */
#define E_Erem		'\166'		/* Removal of a suspended generator */

   /*
    * Co-expression events
    */

#define E_Coact		'\101'		/* Co-expression activation */
#define E_Coret		'\102'		/* Co-expression return */
#define E_Cofail	'\104'		/* Co-expression failure */

   /*
    * Procedure events
    */

#define E_Pcall		'\103'		/* Procedure call */
#define E_Pfail		'\106'		/* Procedure failure */
#define E_Pret		'\122'		/* Procedure return */
#define E_Psusp		'\123'		/* Procedure suspension */
#define E_Presum	'\125'		/* Procedure resumption */
#define E_Prem		'\126'		/* Suspended procedure removal */

#define E_Fcall		'\072'		/* Function call */
#define E_Ffail		'\115'		/* Function failure */
#define E_Fret		'\120'		/* Function return */
#define E_Fsusp		'\127'		/* Function suspension */
#define E_Fresum	'\131'		/* Function resumption */
#define E_Frem		'\133'		/* Function suspension removal */

#define E_Ocall		'\134'		/* Operator call */
#define E_Ofail		'\135'		/* Operator failure */
#define E_Oret		'\140'		/* Operator return */
#define E_Osusp		'\173'		/* Operator suspension */
#define E_Oresum	'\175'		/* Operator resumption */
#define E_Orem		'\177'		/* Operator suspension removal */

   /*
    * Garbage collections
    */

#define E_Collect	'\107'		/* Garbage collection */
#define E_EndCollect	'\360'		/* End of garbage collection */
#define E_TenureString	'\361'		/* Tenure a string region */
#define E_TenureBlock	'\362'		/* Tenure a block region */

/*
 * Termination Events
 */
#define E_Error		'\105'		/* Run-time error */
#define E_Exit		'\130'		/* Program exit */

   /*
    * I/O events
    */
#define E_MXevent	'\370'		/* monitor input event */

#endif					/* EventMon */
