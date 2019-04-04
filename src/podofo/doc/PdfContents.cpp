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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfContents.h"

#include "base/PdfDefinesPrivate.h"
#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfName.h"
#include "base/PdfOutputDevice.h"

#include "PdfDocument.h"
#include "PdfPage.h"

#include <iostream>

namespace PoDoFo {

PdfContents::PdfContents( PdfDocument* pParent )
    : PdfElement( NULL, pParent )
{
    mContObj = this->GetObject();
}

PdfContents::PdfContents( PdfVecObjects* pParent )
    : PdfElement( NULL, pParent )
{
    mContObj = this->GetObject();
}

PdfContents::PdfContents( PdfObject* inObj )
    // A PdfElement expects normally a dictionary
    // But we may get here, a reference, a dictionary
    // or an array. Therefore, tell PdfElement
    // that we also want to accept the datatype of 
    // the object we send in.
    : PdfElement( inObj->GetDataType(), inObj )
{
    if ( this->GetObject()->GetDataType() == ePdfDataType_Reference )
        mContObj = inObj->GetOwner()->GetObject( this->GetObject()->GetReference() );
    else
        mContObj = this->GetObject();
}

PdfContents::PdfContents( PdfPage* pParent ) 
    : PdfElement( NULL, pParent->GetObject()->GetOwner() )
{
    // TODO: Maybe create this only on demand
    pParent->GetObject()->GetDictionary().AddKey( "Contents", this->GetObject()->Reference() );
    mContObj = this->GetObject();
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

