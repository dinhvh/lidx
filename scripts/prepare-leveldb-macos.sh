#!/bin/sh

pushd `dirname $0` > /dev/null
scriptpath=`pwd`
popd > /dev/null
builddir="$scriptpath/builds"
BUILD_TIMESTAMP=`date +'%Y%m%d%H%M%S'`
tempbuilddir="$builddir/workdir/$BUILD_TIMESTAMP"
mkdir -p "$tempbuilddir"

cd "$tempbuilddir"
git clone --depth=1 https://github.com/dinhviethoa/leveldb
cd leveldb
make OPT="-arch x86_64"
mv libleveldb.a libleveldb-x86_64.a
make clean
make OPT="-arch i386"
mv libleveldb.a libleveldb-i386.a
lipo -create libleveldb-x86_64.a libleveldb-i386.a -output libleveldb.a

cd "$scriptpath"
rm -rf ../third-party/leveldb
mkdir -p ../third-party/leveldb
cp -R "$tempbuilddir/leveldb/include" ../third-party/leveldb
mkdir -p ../third-party/leveldb/lib
cp -R "$tempbuilddir/leveldb/libleveldb.a" ../third-party/leveldb/lib

rm -rf "$tempbuilddir"
