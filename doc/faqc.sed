# <<ARIZONA-ONLY>>

# sed directives to regenerate "table of contents" from faq.htm
# (the output of this must be hand-edited back into faq.htm)


/<H2>/s/.*<H2>\(.*\)<\/H2>.*/<P><STRONG>\1<\/STRONG><BR>/p

/<H3><A NAME=/ {
   N
   s/\n/ /
   s/[^"]*"/<A HREF="#/
   s/<\/A> */ /
   s/<\/H3>.*/<\/A><BR>/p
   }
