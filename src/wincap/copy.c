/*
 *  copy.c
 *
 *  Source file for Device-Independent Bitmap (DIB) API.  Provides
 *  the following functions:
 *
 *  CopyWindowToDIB()   - Copies a window to a DIB
 *  CopyScreenToDIB()   - Copies entire screen to a DIB
 *  CopyWindowToBitmap()- Copies a window to a standard Bitmap
 *  CopyScreenToBitmap()- Copies entire screen to a standard Bitmap
 *
 *  The following functions are called from DIBUTIL.C:
 *
 *  DIBToBitmap()       - Creates a bitmap from a DIB
 *  BitmapToDIB()       - Creates a DIB from a bitmap
 *  DIBWidth()          - Gets the width of the DIB
 *  DIBHeight()         - Gets the height of the DIB
 *  CreateDIBPalette()  - Gets the DIB's palette
 *  GetSystemPalette()  - Gets the current palette
 *
 * Development Team: Mark Bader
 *                   Patrick Schreiber
 *                   Garrett McAuliffe
 *                   Eric Flo
 *                   Tony Claflin
 *
 * Written by Microsoft Product Support Services, Developer Support.
 * Copyright (c) 1991 Microsoft Corporation. All rights reserved.
 */

/* header files */
#include <WINDOWS.H>
#include "ERRORS.H"
#include "DIBUTIL.H"
#include "DIBAPI.H"

/*************************************************************************
 *
 * CopyWindowToDIB()
 *
 * Parameters:
 *
 * HWND hWnd        - specifies the window
 *
 * WORD fPrintArea  - specifies the window area to copy into the device-
 *                    independent bitmap
 *
 * Return Value:
 *
 * HDIB             - identifies the device-independent bitmap
 *
 * Description:
 *
 * This function copies the specified part(s) of the window to a device-
 * independent bitmap.
 *
 ************************************************************************/


HDIB CopyWindowToDIB(HWND hWnd, WORD fPrintArea)
{
   HDIB hDIB = NULL;  // handle to DIB

   /* check for a valid window handle */

   if (!hWnd)
      return NULL;
   switch (fPrintArea)
      {
   case PW_WINDOW: // copy entire window
   {
      RECT rectWnd;

      /* get the window rectangle */

      GetWindowRect(hWnd, &rectWnd);

      /*  get the DIB of the window by calling
       *  CopyScreenToDIB and passing it the window rect
       */
      hDIB = CopyScreenToDIB(&rectWnd);
   }
      break;

   case PW_CLIENT: // copy client area
   {
      RECT rectClient;
      POINT pt1, pt2;

      /* get the client area dimensions */

      GetClientRect(hWnd, &rectClient);

      /* convert client coords to screen coords */
      pt1.x = rectClient.left;
      pt1.y = rectClient.top;
      pt2.x = rectClient.right;
      pt2.y = rectClient.bottom;
      ClientToScreen(hWnd, &pt1);
      ClientToScreen(hWnd, &pt2);
      rectClient.left = pt1.x;
      rectClient.top = pt1.y;
      rectClient.right = pt2.x;
      rectClient.bottom = pt2.y;

      /*  get the DIB of the client area by calling
       *  CopyScreenToDIB and passing it the client rect
       */
      hDIB = CopyScreenToDIB(&rectClient);
   }
      break;

   default:    // invalid print area
      return NULL;
      }

   /* return the handle to the DIB */
   return hDIB;
}


/*************************************************************************
 *
 * CopyScreenToDIB()
 *
 * Parameter:
 *
 * LPRECT lpRect    - specifies the window
 *
 * Return Value:
 *
 * HDIB             - identifies the device-independent bitmap
 *
 * Description:
 *
 * This function copies the specified part of the screen to a device-
 * independent bitmap.
 *
 ************************************************************************/


HDIB CopyScreenToDIB(LPRECT lpRect)
{
   HBITMAP hBitmap;    // handle to device-dependent bitmap
   HPALETTE hPalette;  // handle to palette
   HDIB hDIB = NULL;   // handle to DIB

   /*  get the device-dependent bitmap in lpRect by calling
    *  CopyScreenToBitmap and passing it the rectangle to grab
    */

   hBitmap = CopyScreenToBitmap(lpRect);

   /* check for a valid bitmap handle */
   if (!hBitmap)
      return NULL;

   /* get the current palette */
   hPalette = GetSystemPalette();

   /* convert the bitmap to a DIB */
   hDIB = BitmapToDIB(hBitmap, hPalette);

   /* clean up */
   DeleteObject(hBitmap);

   /* return handle to the packed-DIB */
   return hDIB;
}


/*************************************************************************
 *
 * CopyWindowToBitmap()
 *
 * Parameters:
 *
 * HWND hWnd        - specifies the window
 *
 * WORD fPrintArea  - specifies the window area to copy into the device-
 *                    dependent bitmap
 *
 * Return Value:
 *
 * HDIB         - identifies the device-dependent bitmap
 *
 * Description:
 *
 * This function copies the specified part(s) of the window to a device-
 * dependent bitmap.
 *
 ************************************************************************/


HBITMAP CopyWindowToBitmap(HWND hWnd, WORD fPrintArea)
{
   HBITMAP hBitmap = NULL;  // handle to device-dependent bitmap

   /* check for a valid window handle */

   if (!hWnd)
      return NULL;
   switch (fPrintArea)
      {
   case PW_WINDOW: // copy entire window
   {
      RECT rectWnd;

      /* get the window rectangle */

      GetWindowRect(hWnd, &rectWnd);

      /*  get the bitmap of that window by calling
       *  CopyScreenToBitmap and passing it the window rect
       */
      hBitmap = CopyScreenToBitmap(&rectWnd);
   }
      break;

   case PW_CLIENT: // copy client area
   {
      RECT rectClient;
      POINT pt1, pt2;

      /* get client dimensions */

      GetClientRect(hWnd, &rectClient);

      /* convert client coords to screen coords */
      pt1.x = rectClient.left;
      pt1.y = rectClient.top;
      pt2.x = rectClient.right;
      pt2.y = rectClient.bottom;
      ClientToScreen(hWnd, &pt1);
      ClientToScreen(hWnd, &pt2);
      rectClient.left = pt1.x;
      rectClient.top = pt1.y;
      rectClient.right = pt2.x;
      rectClient.bottom = pt2.y;

      /*  get the bitmap of the client area by calling
       *  CopyScreenToBitmap and passing it the client rect
       */
      hBitmap = CopyScreenToBitmap(&rectClient);
   }
      break;

   default:    // invalid print area
      return NULL;
      }

   /* return handle to the bitmap */
   return hBitmap;
}


/*************************************************************************
 *
 * CopyScreenToBitmap()
 *
 * Parameter:
 *
 * LPRECT lpRect    - specifies the window
 *
 * Return Value:
 *
 * HDIB             - identifies the device-dependent bitmap
 *
 * Description:
 *
 * This function copies the specified part of the screen to a device-
 * dependent bitmap.
 *
 ************************************************************************/


HBITMAP CopyScreenToBitmap(LPRECT lpRect)
{
   HDC hScrDC, hMemDC;           // screen DC and memory DC
   HBITMAP hBitmap, hOldBitmap;  // handles to deice-dependent bitmaps
   int nX, nY, nX2, nY2;         // coordinates of rectangle to grab
   int nWidth, nHeight;          // DIB width and height
   int xScrn, yScrn;             // screen resolution

   /* check for an empty rectangle */

   if (IsRectEmpty(lpRect))
      return NULL;

   /*  create a DC for the screen and create
    *  a memory DC compatible to screen DC
    */
   hScrDC = CreateDC("DISPLAY", NULL, NULL, NULL);
   hMemDC = CreateCompatibleDC(hScrDC);

   /* get points of rectangle to grab */
   nX = lpRect->left;
   nY = lpRect->top;
   nX2 = lpRect->right;
   nY2 = lpRect->bottom;

   /* get screen resolution */
   xScrn = GetDeviceCaps(hScrDC, HORZRES);
   yScrn = GetDeviceCaps(hScrDC, VERTRES);

   /* make sure bitmap rectangle is visible */
   if (nX < 0)
      nX = 0;
   if (nY < 0)
      nY = 0;
   if (nX2 > xScrn)
      nX2 = xScrn;
   if (nY2 > yScrn)
      nY2 = yScrn;
   nWidth = nX2 - nX;
   nHeight = nY2 - nY;

   /* create a bitmap compatible with the screen DC */
   hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);

   /* select new bitmap into memory DC */
   hOldBitmap = SelectObject(hMemDC, hBitmap);

   /* bitblt screen DC to memory DC */
   BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, nX, nY, SRCCOPY);

   /*  select old bitmap back into memory DC and get handle to
    *  bitmap of the screen
    */
   hBitmap = SelectObject(hMemDC, hOldBitmap);

   /* clean up */
   DeleteDC(hScrDC);
   DeleteDC(hMemDC);

   /* return handle to the bitmap */
   return hBitmap;
}
