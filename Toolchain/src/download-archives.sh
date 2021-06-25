#!/bin/bash

if [ $# != 2 ]; then
  echo "Usage: download-archives.sh <gcc-version> <binutils-version>"
  exit 1
fi

GCC_VER=$1
BINUTILS_VER=$2

GCC_ARCHIVE_URL="https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VER/gcc-$GCC_VER.tar.xz"
GCC_ARCHIVE_PATH="./gcc-$GCC_VER.tar.xz"
GCC_TARGET_DIR="./gcc"

BINUTILS_ARCHIVE_URL="https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VER.tar.xz"
BINUTILS_ARCHIVE_PATH="./binutils-$BINUTILS_VER.tar.xz"
BINUTILS_TARGET_DIR="./binutils"


#echo "Downloading GCC sources.."
#curl --request GET -L \
#     --url "$GCC_ARCHIVE_URL" \
#     --output "$GCC_ARCHIVE_PATH"
#if [ $? -ne 0 ]; then
#    echo "Failed downloading GCC archive: curl returned $?"
#    exit 1
#fi
#
#
#echo "Downloading binutils sources.."
#curl --request GET -L \
#     --url "$BINUTILS_ARCHIVE_URL" \
#     --output "$BINUTILS_ARCHIVE_PATH"
#if [ $? -ne 0 ]; then
#    echo "Failed downloading binutils archive: curl returned $?"
#    exit 1
#fi

if [ -d "$GCC_TARGET_DIR" ]; then
  echo "Removing previous GCC source folder"
  rm -rf "$GCC_TARGET_DIR"
fi

if [ -d "$BINUTILS_TARGET_DIR" ]; then
  echo "Removing previous binutils source folder"
  rm -rf "$BINUTILS_TARGET_DIR"
fi


mkdir "$GCC_TARGET_DIR"
mkdir "$BINUTILS_TARGET_DIR"


echo "Extracting '$GCC_ARCHIVE_PATH'.."
tar xf "$GCC_ARCHIVE_PATH" -C "$GCC_TARGET_DIR" --strip-components=1
if [ $? -ne 0 ]; then
  echo "Failed extracting GCC archive '$GCC_ARCHIVE_PATH': tar returned $?"
  exit 2
fi


echo "Extracting '$BINUTILS_ARCHIVE_PATH'.."
tar xf "$BINUTILS_ARCHIVE_PATH" -C "$BINUTILS_TARGET_DIR" --strip-components=1
if [ $? -ne 0 ]; then
  echo "Failed extracting binutils archive '$BINUTILS_ARCHIVE_PATH': tar returned $?"
  exit 2
fi


echo "Applying GCC patch.."
patch -p0 <gcc.patch

echo "Applying binutils patch.."
patch -p0 <binutils.patch

echo "Applied all patches"

set -e
echo "Regenerating Makefile.in"
cd $BINUTILS_TARGET_DIR/ld/ && aclocal && automake && cd -
#cd $GCC_TARGET_DIR/libstdc++-v3/ && autoreconf && aclocal && autoconf && cd -


echo "Completed"
exit 0
