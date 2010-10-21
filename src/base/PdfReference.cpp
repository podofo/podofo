/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#include "PdfReference.h"

#include "PdfOutputDevice.h"
#include "PdfDefinesPrivate.h"

#include <sstream>

namespace PoDoFo {

void PdfReference::Write( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* ) const
{
    if( (eWriteMode & ePdfWriteMode_Compact) == ePdfWriteMode_Compact ) 
    {
        // Write space before the reference
        pDevice->Print( " %i %hi R", m_nObjectNo, m_nGenerationNo );
    }
    else
    {
        pDevice->Print( "%i %hi R", m_nObjectNo, m_nGenerationNo );
    }
}

const std::string PdfReference::ToString() const
{
    std::ostringstream out;
    out << m_nObjectNo << " " << m_nGenerationNo << " R";
    return out.str();
}

};
