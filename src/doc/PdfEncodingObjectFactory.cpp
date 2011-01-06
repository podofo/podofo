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

#include "PdfEncodingObjectFactory.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfEncodingFactory.h"
#include "base/PdfObject.h"
#include "base/PdfVecObjects.h"

#include "PdfDifferenceEncoding.h"
#include "PdfIdentityEncoding.h"

namespace PoDoFo {

const PdfEncoding* PdfEncodingObjectFactory::CreateEncoding( PdfObject* pObject )
{
    if( pObject->IsReference() )
    {
        // resolve any references
        pObject = pObject->GetOwner()->GetObject( pObject->GetReference() );
    }

    if( pObject->IsName() )
    {
        const PdfName & rName = pObject->GetName();
        if( rName == PdfName("WinAnsiEncoding") )
            return PdfEncodingFactory::GlobalWinAnsiEncodingInstance();
        else if( rName == PdfName("MacRomanEncoding") )
            return PdfEncodingFactory::GlobalMacRomanEncodingInstance();
        else if( rName == PdfName("StandardEncoding") ) // OC 13.08.2010
            return PdfEncodingFactory::GlobalStandardEncodingInstance();
        else if( rName == PdfName("MacExpertEncoding") ) // OC 13.08.2010 TODO solved
            return PdfEncodingFactory::GlobalMacExpertEncodingInstance();
        else if( rName == PdfName("SymbolEncoding") ) // OC 13.08.2010
            return PdfEncodingFactory::GlobalSymbolEncodingInstance();
        else if( rName == PdfName("ZapfDingbatsEncoding") ) // OC 13.08.2010
            return PdfEncodingFactory::GlobalZapfDingbatsEncodingInstance();
        else if( rName == PdfName("Identity-H") ) 
            return new PdfIdentityEncoding();
    }
    else if( pObject->IsDictionary() )
    {
        return new PdfDifferenceEncoding( pObject );
    }


    PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Unsupported encoding detected!" );

    //return NULL; Unreachable code
}

}; /* namespace PoDoFo */
