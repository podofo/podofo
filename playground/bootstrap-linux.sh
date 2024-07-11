set -xe

export pwd=`pwd`
mkdir -p build-linux-Release
cd build-linux-Release
cmake -DCMAKE_BUILD_TYPE=Release -DARCH=linux-x86_64 -DPODOFO_BUILD_UNSUPPORTED_TOOLS=TRUE ..

cd $pwd
mkdir -p build-linux-Debug
cd build-linux-Debug
cmake -DCMAKE_BUILD_TYPE=Debug -DARCH=linux-x86_64 -DPODOFO_BUILD_UNSUPPORTED_TOOLS=TRUE ..
