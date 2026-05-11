set CWD=%cd%

md build-win-Debug 2> NUL
cd build-win-Debug
cmake -DCMAKE_BUILD_TYPE=Debug -A x64 -DARCH=Win64 -DPODOFO_BUILD_UNSUPPORTED_TOOLS=TRUE -DPODOFO_WITH_WIN32GDI_FONT_SEARCH=ON ..

cd "%CWD%
md build-win-Release 2> NUL
cd build-win-Release
cmake -DCMAKE_BUILD_TYPE=Release -A x64 -DARCH=Win64 -DPODOFO_BUILD_UNSUPPORTED_TOOLS=TRUE -DPODOFO_WITH_WIN32GDI_FONT_SEARCH=ON ..

cd "%CWD%
pause