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
make PLATFORM=IOS

cd "$scriptpath"
rm -rf ../third-party/leveldb-ios
mkdir -p ../third-party/leveldb-ios
cp -R "$tempbuilddir/leveldb/include" ../third-party/leveldb-ios
mkdir -p ../third-party/leveldb-ios/lib
cp -R "$tempbuilddir/leveldb/libleveldb.a" ../third-party/leveldb-ios/lib

rm -rf "$tempbuilddir"
