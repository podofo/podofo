/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <string>
#include <vector>

class ImageConverter {
public:
    ImageConverter();
    ~ImageConverter();

    inline void SetOutputFilename( const char* pszFilename ) {
        m_sOutputFilename = pszFilename;
    }

    inline void AddImage( const char* pszImage ) {
        m_vecImages.push_back( std::string(pszImage) );
    }

    inline void SetUseImageSize( bool bImageSize ) {
        m_bUseImageSize = bImageSize;
    }

    void Work();

private:
    std::vector<std::string> m_vecImages;
    std::string              m_sOutputFilename;
    bool                     m_bUseImageSize;
};
