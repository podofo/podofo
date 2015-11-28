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

#include "PdfFontTrueType.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfName.h"
#include "base/PdfStream.h"

namespace PoDoFo {

PdfFontTrueType::PdfFontTrueType( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                                  PdfVecObjects* pParent, bool bEmbed )
    : PdfFontSimple( pMetrics, pEncoding, pParent )
{
    this->Init( bEmbed, PdfName("TrueType") );
}

PdfFontTrueType::PdfFontTrueType( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                                  PdfObject* pObject )
    : PdfFontSimple( pMetrics, pEncoding, pObject )
{

}

void PdfFontTrueType::EmbedFontFile( PdfObject* pDescriptor )
{
    PdfObject* pContents;
    pdf_long   lSize = 0;
    
    m_bWasEmbedded = true;    
        
    pContents = this->GetObject()->GetOwner()->CreateObject();
    pDescriptor->GetDictionary().AddKey( "FontFile2", pContents->Reference() );

    // if the data was loaded from memory - use it from there
    // otherwise, load from disk
    if ( m_pMetrics->GetFontDataLen() && m_pMetrics->GetFontData() ) 
    {
        // FIXME const_cast<char*> is dangerous if string literals may ever be passed
        char* pBuffer = const_cast<char*>( m_pMetrics->GetFontData() );
        lSize = m_pMetrics->GetFontDataLen();
        
        // Set Length1 before creating the stream
        // as PdfStreamedDocument does not allow 
        // adding keys to an object after a stream was written
        pContents->GetDictionary().AddKey( "Length1", PdfVariant( static_cast<pdf_int64>(lSize) ) );
        pContents->GetStream()->Set( pBuffer, lSize );
    } 
    else 
    {
        PdfFileInputStream stream( m_pMetrics->GetFilename() );
        lSize = stream.GetFileLength();

        // Set Length1 before creating the stream
        // as PdfStreamedDocument does not allow 
        // adding keys to an object after a stream was written
        pContents->GetDictionary().AddKey( "Length1", PdfVariant( static_cast<pdf_int64>(lSize) ) );
        pContents->GetStream()->Set( &stream );
            
    }
}



};

