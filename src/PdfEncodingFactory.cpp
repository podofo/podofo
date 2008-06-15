/***************************************************************************
 *   Copyright (C) 2008 by Dominik Seichter                                *
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

#include "PdfEncodingFactory.h"

#include "PdfEncoding.h"
#include "PdfFont.h"
#include "PdfName.h"
#include "PdfObject.h"

namespace PoDoFo {

const PdfEncoding* const PdfEncodingFactory::CreateEncoding( PdfObject* pObject )
{
    const PdfEncoding* pEncoding = NULL;

    if( pObject->IsReference() )
    {
        // resolve any references
        pObject = pObject->GetOwner()->GetObject( pObject->GetReference() );
    }

    if( pObject->IsName() )
    {
        const PdfName & rName = pObject->GetName();
        if( rName == PdfName("WinAnsiEncoding") )
            return &PdfFont::WinAnsiEncoding;
        else if( rName == PdfName("MacRomanEncoding") )
            return &PdfFont::MacRomanEncoding;
        // TODO:
        //else if( rName == PdfName("MacExpertEncoding") )
        //    return &PdfFont::MacExpertEncoding;
    }
    else if( pObject->IsDictionary() )
    {

    }

    return pEncoding;
}


};
