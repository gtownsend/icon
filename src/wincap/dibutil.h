/*
 *  dibutil.h
 *
 *  Copyright (c) 1991 Microsoft Corporation. All rights reserved.
 *
 *  Header file for Device-Independent Bitmap (DIB) API.  Provides
 *  function prototypes and constants for the following functions:
 *
 *  FindDIBBits()		- Sets pointer to the DIB bits
 *  DIBWidth()			- Gets the DIB width
 *  DIBHeight()			- Gets the DIB height
 *  DIBNumColors()		- Calculates number of colors in the DIB's color table
 *  PaletteSize()		- Calculates the buffer size required by a palette
 *  CreateDIBPalette()	- Creates a palette from a DIB
 *  DIBToBitmap()		- Creates a bitmap from a DIB
 *  BitmapToDIB()		- Creates a DIB from a bitmap
 *  PalEntriesOnDevice()- Gets the number of palette entries
 *  GetSystemPalette()	- Gets the current palette
 *
 */
#include "../wincap/dibapi.h"

/* DIB constants */
#define PALVERSION   0x300

/* DIB macros */
#define WIDTHBYTES(bits)	(((bits) + 31) / 32 * 4)
#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER))

/* function prototypes */
LPSTR    	FindDIBBits(LPSTR lpbi);
DWORD		DIBWidth(LPSTR lpDIB);
DWORD		DIBHeight(LPSTR lpDIB);
WORD     	DIBNumColors(LPSTR lpbi);
WORD     	PaletteSize(LPSTR lpbi);
HPALETTE 	CreateDIBPalette(HDIB hDIB);
HBITMAP  	DIBToBitmap(HDIB hDIB, HPALETTE hPal);
HDIB     	BitmapToDIB(HBITMAP hBitmap, HPALETTE hPal);
int			PalEntriesOnDevice(HDC hDC);
HPALETTE 	GetSystemPalette(void);
