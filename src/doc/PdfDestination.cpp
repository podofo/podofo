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

#include "PdfDestination.h"

#include "base/PdfDictionary.h"
#include "base/PdfDefinesPrivate.h"

#include "PdfAction.h"
#include "PdfDocument.h"
#include "PdfNamesTree.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"

namespace PoDoFo {

const long  PdfDestination::s_lNumDestinations = 19;
const char* PdfDestination::s_names[] = {
    "Fit",
    "FitH",
    "FitV",
    "FitB",
    "FitBH",
    "FitBV",
    NULL
};

PdfDestination::PdfDestination( PdfVecObjects* pParent )
{
    m_pObject = pParent->CreateObject( m_array );
}

PdfDestination::PdfDestination( PdfObject* pObject, PdfDocument* pDocument )
{
    Init( pObject, pDocument );
}

PdfDestination::PdfDestination( PdfObject* pObject, PdfVecObjects* pVecObjects )
{
    PdfDocument* pDocument = pVecObjects->GetParentDocument();
    if( !pDocument ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    Init( pObject, pDocument );
}

PdfDestination::PdfDestination( const PdfPage* pPage, EPdfDestinationFit eFit )
{
    PdfName type = PdfName("Fit");

    if( eFit == ePdfDestinationFit_Fit )
        type = PdfName("Fit");
    else if( eFit == ePdfDestinationFit_FitB )
        type = PdfName("FitB");
    else
    {
        // Peter Petrov 6 June 2008
        // silent mode
        //PODOFO_RAISE_ERROR( ePdfError_InvalidKey );
    }

    m_array.push_back( pPage->GetObject()->Reference() );
    m_array.push_back( type );
    m_pObject = pPage->GetObject()->GetOwner()->CreateObject( m_array );
}

PdfDestination::PdfDestination( const PdfPage* pPage, const PdfRect & rRect )
{
    PdfVariant var;

    rRect.ToVariant( var );

    m_array.push_back( pPage->GetObject()->Reference() );
    m_array.push_back( PdfName("FitR") );
    m_array.insert( m_array.end(), var.GetArray().begin(), var.GetArray().end() );
    m_pObject = pPage->GetObject()->GetOwner()->CreateObject( m_array );
}

PdfDestination::PdfDestination( const PdfPage* pPage, double dLeft, double dTop, double dZoom )
{
    m_array.push_back( pPage->GetObject()->Reference() );
    m_array.push_back( PdfName("XYZ") );
    m_array.push_back( dLeft );
    m_array.push_back( dTop );
    m_array.push_back( dZoom );
    m_pObject = pPage->GetObject()->GetOwner()->CreateObject( m_array );
}

PdfDestination::PdfDestination( const PdfPage* pPage, EPdfDestinationFit eFit, double dValue )
{
    PdfName type;

    if( eFit == ePdfDestinationFit_FitH )
        type = PdfName("FitH");
    else if( eFit == ePdfDestinationFit_FitV )
        type = PdfName("FitV");
    else if( eFit == ePdfDestinationFit_FitBH )
        type = PdfName("FitBH");
    else if( eFit == ePdfDestinationFit_FitBV )
        type = PdfName("FitBV");
    else
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidKey );
    }

    m_array.push_back( pPage->GetObject()->Reference() );
    m_array.push_back( type );
    m_array.push_back( dValue );
    m_pObject = pPage->GetObject()->GetOwner()->CreateObject( m_array );
}

PdfDestination::PdfDestination( const PdfDestination & rhs )
{
    this->operator=( rhs );
}

const PdfDestination & PdfDestination::operator=( const PdfDestination & rhs )
{
    m_array     = rhs.m_array;
    m_pObject	= rhs.m_pObject;

    return *this;
}

void PdfDestination::Init( PdfObject* pObject, PdfDocument* pDocument )
{
    if ( pObject->GetDataType() == ePdfDataType_Array ) 
    {
        m_array = pObject->GetArray();
    }
    else if( pObject->GetDataType() == ePdfDataType_String ) 
    {
        PdfNamesTree* pNames = pDocument->GetNamesTree( ePdfDontCreateObject );
        if( !pNames ) 
        {
            PODOFO_RAISE_ERROR( ePdfError_NoObject );
        }
            
        PdfObject* pValue = pNames->GetValue( "Dests", pObject->GetString() );
        if( !pValue ) 
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidName );
        }

        if( pValue->IsArray() ) 
            m_array = pValue->GetArray();
        else if( pValue->IsDictionary() )
            m_array = pValue->GetDictionary().GetKey( "D" )->GetArray();
    }

    m_pObject = pObject;
}

void PdfDestination::AddToDictionary( PdfDictionary & dictionary ) const
{
    // Do not add empty destinations
    if( !m_array.size() )
        return;

    // since we can only have EITHER a Dest OR an Action
    // we check for an Action, and if already present, we throw
    if ( dictionary.HasKey( PdfName( "A" ) ) )
        PODOFO_RAISE_ERROR( ePdfError_ActionAlreadyPresent );

    dictionary.RemoveKey( "Dest" );
    dictionary.AddKey( "Dest", m_pObject );
}

PdfPage* PdfDestination::GetPage( PdfDocument* pDoc ) 
{
    if( !m_array.size() )
        return NULL;

    // first entry in the array is the page - so just make a new page from it!
    return pDoc->GetPagesTree()->GetPage( m_array[0].GetReference() );
}

PdfPage* PdfDestination::GetPage( PdfVecObjects* pVecObjects )
{
    PdfDocument* pDoc = pVecObjects->GetParentDocument();
    if( !pDoc ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle,
                                 "PdfVecObjects needs a parent PdfDocument to resolve pages." );
    }
     
    return this->GetPage( pDoc );
}

};
