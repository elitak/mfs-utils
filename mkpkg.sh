#!/bin/sh

set -x

TMPDIR=/tmp
DATESTR=`date +'%Y%m%d'`
SRCPKG=$TMPDIR/mfs_src-$DATESTR.tar.bz2
NOARCHPKG=$TMPDIR/mfs_noarch-$DATESTR.tar.bz2
MIPSPKG=$TMPDIR/mfs_mips-$DATESTR.tar.bz2
PPCPKG=$TMPDIR/mfs_ppc-$DATESTR.tar.bz2
I386PKG=$TMPDIR/mfs_i386-$DATESTR.tar.bz2

make clean
(cd ..; tar czvf $SRCPKG mfs_vplay_tserver)
(cd ..; tar czvf $NOARCHPKG mfs_vplay_tserver/{*.txt,README*,*.patch})

make ARCH=i386 STATIC=1
make ARCH=mips
make ARCH=ppc

# Remove some programs that aren't needed in the distribution
rm -f bin.*/{mfs_bitmap,mfs_findzero,mfs_getslice,mfs_poke,mfs_purge,sd-h400_unlock,mfs_dump}
rm -f bin.i386/{vserver,tserver,NowShowing,mfs_tzoffset}
rm -f bin.{mips,ppc}/{vplay,vsplit}

(cd ..; tar cvf - mfs_vplay_tserver/bin.i386 | bzip2 -9 > $I386PKG)
(cd ..; tar cvf - mfs_vplay_tserver/bin.mips | bzip2 -9 > $MIPSPKG)
(cd ..; tar cvf - mfs_vplay_tserver/bin.ppc  | bzip2 -9 > $PPCPKG)

make clean

