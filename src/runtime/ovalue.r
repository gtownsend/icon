/*
 * File: ovalue.r
 *  Contents: nonnull, null, value, conj
 */

"\\x - test x for nonnull value."

operator{0,1} \ nonnull(underef x -> dx)
   abstract {
      return type(x)
      }
   /*
    * If the dereferenced value dx is not null, the pre-dereferenced
    *  x is returned, otherwise, the function fails.
    */
   if is:null(dx) then
      inline {
         fail;
         }
   else {
      inline {
         return x;
         }
      }
end



"/x - test x for null value."

operator{0,1} / null(underef x -> dx)
   abstract {
      return type(x)
      }
   /*
    * If the dereferenced value dx is null, the pre-derefereneced value
    *  x is returned, otherwise, the function fails.
    */
   if is:null(dx) then {
      inline {
         return x;
         }
      }
   else
      inline {
         fail;
      }
end


".x - produce value of x."

operator{1} . value(x)
  abstract {
     return type(x)
     }
  inline {
     return x;
     }
end


"x & y - produce value of y."

operator{1} & conj(underef x, underef y)
   abstract {
      return type(y)
      }
   inline {
      return y;
      }
end
