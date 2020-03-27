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

#include "PdfOwnedDataType.h"
#include "PdfObject.h"
#include "PdfVecObjects.h"

namespace PoDoFo {

PdfOwnedDataType::PdfOwnedDataType()
    : m_pOwner( NULL )
{
}

// NOTE: Don't copy owner. Copied objects must be always detached.
// Ownership will be set automatically elsewhere
PdfOwnedDataType::PdfOwnedDataType( const PdfOwnedDataType &rhs )
    : PdfDataType( rhs ), m_pOwner( NULL )
{
}

PdfObject * PdfOwnedDataType::GetIndirectObject( const PdfReference &rReference ) const
{
    if ( m_pOwner == NULL )
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "Object is a reference but does not have an owner!" );

    return m_pOwner->GetOwner()->GetObject( rReference );
}

void PdfOwnedDataType::SetOwner( PdfObject* pOwner )
{
    PODOFO_ASSERT( pOwner != NULL );
    m_pOwner = pOwner;
}

PdfOwnedDataType & PdfOwnedDataType::operator=( const PdfOwnedDataType & rhs )
{
    // NOTE: Don't copy owner. Objects being assigned will keep current ownership
    PdfDataType::operator=( rhs );
    return *this;
}

PdfVecObjects * PdfOwnedDataType::GetObjectOwner()
{
    return m_pOwner == NULL ? NULL : m_pOwner->GetOwner();
}

};
