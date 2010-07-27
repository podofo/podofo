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

#ifndef _TEST_UTILS_H_
#define _TEST_UTILS_H_

#include <string>

/**
 * This class contains utility methods that are
 * often needed when writing tests.
 */
class TestUtils {

public:
    static std::string getTempFilename();
    static void deleteFile( const char* pszFilename );
    
    /**
     * Read a test data file into memory and return a malloc'ed buffer.
     *
     * @param pszFilename filename of the data file. The path will be determined automatically.
     */
    static char* readDataFile( const char* pszFilename );
};

#endif // _TEST_UTILS_H_
