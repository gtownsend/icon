#!/usr/local/bin/perl
#
# Usage: xpm1to3.pl xpmv1-file > xpmv3-file
#
# Note:  perl (available by ftp on prep.ai.mit.edu) script to convert 
#        "enhanced" xpm v1 X11 pixmap files to xpm v3 (C includable format)
#        pixmap files...
# +---------------------------------------------------------------------------
# WHO:    Richard Hess                    CORP:   Consilium
# TITLE:  Staff Engineer                  VOICE:  [415] 691-6342
#     [ X-SWAT Team:  Special Projects ]  USNAIL: 640 Clyde Court
# UUCP:   ...!uunet!cimshop!rhess                 Mountain View, CA 94043
# +---------------------------------------------------------------------------

sub checkname {
    if ($_[0] ne $_[1]) {
	printf STDERR "warning, name inconsitencies in %s %s!=%s\n", 
                      $_[2], $_[0], $_[1];
    }
}

sub checkmono {
    if ($_[0] ne $_[1]) { return 0; }
    return 1;
}

printf "/* XPM */\n";
($name, $format) = (<> =~ /^#define\s+(\w+)_format\s+(\d+)\s*$/);
($name2, $width) = (<> =~ /^#define\s+(\w+)_width\s+(\d+)\s*$/);
&checkname($name, $name2, "width");
($name2, $height) = (<> =~ /^#define\s+(\w+)_height\s+(\d+)\s*$/);
&checkname($name, $name2, "height");
($name2, $ncolors) = (<> =~ /^#define\s+(\w+)_ncolors\s+(\d+)\s*$/);
&checkname($name, $name2, "ncolors");
($name2, $chars_per_pixel) = (<> =~
/^#define\s+(\w+)_chars_per_pixel\s+(\d+)\s*$/);
&checkname($name, $name2, "chars per pixel");

($name2) = (<> =~ /^static char \*\s*(\w+)_mono\[]\s+=\s+{\s*$/);
$mono = &checkmono($name, $name2);

if ($mono) {
  $idx = 0;
  while ( ($_ = <>) =~ m/^\s*"[^"]+"\s*,\s*"[^"]+"(,)?\s*$/ ) {
    ($codes[$idx], $mono_name[$idx]) = /^\s*"([^"]+)"\s*,\s*"([^"]+)"(,)?\s*$/;
    $idx++;
  }
  if ($idx != $ncolors) {
    printf STDERR "Warning, ncolors mismatch reading mono %d != %d\n",
$ncolors, $idx;
  }

  ($name2) = (<> =~ /^static char \*\s*(\w+)_colors\[]\s+=\s+{\s*$/);
  &checkname($name, $name2, "colors");
}

printf "static char * %s[] = {\n", $name;
printf "/* %s pixmap\n * width height ncolors chars_per_pixel */\n", $name;
printf "\"%s %s %s %s \",\n", $width, $height, $ncolors, $chars_per_pixel;

$idx = 0;
while ( ($_ = <>) =~ m/^\s*"[^"]+"\s*,\s*"[^"]+"(,)?\s*$/ ) {
  ($codes[$idx], $color_name[$idx]) = /^\s*"([^"]+)"\s*,\s*"([^"]+)"(,)?\s*$/;
  $idx++;
}
if ($idx != $ncolors) {
  printf STDERR "Warning, ncolors mismatch reading color %d != %d\n",
$ncolors, $idx;
}

for ($idx=0; $idx<$ncolors; $idx++) {
  if ($mono) {
    printf "\"%s m %s c %s \t s c%d \",\n", $codes[$idx],
$mono_name[$idx], $color_name[$idx], $idx;
  }
  else {
    printf "\"%s c %s \t s c%d \",\n", $codes[$idx], $color_name[$idx], $idx;
  }
}

($name2) = ( <> =~ /^static char \*\s*(\w+)_pixels\[]\s+=\s+{\s*$/);
&checkname($name, $name2, "pixels");

printf "/* pixels */\n";
while ( ! ( ($_ = <>) =~ /^}\s*;\s*$/) ) {
	printf "%s", $_;
}
printf "} ;\n";

# -----------------------------------------------------------------------<eof>
