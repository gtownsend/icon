/*
 * Errors.c
 *
 * Contains error messages for WINCAP
 *
 * These error messages all have constants associated with
 * them, contained in errors.h.
 *
 * Note that not all these messages are used in WINCAP.
 *
 * Copyright (c) 1991 Microsoft Corporation. All rights reserved.
 */
#include <windows.h>
#include "errors.h"

extern char szAppName[];

static char *szErrors[] =
{
   "Not a DIB file!",
   "Couldn't allocate memory!",
   "Error reading file!",
   "Error locking memory!",
   "Error opening file!",
   "Error creating palette!",
   "Error getting a DC!",
   "Error creating Device Dependent Bitmap",
   "StretchBlt() failed!",
   "StretchDIBits() failed!",
   "SetDIBitsToDevice() failed!",
   "Printer: StartDoc failed!",
   "Printing: GetModuleHandle() couldn't find GDI!",
   "Printer: SetAbortProc failed!",
   "Printer: StartPage failed!",
   "Printer: NEWFRAME failed!",
   "Printer: EndPage failed!",
   "Printer: EndDoc failed!",
   "SetDIBits() failed!",
   "File Not Found!",
   "Invalid Handle",
   "General Error on call to DIB function"
};


void DIBError(int ErrNo)
{
   if ((ErrNo < ERR_MIN) || (ErrNo >= ERR_MAX))
      MessageBox(NULL, "Undefined Error!", szAppName, MB_OK | MB_ICONHAND);
   else
      MessageBox(NULL, szErrors[ErrNo], szAppName, MB_OK | MB_ICONHAND);
}
