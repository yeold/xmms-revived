# Building

The classical makefile build commands should be enough:

`./configure`

`make`

`make install`


To run it without make install:

`LD_LIBRARY_PATH=$(pwd)/libxmms/.libs ./xmms/.libs/xmms`

### Prerequsuties for base building
- alsa
- libasound
- libmikmod
- ogg
- vorbis
- libgtk-2
- gtk2
- glib2
- libgdk_pixbuf-2
- libpango
- libcairo-gobject2
- pkg-config
- libflac