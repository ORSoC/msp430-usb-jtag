aclocal --force
autoheader -f
libtoolize --copy --force
automake -caf --foreign
autoconf -f
CC=msp430-gcc CFLAGS="-mmcu=msp430f5510" ./configure --target=msp430 --host=x86_64-unknown-linux-gnu
