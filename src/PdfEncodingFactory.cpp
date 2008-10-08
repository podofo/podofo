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
#include "PdfDifferenceEncoding.h"
#include "PdfFont.h"
#include "util/PdfMutexWrapper.h"
#include "PdfName.h"
#include "PdfObject.h"

namespace PoDoFo {

const PdfDocEncoding*      PdfEncodingFactory::s_pDocEncoding      = NULL;
const PdfWinAnsiEncoding*  PdfEncodingFactory::s_pWinAnsiEncoding  = NULL;
const PdfMacRomanEncoding* PdfEncodingFactory::s_pMacRomanEncoding = NULL;

Util::PdfMutex PdfEncodingFactory::s_mutex;

const PdfEncoding* const PdfEncodingFactory::CreateEncoding( PdfObject* pObject )
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
        // TODO:
        //else if( rName == PdfName("MacExpertEncoding") )
        //    return &PdfFont::MacExpertEncoding;
    }
    else if( pObject->IsDictionary() )
    {
        return new PdfDifferenceEncoding( pObject );
    }


    PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Unsupported encoding detected!" );

    return NULL;
}

const PdfEncoding* PdfEncodingFactory::GlobalPdfDocEncodingInstance()
{
    if(!s_pDocEncoding) // First check
    {
	Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 

	if(!s_pDocEncoding) // Double check
	    s_pDocEncoding = new PdfDocEncoding();
    }

    return s_pDocEncoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalWinAnsiEncodingInstance()
{
    if(!s_pWinAnsiEncoding) // First check
    {
	Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 

	if(!s_pWinAnsiEncoding) // Double check
	    s_pWinAnsiEncoding = new PdfWinAnsiEncoding();
    }

    return s_pWinAnsiEncoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalMacRomanEncodingInstance()
{
    if(!s_pMacRomanEncoding) // First check
    {
	Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 

	if(!s_pMacRomanEncoding) // Double check
	    s_pMacRomanEncoding = new PdfMacRomanEncoding();
    }

    return s_pMacRomanEncoding;
}

};
