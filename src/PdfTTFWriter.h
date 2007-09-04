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

#ifndef _PDF_TTF_WRITER_H_
#define _PDF_TTF_WRITER_H_

#include "PdfDefines.h"

namespace PoDoFo {

class PdfInputDevice;
class PdfOutputDevice;

namespace NonPublic {

/** An internal class which can parse a TrueType font file
 *  and write a subset of this TrueType font back to an output device.
 *
 *  This class is used internally to do font subsetting.
 *
 *  The usual way to use this class is:
 *
 *  PdfTTFWriter writer;
 *  writer.Read( [an input device] );   // read the font from a device
 *  writer.Subset();                    // do the subsetting
 *  writer.Write( [an output device] ); // write the font back to a device
 */
class PODOFO_API PdfTTFWriter {

    // Some common datatypes
    typedef pdf_uint32       pdf_ttf_fixed;
    typedef pdf_uint16       pdf_ttf_ushort;
    typedef pdf_uint32       pdf_ttf_ulong;
    typedef pdf_int16        pdf_ttf_fword;
    typedef pdf_int16        pdf_ttf_short;

    /** The table dictionary is the starting point when reading 
     *  or writing a TTF file.
     */
    struct TTableDirectory {
        pdf_ttf_fixed  sfnt_version;   ///< 0x00010000 for version 1.0
        pdf_ttf_ushort numTables;      ///< Number of tables in this file
        pdf_ttf_ushort searchRange;    ///< (Maximum power of 2 <= numTables) * 16
        pdf_ttf_ushort entrySelector;  ///< Log2( Maximum power of 2 <= numTables)
        pdf_ttf_ushort rangeShift;     ///< numTables * 16 - searchRange
    };

    struct TTableDirectoryEntry {
        pdf_ttf_ulong  tag;            ///< 4 character identifier
        pdf_ttf_ulong  checkSum;       ///< Checksum of the table
        pdf_ttf_ulong  offset;         ///< Offset from the beginning of the file
        pdf_ttf_ulong  length;         ///< Length of this table
    };

    typedef std::vector<TTableDirectoryEntry>         TVecTableDirectoryEntries;
    typedef TVecTableDirectoryEntries::iterator       TIVecTableDirectoryEntries;
    typedef TVecTableDirectoryEntries::const_iterator TCIVecTableDirectoryEntries;

    struct TTable {
        TTable() 
            : data( NULL ) 
        {
        }

        ~TTable() 
        {
            /*
            if( data )
                free( data );
            */
        }

        pdf_ttf_ulong  tag;            ///< 4 character identifier
        pdf_ttf_ulong  length;         ///< Length of this table

        char*          data;           ///< Actual table data buffer
    };

    typedef std::vector<TTable>       TVecTable;
    typedef TVecTable::iterator       TIVecTable;
    typedef TVecTable::const_iterator TCIVecTable;

    struct TMaxP {
        pdf_ttf_fixed  version;              ///< The table versions 0x00010000 for version 1.0
        pdf_ttf_ushort numGlyphs;            ///< The number of glyphs in this font
        pdf_ttf_ushort maxPoints;            ///< Maximum number of points in a non composite glyph
        pdf_ttf_ushort maxContours;          ///< Maximum number of contours in a non composite glyph
        pdf_ttf_ushort maxCompositePoints;   ///< Maximum number of points in a composite glyph
        pdf_ttf_ushort maxCompositeContours; ///< Maximum number of contours in a composite glyph
        pdf_ttf_ushort maxZones;             ///< 1 if instrutions do not use Z0 or 2 if instrutions do use Z0 (twilight zone)
        pdf_ttf_ushort maxTwilightPoints;    ///< Maximum points used in Z0
        pdf_ttf_ushort maxStorage;           ///< Maximum number of storage area locations
        pdf_ttf_ushort maxFunctionsDefs;     ///< Number of FDEF's
        pdf_ttf_ushort maxInstructionDefs;   ///< Number of IDEF's
        pdf_ttf_ushort maxStackElements;     ///< Maximum stack depth
        pdf_ttf_ushort maxSizeOfInstruction; ///< Maximum byte count for glyph instruction
        pdf_ttf_ushort maxComponentElements; ///< Maximum number of components referenced at top level for composite glyph
        pdf_ttf_ushort maxComponentDepth;    ///< Maximum level of recursions; 1 for simple components
    };

    struct THead {
        pdf_ttf_fixed  version;              ///< The table versions 0x00010000 for version 1.0
        pdf_ttf_fixed  revision;             ///< The revision set by the font manufacturer
        pdf_ttf_ulong  checkSumAdjustment;   ///< To compute: set it to 0, sum the ntire font as ULONG, then store 0xB1B0AFBA - sum
        pdf_ttf_ulong  magicNumber;          ///< Set to 0x5F0F3CF5
        pdf_ttf_ushort flags;                ///< Font flags
        pdf_ttf_ushort unitsPerEm;
        char           created[8];
        char           modified[8];
        pdf_ttf_fword  xMin;
        pdf_ttf_fword  yMin;
        pdf_ttf_fword  xMax;
        pdf_ttf_fword  yMax;
        pdf_ttf_ushort macStyle;
        pdf_ttf_ushort lowestRecPPEM;
        pdf_ttf_short  fontDirectionHint;
        pdf_ttf_short  indexToLocForm;       ///< 0 for short offsets, 1 for long offsets
        pdf_ttf_short  glyphDataFormat;      ///< 0 for current format

    };

    struct TGlyphHeader {
        pdf_ttf_short  numberOfContours;     ///< If greater or equal 0, this is a single glyph, if negative it is a composite
        pdf_ttf_fword  xMin;
        pdf_ttf_fword  yMin;
        pdf_ttf_fword  xMax;
        pdf_ttf_fword  yMax;
    };

    typedef std::vector<pdf_ttf_ulong> TVecLoca;
    typedef TVecLoca::iterator         TIVecLoca;
    typedef TVecLoca::const_iterator   TCIVecLoca;

 public:
    /** Create a PdfTTFWriter object.
     *
     */ 
    PdfTTFWriter();

    ~PdfTTFWriter();

    /** Fills the internal data structures
     *  using an existing TrueType font.
     *
     *  \param pDevice the TTF is read from this device
     */
    void Read( PdfInputDevice* pDevice );
    
    /** Do the actual subsetting of the font data
     *  TODO
     */
    void Subset();

    /** Write a TTF font from the current internal structures
     *  to an output device.
     *
     *  \param pDevice write the font to this device
     */
    void Write( PdfOutputDevice* pDevice );

 private:
    
    /** Create a tag name from four characters,
     *  so that the user readable tag can be put into
     *  TTableDirectoryEntry.
     *
     *  \returns the tag as a pdf_ttf_ulong
     */
    inline pdf_ttf_ulong CreateTag( char a, char b, char c, char d ) const;

    /** Calculate the checksum of a table.
     *
     *  The table is interpreted as a byte stream of unsigned longs
     *  and has to be padded to a multiple of 4 bytes
     *
     *  \param pTable pointer to the beginning of the table
     *  \param lLength length of the table
     *
     *  \returns the checksum of the table
     */
    pdf_ttf_ulong CalculateChecksum( const pdf_ttf_ulong* pTable, pdf_ttf_ulong lLength ) const;

    /** Convert a pdf_ttf_ushort between big and little endian
     *
     *  \param pShort a value to swap
     */
    inline void SwapUShort( pdf_ttf_ushort* pShort ) const;

    /** Convert a pdf_ttf_short between big and little endian
     *
     *  \param pShort a value to swap
     */
    inline void SwapShort( pdf_ttf_short* pShort ) const;

    /** Convert a pdf_ttf_ulong between big and little endian
     *
     *  \param pShort a value to swap
     */
    inline void SwapULong( pdf_ttf_ulong* pLong ) const;

    /** Reads the table directory from the current position
     *  of the input device, handling any necessary
     *  conversion from big to little endian.
     *
     *  \param pDevice read from the current position of this device.
     *
     *  \see m_tTableDirectory 
     */
    void ReadTableDirectory( PdfInputDevice* pDevice );

    /** Writes the table directory at the current position
     *  of the output device, handling any necessary
     *  conversion from big to little endian.
     *
     *  \param pDevice write at the current position of this device.
     *
     *  \see m_tTableDirectory 
     */
    void WriteTableDirectory( PdfOutputDevice* pDevice );

    /** Reads a table directory entry from the current position
     *  of the input device, handling any necessary
     *  conversion from big to little endian.
     *
     *  \param pDevice read from the current position of this device.
     *  \param pEntry store the result at this memory location
     */
    void ReadTableDirectoryEntry( PdfInputDevice* pDevice, TTableDirectoryEntry* pEntry );

    /** Writes a table directory entry at the current position
     *  of the output device, handling any necessary
     *  conversion from big to little endian.
     *
     *  \param pDevice write at the current position of this device.
     *  \param pEntry the entry which should be written
     */
    void WriteTableDirectoryEntry( PdfOutputDevice* pDevice, TTableDirectoryEntry* pEntry );

    /** Reads the head table from the current position of 
     *  the input device, handling any necessary conversion 
     *  from big to little endian.
     *
     *  \param pDevice read from the current position of this device.
     *
     *  \see m_tHead 
     */
    void ReadHeadTable( PdfInputDevice* pDevice );

    /** Writes the head table at the current position
     *  of the output device, handling any necessary
     *  conversion from big to little endian.
     *
     *  \param pDevice write at the current position of this device.
     *  \param rToc add the written maxp table to this table of contents
     *
     *  \see m_tHead
     */
    void WriteHeadTable( PdfOutputDevice* pDevice, TVecTableDirectoryEntries & rToc );

    /** Swap the endianess of the head table.
     *  \see m_tHead
     */
    void SwapHeadTable();
    
    /** Reads the maxp table from the current position of 
     *  the input device, handling any necessary conversion 
     *  from big to little endian.
     *
     *  \param pDevice read from the current position of this device.
     *
     *  \see m_tMaxp 
     */
    void ReadMaxpTable( PdfInputDevice* pDevice );

    /** Writes the maxp table at the current position
     *  of the output device, handling any necessary
     *  conversion from big to little endian.
     *
     *  \param pDevice write at the current position of this device.
     *  \param rToc add the written maxp table to this table of contents
     *
     *  \see m_tMaxp
     */
    void WriteMaxpTable( PdfOutputDevice* pDevice, TVecTableDirectoryEntries & rToc );

    void ReadLocaTable( PdfInputDevice* pDevice );
    void WriteLocaTable( PdfOutputDevice* pDevice, TVecTableDirectoryEntries & rToc );
    void WriteGlyfTable( PdfOutputDevice* pDevice, TVecTableDirectoryEntries & rToc );

    /** Swap the endianess of the maxp table.
     *  \see m_tMaxp
     */
    void SwapMaxpTable();

 private:

    TTableDirectory           m_tTableDirectory; ///< The TTF header

    TVecTable                 m_vecTableData;    ///< The actual data of the tables
    TMaxP                     m_tMaxp;           ///< The maximum memory requirements of this font
    THead                     m_tHead;           ///< The head table 

    TVecLoca                  m_tLoca;           ///< The loca table in long format
};


// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfTTFWriter::pdf_ttf_ulong PdfTTFWriter::CreateTag( char a, char b, char c, char d ) const
{
    return ( ( a << 24 )| ( b << 16 ) | ( c << 8 ) | d );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfTTFWriter::SwapUShort( pdf_ttf_ushort* pShort ) const
{
    *pShort = ((*pShort << 8) & 0xFF00) | ((*pShort >> 8) & 0x00FF);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfTTFWriter::SwapShort( pdf_ttf_short* pShort ) const
{
    *pShort = ((*pShort << 8) & 0xFF00) | ((*pShort >> 8) & 0x00FF);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfTTFWriter::SwapULong( pdf_ttf_ulong* pLong ) const
{
    *pLong = ((*pLong << 24) & 0xFF000000) | ((*pLong << 8) & 0x00FF0000) | 
             ((*pLong >> 8) & 0x0000FF00) | ((*pLong >> 24) & 0x000000FF) ;
}

};

};

#endif // _PDF_TTF_WRITER_H_
