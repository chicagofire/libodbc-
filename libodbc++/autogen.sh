libtoolize -c -f &&
autoheader &&
aclocal -I m4 &&
automake -c -a &&
autoconf
