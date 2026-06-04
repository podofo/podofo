set -xe

cmake -B build-macos-Debug -DCMAKE_BUILD_TYPE=Debug
cmake -B build-macos-Release -DCMAKE_BUILD_TYPE=Release
