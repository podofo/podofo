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
#include "PdfAction.h"
#include "PdfDestination.h"
#include "PdfDictionary.h"
#include "PdfObject.h"

namespace PoDoFo {

PdfOutlineItem::PdfOutlineItem( const PdfString & sTitle, const PdfDestination & rDest, 
                                PdfOutlineItem* pParentOutline, PdfVecObjects* pParent )
    : PdfElement( NULL, pParent ), 
      m_pParentOutline( pParentOutline ), m_pPrev( NULL ), m_pNext( NULL ), 
      m_pFirst( NULL ), m_pLast( NULL ), m_pDestination( NULL ), m_pAction( NULL )
{
    if( pParentOutline )
        m_pObject->GetDictionary().AddKey( "Parent", pParentOutline->GetObject()->Reference() );

    this->SetTitle( sTitle );
    this->SetDestination( rDest );
}

PdfOutlineItem::PdfOutlineItem( const PdfString & sTitle, const PdfAction & rAction, 
                                PdfOutlineItem* pParentOutline, PdfVecObjects* pParent )
    : PdfElement( NULL, pParent ), 
      m_pParentOutline( pParentOutline ), m_pPrev( NULL ), m_pNext( NULL ), 
      m_pFirst( NULL ), m_pLast( NULL ), m_pDestination( NULL ), m_pAction( NULL )
{
    if( pParentOutline )
        m_pObject->GetDictionary().AddKey( "Parent", pParentOutline->GetObject()->Reference() );

    this->SetTitle( sTitle );
    this->SetAction( rAction );
}

PdfOutlineItem::PdfOutlineItem( PdfObject* pObject, PdfOutlineItem* pParentOutline, PdfOutlineItem* pPrevious )
    : PdfElement( NULL, pObject ), m_pParentOutline( pParentOutline ), m_pPrev( pPrevious ), 
      m_pNext( NULL ), m_pFirst( NULL ), m_pLast( NULL ), m_pDestination( NULL ), m_pAction( NULL )
{
    PdfReference first, next;

    if( m_pObject->GetDictionary().HasKey( "First" ) )
    {
        first    = m_pObject->GetDictionary().GetKey("First")->GetReference();
        m_pFirst = new PdfOutlineItem( pObject->GetOwner()->GetObject( first ), this, NULL );
    }

    if( m_pObject->GetDictionary().HasKey( "Next" ) )
    {
        next     = m_pObject->GetDictionary().GetKey("Next")->GetReference();
        PdfObject* pObj = pObject->GetOwner()->GetObject( next );

        m_pNext  = new PdfOutlineItem( pObj, NULL, this );
    }
    else
    {
        // if there is no next key,
        // we have to set ourself as the last item of the parent
        if( m_pParentOutline )
            m_pParentOutline->SetLast( this );
    }
}

PdfOutlineItem::PdfOutlineItem( PdfVecObjects* pParent )
    : PdfElement( "Outlines", pParent ), m_pParentOutline( NULL ), m_pPrev( NULL ), 
      m_pNext( NULL ), m_pFirst( NULL ), m_pLast( NULL ), m_pDestination( NULL ), m_pAction( NULL )
{
}

PdfOutlineItem::~PdfOutlineItem()
{
    delete m_pNext;
    delete m_pFirst;
}

PdfOutlineItem* PdfOutlineItem::CreateChild( const PdfString & sTitle, const PdfDestination & rDest )
{
    PdfOutlineItem* pItem = new PdfOutlineItem( sTitle, rDest, this, m_pObject->GetOwner() );

    this->InsertChild( pItem );

    return pItem;
}

void PdfOutlineItem::InsertChild( PdfOutlineItem* pItem )
{
    if( m_pLast )
    {
        m_pLast->SetNext( pItem );
        pItem->SetPrevious( m_pLast );
    }

    m_pLast = pItem;

    if( !m_pFirst )
        m_pFirst = m_pLast;

    m_pObject->GetDictionary().AddKey( "First", m_pFirst->GetObject()->Reference() );
    m_pObject->GetDictionary().AddKey( "Last",  m_pLast->GetObject()->Reference() );
}

PdfOutlineItem* PdfOutlineItem::CreateNext ( const PdfString & sTitle, const PdfDestination & rDest )
{
    PdfOutlineItem* pItem = new PdfOutlineItem( sTitle, rDest, m_pParentOutline, m_pObject->GetOwner() );

    if( m_pNext ) 
    {
        m_pNext->SetPrevious( pItem );
        pItem->SetNext( m_pNext );
    }

    m_pNext = pItem;
    m_pNext->SetPrevious( this );

    m_pObject->GetDictionary().AddKey( "Next", m_pNext->GetObject()->Reference() );

    if( m_pParentOutline && !m_pNext->Next() ) 
        m_pParentOutline->SetLast( m_pNext );

    return m_pNext;
}

PdfOutlineItem* PdfOutlineItem::CreateNext ( const PdfString & sTitle, const PdfAction & rAction )
{
    PdfOutlineItem* pItem = new PdfOutlineItem( sTitle, rAction, m_pParentOutline, m_pObject->GetOwner() );

    if( m_pNext ) 
    {
        m_pNext->SetPrevious( pItem );
        pItem->SetNext( m_pNext );
    }

    m_pNext = pItem;
    m_pNext->SetPrevious( this );

    m_pObject->GetDictionary().AddKey( "Next", m_pNext->GetObject()->Reference() );

    if( m_pParentOutline && !m_pNext->Next() ) 
        m_pParentOutline->SetLast( m_pNext );

    return m_pNext;
}

void PdfOutlineItem::SetPrevious( PdfOutlineItem* pItem )
{
    m_pPrev = pItem;
    m_pObject->GetDictionary().AddKey( "Prev", m_pPrev->GetObject()->Reference() );
}

void PdfOutlineItem::SetNext( PdfOutlineItem* pItem )
{
    m_pNext = pItem;
    m_pObject->GetDictionary().AddKey( "Next", m_pNext->GetObject()->Reference() );
}

void PdfOutlineItem::SetLast( PdfOutlineItem* pItem )
{
    m_pLast = pItem;
    if( m_pLast )
        m_pObject->GetDictionary().AddKey( "Last",  m_pLast->GetObject()->Reference() );
    else 
        m_pObject->GetDictionary().RemoveKey( "Last" );
}

void PdfOutlineItem::SetFirst( PdfOutlineItem* pItem )
{
    m_pFirst = pItem;
    if( m_pFirst )
        m_pObject->GetDictionary().AddKey( "First",  m_pFirst->GetObject()->Reference() );
    else 
        m_pObject->GetDictionary().RemoveKey( "First" );
}

void PdfOutlineItem::Erase()
{
    while( m_pFirst )
    {
        // erase will set a new first
        // if it has a next item
        m_pFirst->Erase();
    }

    if( m_pPrev && m_pNext ) 
    {
        m_pPrev->SetNext    ( m_pNext );
        m_pNext->SetPrevious( m_pPrev );
    }

    if( !m_pPrev && m_pParentOutline )
        m_pParentOutline->SetFirst( m_pNext );

    if( !m_pNext && m_pParentOutline )
        m_pParentOutline->SetLast( m_pPrev );

    m_pNext = NULL;
    delete this;
}

void PdfOutlineItem::SetDestination( const PdfDestination & rDest )
{
    delete m_pDestination;
    m_pDestination = NULL;

    rDest.AddToDictionary( m_pObject->GetDictionary() );
}

PdfDestination* PdfOutlineItem::GetDestination( void )
{
    if( !m_pDestination )
    {
        PdfObject*	dObj = m_pObject->GetIndirectKey( "Dest" );
        if ( !dObj ) 
            return NULL;
    
        m_pDestination = new PdfDestination( dObj );
    }

    return m_pDestination;
}

void PdfOutlineItem::SetAction( const PdfAction & rAction )
{
    delete m_pAction;
    m_pAction = NULL;

    rAction.AddToDictionary( m_pObject->GetDictionary() );
}

PdfAction* PdfOutlineItem::GetAction( void )
{
    if( !m_pAction )
    {
        PdfObject*	dObj = m_pObject->GetIndirectKey( "A" );
        if ( !dObj ) 
            return NULL;
    
        m_pAction = new PdfAction( dObj );
    }

    return m_pAction;
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
    m_pObject->GetDictionary().AddKey( "F", static_cast<long long>(eFormat) );
}

EPdfOutlineFormat PdfOutlineItem::GetTextFormat() const
{
    if( m_pObject->GetDictionary().HasKey( "F" ) )
        return static_cast<EPdfOutlineFormat>(m_pObject->GetIndirectKey( "F" )->GetNumber());

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
// PdfOutlines
///////////////////////////////////////////////////////////////////////////////////

PdfOutlines::PdfOutlines( PdfVecObjects* pParent )
    : PdfOutlineItem( pParent )
{
}

PdfOutlines::PdfOutlines( PdfObject* pObject )
    : PdfOutlineItem( pObject, NULL, NULL )
{
}

PdfOutlineItem* PdfOutlines::CreateRoot( const PdfString & sTitle )
{
    return this->CreateChild( sTitle, PdfDestination( GetObject()->GetOwner() ) );
}

};
