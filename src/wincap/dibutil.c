/*
 *  dibutil.c
 *
 *  Source file for Device-Independent Bitmap (DIB) API.  Provides
 *  the following functions:
 *
 *  FindDIBBits()       - Sets pointer to the DIB bits
 *  DIBWidth()          - Gets the width of the DIB
 *  DIBHeight()         - Gets the height of the DIB
 *  PaletteSize()       - Calculates the buffer size required by a palette
 *  DIBNumColors()      - Calculates number of colors in the DIB's color table
 *  CreateDIBPalette()  - Creates a palette from a DIB
 *  DIBToBitmap()       - Creates a bitmap from a DIB
 *  BitmapToDIB()       - Creates a DIB from a bitmap
 *
 * Development Team: Mark Bader
 *                   Patrick Schreiber
 *                   Garrett Mcauliffe
 *                   Eric Flo
 *                   Tony Claflin
 *
 * Written by Microsoft Product Support Services, Developer Support.
 * Copyright (c) 1991 Microsoft Corporation. All rights reserved.
 */

/* header files */
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include "dibutil.h"


/*************************************************************************
 *
 * FindDIBBits()
 *
 * Parameter:
 *
 * LPSTR lpbi       - pointer to packed-DIB memory block
 *
 * Return Value:
 *
 * LPSTR            - pointer to the DIB bits
 *
 * Description:
 *
 * This function calculates the address of the DIB's bits and returns a
 * pointer to the DIB bits.
 *
 ************************************************************************/


LPSTR FindDIBBits(LPSTR lpbi)
{
   return (lpbi + *(LPDWORD)lpbi + PaletteSize(lpbi));
}


/*************************************************************************
 *
 * DIBWidth()
 *
 * Parameter:
 *
 * LPSTR lpbi       - pointer to packed-DIB memory block
 *
 * Return Value:
 *
 * DWORD            - width of the DIB
 *
 * Description:
 *
 * This function gets the width of the DIB from the BITMAPINFOHEADER
 * width field if it is a Windows 3.0-style DIB or from the BITMAPCOREHEADER
 * width field if it is an OS/2-style DIB.
 *
 ************************************************************************/


DWORD DIBWidth(LPSTR lpDIB)
{
   LPBITMAPINFOHEADER lpbmi;  // pointer to a Win 3.0-style DIB
   LPBITMAPCOREHEADER lpbmc;  // pointer to an OS/2-style DIB

   /* point to the header (whether Win 3.0 and OS/2) */

   lpbmi = (LPBITMAPINFOHEADER)lpDIB;
   lpbmc = (LPBITMAPCOREHEADER)lpDIB;

   /* return the DIB width if it is a Win 3.0 DIB */
   if (lpbmi->biSize == sizeof(BITMAPINFOHEADER))
      return lpbmi->biWidth;
   else  /* it is an OS/2 DIB, so return its width */
      return (DWORD)lpbmc->bcWidth;
}


/*************************************************************************
 *
 * DIBHeight()
 *
 * Parameter:
 *
 * LPSTR lpbi       - pointer to packed-DIB memory block
 *
 * Return Value:
 *
 * DWORD            - height of the DIB
 *
 * Description:
 *
 * This function gets the height of the DIB from the BITMAPINFOHEADER
 * height field if it is a Windows 3.0-style DIB or from the BITMAPCOREHEADER
 * height field if it is an OS/2-style DIB.
 *
 ************************************************************************/


DWORD DIBHeight(LPSTR lpDIB)
{
   LPBITMAPINFOHEADER lpbmi;  // pointer to a Win 3.0-style DIB
   LPBITMAPCOREHEADER lpbmc;  // pointer to an OS/2-style DIB

   /* point to the header (whether OS/2 or Win 3.0 */

   lpbmi = (LPBITMAPINFOHEADER)lpDIB;
   lpbmc = (LPBITMAPCOREHEADER)lpDIB;

   /* return the DIB height if it is a Win 3.0 DIB */
   if (lpbmi->biSize == sizeof(BITMAPINFOHEADER))
      return lpbmi->biHeight;
   else  /* it is an OS/2 DIB, so return its height */
      return (DWORD)lpbmc->bcHeight;
}


/*************************************************************************
 *
 * PaletteSize()
 *
 * Parameter:
 *
 * LPSTR lpbi       - pointer to packed-DIB memory block
 *
 * Return Value:
 *
 * WORD             - size of the color palette of the DIB
 *
 * Description:
 *
 * This function gets the size required to store the DIB's palette by
 * multiplying the number of colors by the size of an RGBQUAD (for a
 * Windows 3.0-style DIB) or by the size of an RGBTRIPLE (for an OS/2-
 * style DIB).
 *
 ************************************************************************/


WORD PaletteSize(LPSTR lpbi)
{
   /* calculate the size required by the palette */
   if (IS_WIN30_DIB (lpbi))
      return (DIBNumColors(lpbi) * sizeof(RGBQUAD));
   else
      return (DIBNumColors(lpbi) * sizeof(RGBTRIPLE));
}


/*************************************************************************
 *
 * DIBNumColors()
 *
 * Parameter:
 *
 * LPSTR lpbi       - pointer to packed-DIB memory block
 *
 * Return Value:
 *
 * WORD             - number of colors in the color table
 *
 * Description:
 *
 * This function calculates the number of colors in the DIB's color table
 * by finding the bits per pixel for the DIB (whether Win3.0 or OS/2-style
 * DIB). If bits per pixel is 1: colors=2, if 4: colors=16, if 8: colors=256,
 * if 24, no colors in color table.
 *
 ************************************************************************/


WORD DIBNumColors(LPSTR lpbi)
{
   WORD wBitCount;  // DIB bit count

   /*  If this is a Windows-style DIB, the number of colors in the
    *  color table can be less than the number of bits per pixel
    *  allows for (i.e. lpbi->biClrUsed can be set to some value).
    *  If this is the case, return the appropriate value.
    */

   if (IS_WIN30_DIB(lpbi))
   {
      DWORD dwClrUsed;

      dwClrUsed = ((LPBITMAPINFOHEADER)lpbi)->biClrUsed;
      if (dwClrUsed)
         return (WORD)dwClrUsed;
   }

   /*  Calculate the number of colors in the color table based on
    *  the number of bits per pixel for the DIB.
    */
   if (IS_WIN30_DIB(lpbi))
      wBitCount = ((LPBITMAPINFOHEADER)lpbi)->biBitCount;
   else
      wBitCount = ((LPBITMAPCOREHEADER)lpbi)->bcBitCount;

   /* return number of colors based on bits per pixel */
   switch (wBitCount)
      {
   case 1:
      return 2;

   case 4:
      return 16;

   case 8:
      return 256;

   default:
      return 0;
      }
}


/*************************************************************************
 *
 * CreateDIBPalette()
 *
 * Parameter:
 *
 * HDIB hDIB        - specifies the DIB
 *
 * Return Value:
 *
 * HPALETTE         - specifies the palette
 *
 * Description:
 *
 * This function creates a palette from a DIB by allocating memory for the
 * logical palette, reading and storing the colors from the DIB's color table
 * into the logical palette, creating a palette from this logical palette,
 * and then returning the palette's handle. This allows the DIB to be
 * displayed using the best possible colors (important for DIBs with 256 or
 * more colors).
 *
 ************************************************************************/


HPALETTE CreateDIBPalette(HDIB hDIB)
{
   LPLOGPALETTE lpPal;      // pointer to a logical palette
   HANDLE hLogPal;          // handle to a logical palette
   HPALETTE hPal = NULL;    // handle to a palette
   int i, wNumColors;       // loop index, number of colors in color table
   LPSTR lpbi;              // pointer to packed-DIB
   LPBITMAPINFO lpbmi;      // pointer to BITMAPINFO structure (Win3.0)
   LPBITMAPCOREINFO lpbmc;  // pointer to BITMAPCOREINFO structure (OS/2)
   BOOL bWinStyleDIB;       // flag which signifies whether this is a Win3.0 DIB

   /* if handle to DIB is invalid, return NULL */

   if (!hDIB)
      return NULL;

   /* lock DIB memory block and get a pointer to it */
   lpbi = (LPSTR) GlobalLock(hDIB);

   /* get pointer to BITMAPINFO (Win 3.0) */
   lpbmi = (LPBITMAPINFO)lpbi;

   /* get pointer to BITMAPCOREINFO (OS/2 1.x) */
   lpbmc = (LPBITMAPCOREINFO)lpbi;

   /* get the number of colors in the DIB */
   wNumColors = DIBNumColors(lpbi);

   /* is this a Win 3.0 DIB? */
   bWinStyleDIB = IS_WIN30_DIB(lpbi);
   if (wNumColors)
   {
      /* allocate memory block for logical palette */
      hLogPal = GlobalAlloc(GHND, sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) *
                            wNumColors);

      /* if not enough memory, clean up and return NULL */
      if (!hLogPal)
      {
         GlobalUnlock(hDIB);
         return NULL;
      }

      /* lock memory block and get pointer to it */
      lpPal = (LPLOGPALETTE)GlobalLock(hLogPal);

      /* set version and number of palette entries */
      lpPal->palVersion = PALVERSION;
      lpPal->palNumEntries = wNumColors;

      /*  store RGB triples (if Win 3.0 DIB) or RGB quads (if OS/2 DIB)
       *  into palette
       */
      for (i = 0; i < wNumColors; i++)
      {
         if (bWinStyleDIB)
         {
            lpPal->palPalEntry[i].peRed = lpbmi->bmiColors[i].rgbRed;
            lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
            lpPal->palPalEntry[i].peBlue = lpbmi->bmiColors[i].rgbBlue;
            lpPal->palPalEntry[i].peFlags = 0;
         }
         else
         {
            lpPal->palPalEntry[i].peRed = lpbmc->bmciColors[i].rgbtRed;
            lpPal->palPalEntry[i].peGreen = lpbmc->bmciColors[i].rgbtGreen;
            lpPal->palPalEntry[i].peBlue = lpbmc->bmciColors[i].rgbtBlue;
            lpPal->palPalEntry[i].peFlags = 0;
         }
      }

      /* create the palette and get handle to it */
      hPal = CreatePalette(lpPal);

      /* if error getting handle to palette, clean up and return NULL */
      if (!hPal)
      {
         GlobalUnlock(hLogPal);
         GlobalFree(hLogPal);
         return NULL;
      }
   }

   /* clean up */
   GlobalUnlock(hLogPal);
   GlobalFree(hLogPal);
   GlobalUnlock(hDIB);

   /* return handle to DIB's palette */
   return hPal;
}


/*************************************************************************
 *
 * DIBToBitmap()
 *
 * Parameters:
 *
 * HDIB hDIB        - specifies the DIB to convert
 *
 * HPALETTE hPal    - specifies the palette to use with the bitmap
 *
 * Return Value:
 *
 * HBITMAP          - identifies the device-dependent bitmap
 *
 * Description:
 *
 * This function creates a bitmap from a DIB using the specified palette.
 * If no palette is specified, default is used.
 *
 ************************************************************************/


HBITMAP DIBToBitmap(HDIB hDIB, HPALETTE hPal)
{
   LPSTR lpDIBHdr, lpDIBBits;  // pointer to DIB header, pointer to DIB bits
   HBITMAP hBitmap;            // handle to device-dependent bitmap
   HDC hDC;                    // handle to DC
   HPALETTE hOldPal = NULL;      // handle to a palette

   /* if invalid handle, return NULL */

   if (!hDIB) {
      return NULL;
      }

   /* lock memory block and get a pointer to it */
   lpDIBHdr = (LPSTR) GlobalLock(hDIB);

   /* get a pointer to the DIB bits */
   lpDIBBits = FindDIBBits(lpDIBHdr);

   /* get a DC */
   hDC = GetDC(NULL);
   if (!hDC)
   {
      /* clean up and return NULL */
      GlobalUnlock(hDIB);
      return NULL;
   }

   /* select and realize palette */
   if (hPal)
      hOldPal = SelectPalette(hDC, hPal, FALSE);
   RealizePalette(hDC);

   /* create bitmap from DIB info. and bits */
   hBitmap = CreateDIBitmap(hDC, (LPBITMAPINFOHEADER)lpDIBHdr, CBM_INIT,
                            lpDIBBits, (LPBITMAPINFO)lpDIBHdr, DIB_RGB_COLORS)
   ;

   /* restore previous palette */
   if (hOldPal)
      SelectPalette(hDC, hOldPal, FALSE);

   /* clean up */
   ReleaseDC(NULL, hDC);
   GlobalUnlock(hDIB);

   /* return handle to the bitmap */
   return hBitmap;
}


/*************************************************************************
 *
 * BitmapToDIB()
 *
 * Parameters:
 *
 * HBITMAP hBitmap  - specifies the bitmap to convert
 *
 * HPALETTE hPal    - specifies the palette to use with the bitmap
 *
 * Return Value:
 *
 * HDIB             - identifies the device-dependent bitmap
 *
 * Description:
 *
 * This function creates a DIB from a bitmap using the specified palette.
 *
 ************************************************************************/


HDIB BitmapToDIB(HBITMAP hBitmap, HPALETTE hPal)
{
   BITMAP bm;                   // bitmap structure
   BITMAPINFOHEADER bi;         // bitmap header
   BITMAPINFOHEADER FAR *lpbi;  // pointer to BITMAPINFOHEADER
   DWORD dwLen;                 // size of memory block
   HANDLE hDIB, h;              // handle to DIB, temp handle
   HDC hDC;                     // handle to DC
   WORD biBits;                 // bits per pixel

   /* check if bitmap handle is valid */

   if (!hBitmap)
      return NULL;

   /* if no palette is specified, use default palette */
   if (hPal == NULL)
      hPal = (HPALETTE) GetStockObject(DEFAULT_PALETTE);

   /* fill in BITMAP structure */
   GetObject(hBitmap, sizeof(bm), (LPSTR)&bm);

   /* calculate bits per pixel */
   biBits = bm.bmPlanes * bm.bmBitsPixel;

   /* initialize BITMAPINFOHEADER */
   bi.biSize = sizeof(BITMAPINFOHEADER);
   bi.biWidth = bm.bmWidth;
   bi.biHeight = bm.bmHeight;
   bi.biPlanes = 1;
   bi.biBitCount = biBits;
   bi.biCompression = DIB_RGB_COLORS;
   bi.biSizeImage = 0;
   bi.biXPelsPerMeter = 0;
   bi.biYPelsPerMeter = 0;
   bi.biClrUsed = 0;
   bi.biClrImportant = 0;

   /* calculate size of memory block required to store BITMAPINFO */
   dwLen = bi.biSize + PaletteSize((LPSTR)&bi);

   /* get a DC */
   hDC = GetDC(NULL);

   /* select and realize our palette */
   hPal = SelectPalette(hDC, hPal, FALSE);
   RealizePalette(hDC);

   /* alloc memory block to store our bitmap */
   hDIB = GlobalAlloc(GHND, dwLen);

   /* if we couldn't get memory block */
   if (!hDIB)
   {
      /* clean up and return NULL */
      SelectPalette(hDC, hPal, TRUE);
      RealizePalette(hDC);
      ReleaseDC(NULL, hDC);
      return NULL;
   }

   /* lock memory and get pointer to it */
   lpbi = (BITMAPINFOHEADER FAR *)GlobalLock(hDIB);

   /* use our bitmap info. to fill BITMAPINFOHEADER */
   *lpbi = bi;

   /*  call GetDIBits with a NULL lpBits param, so it will calculate the
    *  biSizeImage field for us
    */
   GetDIBits(hDC, hBitmap, 0, (WORD)bi.biHeight, NULL, (LPBITMAPINFO)lpbi,
             DIB_RGB_COLORS);

   /* get the info. returned by GetDIBits and unlock memory block */
   bi = *lpbi;
   GlobalUnlock(hDIB);

   /* if the driver did not fill in the biSizeImage field, make one up */
   if (bi.biSizeImage == 0)
      bi.biSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * biBits) * bm.bmHeight;

   /* realloc the buffer big enough to hold all the bits */
   dwLen = bi.biSize + PaletteSize((LPSTR)&bi) + bi.biSizeImage;
   if (h = GlobalReAlloc(hDIB, dwLen, 0))
      hDIB = h;
   else
   {
      /* clean up and return NULL */
      GlobalFree(hDIB);
      hDIB = NULL;
      SelectPalette(hDC, hPal, TRUE);
      RealizePalette(hDC);
      ReleaseDC(NULL, hDC);
      return NULL;
   }

   /* lock memory block and get pointer to it */
   lpbi = (BITMAPINFOHEADER FAR *)GlobalLock(hDIB);

   /*  call GetDIBits with a NON-NULL lpBits param, and actualy get the
    *  bits this time
    */
   if (GetDIBits(hDC, hBitmap, 0, (WORD)bi.biHeight, (LPSTR)lpbi + (WORD)lpbi
                 ->biSize + PaletteSize((LPSTR)lpbi), (LPBITMAPINFO)lpbi,
                 DIB_RGB_COLORS) == 0)
   {
      /* clean up and return NULL */
      GlobalUnlock(hDIB);
      hDIB = NULL;
      SelectPalette(hDC, hPal, TRUE);
      RealizePalette(hDC);
      ReleaseDC(NULL, hDC);
      return NULL;
   }
   bi = *lpbi;

   /* clean up */
   GlobalUnlock(hDIB);
   SelectPalette(hDC, hPal, TRUE);
   RealizePalette(hDC);
   ReleaseDC(NULL, hDC);

   /* return handle to the DIB */
   return hDIB;
}


/*************************************************************************
 *
 * PalEntriesOnDevice()
 *
 * Parameter:
 *
 * HDC hDC          - device context
 *
 * Return Value:
 *
 * int              - number of palette entries on device
 *
 * Description:
 *
 * This function gets the number of palette entries on the specified device
 *
 ************************************************************************/


int PalEntriesOnDevice(HDC hDC)
{
   int nColors;  // number of colors

   /*  Find out the number of palette entries on this
    *  device.
    */

   nColors = GetDeviceCaps(hDC, SIZEPALETTE);

   /*  For non-palette devices, we'll use the # of system
    *  colors for our palette size.
    */
   if (!nColors)
      nColors = GetDeviceCaps(hDC, NUMCOLORS);
   assert(nColors);
   return nColors;
}


/*************************************************************************
 *
 * DIBHeight()
 *
 * Parameter:
 *
 * LPSTR lpbi       - pointer to packed-DIB memory block
 *
 * Return Value:
 *
 * DWORD            - height of the DIB
 *
 * Description:
 *
 * This function returns a handle to a palette which represents the system
 * palette (each entry is an offset into the system palette instead of an
 * RGB with a flag of PC_EXPLICIT).
 *
 ************************************************************************/


HPALETTE GetSystemPalette(void)
{
   HDC hDC;                // handle to a DC
   HPALETTE hPal = NULL;   // handle to a palette
   HANDLE hLogPal;         // handle to a logical palette
   LPLOGPALETTE lpLogPal;  // pointer to a logical palette
   int i, nColors;         // loop index, number of colors

   /* Find out how many palette entries we want. */

   hDC = GetDC(NULL);
   if (!hDC)
      return NULL;
   nColors = PalEntriesOnDevice(hDC);
   ReleaseDC(NULL, hDC);

   /* Allocate room for the palette and lock it. */
   hLogPal = GlobalAlloc(GHND, sizeof(LOGPALETTE) + nColors * sizeof(
                         PALETTEENTRY));

   /* if we didn't get a logical palette, return NULL */
   if (!hLogPal)
      return NULL;

   /* get a pointer to the logical palette */
   lpLogPal = (LPLOGPALETTE)GlobalLock(hLogPal);

   /* set some important fields */
   lpLogPal->palVersion = PALVERSION;
   lpLogPal->palNumEntries = nColors;
   for (i = 0; i < nColors; i++)
   {
      lpLogPal->palPalEntry[i].peBlue = 0;
      *((LPWORD)(&lpLogPal->palPalEntry[i].peRed)) = i;
      lpLogPal->palPalEntry[i].peFlags = PC_EXPLICIT;
   }

   /*  Go ahead and create the palette.  Once it's created,
    *  we no longer need the LOGPALETTE, so free it.
    */
   hPal = CreatePalette(lpLogPal);

   /* clean up */
   GlobalUnlock(hLogPal);
   GlobalFree(hLogPal);
   return hPal;
}
