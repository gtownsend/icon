/*
 * File: fstranl.r
 * String analysis functions: any,bal,find,many,match,upto
 *
 * str_anal is a macro for performing the standard conversions and
 *  defaulting for string analysis functions. It takes as arguments the
 *  parameters for subject, beginning position, and ending position. It
 *  produces declarations for these 3 names prepended with cnv_. These
 *  variables will contain the converted versions of the arguments.
 */
#begdef str_anal(s, i, j)
   declare {
      C_integer cnv_ ## i;
      C_integer cnv_ ## j;
      }

   abstract {
      return integer
      }

   if is:null(s) then {
      inline {
         s = k_subject;
         }
      if is:null(i) then inline {
         cnv_ ## i = k_pos;
         }
      }
   else {
      if !cnv:string(s) then
         runerr(103,s)
      if is:null(i) then inline {
         cnv_ ## i = 1;
         }
      }

   if !is:null(i) then
      if cnv:C_integer(i,cnv_ ## i) then inline {
         if ((cnv_ ## i = cvpos(cnv_ ## i, StrLen(s))) == CvtFail)
            fail;
         }
      else
         runerr(101,i)


    if is:null(j) then inline {
       cnv_ ## j = StrLen(s) + 1;
       }
    else if cnv:C_integer(j,cnv_ ## j) then inline {
       if ((cnv_ ## j = cvpos(cnv_ ## j, StrLen(s))) == CvtFail)
          fail;
       if (cnv_ ## i > cnv_ ## j) {
          register C_integer tmp;
          tmp = cnv_ ## i;
          cnv_ ## i = cnv_ ## j;
          cnv_ ## j = tmp;
          }
       }
    else
       runerr(101,j)

#enddef


"any(c,s,i1,i2) - produces i1+1 if i2 is greater than 1 and s[i] is contained "
"in c and poseq(i2,x) is greater than poseq(i1,x), but fails otherwise."

function{0,1} any(c,s,i,j)
   str_anal( s, i, j )
   if !cnv:tmp_cset(c) then
      runerr(104,c)
   body {
      if (cnv_i == cnv_j)
         fail;
      if (!Testb(StrLoc(s)[cnv_i-1], c))
         fail;
      return C_integer cnv_i+1;
      }
end


"bal(c1,c2,c3,s,i1,i2) - generates the sequence of integer positions in s up to"
" a character of c1 in s[i1:i2] that is balanced with respect to characters in"
" c2 and c3, but fails if there is no such position."

function{*} bal(c1,c2,c3,s,i,j)
   str_anal( s, i, j )
   if !def:tmp_cset(c1,fullcs) then
      runerr(104,c1)
   if !def:tmp_cset(c2,lparcs) then
      runerr(104,c2)
   if !def:tmp_cset(c3,rparcs) then
      runerr(104,c3)

   body {
      C_integer cnt;
      char c;

      /*
       * Loop through characters in s[i:j].  When a character in c2
       * is found, increment cnt; when a character in c3 is found, decrement
       * cnt.  When cnt is 0 there have been an equal number of occurrences
       * of characters in c2 and c3, i.e., the string to the left of
       * i is balanced.  If the string is balanced and the current character
       * (s[i]) is in c, suspend with i.  Note that if cnt drops below
       *  zero, bal fails.
       */
      cnt = 0;
      while (cnv_i < cnv_j) {
         c = StrLoc(s)[cnv_i-1];
         if (cnt == 0 && Testb(c, c1)) {
            suspend C_integer cnv_i;
            }
         if (Testb(c, c2))
            cnt++;
         else if (Testb(c, c3))
            cnt--;
         if (cnt < 0)
            fail;
         cnv_i++;
         }
      /*
       * Eventually fail.
       */
      fail;
      }
end


"find(s1,s2,i1,i2) - generates the sequence of positions in s2 at which "
"s1 occurs as a substring in s2[i1:i2], but fails if there is no such position."

function{*} find(s1,s2,i,j)
   str_anal( s2, i, j )
   if !cnv:string(s1) then
      runerr(103,s1)

   body {
      register char *str1, *str2;
      C_integer s1_len, l, term;

      /*
       * Loop through s2[i:j] trying to find s1 at each point, stopping
       * when the remaining portion s2[i:j] is too short to contain s1.
       * Optimize me!
       */
      s1_len = StrLen(s1);
      term = cnv_j - s1_len;
      while (cnv_i <= term) {
         str1 = StrLoc(s1);
         str2 = StrLoc(s2) + cnv_i - 1;
         l    = s1_len;

         /*
          * Compare strings on a byte-wise basis; if the end is reached
          * before inequality is found, suspend with the position of the
          * string.
          */
         do {
            if (l-- <= 0) {
               suspend C_integer cnv_i;
               break;
               }
            } while (*str1++ == *str2++);
         cnv_i++;
         }
      fail;
      }
end


"many(c,s,i1,i2) - produces the position in s after the longest initial "
"sequence of characters in c in s[i1:i2] but fails if there is none."

function{0,1} many(c,s,i,j)
   str_anal( s, i, j )
   if !cnv:tmp_cset(c) then
      runerr(104,c)
   body {
      C_integer start_i = cnv_i;
      /*
       * Move i along s[i:j] until a character that is not in c is found
       *  or the end of the string is reached.
       */
      while (cnv_i < cnv_j) {
         if (!Testb(StrLoc(s)[cnv_i-1], c))
            break;
         cnv_i++;
         }
      /*
       * Fail if no characters in c were found; otherwise
       *  return the position of the first character not in c.
       */
      if (cnv_i == start_i)
         fail;
      return C_integer cnv_i;
      }
end


"match(s1,s2,i1,i2) - produces i1+*s1 if s1==s2[i1+:*s1], but fails otherwise."

function{0,1} match(s1,s2,i,j)
   str_anal( s2, i, j )
   if !cnv:tmp_string(s1) then
      runerr(103,s1)
   body {
      char *str1, *str2;

      /*
       * Cannot match unless s2[i:j] is as long as s1.
       */
      if (cnv_j - cnv_i < StrLen(s1))
         fail;

      /*
       * Compare s1 with s2[i:j] for *s1 characters; fail if an
       *  inequality is found.
       */
      str1 = StrLoc(s1);
      str2 = StrLoc(s2) + cnv_i - 1;
      for (cnv_j = StrLen(s1); cnv_j > 0; cnv_j--)
         if (*str1++ != *str2++)
            fail;

      /*
       * Return position of end of matched string in s2.
       */
      return C_integer cnv_i + StrLen(s1);
      }
end


"upto(c,s,i1,i2) - generates the sequence of integer positions in s up to a "
"character in c in s[i2:i2], but fails if there is no such position."

function{*} upto(c,s,i,j)
   str_anal( s, i, j )
   if !cnv:tmp_cset(c) then
      runerr(104,c)
   body {
      C_integer tmp;

      /*
       * Look through s[i:j] and suspend position of each occurrence of
       * of a character in c.
       */
      while (cnv_i < cnv_j) {
         tmp = (C_integer)StrLoc(s)[cnv_i-1];
         if (Testb(tmp, c)) {
            suspend C_integer cnv_i;
            }
         cnv_i++;
         }
      /*
       * Eventually fail.
       */
      fail;
      }
end
