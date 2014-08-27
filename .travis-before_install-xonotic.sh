#!/bin/sh

set -ex

if [ "`uname`" = 'Linux' ]; then
  sudo apt-get update -qq
fi

for os in "$@"; do
  case "$os" in
    linux32)
      # Prepare an i386 chroot. This is required as we otherwise can't install
      # our dependencies to be able to compile a 32bit binary. Ubuntu...
      chroot="$PWD"/buildroot.i386
      mkdir -p "$chroot$PWD"
      sudo apt-get install -y debootstrap
      sudo i386 debootstrap --arch=i386 precise "$chroot"
      sudo mount --rbind "$PWD" "$chroot$PWD"
      sudo i386 chroot "$chroot" apt-get install -y \
        build-essential
      # Now install our dependencies.
      sudo i386 chroot "$chroot" apt-get install -y \
        libxpm-dev libsdl1.2-dev libxxf86vm-dev
      wget https://www.libsdl.org/release/SDL2-2.0.3.tar.gz
      tar xf SDL2-2.0.3.tar.gz
      (
      cd SDL2-2.0.3
      sudo i386 chroot "$chroot" sh -c "cd $PWD && ./configure --enable-static --disable-shared"
      sudo i386 chroot "$chroot" make -C "$PWD"
      sudo i386 chroot "$chroot" make -C "$PWD" install
      )
      ;;
    linux64)
      sudo apt-get install -y \
        libxpm-dev libsdl1.2-dev libxxf86vm-dev
      wget https://www.libsdl.org/release/SDL2-2.0.3.tar.gz
      tar xf SDL2-2.0.3.tar.gz
      (
      cd SDL2-2.0.3
      ./configure --enable-static --disable-shared
      make
      sudo make install
      )
      ;;
    win32)
      sudo apt-get install -y \
        mingw-w64 mingw32- mingw32-binutils-
      ;;
    win64)
      sudo apt-get install -y \
        mingw-w64 mingw32- mingw32-binutils-
      ;;
    osx)
      git archive --format=tar --remote=git://de.git.xonotic.org/xonotic/xonotic.git \
        --prefix=SDL.framework/ master:misc/buildfiles/osx/Xonotic.app/Contents/Frameworks/SDL.framework | tar xvf -
      ;;
  esac
done

git archive --format=tar --remote=git://de.git.xonotic.org/xonotic/xonotic.git \
  --prefix=.deps/ master:misc/builddeps | tar xvf -
for X in .deps/*; do
  rsync --remove-source-files -aL "$X"/*/ "$X"/ || true
done
