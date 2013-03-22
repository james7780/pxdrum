echo $1
SRCDIR=/home/james/pspsdl/pad
DESTDIR=/media/$1/PSP/GAME/pad
echo $SRCDIR
echo $DESTDIR
rm -Rvf $DESTDIR/*
cp -u $SRCDIR/EBOOT.PBP $DESTDIR/EBOOT.PBP
cp -u $SRCDIR/PARAM.SFO $DESTDIR
cp -u $SRCDIR/joymap.psp.cfg $DESTDIR
mkdir $DESTDIR/kits
mkdir $DESTDIR/songs
mkdir $DESTDIR/gfx
mkdir $DESTDIR/help

