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

#include "PdfContents.h"

#include "PdfArray.h"
#include "PdfDocument.h"
#include "PdfName.h"
#include "PdfOutputDevice.h"

#include <iostream>

namespace PoDoFo {

PdfContents::PdfContents( PdfDocument* pParent )
    : PdfElement( NULL, pParent )
{
    mContObj = m_pObject;
}

PdfContents::PdfContents( PdfVecObjects* pParent )
    : PdfElement( NULL, pParent )
{
    mContObj = m_pObject;
}

PdfContents::PdfContents( PdfObject* inObj )
    : PdfElement( NULL, inObj )
{
    if ( m_pObject->GetDataType() == ePdfDataType_Reference )
        mContObj = inObj->GetOwner()->GetObject( m_pObject->GetReference() );
    else
        mContObj = m_pObject;
}

PdfObject* PdfContents::GetContentsForAppending() const
{
//    if ( mContObj->GetDataType() == ePdfDataType_Stream || 
//         mContObj->GetDataType() == ePdfDataType_Dictionary ) {

    // Use PdfObject::HasStream() instead of the datatype ePdfDataType_Stream
    // as large parts of the code rely on all PdfObjects having the datatype
    // ePdfDataType_Dictionary wether they have a stream or not
    if( mContObj->GetDataType() == ePdfDataType_Dictionary ) {
        return mContObj;	// just return the stream itself
    } else if ( mContObj->GetDataType() == ePdfDataType_Array ) {
        /*
          Create a new stream, add it to the array, return it
        */
        PdfObject*	newStm = mContObj->GetOwner()->CreateObject();
        newStm->GetStream();
        PdfReference	pdfr( newStm->Reference().ObjectNumber(), newStm->Reference().GenerationNumber() );
        
        PdfArray&	cArr = mContObj->GetArray();
        cArr.push_back( pdfr );
        return newStm;
    } else {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }
}

};

