/*
 * File: fscan.r
 *  Contents: move, pos, tab.
 */

"move(i) - move &pos by i, return substring of &subject spanned."
" Reverses effects if resumed."

function{0,1+} move(i)

   if !cnv:C_integer(i) then
      runerr(101,i)

   abstract {
      return string
      }

   body {
      register C_integer j;
      C_integer oldpos;

      /*
       * Save old &pos.  Local variable j holds &pos before the move.
       */
      oldpos = j = k_pos;

      /*
       * If attempted move is past either end of the string, fail.
       */
      if (i + j <= 0 || i + j > StrLen(k_subject) + 1)
         fail;

      /*
       * Set new &pos.
       */
      k_pos += i;
      EVVal(k_pos, E_Spos);

      /*
       * Make sure i >= 0.
       */
      if (i < 0) {
         j += i;
         i = -i;
         }

      /*
       * Suspend substring of &subject that was moved over.
       */
      suspend string(i, StrLoc(k_subject) + j - 1);

      /*
       * If move is resumed, restore the old position and fail.
       */
      if (oldpos > StrLen(k_subject) + 1)
         runerr(205, kywd_pos);
      else {
         k_pos = oldpos;
         EVVal(k_pos, E_Spos);
         }

      fail;
      }
end


"pos(i) - test if &pos is at position i in &subject."

function{0,1} pos(i)

   if !cnv:C_integer(i) then
      runerr(101, i)

   abstract {
      return integer
      }
   body {
      /*
       * Fail if &pos is not equivalent to i, return i otherwise.
       */
      if ((i = cvpos(i, StrLen(k_subject))) != k_pos)
         fail;
      return C_integer i;
      }
end


"tab(i) - set &pos to i, return substring of &subject spanned."
"Reverses effects if resumed."

function{0,1+} tab(i)

   if !cnv:C_integer(i) then
      runerr(101, i);

   abstract {
      return string
      }

   body {
      C_integer j, t, oldpos;

      /*
       * Convert i to an absolute position.
       */
      i = cvpos(i, StrLen(k_subject));
      if (i == CvtFail)
         fail;

      /*
       * Save old &pos.  Local variable j holds &pos before the tab.
       */
      oldpos = j = k_pos;

      /*
       * Set new &pos.
       */
      k_pos = i;
      EVVal(k_pos, E_Spos);

      /*
       *  Make i the length of the substring &subject[i:j]
       */
      if (j > i) {
         t = j;
         j = i;
         i = t - j;
         }
      else
         i = i - j;

      /*
       * Suspend the portion of &subject that was tabbed over.
       */
      suspend string(i, StrLoc(k_subject) + j - 1);

      /*
       * If tab is resumed, restore the old position and fail.
       */
      if (oldpos > StrLen(k_subject) + 1)
         runerr(205, kywd_pos);
      else {
         k_pos = oldpos;
         EVVal(k_pos, E_Spos);
         }

      fail;
      }
end
