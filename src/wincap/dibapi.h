/*
 *  dibapi.h
 *
 *  Copyright (c) 1991 Microsoft Corporation. All rights reserved
 *
 *  Header file for Device-Independent Bitmap (DIB) API.  Provides
 *  function prototypes and constants for the following functions:
 *
 *  PrintWindow()       - Prints all or part of a window
 *  PrintScreen()       - Prints the entire screen
 *  CopyWindowToDIB()   - Copies a window to a DIB
 *  CopyScreenToDIB()   - Copies entire screen to a DIB
 *  CopyWindowToBitmap()- Copies a window to a standard Bitmap
 *  CopyScreenToBitmap()- Copies entire screen to a standard Bitmap
 *  PrintDIB()          - Prints the specified DIB
 *  SaveDIB()           - Saves the specified dib in a file
 *  LoadDIB()           - Loads a DIB from a file
 *  DestroyDIB()        - Deletes DIB when finished using it
 *
 * See the file DIBAPI.TXT for more information about these functions.
 *
 */

/* Handle to a DIB */
#define HDIB HANDLE

/* Print Area selection */
#define PW_WINDOW        1
#define PW_CLIENT        2

/* Print Options selection */
#define PW_BESTFIT       1
#define PW_STRETCHTOPAGE 2
#define PW_SCALE         3

/* Function prototypes */
WORD    PrintWindow(HWND, WORD, WORD, WORD, WORD, LPSTR);
WORD    PrintScreen(LPRECT, WORD, WORD, WORD, LPSTR);
HDIB   	CopyWindowToDIB(HWND, WORD);
HDIB   	CopyScreenToDIB(LPRECT);
HBITMAP	CopyWindowToBitmap(HWND, WORD);
HBITMAP	CopyScreenToBitmap(LPRECT);
WORD    PrintDIB(HDIB, WORD, WORD, WORD, LPSTR);
WORD   	SaveDIB(HDIB, LPSTR);
HDIB   	LoadDIB(LPSTR);
WORD   	DestroyDIB(HDIB);
