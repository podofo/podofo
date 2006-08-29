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

#include "PdfOutlines.h"

#include "PdfArray.h"
#include "PdfDestination.h"
#include "PdfDictionary.h"
#include "PdfObject.h"

namespace PoDoFo {

PdfOutlineItem::PdfOutlineItem( const PdfString & sTitle, const PdfDestination & rDest, PdfOutlineItem* pParentOutline, PdfVecObjects* pParent )
    : PdfElement( NULL, pParent ), m_pParentOutline( pParentOutline ), m_pPrev( NULL ), m_pNext( NULL )
{
    m_pObject->GetDictionary().AddKey( "Parent", pParentOutline->Object()->Reference() );

    this->SetTitle( sTitle );
    this->SetDestination( rDest );
}

PdfOutlineItem::PdfOutlineItem( PdfObject* pObject )
    : PdfElement( NULL,pObject )
{
    m_pParentOutline = NULL;
}

PdfOutlineItem::PdfOutlineItem( PdfVecObjects* pParent )
    : PdfElement( "Outlines", pParent ), m_pParentOutline( NULL ), m_pPrev( NULL ), m_pNext( NULL )
{

}

PdfOutlineItem::~PdfOutlineItem()
{
    TIOutlineItemList it = m_lstItems.begin();

    while( it != m_lstItems.end() )
    {
        delete (*it);
        ++it;
    }
}

PdfOutlineItem* PdfOutlineItem::CreateChild( const PdfString & sTitle, const PdfDestination & rDest )
{
    PdfOutlineItem* pItem = new PdfOutlineItem( sTitle, rDest, this, m_pObject->GetParent() );
    PdfOutlineItem* pLast = this->Last();

    if( pLast )
    {
        pLast->SetNext( pItem );
        pItem->SetPrevious( pLast );
    }

    m_lstItems.push_back( pItem );

    m_pObject->GetDictionary().AddKey( "First", this->First()->Object()->Reference() );
    m_pObject->GetDictionary().AddKey( "Last", this->Last()->Object()->Reference() );

    return pItem;
}

PdfOutlineItem* PdfOutlineItem::CreateNext ( const PdfString & sTitle, const PdfDestination & rDest )
{
    PdfOutlineItem* pItem = new PdfOutlineItem( sTitle, rDest, m_pParentOutline, m_pObject->GetParent() );

    if( m_pNext ) 
    {
        m_pNext->SetPrevious( pItem );
        pItem->SetNext( m_pNext );
    }

    m_pNext = pItem;
    m_pNext->SetPrevious( this );

    m_pObject->GetDictionary().AddKey( "Next", m_pNext->Object()->Reference() );

    if( m_pParentOutline ) 
        m_pParentOutline->AppendChild( this, m_pNext );

    return m_pNext;
}

void PdfOutlineItem::SetPrevious( PdfOutlineItem* pItem )
{
    m_pPrev = pItem;
    m_pObject->GetDictionary().AddKey( "Prev", m_pPrev->Object()->Reference() );
}

void PdfOutlineItem::SetNext( PdfOutlineItem* pItem )
{
    m_pNext = pItem;
    m_pObject->GetDictionary().AddKey( "Next", m_pPrev->Object()->Reference() );
}

void PdfOutlineItem::AppendChild( PdfOutlineItem* pItem, PdfOutlineItem* pChild )
{
    TIOutlineItemList it = std::find( m_lstItems.begin(), m_lstItems.end(), pItem );
        
    if( it == m_lstItems.end() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    ++it;
    if( it != m_lstItems.end() )
        m_lstItems.insert( ++it, pChild );
    else
        m_lstItems.push_back( pChild );

    m_pObject->GetDictionary().AddKey( "Last", this->Last()->Object()->Reference() );
}

void PdfOutlineItem::SetDestination( const PdfDestination & rDest )
{
    rDest.AddToDictionary( m_pObject->GetDictionary() );
}

void PdfOutlineItem::SetTitle( const PdfString & sTitle )
{
    m_pObject->GetDictionary().AddKey( "Title", sTitle );
}

const PdfString & PdfOutlineItem::GetTitle() const
{
    return m_pObject->GetIndirectKey( "Title" )->GetString();
}

void PdfOutlineItem::SetTextFormat( EPdfOutlineFormat eFormat )
{
    m_pObject->GetDictionary().AddKey( "F", (long)eFormat );
}

EPdfOutlineFormat PdfOutlineItem::GetTextFormat() const
{
    if( m_pObject->GetDictionary().HasKey( "F" ) )
        return (EPdfOutlineFormat)m_pObject->GetIndirectKey( "F" )->GetNumber();

    return ePdfOutlineFormat_Default;
}

void PdfOutlineItem::SetTextColor( double r, double g, double b )
{
    PdfArray color;
    color.push_back( r );
    color.push_back( g );
    color.push_back( b );

    m_pObject->GetDictionary().AddKey( "C", color );
}


double PdfOutlineItem::GetTextColorRed() const
{
    if( m_pObject->GetDictionary().HasKey( "C" ) )
        return m_pObject->GetIndirectKey( "C" )->GetArray()[0].GetReal();

    return 0.0;
}

double PdfOutlineItem::GetTextColorGreen() const
{
    if( m_pObject->GetDictionary().HasKey( "C" ) )
        return m_pObject->GetIndirectKey( "C" )->GetArray()[1].GetReal();

    return 0.0;
}

double PdfOutlineItem::GetTextColorBlue() const
{
    if( m_pObject->GetDictionary().HasKey( "C" ) )
        return m_pObject->GetIndirectKey( "C" )->GetArray()[2].GetReal();

    return 0.0;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

PdfOutlines::PdfOutlines( PdfVecObjects* pParent )
    : PdfOutlineItem( pParent )
{
}

PdfOutlines::PdfOutlines( PdfObject* pObject )
    : PdfOutlineItem( (PdfVecObjects*)NULL )
{
}

PdfOutlineItem* PdfOutlines::CreateRoot( const PdfString & sTitle )
{
    return this->CreateChild( sTitle, PdfDestination() );
}

};
