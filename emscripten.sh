#!/bin/bash

set -e

LIBXML_VERSION=2.9.4
SDL_MIXER_VERSION=2.0.1

cd lib

if [ ! -d "libxml2-$LIBXML_VERSION" ]; then
    wget ftp://xmlsoft.org/libxml2/libxml2-$LIBXML_VERSION.tar.gz
    tar -xf libxml2-$LIBXML_VERSION.tar.gz
    cd libxml2-$LIBXML_VERSION
    patch -p1 < ../libxml2.patch
    emconfigure ./configure --without-python
    emmake make
    cd ..
fi

if [ ! -d "SDL2_mixer-$SDL_MIXER_VERSION" ]; then
    wget https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-$SDL_MIXER_VERSION.tar.gz
    tar -xf SDL2_mixer-$SDL_MIXER_VERSION.tar.gz
    cd SDL2_mixer-$SDL_MIXER_VERSION
    emconfigure ./configure
    emmake make
    cd ..
fi

cd ..

export FREETYPE_CFLAGS="-s USE_FREETYPE=1"
export FREETYPE_LIBS=" "
export XML_CFLAGS="-I/$PWD/lib/libxml2-$LIBXML_VERSION/include"
export XML_LIBS=" "
export VORBIS_CFLAGS="-s USE_VORBIS=1"
export VORBIS_LIBS=" "
export VORBISFILE_CFLAGS=" "
export VORBISFILE_LIBS=" "
export PNG_CFLAGS="-s USE_LIBPNG=1"
export PNG_LIBS=" "
export SDLMIXER_CFLAGS="-I/$PWD/lib/SDL2_mixer-$SDL_MIXER_VERSION"
export SDLMIXER_LIBS=" "

emconfigure ./configure --disable-debug --without-openal --disable-sdltest

emmake make

if [ ! -f naev.bc ]; then
    ln -s src/naev naev.bc
fi

emcc -s ALLOW_MEMORY_GROWTH=1 -s USE_VORBIS=1 -s USE_OGG=1 -s USE_SDL=2 -s USE_LIBPNG=1 -s LEGACY_GL_EMULATION=1 -s USE_FREETYPE=1 lib/csparse/libcsparse.a lib/libxml2-$LIBXML_VERSION/.libs/libxml2.so lib/SDL2_mixer-$SDL_MIXER_VERSION/build/.libs/libSDL2_mixer.so naev.bc --preload-file dat --preload-file AUTHORS -o naev.html
