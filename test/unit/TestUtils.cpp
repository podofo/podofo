/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "TestUtils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

#ifdef CreateFont
#undef CreateFont
#endif // CreateFont

#ifdef DrawText
#undef DrawText
#endif // DrawText

#endif // _WIN32 || _WIN64

#include <podofo.h>

std::string TestUtils::getTempFilename()
{
    const long lLen = 256;
    char tmpFilename[lLen];
#if defined(_WIN32) || defined(_WIN64)
	char tmpDir[lLen];
	GetTempPathA(lLen, tmpDir);
	GetTempFileNameA(tmpDir, "podofo", 0, tmpFilename);
#else
    strncpy( tmpFilename, "/tmp/podofoXXXXXX", lLen);
    int handle = mkstemp(tmpFilename);
    close(handle);
#endif // _WIN32 || _WIN64

    printf("Created tempfile: %s\n", tmpFilename);
    std::string sFilename = tmpFilename;
    return sFilename;
}

void TestUtils::deleteFile( const char* pszFilename )
{
#if defined(_WIN32) || defined(_WIN64)
    _unlink(pszFilename);
#else
    unlink(pszFilename);
#endif // _WIN32 || _WIN64
}

char* TestUtils::readDataFile( const char* pszFilename )
{
    // TODO: determine correct prefix during runtime
    std::string sFilename = "/home/dominik/podofotmp/test/unit/data/";
    sFilename = sFilename + pszFilename;
    PoDoFo::PdfFileInputStream stream( sFilename.c_str() );
    long lLen = stream.GetFileLength();

    char* pBuffer = static_cast<char*>(malloc(sizeof(char) * lLen));
    stream.Read(pBuffer, lLen);

    return pBuffer;
}


