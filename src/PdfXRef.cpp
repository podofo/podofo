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

#include "PdfXRef.h"

#include "PdfOutputDevice.h"

#include <algorithm>

namespace PoDoFo {

PdfXRef::PdfXRef() 
{

}

PdfXRef::~PdfXRef() 
{

}

void PdfXRef::AddObject( const PdfReference & rRef, long lOffset, bool bUsed )
{
    bool bSort; 

    if( bUsed )
    {
        PdfXRef::TXRefItem item;
    
        item.reference = rRef;
        item.lOffset   = lOffset;

        bSort = m_vecXRef.size() ? item.reference < m_vecXRef.back().reference : false;
        m_vecXRef.push_back( item );

        if( bSort )
        {
            // TODO: Maybe it is faster to sort, after all items have been added
            std::sort( m_vecXRef.begin(), m_vecXRef.end() );
        }
    }
    else
    {
        bSort = m_vecFreeObjects.size() ? rRef < m_vecFreeObjects.back() : false;
        m_vecFreeObjects.push_back( rRef );

        if( bSort )
        {
            // TODO: Maybe it is faster to sort, after all items have been added
            std::sort( m_vecFreeObjects.begin(), m_vecFreeObjects.end() );
        }
    }
}

void PdfXRef::Write( PdfOutputDevice* pDevice )
{
    PdfXRef::TCIVecXRefItems  it     = m_vecXRef.begin();
    PdfXRef::TCIVecReferences itFree = m_vecFreeObjects.begin();

    int nFirst       = 0;
    int nCount       = 0;

    pDevice->Print( "xref\n" );
    while( it != m_vecXRef.end() ) 
    {
        nCount       = GetItemCount( it, itFree );
        nFirst       = (*it).reference.ObjectNumber();

        if( itFree != m_vecFreeObjects.end() )
            nFirst = PDF_MIN( nFirst, (*itFree).ObjectNumber() );

        if( nFirst == 1 )
            --nFirst;

        // when there is only one, then we need to start with 0 and the bogus object...
        //printf("XRef section: %u %u\n", nFirst, nCount );
        pDevice->Print( "%u %u\n", nFirst, nCount );

        if( !nFirst ) 
        {
            pDevice->Print( "%0.10i %0.5i %c \n",
                            m_vecFreeObjects.size() ? m_vecFreeObjects.front().ObjectNumber() : 0,
                            EMPTY_OBJECT_OFFSET, 'f' );
        }

        while( --nCount > 0 && it != m_vecXRef.end() ) 
        {
            while( itFree != m_vecFreeObjects.end() &&
                   *itFree < (*it).reference && nCount )
            {
                int nGen = (*itFree).GenerationNumber();
                ++itFree;
                
                // write free object
                pDevice->Print( "%0.10i %0.5i f \n", 
                                itFree != m_vecFreeObjects.end() ? (*itFree).ObjectNumber() : 0,
                                nGen );
                --nCount;
            }

            //printf("Writing Object %i\n", (*it).reference.ObjectNumber() );
            pDevice->Print( "%0.10i %0.5i n \n", (*it).lOffset, (*it).reference.GenerationNumber() );
            ++it;
        }
    }    

    PODOFO_RAISE_LOGIC_IF( nCount != 0, "PdfXRef::Write() nCount != 0" );
}

int PdfXRef::GetItemCount( PdfXRef::TCIVecXRefItems it, PdfXRef::TCIVecReferences itFree ) const
{
    unsigned int nCur = (*it).reference.ObjectNumber();
    unsigned int nCnt = 1;

    if( itFree != m_vecFreeObjects.end() && (*itFree).ObjectNumber() < (*it).reference.ObjectNumber())
    {
        nCur = (*itFree).ObjectNumber();
        ++itFree;
    }
    else
        ++it;

    while( it != m_vecXRef.end() )
    {
        while( itFree != m_vecFreeObjects.end() &&
               *itFree < (*it).reference && 
               !((*it).reference.ObjectNumber() >= nCur))
        {
            ++itFree;
            ++nCnt;
            ++nCur;
        }

        if( (*it).reference.ObjectNumber() != ++nCur )
            break;

        ++nCnt;
        ++it;
    }

    return ++nCnt;
}

unsigned int PdfXRef::GetSize() const
{
    // the list is assumed to be sorted

    if( !m_vecXRef.size() )
        return 0;
    else
        return m_vecXRef.back().reference.ObjectNumber() + 1;
}

};
