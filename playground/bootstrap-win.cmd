set CWD=%cd%

md build-win-Debug 2> NUL
cd build-win-Debug
cmake -DCMAKE_BUILD_TYPE=Debug -A x64 -DARCH=Win64 -DPODOFO_ENABLE_TOOLS=TRUE ..

cd "%CWD%
md build-win-Release 2> NUL
cd build-win-Release
cmake -DCMAKE_BUILD_TYPE=Release -A x64 -DARCH=Win64 -DPODOFO_ENABLE_TOOLS=TRUE ..

cd "%CWD%
pause