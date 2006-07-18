/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#include "PdfPage.h" 
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfRect.h"
#include "PdfVariant.h"
#include "PdfWriter.h"


namespace PoDoFo {

PdfPage::PdfPage( PdfDocument* inOwningDoc, unsigned int nObjectNo, unsigned int nGenerationNo )
    : PdfCanvas(), m_pDocument( inOwningDoc ), m_pObject( new PdfObject(nObjectNo, nGenerationNo, "Page") )
{
    // The PDF specification suggests that we send all available PDF Procedure sets
    m_pObject->GetDictionary().AddKey( "Resources", PdfObject( PdfDictionary() ) );

    m_pResources = m_pObject->GetDictionary().GetKey( "Resources" );
    m_pResources->GetDictionary().AddKey( "ProcSet", PdfCanvas::ProcSet() );
}

PdfPage::PdfPage( PdfDocument* inOwningDoc, PdfObject* inObject )
    : PdfCanvas(), m_pDocument( inOwningDoc ), m_pObject( inObject )
{
    m_pResources = m_pObject->GetDictionary().GetKey( "Resources" );
    m_pContents = m_pObject->GetIndirectKey( "Contents" );
}

PdfPage::~PdfPage()
{

}

void PdfPage::Init( const TSize & tSize, PdfVecObjects* pParent )
{
    PdfVariant rect;

    m_pContents = pParent->CreateObject();

    m_tPageSize = tSize;

    PdfRect( 0, 0, tSize.lWidth, tSize.lHeight ).ToVariant( rect );

    m_pObject->GetDictionary().AddKey( "MediaBox", rect );
    m_pObject->GetDictionary().AddKey( PdfName::KeyContents, m_pContents->Reference() );
}

TSize PdfPage::CreateStandardPageSize( const EPdfPageSize ePageSize )
{
    TSize tSize;

    switch( ePageSize ) 
    {
        case ePdfPageSize_A4:
            tSize.lWidth  = 595;
            tSize.lHeight = 842;
            break;

        case ePdfPageSize_Letter:
            tSize.lWidth  = 612;
            tSize.lHeight = 792;
            break;
            
        case ePdfPageSize_Legal:
            tSize.lWidth  = 612;
            tSize.lHeight = 1008;
            break;
            
        case ePdfPageSize_A3:
            tSize.lWidth  = 842;
            tSize.lHeight = 1190;
            break;
            
        default:
            tSize.lWidth = tSize.lHeight = 0;
            break;
    }

    return tSize;
}

PdfObject* PdfPage::GetInheritedKeyFromObject( const char* inKey, PdfObject* inObject ) const
{
    PdfObject* pObj = NULL;

    // check for it in the object itself
    if ( inObject->GetDictionary().HasKey( inKey ) ) 
    {
        pObj = inObject->GetDictionary().GetKey( inKey );
        if ( !pObj->IsNull() ) 
            return pObj;
    }
    
    // if we get here, we need to go check the parent - if there is one!
    if( inObject->GetDictionary().HasKey( "Parent" ) ) 
    {
        pObj = inObject->GetIndirectKey( "Parent" );
        if( pObj )
            pObj = GetInheritedKeyFromObject( inKey, pObj );
    }

    return pObj;
}

const PdfRect PdfPage::GetPageBox( const char* inBox ) const
{
    PdfRect	 pageBox;
    PdfObject*   pObj;
        
    // Take advantage of inherited values - walking up the tree if necessary
    pObj = GetInheritedKeyFromObject( inBox, m_pObject );
    
    // assign the value of the box from the array
    if ( pObj && pObj->IsArray() )
        pageBox.FromArray( pObj->GetArray() );
    
    return pageBox;
}

const int PdfPage::GetRotation() const 
{ 
    int rot = 0;
    
    PdfObject* pObj = GetInheritedKeyFromObject( "Rotate", m_pObject ); 
    if ( pObj && pObj->IsNumber() )
        rot = pObj->GetNumber();
    
    return rot;
}

const int PdfPage::GetNumAnnots() const
{
    int	       numAnnots = 0;
    PdfObject* pObj;
    // check for it in the object itself
    if ( m_pObject->GetDictionary().HasKey( "Annots" ) ) 
    {
        pObj = m_pObject->GetIndirectKey( "Annots" );
        if( pObj && pObj->IsArray() )
            return pObj->GetArray().size();
    }

    return numAnnots;
}

};
