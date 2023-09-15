#!/bin/sh

# This script is used to compile the executable for a public release package.
# You don't need it for interim development builds.

PLAT=$1
CLEAN=$2
ARGS="Usage: compile [win32|macosx|linux] [clean: true|false]"

if [ "$PLAT" != "win32" -a "$PLAT" != "macosx" -a "$PLAT" != "linux" ]; then
    echo $ARGS
    exit 1
fi

# linux function that finds the named library LIBNAME in the 
# executable EXEPATH's dependency list and copies it to DESTPATH
copy_dependency_lib()
{
    local LIBNAME=$1
    local EXEPATH=$2
    local DESTPATH=$3

    LDDOUT=`ldd $EXEPATH`
    if [ $? -ne 0 ]; then
        # error: ldd failed to run.
        return 1
    fi
    LDDOUT=`ldd $EXEPATH | grep "$LIBNAME"`
    if [ $? -ne 0 ]; then
        # library not found.  this isn't an error.
        echo "Library $LIBNAME was not found in $EXEPATH, skipping"
        return 0
    fi

    SRCPATH=`echo $LDDOUT | awk '{ print $3 }'`
    DEPNAME=`echo $LDDOUT | awk '{ print $1 }'`
    if [ $? -ne 0 ]; then
        return 1
    else
        if [ ! -d $DESTPATH ]; then
            mkdir -p $DESTPATH
        fi
        cp -v $SRCPATH $DESTPATH/$DEPNAME
        if [ $? -ne 0 ]; then
            return 1
        fi
        chmod u+w $DESTPATH/$DEPNAME
    fi
    echo "Copied $SRCPATH to $DESTPATH/$DEPNAME"
    return 0
}

cd ..

if [ "$PLAT" = "win32" ]; then
    echo "win32 compile unimplemented"
fi
if [ "$PLAT" = "macosx" ]; then
    echo "macosx compile unimplemented"
fi
if [ "$PLAT" = "linux" ]; then
    COMPILER=GCC2
    BUILD=RELEASE
    make -f mk/configure.mk OS=LINUX COMPILER=$COMPILER BUILD=$BUILD
    if [ "$CLEAN" == "true" ]; then
        echo "Making clean"
        make clean
    fi
    make -j2 
    if [ $? -ne 0 ]; then
        echo "Errors during compile, aborted"
        cd install_build
        exit 1
    fi

    echo "Cleaning example/lib..."
    rm -rf example/lib/*
    echo "Looking for library dependencies..."
    # find the exe's libstdc++ and stick it in the example/lib subfolder
    copy_dependency_lib stdc++ engine/out.$COMPILER.$BUILD/torqueDemo.bin example/lib
    if [ $? -ne 0 ]; then
        echo "Failed to copy stdc++ to example/lib.  You will need to copy it manually."
    fi

    # gcc3 also uses the "gcc_s" library
    copy_dependency_lib gcc_s engine/out.$COMPILER.$BUILD/torqueDemo.bin example/lib
    if [ $? -ne 0 ]; then
        echo "Failed to copy gcc_s example/lib.  You will need to copy it manually."
    fi

    # while we're at it, copy the SDL and openal libraries
    cp -av lib/sdl/linux/*SDL* example/lib
    cp -av lib/openal/LINUX/libopenal.so example/lib

    # strip all libraries
    strip -p example/lib/*
fi

cd install_build
