/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "PdfAcroForm.h"

#include "PdfArray.h" 
#include "PdfDictionary.h"

namespace PoDoFo {

/*
  We use NULL for the PdfElement name, since the AcroForm dict
  does NOT have a /Type key!
*/
PdfAcroForm::PdfAcroForm( PdfVecObjects* pParent )
    : PdfElement( NULL, pParent ), m_pCatalog( NULL )
{
    // Initialize with an empty fields array
    m_pObject->GetDictionary().AddKey( PdfName("Fields"), PdfArray() );
}

PdfAcroForm::PdfAcroForm( PdfObject* pObject, PdfObject* pCatalog )
    : PdfElement( NULL, pObject ), m_pCatalog( pCatalog )
{

}

int PdfAcroForm::GetCount()
{
    PdfObject* pFields = m_pObject->GetDictionary().GetKey( PdfName("Fields") );
    if( pFields ) 
    {
        return pFields->GetArray().size();
    }
    else
    {
        PODOFO_RAISE_ERROR( ePdfError_NoObject );
    }
}

PdfObject* PdfAcroForm::GetField( int nIndex )
{

}

};
