/*
 * Definitions of functions.
 */

FncDef(abs,1)
FncDef(acos,1)
FncDef(any,4)
FncDef(args,1)
FncDef(asin,1)
FncDef(atan,2)
FncDef(bal,6)
FncDef(center,3)
FncDef(char,1)
FncDef(chdir,1)
FncDef(close,1)
FncDef(collect,2)
FncDef(copy,1)
FncDef(cos,1)
FncDef(cset,1)
FncDef(delay,1)
FncDef(delete,2)
FncDefV(detab)
FncDef(dtor,1)
FncDefV(entab)
FncDef(errorclear,0)
FncDef(exit,1)
FncDef(exp,2)
FncDef(find,4)
FncDef(flush,1)
FncDef(function,0)
FncDef(get,2)
FncDef(getenv,1)
FncDef(iand,2)
FncDef(icom,1)
FncDef(image,1)
FncDef(insert,3)
FncDef(integer,1)
FncDef(ior,2)
FncDef(ishift,2)
FncDef(ixor,2)
FncDef(key,2)
FncDef(left,3)
FncDef(list,2)
FncDef(log,1)
FncDef(many,4)
FncDef(map,3)
FncDef(match,4)
FncDef(member,1)
FncDef(move,1)
FncDef(numeric,1)
FncDef(ord,1)
FncDef(pop,1)
FncDef(pos,1)
FncDef(pull,1)
FncDefV(push)
FncDefV(put)
FncDef(read,2)
FncDef(reads,2)
FncDef(real,1)
FncDef(remove,2)
FncDef(rename,1)
FncDef(repl,2)
FncDef(reverse,1)
FncDef(right,3)
FncDef(rtod,1)
FncDefV(runerr)
FncDef(seek,2)
FncDef(seq,2)
FncDef(serial,1)
FncDef(set,1)
FncDef(sin,1)
FncDef(sort,2)
FncDef(sortf,2)
FncDef(sqrt,1)
FncDefV(stop)
FncDef(string,1)
FncDef(system,1)
FncDef(tab,1)
FncDef(table,1)
FncDef(tan,1)
FncDef(trim,2)
FncDef(type,1)
FncDef(upto,4)
FncDef(where,1)
FncDefV(write)
FncDefV(writes)

#ifdef Graphics
   FncDefV(open)
#else					/* Graphics */
   FncDef(open,3)
#endif					/* Graphics */

#ifdef MultiThread
   FncDef(display,3)
   FncDef(name,2)
   FncDef(proc,3)
   FncDef(variable,3)
#else					/* MultiThread */
   FncDef(display,2)
   FncDef(name,1)
   FncDef(proc,2)
   FncDef(variable,1)
#endif					/* MultiThread */

/*
 * Dynamic loading.
 */
#ifdef LoadFunc
   FncDef(loadfunc,2)
#endif					/* LoadFunc */

/*
 * External functions.
 */
#ifdef ExternalFunctions
   FncDefV(callout)
#endif					/* ExternalFunctions */

/*
 * File attribute function.
 */
#ifdef FAttrib
   FncDefV(fattrib)
#endif					/* FAttrib */

/*
 * Keyboard Functions
 */
#ifdef KeyboardFncs
   FncDef(getch,0)
   FncDef(getche,0)
   FncDef(kbhit,0)
#endif					/* KeyboardFncs */

/*
 * Event processing functions.
 */
#ifdef EventMon
   FncDef(EvGet,2)
   FncDef(event,3)
   FncDef(eventmask,2)
   FncDef(opmask,2)
#endif					/* EventMon */

/*
 * Graphics functions.
 */
#ifdef Graphics
   FncDef(Active,0)
   FncDefV(Alert)
   FncDefV(Bg)
   FncDefV(Clip)
   FncDefV(Clone)
   FncDefV(Color)
   FncDefV(ColorValue)
   FncDefV(CopyArea)
   FncDefV(Couple)
   FncDefV(DrawArc)
   FncDefV(DrawCircle)
   FncDefV(DrawCurve)
   FncDefV(DrawImage)
   FncDefV(DrawLine)
   FncDefV(DrawPoint)
   FncDefV(DrawPolygon)
   FncDefV(DrawRectangle)
   FncDefV(DrawSegment)
   FncDefV(DrawString)
   FncDefV(EraseArea)
   FncDefV(Event)
   FncDefV(Fg)
   FncDefV(FillArc)
   FncDefV(FillCircle)
   FncDefV(FillPolygon)
   FncDefV(FillRectangle)
   FncDefV(Font)
   FncDefV(FreeColor)
   FncDefV(GotoRC)
   FncDefV(GotoXY)
   FncDefV(Lower)
   FncDefV(NewColor)
   FncDefV(PaletteChars)
   FncDefV(PaletteColor)
   FncDefV(PaletteKey)
   FncDefV(Pattern)
   FncDefV(Pending)
   FncDefV(Pixel)
   FncDef(QueryPointer,1)
   FncDefV(Raise)
   FncDefV(ReadImage)
   FncDefV(TextWidth)
   FncDef(Uncouple,1)
   FncDefV(WAttrib)
   FncDefV(WDefault)
   FncDefV(WFlush)
   FncDef(WSync,1)
   FncDefV(WriteImage)
   /*
    * Native function extensions for Windows
    */
   #ifdef WinExtns
      FncDefV(WinPlayMedia)
      FncDefV(WinEditRegion)
      FncDefV(WinButton)
      FncDefV(WinScrollBar)
      FncDefV(WinMenuBar)
      FncDefV(WinColorDialog)
      FncDefV(WinFontDialog)
      FncDefV(WinOpenDialog)
      FncDefV(WinSaveDialog)
      FncDefV(WinSelectDialog)
   #endif				/* WinExtns */
#endif					/* Graphics */

#ifdef MultiThread
   /*
    * These functions are part of the MultiThread extensions.
    */
   FncDef(cofail,1)
   FncDef(globalnames,1)
   FncDef(fieldnames,1)
   FncDef(localnames,2)
   FncDef(staticnames,2)
   FncDef(paramnames,2)
   FncDef(structure,1)
   /*
    * These functions are inherent to MultiThread and multiple Icon programs
    */
   FncDefV(load)
   FncDef(parent,1)
   FncDef(keyword,2)
#endif					/* MultiThread */
