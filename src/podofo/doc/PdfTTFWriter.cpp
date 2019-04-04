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

#include "PdfTTFWriter.h"

#include "PdfInputDevice.h"
#include "PdfOutputDevice.h"
#include "PdfRefCountedBuffer.h"
#include "PdfString.h"
#include "PdfDefinesPrivate.h"

#include <algorithm>
#include <cmath>

#include <ft2build.h>
#include FT_FREETYPE_H

#if defined(_MSC_VER)  &&  _MSC_VER == 1500	// Ignore those warnings from MS-VS2008
#pragma warning(disable: 4018)
#pragma warning(disable: 4244)
#endif

namespace {
// MSVC++ does not provide the c99 exp2(..) and log2(..) in <cmath> since they
// are not required by C++ . Use the gcc ones if we're on gcc.
#if defined(__GNUC__)
inline double PdfLog2(double x) { return log2(x); }
inline double PdfExp2(double x) { return exp2(x); }
#else
inline double PdfLog2(double x) { return log(x)/log(2.0f); }
inline double PdfExp2(double x) { return pow(static_cast<double>(2.0f),static_cast<double>(x)); }
#endif
}

namespace PoDoFo {

extern bool podofo_is_little_endian();

#define READ_TTF_USHORT( value ) { pDevice->Read( reinterpret_cast<char*>(&(value)), sizeof(pdf_ttf_ushort) ); \
                                 if( podofo_is_little_endian() ) \
                                     this->SwapUShort( &(value) ); }

#define READ_TTF_SHORT( value )  { pDevice->Read( reinterpret_cast<char*>(&(value)), sizeof(pdf_ttf_short) ); \
                                 if( podofo_is_little_endian() ) \
                                     this->SwapShort( &(value) ); }

#define READ_TTF_ULONG( value )  { pDevice->Read( reinterpret_cast<char*>(&(value)), sizeof(pdf_ttf_ulong) ); \
                                 if( podofo_is_little_endian() ) \
                                     this->SwapULong( &(value) ); }

#define WRITE_TTF_USHORT( value ) { { if( podofo_is_little_endian() ) \
                                        this->SwapUShort( &(value) ); } \
                                      pDevice->Write( reinterpret_cast<char*>(&(value)), sizeof(pdf_ttf_ushort) ); }

#define WRITE_TTF_SHORT( value ) { { if( podofo_is_little_endian() ) \
                                        this->SwapShort( &(value) ); } \
                                      pDevice->Write( reinterpret_cast<char*>(&(value)), sizeof(pdf_ttf_short) ); }
                                    
#define WRITE_TTF_ULONG( value ) { { if( podofo_is_little_endian() ) \
                                        this->SwapULong( &(value) ); } \
                                      pDevice->Write( reinterpret_cast<char*>(&(value)), sizeof(pdf_ttf_ulong) ); }

namespace NonPublic {

/**
 * The TTF format.
 *
 * - Big endian
 *
 * - Required tables:
 *   cmap character to glyph mapping                 CHK
 *   glyf glyph data                                 CHK
 *   head font header                                CHK
 *   hhea horizontal header                          CHK
 *   hmtx horizontal metrics                         CHK
 *   loca index to location                          CHK
 *   maxp maximum profile                            CHK
 *   name naming table                               CHK                     
 *   post PostScript information
 *   OS/2 OS/2 and Windows specific metrics          CHK
 *
 */

PdfTTFWriter::PdfTTFWriter()
    : m_lGlyphDataOffset( -1L ), m_lCMapOffset( -1L ), m_pRefBuffer( NULL )
{
    m_vecGlyphIndeces.push_back( static_cast<int>('H') );
    m_vecGlyphIndeces.push_back( static_cast<int>('a') );
    m_vecGlyphIndeces.push_back( static_cast<int>('l') );
    m_vecGlyphIndeces.push_back( static_cast<int>('o') );
    m_vecGlyphIndeces.push_back( static_cast<int>(' ') );
    m_vecGlyphIndeces.push_back( static_cast<int>('W') );
    m_vecGlyphIndeces.push_back( static_cast<int>('r') );
    m_vecGlyphIndeces.push_back( static_cast<int>('d') );
    m_vecGlyphIndeces.push_back( static_cast<int>('!') );
    // Composites do not work yet:
    m_vecGlyphIndeces.push_back( 0x00E4 ); // A dieresis

    std::sort( m_vecGlyphIndeces.begin(), m_vecGlyphIndeces.end() );
}

PdfTTFWriter::PdfTTFWriter( const std::vector<int> & rvecGlyphs )
    : m_lGlyphDataOffset( -1L ), m_lCMapOffset( -1L ), m_pRefBuffer( NULL )
{
    m_vecGlyphIndeces = rvecGlyphs;

    std::sort( m_vecGlyphIndeces.begin(), m_vecGlyphIndeces.end() );
}

PdfTTFWriter::~PdfTTFWriter()
{
    delete m_pRefBuffer;
}

void PdfTTFWriter::Read( PdfInputDevice* pDevice )
{
    long lHead = -1;
    long lHHea = -1;
    long lLoca = -1;
    long lMaxp = -1;
    long lOs2  = -1;
    long lHmtx = -1;
    long lPost = -1;

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
            m_lGlyphDataOffset = entry.offset;
        else if( entry.tag == this->CreateTag( 'm', 'a', 'x', 'p' ) ) 
            lMaxp = entry.offset;
        else if( entry.tag == this->CreateTag( 'h', 'e', 'a', 'd' ) ) 
            lHead = entry.offset;
        else if( entry.tag == this->CreateTag( 'c', 'm', 'a', 'p' ) )
            m_lCMapOffset = entry.offset;
        else if( entry.tag == this->CreateTag( 'h', 'h', 'e', 'a' ) )
            lHHea = entry.offset;
        else if( entry.tag == this->CreateTag( 'O', 'S', '/', '2' ) )
            lOs2 = entry.offset;
        else if( entry.tag == this->CreateTag( 'h', 'm', 't', 'x' ) )
            lHmtx = entry.offset;
        else if( entry.tag == this->CreateTag( 'p', 'o', 's', 't' ) )
            lPost = entry.offset;

        vecTables.push_back( entry );
    }

    // check if all required tables have been found
    if( lLoca == -1 ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "Table 'loca' not found." ); 
    }
    else if( m_lGlyphDataOffset == -1 ) 
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
    else if( m_lCMapOffset == -1 ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "Table 'cmap' not found." ); 
    }
    else if( lHHea == -1 ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "Table 'hhea' not found." ); 
    }
    else if( lOs2 == -1 ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "Table 'OS/2' not found." ); 
    }
    else if( lHmtx == -1 ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "Table 'hmtx' not found." ); 
    }
    else if( lPost == -1 ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "Table 'post' not found." ); 
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

    // Read hhea table
    pDevice->Seek( lHHea );
    this->ReadHHeaTable( pDevice );

    // Read cmap table
    pDevice->Seek( m_lCMapOffset );
    this->ReadCmapTable( pDevice );

    // Read glyf table
    pDevice->Seek( m_lGlyphDataOffset );
    this->ReadGlyfTable( pDevice );

    // Read OS/2 table
    pDevice->Seek( lOs2 );
    this->ReadOs2Table( pDevice );

    // Read hmtx table
    pDevice->Seek( lHmtx );
    this->ReadHmtxTable( pDevice );

    // Read post table
    pDevice->Seek( lPost );
    this->ReadPostTable( pDevice );

    // read the remaining data tables
    TIVecTableDirectoryEntries it = vecTables.begin(); 
    while( it != vecTables.end() ) 
    {
        // skip the 4 tables we have alread read
        if( (*it).tag != this->CreateTag( 'g', 'l', 'y', 'f' ) &&
            (*it).tag != this->CreateTag( 'h', 'e', 'a', 'd' ) &&
            (*it).tag != this->CreateTag( 'l', 'o', 'c', 'a' ) &&
            (*it).tag != this->CreateTag( 'm', 'a', 'x', 'p' ) &&
            (*it).tag != this->CreateTag( 'h', 'h', 'e', 'a' ) &&
            (*it).tag != this->CreateTag( 'c', 'm', 'a', 'p' ) &&
            (*it).tag != this->CreateTag( 'O', 'S', '/', '2' ) &&
            (*it).tag != this->CreateTag( 'n', 'a', 'm', 'e' ) )
        {
            TTable table;
            table.tag    = (*it).tag;
            table.length = (*it).length; 
            table.data   = static_cast<char*>(podofo_calloc( (*it).length, sizeof(char) ));
            
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

    /*
    printf("        : Got table: %c%c%c%c length=%u\n", 
           (pEntry->tag >> 24) & 0xff,
           (pEntry->tag >> 16) & 0xff,
           (pEntry->tag >>  8) & 0xff,
           (pEntry->tag      ) & 0xff, 
           pEntry->length );
    */
}

void PdfTTFWriter::ReadOs2Table( PdfInputDevice* pDevice )
{
   pDevice->Read( reinterpret_cast<char*>(&m_tOs2), sizeof(TOs2) );

    if( podofo_is_little_endian() ) 
        SwapOs2Table();
}

void PdfTTFWriter::ReadHmtxTable( PdfInputDevice* pDevice )
{
    // read numOfHMetrics long values
    TLongHorMetric longValue;
    int            n            = m_tHHea.numberOfHMetrics;

    while( n-- ) 
    {
        READ_TTF_USHORT( longValue.advanceWidth );
        READ_TTF_SHORT ( longValue.leftSideBearing );

        m_vecHmtx.push_back( longValue );
    }

    // read numGlyphs - numOfHMetrics short values
    n = m_tHHea.numberOfHMetrics - m_tMaxp.numGlyphs;
    while( n-- ) 
    {
        // advanceWidth stays the same as in the last read long value
        READ_TTF_SHORT ( longValue.leftSideBearing );
        m_vecHmtx.push_back( longValue );
    }
}

void PdfTTFWriter::SwapOs2Table() 
{
    SwapUShort ( &m_tOs2.version );
    SwapShort  ( &m_tOs2.xAvgCharWidth );
    SwapUShort ( &m_tOs2.usWeightClass );
    SwapUShort ( &m_tOs2.usWidthClass );
    SwapShort  ( &m_tOs2.fsType );
    SwapShort  ( &m_tOs2.ySubscriptXSize );
    SwapShort  ( &m_tOs2.ySubscriptYSize );
    SwapShort  ( &m_tOs2.ySubscriptXOffset );
    SwapShort  ( &m_tOs2.ySubscriptYOffset );
    SwapShort  ( &m_tOs2.ySuperscriptXSize );
    SwapShort  ( &m_tOs2.ySuperscriptYSize );
    SwapShort  ( &m_tOs2.ySuperscriptXOffset );
    SwapShort  ( &m_tOs2.ySuperscriptYOffset );
    SwapShort  ( &m_tOs2.yStrikeoutSize );
    SwapShort  ( &m_tOs2.yStrikeoutPosition );
    SwapShort  ( &m_tOs2.sFamilyClass );
    SwapULong  ( &m_tOs2.ulUnicodeRange1 );
    SwapULong  ( &m_tOs2.ulUnicodeRange2 );
    SwapULong  ( &m_tOs2.ulUnicodeRange3 );
    SwapULong  ( &m_tOs2.ulUnicodeRange4 );
    SwapUShort ( &m_tOs2.fsSelection );
    SwapUShort ( &m_tOs2.usFirstCharIndex );
    SwapUShort ( &m_tOs2.usLastCharIndex );
    SwapUShort ( &m_tOs2.sTypoAscender );
    SwapUShort ( &m_tOs2.sTypoDescender );
    SwapUShort ( &m_tOs2.sTypoLineGap );
    SwapUShort ( &m_tOs2.usWinAscent );
    SwapUShort ( &m_tOs2.usWinDescent );
    SwapULong  ( &m_tOs2.ulCodePageRange1 );
    SwapULong  ( &m_tOs2.ulCodePageRange2 );
}

void PdfTTFWriter::ReadHeadTable( PdfInputDevice* pDevice )
{
    pDevice->Read( reinterpret_cast<char*>(&m_tHead), sizeof(THead) );

    if( podofo_is_little_endian() ) 
        SwapHeadTable();
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

void PdfTTFWriter::ReadHHeaTable( PdfInputDevice* pDevice )
{
    pDevice->Read( reinterpret_cast<char*>(&m_tHHea), sizeof(THHea) );

    if( podofo_is_little_endian() )
        SwapHHeaTable();
}

void PdfTTFWriter::SwapHHeaTable() 
{
    SwapFWord  ( &m_tHHea.advanceWidthMax );
    SwapFWord  ( &m_tHHea.minLeftSideBearing );
    SwapFWord  ( &m_tHHea.minRightSideBearing );
    SwapFWord  ( &m_tHHea.xMaxExtent );
    SwapUShort ( &m_tHHea.numberOfHMetrics );
}

void PdfTTFWriter::ReadPostTable( PdfInputDevice* pDevice )
{
    pDevice->Read( reinterpret_cast<char*>(&m_tPost), sizeof(TPost) );

    if( podofo_is_little_endian() )
        SwapPostTable();
}

void PdfTTFWriter::SwapPostTable() 
{
    /*
    pdf_ttf_fixed format;
    pdf_ttf_fixed italicAngle;
    pdf_ttf_fword underlinePosition;
    pdf_ttf_fword underlineThickness;
    pdf_ttf_ulong isFixedPitch;
    pdf_ttf_ulong minMemType42;
    pdf_ttf_ulong maxMemType42;
    pdf_ttf_ulong minMemType1;
    pdf_ttf_ulong maxMemType1;
    */  
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

void PdfTTFWriter::ReadCmapTable( PdfInputDevice* pDevice )
{
    TCMapEntry     entry;
    int            nUnicodeIndex = -1;
    pdf_ttf_ushort tableVersion;
    pdf_ttf_ushort numberOfTables;

    std::vector<TCMapEntry> cmap;

    READ_TTF_USHORT( tableVersion   );
    READ_TTF_USHORT( numberOfTables );
    
    while( numberOfTables-- ) 
    {
        READ_TTF_USHORT( entry.platformId );
        READ_TTF_USHORT( entry.encodingId );
        READ_TTF_ULONG ( entry.offset );
        //printf("Got cmap table: %u %u at %u\n",entry.platformId, entry.encodingId, entry.offset );

        cmap.push_back( entry );

        if( entry.platformId == 3 && entry.encodingId == 1 )
            nUnicodeIndex = cmap.size() - 1;
    }

    if( nUnicodeIndex == -1 ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "No unicode cmap table found." );
        // TODO: Use other tables to build a unicode table.
    }

    m_lCMapOffset += cmap[nUnicodeIndex].offset; // Reset current cmap offset to actual cmap offset
    pDevice->Seek( m_lCMapOffset );
    
    READ_TTF_USHORT( format4.format );
    READ_TTF_USHORT( format4.length );
    READ_TTF_USHORT( format4.version );
    READ_TTF_USHORT( format4.segCountX2 );
    READ_TTF_USHORT( format4.searchRange );
    READ_TTF_USHORT( format4.entrySelector );
    READ_TTF_USHORT( format4.rangeShift );

    /*
    printf("Format: %i\n", format4.format );
    printf("Length: %i\n", format4.length );
    printf("SegCountX2: %i\n", format4.segCountX2 );
    printf("Range Shift: %i\n", format4.rangeShift );
    *    */
    int            i;
    const int      nSegCount = format4.segCountX2 >> 1;
    pdf_ttf_ushort nReservedPad;

    m_ranges.resize( nSegCount );

    for( i=0;i<nSegCount;i++ )
        READ_TTF_USHORT( m_ranges[i].nEnd );

    READ_TTF_USHORT( nReservedPad );

    for( i=0;i<nSegCount;i++ )
        READ_TTF_USHORT( m_ranges[i].nStart );

    for( i=0;i<nSegCount;i++ )
        READ_TTF_SHORT( m_ranges[i].nDelta );

    for( i=0;i<nSegCount;i++ )
    {
        long lPos = pDevice->Tell();
        READ_TTF_USHORT( m_ranges[i].nOffset );
        /*
        if( m_ranges[i].nOffset ) 
        {
            printf("BEFORE=%i\n", m_ranges[i].nOffset );
            m_ranges[i].nOffset -= ((nSegCount - i) * 2);
            printf("AFTER =%i\n", static_cast<pdf_ttf_short>(m_ranges[i].nOffset) );
        }
        */

#if 0
        if( m_ranges[i].nOffset )
        {
            if( m_ranges[i].nStart == 160 ) 
            {
                /*
         if(theLowByte >= firstCode && theLowByte < (firstCode + Int16FromMOTA(subHeader2s[k].entryCount))) {
             return *((&(subHeader2s[0].idRangeOffset))
                      + (Int16FromMOTA(subHeader2s[0].idRangeOffset)/2)
                      + theLowByte                                     
                      - Int16FromMOTA(subHeader2s[0].firstCode)
                      );
            */

                int c = 228;
                printf("OFFSET1 = %i\n", m_ranges[i].nOffset );
                printf("OFFSET2 = %i\n", (c - m_ranges[i].nStart) );
                printf("POS     = %i\n", lPos );

                //long lAddress = (m_ranges[i].nOffset/2 +  (c - m_ranges[i].nStart) + lPos );
                long lAddress = lPos + m_ranges[i].nOffset/2 + (c - m_ranges[i].nStart);
                printf("GOT ADDRESS: %x\n", lAddress - m_lCMapOffset );
                pDevice->Seek( lAddress );
                pdf_ttf_ushort u;
                READ_TTF_USHORT( u );
                printf("Index=%u\n", u );
                pDevice->Seek( lPos + sizeof(pdf_ttf_ushort) );
            }
        }
#endif // 0
    }

    m_lCMapOffset  = (pDevice->Tell() - m_lCMapOffset);
    long           lGlyphIdArrayLen = (format4.length - m_lCMapOffset) / sizeof(pdf_ttf_ushort); 
    pdf_ttf_ushort glyphId;

    m_vecGlyphIds.reserve( lGlyphIdArrayLen );
    while( lGlyphIdArrayLen-- ) 
    {
        READ_TTF_USHORT( glyphId );
        m_vecGlyphIds.push_back( glyphId );
    }

    // in case of broken TTF we have to sort this table
    std::sort( m_ranges.begin(), m_ranges.end() );
    
    /*
    for( i=0;i<nSegCount;i++ ) 
    {
        printf("Range: %x - %x Delta: %6i Offset: %6u\n", m_ranges[i].nStart, m_ranges[i].nEnd, 
               m_ranges[i].nDelta, m_ranges[i].nOffset );
        
    }
    */
}

void PdfTTFWriter::SwapGlyfHeader( TGlyphHeader* pHeader )
{
    SwapShort ( &pHeader->numberOfContours );
    SwapFWord ( &pHeader->xMin );
    SwapFWord ( &pHeader->yMin );
    SwapFWord ( &pHeader->xMax );
    SwapFWord ( &pHeader->yMax );
}

#define ARG_1_AND_2_ARE_WORDS      0x0001
#define ARGS_ARE_XY_VALUES         0x0002
#define ROUND_XY_TO_GRID           0x0004
#define WE_HAVE_A_SCALE            0x0008
#define RESERVED                   0x0010
#define MORE_COMPONENTS            0x0020
#define WE_HAVE_AN_X_AND_Y_SCALE   0x0040
#define WE_HAVE_A_TWO_BY_TWO       0x0080
#define WE_HAVE_INSTRUCTIONS       0x0100
#define USE_MY_METRICS             0x0200

void PdfTTFWriter::ReadGlyfTable( PdfInputDevice* pDevice )
{
    std::vector<int>::const_iterator it = m_vecGlyphIndeces.begin();
    long                             lOffset;
    long                             lLength;

    while( it != m_vecGlyphIndeces.end() )
    {
        lOffset = this->GetGlyphDataLocation( *it, &lLength, pDevice );
        if( lOffset != -1 )
        { 
            PdfRefCountedBuffer buffer( lLength );

            pDevice->Seek( lOffset );
            pDevice->Read( buffer.GetBuffer(), lLength );

            PdfTTFGlyph glyph( *it );
            glyph.m_buffer = buffer;
            m_vecGlyphs.push_back( glyph );

            //this->LoadGlyph( *it, lOffset, pDevice );
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "Charnotfound" );
        }

        ++it;
    }
}

void PdfTTFWriter::LoadGlyph( int nIndex, long lOffset, PdfInputDevice* pDevice )
{
    printf("!!!! Loading glyph %i\n", nIndex );
    PdfTTFGlyph glyph( nIndex );

    pDevice->Seek( lOffset );
    pDevice->Read( reinterpret_cast<char*>(&glyph.m_tHeader), sizeof(TGlyphHeader) );
    if( podofo_is_little_endian() )
        SwapGlyfHeader( &glyph.m_tHeader );

    glyph.SetComposite( glyph.m_tHeader.numberOfContours == -1 );
    printf("Glyph with index %i is %s. (contours) = %i\n", nIndex, glyph.IsComposite() ? "composite" : "simple", glyph.m_tHeader.numberOfContours );

    if( !glyph.IsComposite() )
    {
        // Read the end points
        int            nContours = glyph.m_tHeader.numberOfContours;
        pdf_ttf_ushort nEndPoint;
        
        glyph.vecEndPoints.reserve( nContours );
        while( nContours-- ) 
        {
            READ_TTF_USHORT( nEndPoint );
            
            printf("Reading endpoint: %i\n", nEndPoint );
            glyph.vecEndPoints.push_back( nEndPoint );
        }

        // read instructions 
        pDevice->Read( reinterpret_cast<char*>(&glyph.m_nInstructionLength), sizeof(pdf_ttf_ushort) );
        if( podofo_is_little_endian() )
            SwapUShort( &glyph.m_nInstructionLength );
        
        printf("Reading instructions: %i\n", glyph.m_nInstructionLength );
        if( glyph.m_nInstructionLength ) 
        {
            glyph.m_pInstructions = static_cast<char*>(podofo_calloc( glyph.m_nInstructionLength, sizeof(char) ));
            if( !glyph.m_pInstructions ) 
            {
                PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
            }
            
            pDevice->Read( glyph.m_pInstructions, glyph.m_nInstructionLength );
        }
        
        
        unsigned char  flag;
        unsigned char  repeat;
        int      nPoints = glyph.vecEndPoints.back();

        // read flags
        printf("Reading flags: %i\n", nPoints );
        while( --nPoints >= 0 ) 
        {
            pDevice->Read( reinterpret_cast<char*>(&flag), sizeof(char) );
            glyph.vecFlagsOrig.push_back( flag );
            if( (flag & 0x08) == 0x08 ) // i.e. the next byte tells us how often this flag is to be repeated
            {
                pDevice->Read( reinterpret_cast<char*>(&repeat), sizeof(char) );
                glyph.vecFlagsOrig.push_back( repeat );
                /*
                while( repeat-- )
                    glyph.vecFlags.push_back( flag );
                */
            }
        };


        printf("!!!XXXXXXX\n");
        ReadSimpleGlyfCoordinates( pDevice, glyph.vecFlags, glyph.vecXCoordinates, 0x02, 0x10 );
        printf("!!!YYYYYYY\n");
        ReadSimpleGlyfCoordinates( pDevice, glyph.vecFlags, glyph.vecYCoordinates, 0x04, 0x20 );

        /*
        for( int z=0;z<glyph.vecXCoordinates.size();z++ ) 
        {
            printf("Coordinate %i %i\n", glyph.vecXCoordinates[z], glyph.vecYCoordinates[z] );
        }
        */
    }
    else
    {
        // read a composite glyph

        pdf_ttf_ushort flags;
        do {
            pdf_ttf_ushort glyphIndex;
            
            pDevice->Read( reinterpret_cast<char*>(&flags),      sizeof(pdf_ttf_ushort) );
            pDevice->Read( reinterpret_cast<char*>(&glyphIndex), sizeof(pdf_ttf_ushort) );

            if( podofo_is_little_endian() )
            {
                this->SwapUShort( &flags      );
                this->SwapUShort( &glyphIndex );
            }

            printf("glyphIndex=%i and should be %i\n", glyphIndex, nIndex );
            if( glyphIndex != nIndex )
            {
                PODOFO_RAISE_ERROR( ePdfError_InvalidFontFile );
            }

            if( flags & ARG_1_AND_2_ARE_WORDS ) 
            {
                pDevice->Read( reinterpret_cast<char*>(&glyph.arg1), sizeof(pdf_ttf_short) );
                pDevice->Read( reinterpret_cast<char*>(&glyph.arg2), sizeof(pdf_ttf_short) );
                
                if( podofo_is_little_endian() ) 
                {
                    this->SwapShort( &glyph.arg1 );
                    this->SwapShort( &glyph.arg2 );
                }
            }
            else
            {
                char cArg1;
                char cArg2;
                
                pDevice->Read( &cArg1, sizeof(char) );
                pDevice->Read( &cArg2, sizeof(char) );
                
                glyph.arg1 = cArg1;
                glyph.arg2 = cArg2;
            }
            
//          glyph.xx = glyph.yy = 0x10000L;
            glyph.xx = glyph.yy = 0x00;
            
            if ( flags & WE_HAVE_A_SCALE ) 
            {
                //F2Dot14  scale;    /* Format 2.14 */
                pDevice->Read( reinterpret_cast<char*>(&glyph.xx), sizeof(pdf_ttf_short) );
                if( podofo_is_little_endian() )
                    this->SwapShort( &glyph.xx );
                
                glyph.yy = glyph.xx;
            } 
            else if ( flags & WE_HAVE_AN_X_AND_Y_SCALE ) 
            {
                //F2Dot14  xscale;    /* Format 2.14 */
                //F2Dot14  yscale;    /* Format 2.14 */
                pDevice->Read( reinterpret_cast<char*>(&glyph.xx), sizeof(pdf_ttf_short) );
                pDevice->Read( reinterpret_cast<char*>(&glyph.yy), sizeof(pdf_ttf_short) );
                if( podofo_is_little_endian() )
                {
                    this->SwapShort( &glyph.xx );
                    this->SwapShort( &glyph.yy );
                }
            } 
            else if ( flags & WE_HAVE_A_TWO_BY_TWO ) 
            {
                pDevice->Read( reinterpret_cast<char*>(&glyph.xx), sizeof(pdf_ttf_short) );
                pDevice->Read( reinterpret_cast<char*>(&glyph.yx), sizeof(pdf_ttf_short) );
                pDevice->Read( reinterpret_cast<char*>(&glyph.yy), sizeof(pdf_ttf_short) );
                pDevice->Read( reinterpret_cast<char*>(&glyph.xy), sizeof(pdf_ttf_short) );
                if( podofo_is_little_endian() )
                {
                    this->SwapShort( &glyph.xx );
                    this->SwapShort( &glyph.yx );
                    this->SwapShort( &glyph.yy );
                    this->SwapShort( &glyph.xy );
                }
                
                //F2Dot14  xscale;    /* Format 2.14 */
                //F2Dot14  scale01;   /* Format 2.14 */
                //F2Dot14  scale10;   /* Format 2.14 */
                //F2Dot14  yscale;    /* Format 2.14 */
            }
        }
        while( flags & MORE_COMPONENTS );
           
        if( flags & WE_HAVE_INSTRUCTIONS )
        {
            pDevice->Read( reinterpret_cast<char*>(&glyph.m_nInstructionLength), sizeof(pdf_ttf_ushort) );
            if( podofo_is_little_endian() )
                this->SwapUShort( &glyph.m_nInstructionLength );
            
            if( glyph.m_nInstructionLength ) 
            {
                glyph.m_pInstructions = static_cast<char*>(podofo_calloc( glyph.m_nInstructionLength,  sizeof(char) ));
                if( !glyph.m_pInstructions ) 
                {
                    PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
                }
                
                pDevice->Read( glyph.m_pInstructions, glyph.m_nInstructionLength );
            }
        }
    }

    m_vecGlyphs.push_back( glyph );
}


void PdfTTFWriter::Subset()
{

}

// -----------------------------------------------------
// Writing out a TTF file from memory
// -----------------------------------------------------
void PdfTTFWriter::Write( PdfOutputDevice* pDevice )
{
    m_tTableDirectory.numTables = 9; // DS TMP

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
    this->WriteTable( pDevice, vecToc, 
                      this->CreateTag( 'm', 'a', 'x', 'p' ), &PdfTTFWriter::WriteMaxpTable );
    this->WriteTable( pDevice, vecToc, 
                      this->CreateTag( 'h', 'e', 'a', 'd' ), &PdfTTFWriter::WriteHeadTable );
    this->WriteTable( pDevice, vecToc, 
                      this->CreateTag( 'g', 'l', 'y', 'f' ), &PdfTTFWriter::WriteGlyfTable );
    this->WriteTable( pDevice, vecToc, 
                      this->CreateTag( 'c', 'm', 'a', 'p' ), &PdfTTFWriter::WriteCMapTable );
    this->WriteTable( pDevice, vecToc, 
                      this->CreateTag( 'l', 'o', 'c', 'a' ), &PdfTTFWriter::WriteLocaTable );
    this->WriteTable( pDevice, vecToc, 
                      this->CreateTag( 'h', 'h', 'e', 'a' ), &PdfTTFWriter::WriteHHeaTable );
    this->WriteTable( pDevice, vecToc, 
                      this->CreateTag( 'O', 'S', '/', '2' ), &PdfTTFWriter::WriteOs2Table );
    this->WriteTable( pDevice, vecToc, 
                      this->CreateTag( 'n', 'a', 'm', 'e' ), &PdfTTFWriter::WriteNameTable );
    this->WriteTable( pDevice, vecToc, 
                      this->CreateTag( 'h', 'm', 't', 'x' ), &PdfTTFWriter::WriteHmtxTable );
    this->WriteTable( pDevice, vecToc, 
                      this->CreateTag( 'p', 'o', 's', 't' ), &PdfTTFWriter::WritePostTable );


    // TODO:Check why this is commented. Without this code, we will have a memory leak because (*it).data is never podofo_free'ed
    /*
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
        podofo_free( (*it).data );
        ++it;
    }
    */

    // write actual table of contents
    pDevice->Seek( lTableOffset );
    TIVecTableDirectoryEntries itToc = vecToc.begin(); 
    while( itToc != vecToc.end() ) 
    {
        this->WriteTableDirectoryEntry( pDevice, &(*itToc) );
        ++itToc;
    }
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

void PdfTTFWriter::WriteGlyfTable( PdfOutputDevice* pDevice )
{
    TIVecGlyphs it = m_vecGlyphs.begin();
    int nPosition  = 0;

    while( it != m_vecGlyphs.end() )
    {
        // set the position of the glyph so that a cmap can be generated
        (*it).SetPosition( nPosition++ );

        pDevice->Write( (*it).m_buffer.GetBuffer(), (*it).m_buffer.GetSize() );

        // add value to new loca table so that it can be created correctly
        m_vecLoca.push_back( pDevice->Tell() ); 

#if 0

        if( podofo_is_little_endian() )
            SwapGlyfHeader( &(*it).m_tHeader );
        pDevice->Write( reinterpret_cast<char*>(&(*it).m_tHeader), sizeof(TGlyphHeader) );

        if( (*it).IsComposite() ) 
        {
            // TODO: Write a composite glyph

        }
        else
        {
            // Write a simple glyph
            std::vector<pdf_ttf_ushort>::iterator itEndPoints = (*it).vecEndPoints.begin();
            while( itEndPoints != (*it).vecEndPoints.end() )
            {
                WRITE_TTF_USHORT( *itEndPoints );
                ++itEndPoints;
            }

            pdf_ttf_ushort nLength = (*it).GetInstrunctionLength();
            WRITE_TTF_USHORT( nLength );
            pDevice->Write( (*it).GetInstrunctions(), (*it).GetInstrunctionLength() );
            printf("?????= Glyph flags size=%i\n", (*it).vecFlagsOrig.size() );
            pDevice->Write( reinterpret_cast<char*>(&(*it).vecFlagsOrig[0]), sizeof(char) * (*it).vecFlagsOrig.size() );

            WriteSimpleGlyfCoordinates( pDevice, (*it).vecFlags, (*it).vecXCoordinates, 0x02, 0x10 );
            WriteSimpleGlyfCoordinates( pDevice, (*it).vecFlags, (*it).vecYCoordinates, 0x04, 0x20 );
        }
#endif // 0
        ++it;
    }
    
    // add an additional entry to loca so that the length of the last character can be determined
    m_vecLoca.push_back( pDevice->Tell() ); 
}

void PdfTTFWriter::WriteCMapTable( PdfOutputDevice* pDevice )
{
    pdf_ttf_ushort nValue = 0;
    pdf_ttf_ulong  lValue = 12;
    WRITE_TTF_USHORT( nValue );  // table version
    nValue = 1;
    WRITE_TTF_USHORT( nValue );  // number of tables
    nValue = 3;
    WRITE_TTF_USHORT( nValue );  // platform id (microsoft)
    nValue = 1;
    WRITE_TTF_USHORT( nValue );  // encoding id (unicode)
    WRITE_TTF_ULONG ( lValue );  // offset
    
    // create a cmap table in ram
    std::vector<TCMapRange> vecRanges;
    TCMapRange   current; 
    TCIVecGlyphs it     = m_vecGlyphs.begin();
    bool         bReset = true;

    while( it != m_vecGlyphs.end() )
    {
        if( bReset ) 
        {
            if( it != m_vecGlyphs.begin() )
                vecRanges.push_back( current );

            current.nStart  = (*it).GetIndex();
            current.nEnd    = (*it).GetIndex();
            current.nDelta  = -((*it).GetIndex() - (*it).GetPosition());
            current.nOffset = 0;
            bReset          = false;
            ++it;
        }
        else
        {
            if( (*it).GetIndex() == current.nEnd + 1 ) 
            {
                current.nEnd++;
                ++it;
            }
            else
                bReset = true;
        }
    }

    if( !bReset )
        vecRanges.push_back( current );

    // create the ending section
    current.nStart  = 0xFFFF;
    current.nEnd    = 0xFFFF;
    current.nDelta  = 0;
    current.nOffset = 0;

    vecRanges.push_back( current );

    int            i;
    double         dSearchRange = 2.0 * ( PdfExp2( floor( PdfLog2( vecRanges.size() ) ) ) );
    pdf_ttf_ushort nSearchRange = static_cast<pdf_ttf_ushort>(dSearchRange);

    // write the actual cmap table
    nValue = 4; 
    WRITE_TTF_USHORT( nValue ); // format
    nValue = vecRanges.size() * sizeof(pdf_ttf_ushort) * 4 + 16; // 4 parallel array per segment + 16 bytes of header
    WRITE_TTF_USHORT( nValue ); // length
    nValue = 0;
    WRITE_TTF_USHORT( nValue ); // version
    nValue = vecRanges.size() * 2;
    WRITE_TTF_USHORT( nValue ); // seg count * 2
    nValue = nSearchRange;
    WRITE_TTF_USHORT( nValue ); // search range
    nValue = static_cast<pdf_ttf_ushort>(PdfLog2( vecRanges.size() >> 1 ));
    WRITE_TTF_USHORT( nValue ); // entry selector
    nValue = 2 * vecRanges.size() - nSearchRange;
    WRITE_TTF_USHORT( nValue ); // range shift
    
    for( i=0;i<static_cast<int>(vecRanges.size());i++ )
    {
        WRITE_TTF_USHORT( vecRanges[i].nEnd );
    }
    
    nValue = 0;
    WRITE_TTF_USHORT( nValue ); // reserve pad

    for( i=0;i<static_cast<int>(vecRanges.size());i++ )
    {
        WRITE_TTF_USHORT( vecRanges[i].nStart );
    }

    for( i=0;i<static_cast<int>(vecRanges.size());i++ )
    {
        WRITE_TTF_SHORT( vecRanges[i].nDelta );
    }

    for( i=0;i<static_cast<int>(vecRanges.size());i++ )
    {
        WRITE_TTF_USHORT( vecRanges[i].nOffset );
    }
}

void PdfTTFWriter::WriteHHeaTable( PdfOutputDevice* pDevice )
{
    // We always write the long loca format
    // numberOfHMetrics has to be equal to numGlyphs so
    // that we have only to write LongHorMetric values to Hmtx and no short values
    m_tHHea.numberOfHMetrics = m_vecGlyphs.size();

    if( podofo_is_little_endian() ) 
        SwapHHeaTable();

    pDevice->Write( reinterpret_cast<char*>(&m_tHHea), sizeof(THHea) );
}

void PdfTTFWriter::WriteHmtxTable( PdfOutputDevice* pDevice )
{
    TLongHorMetric entry;
    TCIVecGlyphs   it = m_vecGlyphs.begin();

    while( it != m_vecGlyphs.end() )
    {
        entry = m_vecHmtx[(*it).GetIndex()];
        
        WRITE_TTF_USHORT( entry.advanceWidth    );
        WRITE_TTF_SHORT ( entry.leftSideBearing );

        ++it;
    }
}

void PdfTTFWriter::WriteLocaTable( PdfOutputDevice* pDevice )
{
    TCIVecLoca    it    = m_vecLoca.begin();
    pdf_ttf_ulong lValue;

    // Write a 0 value for first glyph
    lValue = 0x00;
    this->SwapULong( &lValue );
    pDevice->Write( reinterpret_cast<const char*>(&lValue), sizeof(pdf_ttf_ulong) );

    while( it != m_vecLoca.end() )
    {
        lValue = (*it);
        this->SwapULong( &lValue );
        pDevice->Write( reinterpret_cast<const char*>(&lValue), sizeof(pdf_ttf_ulong) );

        ++it;
    }
}

void PdfTTFWriter::WriteMaxpTable( PdfOutputDevice* pDevice )
{
    m_tMaxp.numGlyphs = m_vecGlyphs.size();

    if( podofo_is_little_endian() )
        SwapMaxpTable();

    pDevice->Write( reinterpret_cast<char*>(&m_tMaxp), sizeof(TMaxP) );
}

void PdfTTFWriter::WriteNameTable( PdfOutputDevice* pDevice )
{
    const char* pszFontName = "PoDoFo"; 
    PdfString   unicodeName = PdfString( pszFontName ).ToUnicode();
    long  lLen              = unicodeName.GetLength();

    // Create a custom nametable
    TNameTable tNameTable;

    tNameTable.format       = 0;
    tNameTable.numRecords   = 1;
    tNameTable.offset       = 6;
    tNameTable.platformId   = 0;
    tNameTable.encodingId   = 3;
    tNameTable.languageId   = 0x0809;
    tNameTable.nameId       = 6;
    tNameTable.stringLength = static_cast<pdf_ttf_ushort>(lLen);
    tNameTable.stringOffset = 12;
    
    SwapUShort( &tNameTable.format );
    SwapUShort( &tNameTable.numRecords );
    SwapUShort( &tNameTable.offset );
    SwapUShort( &tNameTable.platformId );
    SwapUShort( &tNameTable.encodingId );
    SwapUShort( &tNameTable.languageId );
    SwapUShort( &tNameTable.nameId );
    SwapUShort( &tNameTable.stringLength );
    SwapUShort( &tNameTable.stringOffset );

    pDevice->Write( reinterpret_cast<char*>(&tNameTable), sizeof(TNameTable) );
    pDevice->Write( unicodeName.GetString(), lLen );

}

void PdfTTFWriter::WriteOs2Table( PdfOutputDevice* pDevice )
{
    // We always write the long loca format
    if( podofo_is_little_endian() ) 
        SwapOs2Table();

    pDevice->Write( reinterpret_cast<char*>(&m_tOs2), sizeof(TOs2) );
}
 
void PdfTTFWriter::WriteHeadTable( PdfOutputDevice* pDevice )
{
    // We always write the long loca format
    m_tHead.indexToLocForm = 1;

    if( podofo_is_little_endian() ) 
        SwapHeadTable();

    pDevice->Write( reinterpret_cast<char*>(&m_tHead), sizeof(THead) );
}

void PdfTTFWriter::WritePostTable( PdfOutputDevice* pDevice )
{
    m_tPost.format = 0x00020000;

    // write table header
    pDevice->Write( reinterpret_cast<char*>(&m_tPost), sizeof(TPost) );

    // write format 2 post table
    int nNameIndexValue = 258;

    pdf_ttf_ushort nameIndex;
    pdf_ttf_ushort numberOfGlyphs = m_tMaxp.numGlyphs; // this value has already been swapped when writing the maxp-table
    pDevice->Write( reinterpret_cast<char*>(&numberOfGlyphs), sizeof(pdf_ttf_ushort) );

    SwapUShort( &numberOfGlyphs ); // swap it to native format so that we can use it
    while( numberOfGlyphs-- )
    {
        nameIndex = nNameIndexValue++;
        SwapUShort( &nameIndex );
        pDevice->Write( reinterpret_cast<char*>(&nameIndex), sizeof(pdf_ttf_ushort) );
    }

    // write names
    std::vector<int>::const_iterator it = m_vecGlyphIndeces.begin();
    while( it != m_vecGlyphIndeces.end() )
    {
        const char*   pszName = "Test";
        unsigned char cLen    = static_cast<unsigned char>(strlen(pszName));
        pDevice->Write( reinterpret_cast<char*>(&cLen), 1 );
        pDevice->Write( pszName, cLen );

        ++it;
    }
}

// -----------------------------------------------------
// Helper functions
// -----------------------------------------------------
PdfTTFWriter::pdf_ttf_ulong PdfTTFWriter::CalculateChecksum( const pdf_ttf_ulong* pTable, pdf_ttf_ulong lLength ) const
{
    // This code is taken from the TTF specification
    pdf_ttf_ulong        lSum = 0L;
    const pdf_ttf_ulong* pEnd = pTable + ((lLength+3) & ~3) / sizeof(pdf_ttf_ulong);
    while( pTable < pEnd ) 
        lSum += *pTable++;

    return lSum;
}

void PdfTTFWriter::WriteTable( PdfOutputDevice* pDevice, TVecTableDirectoryEntries & rToc, 
                               pdf_ttf_ulong tag, void (PdfTTFWriter::*WriteTableFunc)( PdfOutputDevice* ) )
{
    if( !m_pRefBuffer ) 
    {
        // start with a 4MB buffer
        const long l4MB = 4 * 1024 * 1024; 
        m_pRefBuffer = new PdfRefCountedBuffer( l4MB );
    }

    PdfOutputDevice memDevice( m_pRefBuffer );

    TTableDirectoryEntry entry;
    entry.tag      = tag;
    entry.checkSum = 0;
    entry.length   = 0;
    entry.offset   = pDevice->Tell();

    (this->*WriteTableFunc)( &memDevice );

    // create toc entry
    entry.checkSum = this->CalculateChecksum( reinterpret_cast<pdf_ttf_ulong*>(m_pRefBuffer->GetBuffer()), memDevice.GetLength() );;
    entry.length   = memDevice.GetLength();
    rToc.push_back( entry );

    // write data to the real device
    pDevice->Write( m_pRefBuffer->GetBuffer(), memDevice.GetLength() );
}

void PdfTTFWriter::ReadSimpleGlyfCoordinates( PdfInputDevice* pDevice, const std::vector<unsigned char> & rvecFlags, 
                                              std::vector<pdf_ttf_short> & rvecCoordinates, int nFlagShort, int nFlag )
{
    pdf_ttf_short longCoordinate;
    unsigned char shortCoordinate;

    printf("~~~~~~~~~~~\n");
    printf("FLAGS: %i\n", rvecFlags.size() );
    std::vector<unsigned char>::const_iterator itFlags = rvecFlags.begin();
    while( itFlags != rvecFlags.end() )
    {
        if( (*itFlags & nFlagShort) == nFlagShort ) 
        {
            // read a 1 byte long coordinate
            pDevice->Read( reinterpret_cast<char*>(&shortCoordinate), sizeof(char) );
            printf("Got short: %i\n", shortCoordinate );
            longCoordinate = static_cast<pdf_ttf_short>(shortCoordinate);
            if( (*itFlags & nFlag) == nFlag )
                longCoordinate = -longCoordinate;
        }
        else 
        {
            // read a 2 byte long coordinate
            if( (*itFlags & nFlag) == nFlag )
            {
                // DO NOTHING
                // the value of longCoordinate is the same as the last value
                // so simply reuse the old value
            }
            else
            {
                pdf_ttf_short coordinate;
                READ_TTF_SHORT( coordinate );
                printf("Got long: %i\n", longCoordinate );
                longCoordinate += coordinate;
            }
        }

        printf("Pushing: %i\n", longCoordinate );
        rvecCoordinates.push_back( longCoordinate );
        ++itFlags;
    }
    
}

void PdfTTFWriter::WriteSimpleGlyfCoordinates( PdfOutputDevice* pDevice, const std::vector<unsigned char> & rvecFlags, 
                                               std::vector<pdf_ttf_short> & rvecCoordinates, int nFlagShort, int nFlag )
{
    pdf_ttf_short longCoordinate;
    pdf_ttf_short lastCoordinate;
    char          shortCoordinate;
    
    std::vector<unsigned char>::const_iterator    itFlags       = rvecFlags.begin();
    std::vector<pdf_ttf_short>::iterator itCoordinates = rvecCoordinates.begin(); 
    while( itFlags != rvecFlags.end() )
    {
        longCoordinate = (*itCoordinates)++;

        if( (*itFlags & nFlagShort) == nFlagShort ) 
        {
            // read a 1 byte long coordinate
            if( (*itFlags & nFlag) == nFlag )
                longCoordinate = -longCoordinate;

            shortCoordinate = static_cast<char>(longCoordinate);
            pDevice->Write( &shortCoordinate, sizeof(char) );

            lastCoordinate  = longCoordinate; // TODO: check if it is ok to assign a negative value here
        }
        else 
        {
            // read a 2 byte long coordinate
            if( (*itFlags & nFlag) == nFlag )
            {
                // DO NOTHING
                // the value of longCoordinate is the same as the last value
                // so simply reuse the old value
                longCoordinate = lastCoordinate;
            }
            else
            {
                longCoordinate = longCoordinate - lastCoordinate;
                lastCoordinate = longCoordinate + lastCoordinate;
            }
            if( podofo_is_little_endian() )
                this->SwapShort( &longCoordinate );
            pDevice->Write( reinterpret_cast<char*>(&longCoordinate), sizeof(pdf_ttf_short) );
       
        }

        ++itFlags;
    }
 
}


long PdfTTFWriter::GetGlyphDataLocation( unsigned int nIndex, long* plLength, PdfInputDevice* pDevice ) const
{
    // find the correct cmap range
    std::vector<TCMapRange>::const_iterator it = m_ranges.begin();
    // TODO: binary search!!!
    while( it != m_ranges.end() )
    {
        if( (*it).nStart <= nIndex && (*it).nEnd > nIndex ) 
        {
            // we got a position!!
            printf("Found Range for %u: %6u - %6u Delta: %6i Offset: %6u\n", nIndex, (*it).nStart, (*it).nEnd, 
                   (*it).nDelta, (*it).nOffset );
            
            if( (*it).nOffset )
            {
                const int nSegCount = format4.segCountX2 >> 1;
                
                int j = (*it).nOffset - (nSegCount - (it-m_ranges.begin())*2);
                printf("j1 = %i\n", j );
                j = (nIndex - (*it).nStart) + j/2;
                printf("j2 = %i\n", j );
                nIndex = m_vecGlyphIds[j];
                printf("nIndex=%i\n", nIndex );

                
                // 81 ??? 
                // 1314

                /*
                long lAddress = (*it).nOffset/2 + (nIndex - (*it).nStart) + (*it).nOffset;
                pDevice->Seek( lAddress + m_lCMapOffset );

                pdf_ttf_ushort glyph;
                READ_TTF_USHORT( glyph );

                printf("lAddress alone=%lx %li\n", lAddress, lAddress );
                printf("lAddress=%lx\n", lAddress + m_lCMapOffset );
                nIndex = glyph;
                printf("glyph=%u\n", glyph );
                */
                //nIndex   = 108;
                //lAddress = 0x858a;
                //nIndex = *lAddress nach Seek
            }


            nIndex = static_cast<unsigned int>( (nIndex + (*it).nDelta & 0xFFFFU ) );
            break;
        }

        ++it;
    }

    if( it == m_ranges.end() ) // go to "missing glyph" if no glyph was found
        nIndex = 0;

    printf("INDEX: %i RANGE(0 - %i)\n", nIndex, m_tLoca.size() );
    // check if the glyph index is in our current range
    if( nIndex < 0 || nIndex > m_tLoca.size() )
        return -1;

    if( nIndex + 1 < m_tLoca.size() )
        *plLength = m_tLoca[nIndex+1] - m_tLoca[nIndex];
    else
        *plLength = 0;

    printf("Reading from index: %i (max: %i) len=%i\n", nIndex, m_tLoca.size(), *plLength );


    return m_lGlyphDataOffset + m_tLoca[nIndex];
}

};

};
