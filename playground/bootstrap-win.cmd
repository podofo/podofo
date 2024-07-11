set CWD=%cd%

md build-win-Debug 2> NUL
cd build-win-Debug
cmake -DCMAKE_BUILD_TYPE=Debug -A x64 -DARCH=Win64 -DPODOFO_BUILD_UNSUPPORTED_TOOLS=TRUE -DPODOFO_WANT_WIN32GDI=TRUE ..

cd "%CWD%
md build-win-Release 2> NUL
cd build-win-Release
cmake -DCMAKE_BUILD_TYPE=Release -A x64 -DARCH=Win64 -DPODOFO_BUILD_UNSUPPORTED_TOOLS=TRUE -DPODOFO_WANT_WIN32GDI=TRUE ..

cd "%CWD%
pause