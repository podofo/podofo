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

#include "PdfElement.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfObject.h"
#include "base/PdfVecObjects.h"

#include "PdfStreamedDocument.h"

#include <string.h>

namespace PoDoFo {

PdfElement::PdfElement( const char* pszType, PdfVecObjects* pParent )
{
    m_pObject = pParent->CreateObject( pszType );
}

PdfElement::PdfElement( const char* pszType, PdfDocument* pParent )
{
    m_pObject = pParent->m_vecObjects.CreateObject( pszType );
}

PdfElement::PdfElement( const char* pszType, PdfObject* pObject )
{
    if( !pObject )         
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pObject = pObject;

    if( !m_pObject->IsDictionary() ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    if( pszType
        && m_pObject->GetDictionary().HasKey( PdfName::KeyType )
        && m_pObject->GetIndirectKeyAsName( PdfName::KeyType ) != pszType ) 
    {
        PdfError::LogMessage( eLogSeverity_Debug, "Expected key %s but got key %s.", 
                              pszType, m_pObject->GetIndirectKeyAsName( PdfName::KeyType ).GetName().c_str() );

        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }
}

PdfElement::PdfElement( EPdfDataType eExpectedDataType, PdfObject* pObject ) 
{
    if( !pObject )         
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pObject = pObject;

    if( m_pObject->GetDataType() != eExpectedDataType ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }
}

PdfElement::~PdfElement()
{
}

const char* PdfElement::TypeNameForIndex( int i, const char** ppTypes, long lLen ) const
{
    return ( i >= lLen ? NULL : ppTypes[i] );
}

int PdfElement::TypeNameToIndex( const char* pszType, const char** ppTypes, long lLen, int nUnknownValue ) const
{
    int i;

    if( !pszType )
        return nUnknownValue;

    for( i=0; i<lLen; i++ )
    {
        if( ppTypes[i] && strcmp( pszType, ppTypes[i] ) == 0 ) 
        {
            return i;
        }
    }
    
    return nUnknownValue;
}
PdfObject* PdfElement::CreateObject( const char* pszType )
{
    return m_pObject->GetOwner()->CreateObject( pszType );
}


};
