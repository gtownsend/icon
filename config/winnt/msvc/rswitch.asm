	page	80,132
;
;   Coswitch for Icon V8 interpreter running in 32 bit protected mode
;   under OS/2 2.0, compiled with OptLink (the C Set/2 default).
;
;   Author:  Robert Goldberg
;
;   Modified for OS/2 2.0: Mark Emmer
;
;   Note:  Must assemble with /Ml switch to preserve case of names!
;
;   Coswitch algorithm in C:
;
;   coswitch( old_cs, new_cs, first )
;   int	*old_cs, *new_cs, first;
;   {
;	/* save SP, frame pointers, and other registers in old_cs */
;	if ( first == 0 )
;	{
;	    /* load sp from new_sp[0] and clear frame pointers */
;	    interp( 0, 0 );
;	    syserr( "interp() returned in coswitch" );
;	}
;	else
;	{
;	    /* load sp, frame pointers and other registers from new_cs */
;	}
;   }
;
;   Define external functions:
;
		.386
		extrn		_new_context:near
		extrn		_syserr:near
;
;   Define error message in appropriate segment.
;
		assume		CS:FLAT, DS:FLAT
		public		errormsg
DATA32		segment		dword use32 'DATA'
errormsg	db		'interp() returned in coswitch',0
DATA32		ends
;
;   Define function coswitch in appropriate segment and group.
;
CODE32		segment		dword use32 'CODE'
		public		_coswitch
_coswitch 	proc   		near
;
;   Save environment at point of call.
;
;   According to C Set/2 documentation (Chapter 16 - Calling Conventions)
;   registers EAX, ECX, and EDX are volatile, and EBP, ESI, EDI, and EBX
;   must be preserved across calls.  So, save all these registers plus ESP
;   in area pointed to by old_cs.
;
;   coswitch prototype:  int coswitch(int *old, int *new, int first);
;
;   With optlink calling conventions, the three arguments are passed
;   in registers EAX, EDX, and ECX respectively.  There are slots for
;   them on the stack, but they have not been pushed there:
;
;   At function entry:
;
;                    +----------------+
;      high mem      | slot for first |
;                    +----------------+
;                    | slot for *new  |
;                    +----------------+
;                    | slot for *old  |
;                    +----------------+
;      low mem       |   return EIP   | <-- ESP
;                    +----------------+
;
; Code not needed because of Optlink commented out here:
;
	mov	eax,[esp+4]
	mov	edx,[esp+8]
	mov	ecx,[esp+12]
;
; Remaining code written to work with or without Optlink:

	mov	0[eax],esp	    ; save esp in old save area
	mov	4[eax],ebp	    ; save ebp
	mov	8[eax],esi	    ; save esi
	mov	12[eax],edi	    ; save edi
	mov	16[eax],ebx	    ; save ebx
;
;   Check first flag to see if first activation of this coexpression.
;
	jecxz	iszero		    ; jump if zero
;
;   First is not 0 -
;
;   Restore environment from new_cs.
;
	mov	esp,0[edx]	    ; restore esp
	mov	ebp,4[edx]	    ; restore ebp
	mov	esi,8[edx]	    ; restore esi
	mov	edi,12[edx]	    ; restore edi
	mov	ebx,16[edx]	    ; restore ebx
	ret			    ; return
;
;   First is 0 -
;
;   Set things up for first activation of this coexpression.
;
iszero:	mov	esp,0[edx]	    ; set ESP from new_cs
	mov	ebp,esp	    ; set EBP equal to ESP (can't hurt?)
	xor	eax,eax	    ; get a zero for first arg to interp
	xor	edx,edx	    ;  "      "   "  second arg to interp
	push	edx		    ; create stack slots
	push	eax
	call	_new_context	    ; interp(0,0) - should not return!
	add	esp,8
	lea	eax,errormsg	    ; pass offset to error message in EAX
	push	eax		    ; create stack slot
	call	_syserr		    ; terminate with error
_coswitch	endp
CODE32		ends
		end
