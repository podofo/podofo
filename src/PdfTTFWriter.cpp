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

#include "PdfTTFWriter.h"

#include "PdfInputDevice.h"
#include "PdfOutputDevice.h"

namespace PoDoFo {

extern bool podofo_is_little_endian();


namespace NonPublic {

/**
 * The TTF format.
 *
 * - Big endian
 *
 * - Required tables:
 *   cmap character to glyph mapping
 *   glyf glyph data
 *   head font header
 *   hhea horizontal header
 *   hmtx horizontal metrics
 *   loca index to location
 *   maxp maximum profile
 *   name naming table
 *   post PostScript information
 *   OS/2 OS/2 and Windows specific metrics
 *
 */

PdfTTFWriter::PdfTTFWriter()
{

}

PdfTTFWriter::~PdfTTFWriter()
{

}

void PdfTTFWriter::Read( PdfInputDevice* pDevice )
{
    long lGlyf = -1;
    long lHead = -1;
    long lLoca = -1;
    long lMaxp = -1;

    // Read the table directory
    this->ReadTableDirectory( pDevice );

    // read the table of contents
    TVecTableDirectoryEntries vecTables;
    TTableDirectoryEntry      entry;

    vecTables.reserve( m_tTableDirectory.numTables );
    for( int i=0;i<m_tTableDirectory.numTables;i++ ) 
    {
        this->ReadTableDirectoryEntry( pDevice, &entry );

        if( entry.tag == this->CreateTag( 'l', 'o', 'c', 'a' ) ) 
            lLoca = entry.offset;
        else if( entry.tag == this->CreateTag( 'g', 'l', 'y', 'f' ) ) 
            lGlyf = entry.offset;
        else if( entry.tag == this->CreateTag( 'm', 'a', 'x', 'p' ) ) 
            lMaxp = entry.offset;
        else if( entry.tag == this->CreateTag( 'h', 'e', 'a', 'd' ) ) 
            lHead = entry.offset;

        vecTables.push_back( entry );
    }

    // check if all required tables have been found
    if( lLoca == -1 ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "Table 'loca' not found." ); 
    }
    else if( lGlyf == -1 ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "Table 'glyf' not found." ); 
    }
    else if( lMaxp == -1 ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "Table 'maxp' not found." ); 
    }
    else if( lHead == -1 ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "Table 'head' not found." ); 
    }

    // Read head table
    pDevice->Seek( lHead );
    this->ReadHeadTable( pDevice );

    // Read maxp table
    pDevice->Seek( lMaxp );
    this->ReadMaxpTable( pDevice );

    // Read loca table
    pDevice->Seek( lLoca );
    this->ReadLocaTable( pDevice );

    // read the remaining data tables
    TIVecTableDirectoryEntries it = vecTables.begin(); 
    while( it != vecTables.end() ) 
    {
        // skip the 4 tables we have alread read
        if( (*it).tag != this->CreateTag( 'g', 'l', 'y', 'f' ) &&
            (*it).tag != this->CreateTag( 'h', 'e', 'a', 'd' ) &&
            (*it).tag != this->CreateTag( 'l', 'o', 'c', 'a' ) &&
            (*it).tag != this->CreateTag( 'm', 'a', 'x', 'p' ) ) 
        {
            TTable table;
            table.tag    = (*it).tag;
            table.length = (*it).length; 
            table.data   = static_cast<char*>(malloc( sizeof(char) * (*it).length ));
            
            pDevice->Seek( (*it).offset );
            pDevice->Read( table.data, (*it).length ); 
            
            m_vecTableData.push_back( table );
        }

            
        ++it;
    }
}
    
void PdfTTFWriter::ReadTableDirectory( PdfInputDevice* pDevice )
{
    pDevice->Read( reinterpret_cast<char*>(&m_tTableDirectory), sizeof(TTableDirectory) );

    if( podofo_is_little_endian() ) 
    {
        // Swap bytes
        SwapUShort( &m_tTableDirectory.numTables );
        SwapUShort( &m_tTableDirectory.searchRange );
        SwapUShort( &m_tTableDirectory.entrySelector );
        SwapUShort( &m_tTableDirectory.rangeShift );
    }

    printf("Read TTF: numTables=%i\n", m_tTableDirectory.numTables );
}

void PdfTTFWriter::WriteTableDirectory( PdfOutputDevice* pDevice )
{
    if( podofo_is_little_endian() ) 
    {
        // Swap bytes
        SwapUShort( &m_tTableDirectory.numTables );
        SwapUShort( &m_tTableDirectory.searchRange );
        SwapUShort( &m_tTableDirectory.entrySelector );
        SwapUShort( &m_tTableDirectory.rangeShift );
    }

    pDevice->Write( reinterpret_cast<char*>(&m_tTableDirectory), sizeof(TTableDirectory) );
}

void PdfTTFWriter::ReadTableDirectoryEntry( PdfInputDevice* pDevice, TTableDirectoryEntry* pEntry ) 
{
    pDevice->Read( reinterpret_cast<char*>(pEntry), sizeof(TTableDirectoryEntry) );

    if( podofo_is_little_endian() )
    {
        SwapULong( &pEntry->tag );
        SwapULong( &pEntry->checkSum );
        SwapULong( &pEntry->offset );
        SwapULong( &pEntry->length );
    }

    printf("        : Got table: %c%c%c%c length=%u\n", 
           (pEntry->tag >> 24) & 0xff,
           (pEntry->tag >> 16) & 0xff,
           (pEntry->tag >>  8) & 0xff,
           (pEntry->tag      ) & 0xff, 
           pEntry->length );
}

void PdfTTFWriter::WriteTableDirectoryEntry( PdfOutputDevice* pDevice, TTableDirectoryEntry* pEntry )
{
    if( podofo_is_little_endian() )
    {
        SwapULong( &pEntry->tag );
        SwapULong( &pEntry->checkSum );
        SwapULong( &pEntry->offset );
        SwapULong( &pEntry->length );
    }

    pDevice->Write( reinterpret_cast<char*>(pEntry), sizeof(TTableDirectoryEntry) );
}

void PdfTTFWriter::ReadHeadTable( PdfInputDevice* pDevice )
{
    pDevice->Read( reinterpret_cast<char*>(&m_tHead), sizeof(THead) );

    if( podofo_is_little_endian() ) 
        SwapHeadTable();
}
 
void PdfTTFWriter::WriteHeadTable( PdfOutputDevice* pDevice, TVecTableDirectoryEntries & rToc )
{
    // We always write the long loca format
    m_tHead.indexToLocForm = 1;

    if( podofo_is_little_endian() ) 
        SwapHeadTable();

    TTableDirectoryEntry entry;
    entry.tag      = this->CreateTag( 'h', 'e', 'a', 'd' );
    entry.checkSum = this->CalculateChecksum( reinterpret_cast<const pdf_ttf_ulong*>(&m_tHead), sizeof(THead) );
    entry.offset   = pDevice->GetLength();
    entry.length   = sizeof(THead);

    rToc.push_back( entry );

    pDevice->Write( reinterpret_cast<char*>(&m_tHead), sizeof(THead) );
}
   
void PdfTTFWriter::SwapHeadTable() 
{
    // Swap bytes
    SwapULong ( &m_tHead.checkSumAdjustment );
    SwapULong ( &m_tHead.magicNumber );
    SwapUShort( &m_tHead.flags );
    SwapUShort( &m_tHead.unitsPerEm );
    SwapShort ( &m_tHead.fontDirectionHint );
    SwapShort ( &m_tHead.indexToLocForm );
    SwapShort ( &m_tHead.glyphDataFormat );
    
    // Do not care for other values
}

void PdfTTFWriter::ReadMaxpTable( PdfInputDevice* pDevice )
{
    pDevice->Read( reinterpret_cast<char*>(&m_tMaxp), sizeof(TMaxP) );

    if( podofo_is_little_endian() )
        SwapMaxpTable();
}

void PdfTTFWriter::WriteMaxpTable( PdfOutputDevice* pDevice, TVecTableDirectoryEntries & rToc )
{
    if( podofo_is_little_endian() )
        SwapMaxpTable();

    TTableDirectoryEntry entry;
    entry.tag      = this->CreateTag( 'm', 'a', 'x', 'p' );
    entry.checkSum = this->CalculateChecksum( reinterpret_cast<const pdf_ttf_ulong*>(&m_tMaxp), sizeof(TMaxP) );
    entry.offset   = pDevice->GetLength();
    entry.length   = sizeof(TMaxP);

    rToc.push_back( entry );

    pDevice->Write( reinterpret_cast<char*>(&m_tMaxp), sizeof(TMaxP) );
}

void PdfTTFWriter::SwapMaxpTable() 
{
    SwapUShort( &m_tMaxp.numGlyphs );
    SwapUShort( &m_tMaxp.maxPoints );
    SwapUShort( &m_tMaxp.maxContours );
    SwapUShort( &m_tMaxp.maxCompositePoints );
    SwapUShort( &m_tMaxp.maxCompositeContours );
    SwapUShort( &m_tMaxp.maxZones );
    SwapUShort( &m_tMaxp.maxTwilightPoints );
    SwapUShort( &m_tMaxp.maxStorage );
    SwapUShort( &m_tMaxp.maxFunctionsDefs );
    SwapUShort( &m_tMaxp.maxInstructionDefs );
    SwapUShort( &m_tMaxp.maxStackElements );
    SwapUShort( &m_tMaxp.maxSizeOfInstruction );
    SwapUShort( &m_tMaxp.maxComponentElements );
    SwapUShort( &m_tMaxp.maxComponentDepth );
}

void PdfTTFWriter::ReadLocaTable( PdfInputDevice* pDevice )
{
    int           n      = m_tMaxp.numGlyphs + 1;
    pdf_ttf_ulong lValue;

    if( m_tHead.indexToLocForm == 0 )
    {
        // short offsets
        pdf_ttf_ushort value;
        while( n-- ) 
        {
            pDevice->Read( reinterpret_cast<char*>(&value), sizeof(pdf_ttf_ushort) );
            this->SwapUShort( &value );

            lValue = value;

            m_tLoca.push_back( lValue );
        }
            
    }
    else if( m_tHead.indexToLocForm == 1 )
    {
        // long offsets
        while( n-- ) 
        {
            pDevice->Read( reinterpret_cast<char*>(&lValue), sizeof(pdf_ttf_ulong) );
            this->SwapULong( &lValue );

            m_tLoca.push_back( lValue );
        }
    }
    else
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "Format of loca table not recognized." );
    }
}

void PdfTTFWriter::WriteLocaTable( PdfOutputDevice* pDevice, TVecTableDirectoryEntries & rToc )
{
    TCIVecLoca    it = m_tLoca.begin();
    pdf_ttf_ulong lValue;

    TTableDirectoryEntry entry;
    entry.tag      = this->CreateTag( 'l', 'o', 'c', 'a' );
    entry.checkSum = 0;
    entry.offset   = pDevice->GetLength();
    entry.length   = sizeof(pdf_ttf_ulong) * m_tLoca.size();

    while( it != m_tLoca.end() )
    {
        lValue = (*it);

        // create the checksum for the table of contents entry
        entry.checkSum += lValue;

        this->SwapULong( &lValue );
        pDevice->Write( reinterpret_cast<const char*>(&lValue), sizeof(pdf_ttf_ulong) );

        ++it;
    }


    rToc.push_back( entry );
}

void PdfTTFWriter::WriteGlyfTable( PdfOutputDevice* pDevice, TVecTableDirectoryEntries & rToc )
{

}

void PdfTTFWriter::Subset()
{

}

void PdfTTFWriter::Write( PdfOutputDevice* pDevice )
{
    TTableDirectoryEntry      entry;
    TVecTableDirectoryEntries vecToc;
    const long                lTableOffset = sizeof(TTableDirectory);
    const long                lNumTables   = m_tTableDirectory.numTables;

    this->WriteTableDirectory( pDevice );
    
    // write dummy table of contents
    memset( &entry, 0, sizeof(TTableDirectoryEntry) );
    for( unsigned int i=0; i<static_cast<unsigned int>(lNumTables); i++ )  
        pDevice->Write( reinterpret_cast<const char*>(&entry), sizeof(TTableDirectoryEntry) );

    // write contents
    this->WriteMaxpTable( pDevice, vecToc );
    this->WriteHeadTable( pDevice, vecToc );
    this->WriteLocaTable( pDevice, vecToc );
    this->WriteGlyfTable( pDevice, vecToc );

    TCIVecTable it = m_vecTableData.begin();
    while( it != m_vecTableData.end() ) 
    {
        TTableDirectoryEntry entry;
        entry.tag      = (*it).tag;
        entry.checkSum = this->CalculateChecksum( reinterpret_cast<const pdf_ttf_ulong*>(&(*it).data), (*it).length );
        entry.offset   = pDevice->GetLength();
        entry.length   = (*it).length;
        
        vecToc.push_back( entry );

        pDevice->Write( (*it).data, (*it).length );
        free( (*it).data );
        ++it;
    }

    // write actual table of contents
    pDevice->Seek( lTableOffset );
    TIVecTableDirectoryEntries itToc = vecToc.begin(); 
    while( itToc != vecToc.end() ) 
    {
        this->WriteTableDirectoryEntry( pDevice, &(*itToc) );
        ++itToc;
    }
}

PdfTTFWriter::pdf_ttf_ulong PdfTTFWriter::CalculateChecksum( const pdf_ttf_ulong* pTable, pdf_ttf_ulong lLength ) const
{
    // This code is taken from the TTF specification
    pdf_ttf_ulong        lSum = 0L;
    const pdf_ttf_ulong* pEnd = pTable + ((lLength+3) & ~3) / sizeof(pdf_ttf_ulong);
    while( pTable < pEnd ) 
        lSum += *pTable++;

    return lSum;
}


};

};
