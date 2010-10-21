/***************************************************************************
 *   Copyright (C) 2008 by Dominik Seichter                                *
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

#include "PdfFontTTFSubset.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfInputDevice.h"
#include "base/PdfOutputDevice.h"

#include <cstring>
#include <iostream>
#include <algorithm>

namespace PoDoFo {

static const unsigned int __LENGTH_HEADER12      = 12;
static const unsigned int __LENGTH_OFFSETTABLE16 = 16;
static const unsigned int __LENGTH_DWORD	 = 4;
static const unsigned int __LENGTH_WORD		 = 2;

//Big-endian to little-endian or little-endian to big endian.
#ifdef PODOFO_IS_LITTLE_ENDIAN

#ifdef PODOFO_IS_LITTLE_ENDIAN
inline unsigned long Big2Little(unsigned long big)
{
    return ((big << 24) & 0xFF000000) | ((big << 8) & 0x00FF0000) | 
        ((big >> 8) & 0x0000FF00) | ((big >> 24) & 0x000000FF) ;
}

inline unsigned short Big2Little(unsigned short big)
{
    return ((big << 8) & 0xFF00) | ((big >> 8) & 0x00FF);
}

inline short Big2Little(short big)
{
    return ((big << 8) & 0xFF00) | ((big >> 8) & 0x00FF);
}
#endif // PODOFO_IS_LITTLE_ENDIAN
#else
#define Big2Little( x ) x
#endif // PODOFO_IS_LITTLE_ENDIAN

//Get the number of bytes to pad the ul, because of 4-byte-alignment.
unsigned int GetPadding(unsigned long ul);	


PdfFontTTFSubset::PdfFontTTFSubset( const char* pszFontFileName, PdfFontMetrics* pMetrics, unsigned short nFaceIndex )
    : m_pMetrics( pMetrics ), m_faceIndex( nFaceIndex ), m_bOwnDevice( true )
{
    //File type is now distinguished by ext, which might cause problems.
    const char* pname = pszFontFileName;
    const char* ext   = pname + strlen(pname) - 3;

    if (PoDoFo::compat::strcasecmp(ext,"ttf") == 0)
    {
        m_eFontFileType = eFontFileType_TTF;
    }
    else if (PoDoFo::compat::strcasecmp(ext,"ttc") == 0)
    {
        m_eFontFileType = eFontFileType_TTC;
    }
    else if (PoDoFo::compat::strcasecmp(ext,"otf") == 0)
    {
        m_eFontFileType = eFontFileType_OTF;
    }
    else
    {
        m_eFontFileType = eFontFileType_Unknown;
    }

    m_pDevice = new PdfInputDevice( pszFontFileName );

    // For any fonts, assume that glyph 0 is needed.
    m_vGlyphIndice.push_back(0);	
}

PdfFontTTFSubset::PdfFontTTFSubset( PdfInputDevice* pDevice, PdfFontMetrics* pMetrics, EFontFileType eType, unsigned short nFaceIndex )
    : m_pMetrics( pMetrics ), m_eFontFileType( eType ), m_faceIndex(nFaceIndex),
      m_pDevice( pDevice ), m_bOwnDevice( false )
{
    // For any fonts, assume that glyph 0 is needed.
    m_vGlyphIndice.push_back(0);	
}

PdfFontTTFSubset::~PdfFontTTFSubset()
{
    if( m_bOwnDevice ) {
        delete m_pDevice;
        m_pDevice = NULL;
    }
}

void PdfFontTTFSubset::AddGlyph( unsigned short nGlyphIndex )
{
    // Do a sorted insert
    std::pair<std::vector<unsigned short>::iterator,
        std::vector<unsigned short>::const_iterator> it =
        std::equal_range( m_vGlyphIndice.begin(), m_vGlyphIndice.end(), 
                          nGlyphIndex );
    
if( it.first == it.second )
{
    m_vGlyphIndice.insert( it.first, nGlyphIndex );
}
}

    void PdfFontTTFSubset::Init()
{
    GetStartOfTTFOffsets();
    GetNumberOfTables();
    InitTables();
    GetNumberOfGlyphs();
    SeeIfLongLocaOrNot();
}

unsigned long PdfFontTTFSubset::GetTableOffset( const char* pszTableName )
{
    std::vector<TTrueTypeTable>::const_iterator it = m_vTable.begin();

    for (; it != m_vTable.end(); it++)
    {
        if (it->m_strTableName == pszTableName)
            return it->m_offset;
    }

    return 0L;
}

void PdfFontTTFSubset::GetNumberOfGlyphs()
{
    unsigned long ulMaxpOffset = GetTableOffset( "maxp" );

    GetData( ulMaxpOffset+__LENGTH_DWORD*1,&m_numGlyphs,__LENGTH_DWORD);
    m_numGlyphs = Big2Little(m_numGlyphs);

    std::cout << "Number of Glyphs:	"<< m_numGlyphs << std::endl;
}

void PdfFontTTFSubset::InitTables()
{
    for (int i=0; i < m_numTables; i++)
    {
        TTrueTypeTable tbl;

        //Name of each table:
        GetData( m_ulStartOfTTFOffsets+__LENGTH_HEADER12+__LENGTH_OFFSETTABLE16*i, tbl.m_tableName, __LENGTH_DWORD );

        //String name of each table:
        tbl.m_strTableName.assign( reinterpret_cast<char*>(tbl.m_tableName), __LENGTH_DWORD );

        //Checksum of each table:
        GetData( m_ulStartOfTTFOffsets+__LENGTH_HEADER12+__LENGTH_OFFSETTABLE16*i+__LENGTH_DWORD*1,&tbl.m_checksum,__LENGTH_DWORD);
        tbl.m_checksum = Big2Little(tbl.m_checksum);

        //Offset of each table:
        GetData( m_ulStartOfTTFOffsets+__LENGTH_HEADER12+__LENGTH_OFFSETTABLE16*i+__LENGTH_DWORD*2,&tbl.m_offset,__LENGTH_DWORD);
        tbl.m_offset = Big2Little(tbl.m_offset);

        //Length of each table:
        GetData( m_ulStartOfTTFOffsets+__LENGTH_HEADER12+__LENGTH_OFFSETTABLE16*i+__LENGTH_DWORD*3,&tbl.m_length,__LENGTH_DWORD);
        tbl.m_length = Big2Little(tbl.m_length);

        //It seems that the EBDT, EBLC, EBSC table can be erased.
        if (tbl.m_strTableName == "EBDT")
            /*||tbl.m_strTableName == string("EBLC")||tbl.m_strTableName == string("EBSC")*/
        {
            continue;
        }

        m_vTable.push_back(tbl);		

    }
    m_numTables = static_cast<unsigned short>(m_vTable.size());
}

void PdfFontTTFSubset::GetStartOfTTFOffsets()
{
	
    switch (m_eFontFileType)
    {
        case eFontFileType_TTF:
        case eFontFileType_OTF:
            m_ulStartOfTTFOffsets = 0x0;
            break;
        case eFontFileType_TTC:
        {
            unsigned long ulnumFace;
            GetData( 8,&ulnumFace,4);
            ulnumFace = Big2Little(ulnumFace);
	    
            GetData( (3+m_faceIndex)*__LENGTH_DWORD,&m_ulStartOfTTFOffsets,__LENGTH_DWORD);
            m_ulStartOfTTFOffsets = Big2Little(m_ulStartOfTTFOffsets);
        }
        break;
        case eFontFileType_Unknown:
            break;
        default:
            return;
    }
}

void PdfFontTTFSubset::GetNumberOfTables()
{
    GetData( m_ulStartOfTTFOffsets+1*__LENGTH_DWORD,&m_numTables,__LENGTH_WORD);
    m_numTables = Big2Little(m_numTables);
}

void PdfFontTTFSubset::SeeIfLongLocaOrNot()
{
    unsigned short usIsLong; //1 for long
    unsigned long ulHeadOffset = GetTableOffset("head");
    GetData( ulHeadOffset+50,&usIsLong,__LENGTH_WORD);
    usIsLong = Big2Little(usIsLong);
	m_bIsLongLoca = (usIsLong == 0 ? false : true);
}

void PdfFontTTFSubset::BuildFont( PdfOutputDevice* pOutputDevice )
{
    Init();
	
    // Not necessary as we do a sorted insert
    //std::sort(m_vGlyphIndice.begin(),m_vGlyphIndice.end());
	
    //Find the font offset table:
    unsigned long ulStartOfTTFOffsets = m_ulStartOfTTFOffsets;

    // Deal with glyph indeces which are
    // not part of this font. Remove any
    // glyph that cannot be embedded therefore
    std::vector<unsigned short>::iterator itGlyphIndice = m_vGlyphIndice.begin();
    for (; itGlyphIndice != m_vGlyphIndice.end();)
    {
        if (*itGlyphIndice > m_numGlyphs)
        {
            itGlyphIndice = m_vGlyphIndice.erase(itGlyphIndice);
        }
        else
        {
            itGlyphIndice++;
        }
    }

    //==============================================================================
    // Make a new font:
	
    // Make the glyf table be the last table, just for convenience:
	long i;
    for (i = 0; i < m_numTables; i++)
    {
        if (m_vTable[i].m_strTableName == "glyf")
        {
            TTrueTypeTable tmpTbl = m_vTable.back();
            m_vTable.back() = m_vTable[i];
            m_vTable[i] = tmpTbl;
            break;
        }
    }

    unsigned long ulGlyfTableOffset = GetTableOffset("glyf");
    unsigned long ulLocaTableOffset = GetTableOffset("loca");

    std::vector<TTrueTypeTable> vOldTable = m_vTable;	//vOldTable will be used later.

    //Print the old table information:
    for ( i = 0; i < m_numTables; i++)
    {
        std::cout << "OldTable:\t" << m_vTable[i].m_strTableName << std::endl;
        std::cout << "\tOffSet:\t" << m_vTable[i].m_offset << std::endl;
        std::cout << "\t\tLength:\t"<<m_vTable[i].m_length << std::endl;
    }
	
    //Change the offsets:
    m_vTable[0].m_offset = __LENGTH_HEADER12+__LENGTH_OFFSETTABLE16*m_numTables;
    for ( i = 1; i < m_numTables; i++)
    {
        m_vTable[i].m_offset = m_vTable[i-1].m_offset+m_vTable[i-1].m_length;
        unsigned pad = GetPadding(m_vTable[i-1].m_length);
        m_vTable[i].m_offset += pad;
    }
	
    //Print the new table information:
    for ( i = 0; i < m_numTables; i++)
    {
        std::cout << "NewTable:\t" << m_vTable[i].m_strTableName <<std::endl;
        std::cout << "\tOffSet:\t" << m_vTable[i].m_offset <<std::endl;
        std::cout << "\t\tLength:\t"<<m_vTable[i].m_length <<std::endl;
    }

    //The glyph data:
    if (m_bIsLongLoca)
    {
        std::vector<TGlyphData> vGD;
        TGlyphData gd;
        unsigned long ulNextGlyphAddress;
	
        //Deal with the composite glyphs:
        //Sometimes glyph A will use glyph B, so glyph B must be embeded also, but whether glyph B is using another glyph has not been taken into account.
        std::vector<unsigned short> vGlyphIndice = m_vGlyphIndice;		//Temp vGlyphIndice.
        for ( i = 0; i < static_cast<long>(vGlyphIndice.size()); i++)
        {
            gd.glyphIndex = vGlyphIndice[i];
            GetData(  ulLocaTableOffset+__LENGTH_DWORD*gd.glyphIndex,&gd.glyphOldAddress,__LENGTH_DWORD);
            gd.glyphOldAddress = Big2Little(gd.glyphOldAddress);
		
            short sIsComp;
            GetData(  ulGlyfTableOffset+gd.glyphOldAddress,&sIsComp,__LENGTH_WORD);
            sIsComp = Big2Little(sIsComp);
            if (sIsComp >= 0)
            {
                continue;
            }
            else
            {
                unsigned long lOffset = ulGlyfTableOffset+gd.glyphOldAddress+10;
                const int ARG_1_AND_2_ARE_WORDS = 0x01;
                const int MORE_COMPONENTS       = 0x20;
                unsigned short flags;
                unsigned short glyphIndex;
                do {
                    GetData(  lOffset, &flags,__LENGTH_WORD);
                    lOffset += __LENGTH_WORD;
                    GetData(  lOffset, &glyphIndex,__LENGTH_WORD);
                    lOffset += __LENGTH_WORD;
                    
                    flags = Big2Little( flags );
                    if( flags & ARG_1_AND_2_ARE_WORDS) 
                    {
                        // Skip two args
                        lOffset += (2*__LENGTH_WORD);
                    }
                    else 
                    {
                        // Skip one arg
                        lOffset += __LENGTH_WORD;
                    }
                    
                    glyphIndex = Big2Little( glyphIndex );
                    this->AddGlyph( glyphIndex );
                } while ( flags & MORE_COMPONENTS );
                /*
                unsigned short usNewGlyphIndex;
                GetData(  ulGlyfTableOffset+gd.glyphOldAddress+12,&usNewGlyphIndex,__LENGTH_WORD);
                usNewGlyphIndex = Big2Little(usNewGlyphIndex);

                this->AddGlyph( usNewGlyphIndex );
                */
            }
        }

	
        // Let us assume that the first glyphindex in the vector is always 0:
        for ( i = 0; i < static_cast<long>(m_vGlyphIndice.size()); i++)
        {
            gd.glyphIndex = m_vGlyphIndice[i];
            GetData( ulLocaTableOffset+__LENGTH_DWORD*gd.glyphIndex,&gd.glyphOldAddress,__LENGTH_DWORD);
            gd.glyphOldAddress = Big2Little(gd.glyphOldAddress);
            GetData( ulLocaTableOffset+__LENGTH_DWORD*(gd.glyphIndex+1),&ulNextGlyphAddress,__LENGTH_DWORD);
            ulNextGlyphAddress = Big2Little(ulNextGlyphAddress);
            gd.glyphLength = ulNextGlyphAddress-gd.glyphOldAddress;
            vGD.push_back(gd);
        }
	
        //The last glyph, whose index seems not needed.
        ulNextGlyphAddress = m_vTable.back().m_length;
        GetData( ulLocaTableOffset+(m_numGlyphs)*__LENGTH_DWORD,&gd.glyphOldAddress,__LENGTH_DWORD);	//?
        gd.glyphOldAddress = Big2Little(gd.glyphOldAddress);
        gd.glyphLength = ulNextGlyphAddress-gd.glyphOldAddress;
        vGD.push_back(gd);
        vGD[0].glyphNewAddress = 0x0;
        for ( i = 1; i < static_cast<long>(vGD.size()); i++)
        {
            vGD[i].glyphNewAddress = vGD[i-1].glyphNewAddress+vGD[i-1].glyphLength;
        }
	
        //New glyf table length:
        unsigned long ulNewGlyfTableLength = vGD.back().glyphNewAddress+vGD.back().glyphLength;
        m_vTable.back().m_length = ulNewGlyfTableLength;
	
        //Writing the header:
        unsigned char* buf = new unsigned char[3*__LENGTH_DWORD];
        GetData( ulStartOfTTFOffsets,buf,3*__LENGTH_DWORD);
        pOutputDevice->Write( reinterpret_cast<char*>(buf), 3*__LENGTH_DWORD );
        delete[]  buf;
        buf = new unsigned char[m_numTables*__LENGTH_DWORD*4];
	
        for ( i = 0; i < m_numTables; i++)
        {
            //The table name:
            for (int j(0); j < static_cast<int>(__LENGTH_DWORD); j++)
            {
                buf[i*__LENGTH_DWORD*4 + j] = m_vTable[i].m_tableName[j]; 
            }
            //The checksum:
            unsigned long ultemp = Big2Little(m_vTable[i].m_checksum);
            for (int k(0); k < static_cast<int>(__LENGTH_DWORD); k++)
            {
                unsigned char ch = static_cast<unsigned char>(ultemp%256);
                buf[i*__LENGTH_DWORD*4 + 4 + k] = ch;
                ultemp /= 256;
            }
            //The offset:
            ultemp = Big2Little(m_vTable[i].m_offset);
            for (int m(0); m < static_cast<int>(__LENGTH_DWORD); m++)
            {
                unsigned char ch = static_cast<unsigned char>(ultemp%256);
                buf[i*__LENGTH_DWORD*4 + 8 + m] = ch;
                ultemp /= 256;
            }
            //The length:
            ultemp = Big2Little(m_vTable[i].m_length);
            for (int n(0); n < static_cast<int>(__LENGTH_DWORD); n++)
            {
                unsigned char ch = static_cast<unsigned char>(ultemp%256);
                buf[i*__LENGTH_DWORD*4 + 12 + n] = ch;
                ultemp /= 256;
            }
        }
        pOutputDevice->Write( reinterpret_cast<char*>(buf), m_numTables*__LENGTH_DWORD*4 );
        delete[] buf;
	
        //Writing new tables:
        vGD.back().glyphIndex = m_numGlyphs;
        for ( i = 0; i < m_numTables-1; i++)
        {
            if (m_vTable[i].m_strTableName != "loca")
            {
                //string str = m_vTable[i].m_strTableName;
                //unsigned long length = m_vTable[i].m_length;
                //unsigned long offset = m_vTable[i].m_offset;
                //unsigned long x = m_vTable[i].m_length;
                char *buf = new  char[m_vTable[i].m_length];
                //unsigned long olength = vOldTable[i].m_length;
                //unsigned long ooffset = vOldTable[i].m_offset;
                GetData( vOldTable[i].m_offset,buf,vOldTable[i].m_length);
                pOutputDevice->Write( reinterpret_cast<char*>(buf), vOldTable[i].m_length );
                delete[] buf;
                buf = 0;
            }
            else
            {
                //The loca table:
                for (unsigned long j(0); j < vGD.size()-1; j++)
                {
                    unsigned long ultemp = vGD[j].glyphNewAddress;
                    ultemp = Big2Little(ultemp);
                    buf = new unsigned char[__LENGTH_DWORD];
                    for (int k(0); k < static_cast<int>(__LENGTH_DWORD); k++)
                    {
                        unsigned char ch = static_cast<unsigned char>(ultemp%256);
                        buf[k] = ch;
                        ultemp /= 256;
                    }
                    pOutputDevice->Write( reinterpret_cast<char*>(buf), __LENGTH_DWORD );
                    ultemp = vGD[j+1].glyphNewAddress;
                    ultemp = Big2Little(ultemp);
                    for (int n(0); n < static_cast<int>(__LENGTH_DWORD); n++)
                    {
                        unsigned char ch = static_cast<unsigned char>(ultemp%256);
                        buf[n] = ch;
                        ultemp /= 256;
                    }
                    for (unsigned long m(vGD[j].glyphIndex+1); m < vGD[j+1].glyphIndex; m++)
                    {
                        pOutputDevice->Write( reinterpret_cast<char*>(buf), __LENGTH_DWORD );
                    }
                    delete[] buf;
                }
                //The last glyph address:
		
                buf = new unsigned char[__LENGTH_DWORD];
                unsigned long ultemp = vGD.back().glyphNewAddress;
                ultemp = Big2Little(ultemp);
                for (int n(0); n < static_cast<int>(__LENGTH_DWORD); n++)
                {
                    unsigned char ch = static_cast<unsigned char>(ultemp%256);
                    buf[n] = ch;
                    ultemp /= 256;
                }
                pOutputDevice->Write( reinterpret_cast<char*>(buf), __LENGTH_DWORD );
                delete[] buf;				
            }
            //Paddings:
            unsigned pad = GetPadding(m_vTable[i].m_length);
            if (pad != 0)
            {
                buf = new unsigned char[pad];
                pOutputDevice->Write( reinterpret_cast<char*>(buf), pad );
                delete[] buf;
            }
	    
        }
        //Writing the last table, glyf:
        for ( i = 0; i < static_cast<long>(vGD.size()); i++)
        {
            buf = new unsigned char[vGD[i].glyphLength];
            GetData( ulGlyfTableOffset+vGD[i].glyphOldAddress,buf,vGD[i].glyphLength);
            pOutputDevice->Write( reinterpret_cast<char*>(buf), vGD[i].glyphLength );
            delete[] buf;
        }
    }
    else
    {
        std::vector<TGlyphDataShort> vsGD;
        TGlyphDataShort gd;
        unsigned short usNextGlyphAddress;
        unsigned long  usNextGlyphAddressLong;
	
        //Deal with the composite glyphs:
        //Temp vGlyphIndice:
        std::vector<unsigned short> vGlyphIndice = m_vGlyphIndice;
        for ( i = 0; i < static_cast<long>(vGlyphIndice.size()); i++)
        {
            gd.glyphIndex = vGlyphIndice[i];
            GetData(  ulLocaTableOffset+__LENGTH_WORD*gd.glyphIndex,&gd.glyphOldAddress,__LENGTH_WORD);
            gd.glyphOldAddress = Big2Little(gd.glyphOldAddress);
            gd.glyphOldAddressLong = gd.glyphOldAddress*2;
            short sIsComp;
            GetData(  ulGlyfTableOffset+gd.glyphOldAddressLong,&sIsComp,__LENGTH_WORD);
            sIsComp = Big2Little(sIsComp);
            if (sIsComp >= 0)
            {
                continue;
            }
            else
            {
                unsigned short usNewGlyphIndex;
                GetData(  ulGlyfTableOffset+gd.glyphOldAddressLong+12,&usNewGlyphIndex,__LENGTH_WORD);
                usNewGlyphIndex = Big2Little(usNewGlyphIndex);

                this->AddGlyph( usNewGlyphIndex );
            }
        }

        //Let us assume that the first glyphindex in the vector is always 0:
        for ( i = 0; i < static_cast<long>(m_vGlyphIndice.size()); i++)
        {
            gd.glyphIndex = m_vGlyphIndice[i];
            GetData( ulLocaTableOffset+__LENGTH_WORD*gd.glyphIndex,&gd.glyphOldAddress,__LENGTH_WORD);
            gd.glyphOldAddress = Big2Little(gd.glyphOldAddress);
            gd.glyphOldAddressLong = gd.glyphOldAddress*2;
            GetData( ulLocaTableOffset+__LENGTH_WORD*(gd.glyphIndex+1),&usNextGlyphAddress,__LENGTH_WORD);
            usNextGlyphAddress = Big2Little(usNextGlyphAddress);
            usNextGlyphAddressLong = usNextGlyphAddress*2;
            gd.glyphLength = static_cast<unsigned short>(usNextGlyphAddressLong-gd.glyphOldAddressLong);
            vsGD.push_back(gd);
        }
	
        //The last glyph, glyph index seems not needed.
        usNextGlyphAddressLong = m_vTable.back().m_length;
        GetData( ulLocaTableOffset+(m_numGlyphs)*__LENGTH_WORD,&gd.glyphOldAddress,__LENGTH_WORD);	//?
        gd.glyphOldAddress = Big2Little(gd.glyphOldAddress);
        gd.glyphOldAddressLong = gd.glyphOldAddress*2;
        gd.glyphLength = static_cast<unsigned short>(usNextGlyphAddressLong-gd.glyphOldAddressLong);
        vsGD.push_back(gd);
        vsGD[0].glyphNewAddress = 0x0;
        vsGD[0].glyphNewAddressLong = 0x0;
        for ( i = 1; i < static_cast<long>(vsGD.size()); i++)
        {
            vsGD[i].glyphNewAddressLong = vsGD[i-1].glyphNewAddressLong+vsGD[i-1].glyphLength;
            vsGD[i].glyphNewAddress = static_cast<unsigned short>(vsGD[i].glyphNewAddressLong/2);
        }
	
        //New glyf table length:
        unsigned long usNewGlyfTableLength = vsGD.back().glyphNewAddressLong+vsGD.back().glyphLength;
        m_vTable.back().m_length = usNewGlyfTableLength;
        //Writing the header:
        unsigned char* buf = new unsigned char[3*__LENGTH_DWORD];
        GetData( ulStartOfTTFOffsets,buf,3*__LENGTH_DWORD);
        pOutputDevice->Write( reinterpret_cast<char*>(buf), 3*__LENGTH_DWORD );
        delete[]  buf;
        buf = new unsigned char[m_numTables*__LENGTH_DWORD*4];
        for ( i = 0; i < m_numTables; i++)
        {
            //The table name:
            for (int j(0); j < static_cast<int>(__LENGTH_DWORD); j++)
            {
                buf[i*__LENGTH_DWORD*4 + j] = m_vTable[i].m_tableName[j]; 
            }
            //The checksum:
            unsigned long ultemp = Big2Little(m_vTable[i].m_checksum);
            for (int k(0); k < static_cast<int>(__LENGTH_DWORD); k++)
            {
                unsigned char ch = static_cast<unsigned char>(ultemp%256);
                buf[i*__LENGTH_DWORD*4 + 4 + k] = ch;
                ultemp /= 256;
            }
            //The offset:
            ultemp = Big2Little(m_vTable[i].m_offset);
            for (int m(0); m < static_cast<int>(__LENGTH_DWORD); m++)
            {
                unsigned char ch = static_cast<unsigned char>(ultemp%256);
                buf[i*__LENGTH_DWORD*4 + 8 + m] = ch;
                ultemp /= 256;
            }
            //The length:
            ultemp = Big2Little(m_vTable[i].m_length);
            for (int n(0); n < static_cast<int>(__LENGTH_DWORD); n++)
            {
                unsigned char ch = static_cast<unsigned char>(ultemp%256);
                buf[i*__LENGTH_DWORD*4 + 12 + n] = ch;
                ultemp /= 256;
            }
        }
        pOutputDevice->Write( reinterpret_cast<char*>(buf), m_numTables*__LENGTH_DWORD*4 );
        delete[] buf;
        //Writing new tables:
        vsGD.back().glyphIndex = m_numGlyphs;
        for ( i = 0; i < m_numTables-1; i++)
        {
            if (m_vTable[i].m_strTableName != "loca")
            {
                //string str = m_vTable[i].m_strTableName;
                //unsigned long length = m_vTable[i].m_length;
                //unsigned long offset = m_vTable[i].m_offset;
                char *buf = new  char[m_vTable[i].m_length];
                //unsigned long olength = vOldTable[i].m_length;
                //unsigned long ooffset = vOldTable[i].m_offset;
                GetData( vOldTable[i].m_offset,buf,vOldTable[i].m_length);
                pOutputDevice->Write( reinterpret_cast<char*>(buf), vOldTable[i].m_length );
                delete[] buf;
                buf = 0;
            }
            else
            {
                //The loca table:
                for (unsigned long j(0); j < vsGD.size()-1; j++)
                {
                    unsigned short ustemp = vsGD[j].glyphNewAddress;
                    ustemp = Big2Little(ustemp);
                    buf = new unsigned char[__LENGTH_WORD];
                    for (int k(0); k < static_cast<int>(__LENGTH_WORD); k++)
                    {
                        unsigned char ch = ustemp%256;
                        buf[k] = ch;
                        ustemp /= 256;
                    }
                    pOutputDevice->Write( reinterpret_cast<char*>(buf), __LENGTH_WORD );
                    ustemp = vsGD[j+1].glyphNewAddress;
                    ustemp = Big2Little(ustemp);
                    for (int n(0); n < static_cast<int>(__LENGTH_WORD); n++)
                    {
                        unsigned char ch = ustemp%256;
                        buf[n] = ch;
                        ustemp /= 256;
                    }
                    for (unsigned long m(vsGD[j].glyphIndex+1); m < vsGD[j+1].glyphIndex; m++)
                    {
                        pOutputDevice->Write( reinterpret_cast<char*>(buf), __LENGTH_WORD );
                    }
                    delete[] buf;
                }
                //The last glyph address:
		
                buf = new unsigned char[__LENGTH_WORD];
                unsigned short ustemp = vsGD.back().glyphNewAddress;
                ustemp = Big2Little(ustemp);
                for (int n(0); n < static_cast<int>(__LENGTH_WORD); n++)
                {
                    unsigned char ch = ustemp%256;
                    buf[n] = ch;
                    ustemp /= 256;
                }
                //vsGD.back().glyphLength = 0;
                pOutputDevice->Write( reinterpret_cast<char*>(buf), __LENGTH_WORD );
		
                delete[] buf;
            }
            //Paddings:
            unsigned pad = GetPadding(m_vTable[i].m_length);
            if (pad != 0)
            {
                buf = new unsigned char[pad];
                pOutputDevice->Write( reinterpret_cast<char*>(buf), pad );
                delete[] buf;
            }
	    
        }
        //Writing the last table, glyf:
        for ( i = 0; i < static_cast<long>(vsGD.size()); i++)
        {
            buf = new unsigned char[vsGD[i].glyphLength];
            GetData( ulGlyfTableOffset+vsGD[i].glyphOldAddressLong,buf,vsGD[i].glyphLength);
            pOutputDevice->Write( reinterpret_cast<char*>(buf), vsGD[i].glyphLength );
            delete[] buf;
        }
    }
}

void PdfFontTTFSubset::GetData(unsigned long offset, void* address, unsigned long sz)
{
    m_pDevice->Seek( offset );
    m_pDevice->Read( static_cast<char*>(address), sz );
}


// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned int GetPadding(unsigned long ul)
{
    ul %= 4;
    if (ul != 0)
    {
        ul = 4-ul;
    }
    
    return ul;
}


}; /* PoDoFo */

