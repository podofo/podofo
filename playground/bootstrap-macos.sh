set -xe

export pwd=`pwd`
mkdir -p build-macos-Release
cd build-macos-Release
cmake -DCMAKE_BUILD_TYPE=Release -DARCH=macosx-x86_64 -DPODOFO_BUILD_UNSUPPORTED_TOOLS=TRUE ..

cd $pwd
mkdir -p build-macos-Debug
cd build-macos-Debug
cmake -DCMAKE_BUILD_TYPE=Debug -DARCH=macosx-x86_64 -DPODOFO_BUILD_UNSUPPORTED_TOOLS=TRUE ..
