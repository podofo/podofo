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

#ifndef _COLORSPACE_H_
#define _COLORSPACE_H_

#include <podofo.h>

/**
 * A more powerful representation
 * of a colorspace than EPdfColorSpace.
 *
 */
class ColorSpace {
public:
    /**
     * Construct a DeviceGray ColorSpace
     */
    ColorSpace();
    
    /**
     * Construct a colorspace from a colorspace name
     */
    ColorSpace(const PoDoFo::PdfName & rName);
    
    /**
     * Copy constructor
     */
    ColorSpace(const ColorSpace & rhs);
    ~ColorSpace();
    
    const ColorSpace & operator=(const ColorSpace & rhs);

    /**
     * Checks if this is a "simple"
     * colorspace (i.e. DeviceGray, DeviceRGB, DeviceCMYK).
     * \returns true if this is a simple colospace
     */
    bool IsSimpleColorSpace() const;

    /**
     * Convert this object into an EPdfColorSpace
     * enum.
     * \returns enum value if possible or ePdfColorSpace_Unknown
     */
    PoDoFo::EPdfColorSpace ConvertToPdfColorSpace() const;

    /**
     * Retrieve the internal name.
     * \return the internal name
     */
    inline const PoDoFo::PdfName & GetName() const;

private:
    PoDoFo::PdfName m_name;

};

const PoDoFo::PdfName & ColorSpace::GetName() const
{
    return m_name;
}

#endif // _COLORSPACE_H_
