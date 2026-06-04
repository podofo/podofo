set -xe

cmake -B build-linux-Debug -DCMAKE_BUILD_TYPE=Debug
cmake -B build-linux-Release -DCMAKE_BUILD_TYPE=Release
