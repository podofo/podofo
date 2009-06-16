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

#include "PdfEncoding.h"

#include "PdfDictionary.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfLocale.h"
#include "util/PdfMutexWrapper.h"

#include <stdlib.h>
#include <string.h>
#include <sstream>

namespace PoDoFo {

PdfEncoding::PdfEncoding( int nFirstChar, int nLastChar )
    : m_nFirstChar( nFirstChar ), m_nLastChar( nLastChar )
{
    if( !(m_nFirstChar < m_nLastChar) )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_ValueOutOfRange, "PdfEncoding: nFirstChar must be smaller than nLastChar" ); 
    }
}

PdfEncoding::~PdfEncoding()
{

}

// -----------------------------------------------------
// PdfSimpleEncoding
// -----------------------------------------------------
PdfSimpleEncoding::PdfSimpleEncoding( const PdfName & rName )
    : PdfEncoding( 0, 255 ), m_name( rName ), m_pEncodingTable( NULL )
{
}

PdfSimpleEncoding::~PdfSimpleEncoding() 
{
    free( m_pEncodingTable );
}

void PdfSimpleEncoding::InitEncodingTable() 
{
    Util::PdfMutexWrapper wrapper( m_mutex );
    const long         lTableLength     = 0xffff;
    const pdf_utf16be* cpUnicodeTable   = this->GetToUnicodeTable();

    if( !m_pEncodingTable ) // double check
    {
        m_pEncodingTable = static_cast<char*>(malloc(sizeof(char)*lTableLength));
    
        // fill the table with 0
        memset( m_pEncodingTable, 0, lTableLength * sizeof(char) ); 
        // fill the table with data
        for( int i=0;i<256;i++ )
            m_pEncodingTable[ cpUnicodeTable[i] ] = i;
    }
}

void PdfSimpleEncoding::AddToDictionary( PdfDictionary & rDictionary ) const
{
    rDictionary.AddKey( PdfName("Encoding"), m_name );
}

pdf_utf16be PdfSimpleEncoding::GetCharCode( int nIndex ) const
{
    if( nIndex < this->GetFirstChar() ||
        nIndex > this->GetLastChar() )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    const pdf_utf16be* cpUnicodeTable   = this->GetToUnicodeTable();

#ifdef PODOFO_IS_LITTLE_ENDIAN
    return ((cpUnicodeTable[nIndex] & 0xff00) >> 8) | ((cpUnicodeTable[nIndex] & 0xff) << 8);
#else
    return cpUnicodeTable[nIndex];
#endif // PODOFO_IS_LITTLE_ENDIAN

}

PdfString PdfSimpleEncoding::ConvertToUnicode( const PdfString & rEncodedString, const PdfFont* ) const
{
    const pdf_utf16be* cpUnicodeTable = this->GetToUnicodeTable();
    pdf_long           lLen           = rEncodedString.GetLength();

    if( !lLen )
        return PdfString("");

    pdf_utf16be* pszStringUtf16 = static_cast<pdf_utf16be*>(malloc(sizeof(pdf_utf16be) * (lLen + 1)) );
    if( !pszStringUtf16 ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }

    const char* pszString = rEncodedString.GetString();
    for( int i=0;i<lLen;i++ )
    {
#ifdef PODOFO_IS_BIG_ENDIAN
        pszStringUtf16[i] = cpUnicodeTable[ static_cast<unsigned char>(*pszString) ];
#else
        pszStringUtf16[i] =             
            ((( cpUnicodeTable[ static_cast<unsigned char>(*pszString) ] << 8 ) & 0xff00) | 
             (( cpUnicodeTable[ static_cast<unsigned char>(*pszString) ] >> 8 ) & 0x00ff));
#endif // PODOFO_IS_BIG_ENDIAN
        ++pszString;
    }

    pszStringUtf16[lLen] = 0;

    PdfString sStr( pszStringUtf16 );
    free( pszStringUtf16 );
    
    return sStr;
}

PdfString PdfSimpleEncoding::ConvertToEncoding( const PdfString & rString, const PdfFont* ) const
{
    if( !m_pEncodingTable )
        const_cast<PdfSimpleEncoding*>(this)->InitEncodingTable();

    PdfString sSrc = rString.ToUnicode(); // make sure the string is unicode and not PdfDocEncoding!
    pdf_long  lLen = sSrc.GetCharacterLength();

    if( !lLen )
        return PdfString("");

    char* pDest = static_cast<char*>(malloc( sizeof(char) * (lLen + 1) ));
    if( !pDest ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }
        
    const pdf_utf16be* pszUtf16 = sSrc.GetUnicode();
    char*              pCur     = pDest;

    for( int i=0;i<lLen;i++ ) 
    {
        pdf_utf16be val = pszUtf16[i];
#ifdef PODOFO_IS_LITTLE_ENDIAN
        val = ((val & 0xff00) >> 8) | ((val & 0xff) << 8);
#endif // PODOFO_IS_LITTLE_ENDIAN

        *pCur = m_pEncodingTable[val]; 

        if( *pCur ) // ignore 0 characters, as they cannot be converted to the current encoding
            ++pCur; 
    }
    
    *pCur = '\0';
    
    PdfString sDest( pDest ); // fake a PdfDocEncoding string .... would be more clear if we return a buffer
    free( pDest );

    return sDest;
}

// -----------------------------------------------------
// PdfDocEncoding
// -----------------------------------------------------

// -----------------------------------------------------
// 
// -----------------------------------------------------
const pdf_utf16be* PdfDocEncoding::GetToUnicodeTable() const
{
    return PdfDocEncoding::s_cEncoding;
}

const pdf_utf16be PdfDocEncoding::s_cEncoding[256] = {
    0x0000,
    0x0001,
    0x0002,
    0x0003,
    0x0004,
    0x0005,
    0x0006,
    0x0007,
    0x0008,
    0x0009,
    0x000A,
    0x000B,
    0x000C,
    0x000D,
    0x000E,
    0x000F,
    0x0010,
    0x0011,
    0x0012,
    0x0013,
    0x0014,
    0x0015,
    0x0017,
    0x0017,
    0x02D8,
    0x02C7, // dec 25
    0x02C6,
    0x02D9,
    0x02DD,
    0x02DB,
    0x02DA,
    0x02DC,
    0x0020,
    0x0021,
    0x0022,
    0x0023,
    0x0024,
    0x0025,
    0x0026,
    0x0027,
    0x0028,
    0x0029,
    0x002A,
    0x002B,
    0x002C,
    0x002D,
    0x002E,
    0x002F,
    0x0030,
    0x0031,
    0x0032,
    0x0033,
    0x0034,
    0x0035,
    0x0036,
    0x0037,
    0x0038,
    0x0039, // dec 57 
    0x003A,
    0x003B,
    0x003C,
    0x003D,
    0x003E,
    0x003F,
    0x0040,
    0x0041,
    0x0042,
    0x0043,
    0x0044,
    0x0045,
    0x0046,
    0x0047,
    0x0048,
    0x0049,
    0x004A,
    0x004B,
    0x004C,
    0x004D,
    0x004E,
    0x004F,
    0x0050,
    0x0051,
    0x0052,
    0x0053,
    0x0054,
    0x0055,
    0x0056,
    0x0057,
    0x0058,
    0x0059, // 89
    0x005A,
    0x005B,
    0x005C,
    0x005D,
    0x005E,
    0x005F,
    0x0060,
    0x0061,
    0x0062,
    0x0063,
    0x0064,
    0x0065,
    0x0066,
    0x0067,
    0x0068,
    0x0069,
    0x006A,
    0x006B,
    0x006C,
    0x006D,
    0x006E,
    0x006F,
    0x0070,
    0x0071,
    0x0072,
    0x0073,
    0x0074,
    0x0075,
    0x0076,
    0x0077,
    0x0078,
    0x0079, //121 
    0x007A,
    0x007B,
    0x007C,
    0x007D,
    0x007E,
    0x0000, // Undefined
    0x2022,
    0x2020,
    0x2021,
    0x2026,
    0x2014,
    0x2013,
    0x0192,
    0x2044,
    0x2039,
    0x203A,
    0x2212,
    0x2030,
    0x201E,
    0x201C,
    0x201D,
    0x2018,
    0x2019,
    0x201A,
    0x2122,
    0xFB01, // dec147 
    0xFB02,
    0x0141,
    0x0152,
    0x0160,
    0x0178,
    0x017D,
    0x0131,
    0x0142,
    0x0153,
    0x0161,
    0x017E,
    0x0000, // Undefined
    0x20AC, // Euro
    0x00A1,
    0x00A2,
    0x00A3,
    0x00A4,
    0x00A5,
    0x00A6,
    0x00A7,
    0x00A8,
    0x00A9,
    0x00AA,
    0x00AB,
    0x00AC,
    0x0000, // Undefined
    0x00AE,
    0x00AF,
    0x00B0,
    0x00B1,
    0x00B2,
    0x00B3,
    0x00B4,
    0x00B5,
    0x00B6,
    0x00B7,
    0x00B8,
    0x00B9,
    0x00BA,
    0x00BB,
    0x00BC,
    0x00BD,
    0x00BE,
    0x00BF,
    0x00C0,
    0x00C1,
    0x00C2,
    0x00C3,
    0x00C4,
    0x00C5,
    0x00C6,
    0x00C7,
    0x00C8,
    0x00C9,
    0x00CA,
    0x00CB,
    0x00CC,
    0x00CD,
    0x00CE,
    0x00CF,
    0x00D0,
    0x00D1,
    0x00D2,
    0x00D3,
    0x00D4,
    0x00D5,
    0x00D6,
    0x00D7,
    0x00D8,
    0x00D9,
    0x00DA,
    0x00DB,
    0x00DC,
    0x00DD,
    0x00DE,
    0x00DF,
    0x00E0,
    0x00E1,
    0x00E2,
    0x00E3,
    0x00E4,
    0x00E5,
    0x00E6,
    0x00E7,
    0x00E8,
    0x00E9,
    0x00EA,
    0x00EB,
    0x00EC,
    0x00ED,
    0x00EE,
    0x00EF,
    0x00F0,
    0x00F1,
    0x00F2,
    0x00F3,
    0x00F4,
    0x00F5,
    0x00F6,
    0x00F7,
    0x00F8,
    0x00F9,
    0x00FA,
    0x00FB,
    0x00FC,
    0x00FD,
    0x00FE,
    0x00FF
};

// -----------------------------------------------------
// PdfWinAnsiEncoding
// See: http://www.microsoft.com/globaldev/reference/sbcs/1252.mspx
// -----------------------------------------------------

// -----------------------------------------------------
// 
// -----------------------------------------------------
const pdf_utf16be* PdfWinAnsiEncoding::GetToUnicodeTable() const
{
    return PdfWinAnsiEncoding::s_cEncoding;
}

const pdf_utf16be PdfWinAnsiEncoding::s_cEncoding[256] = {
    0x0000, // NULL
    0x0001, // START OF HEADING
    0x0002, // START OF TEXT
    0x0003, // END OF TEXT
    0x0004, // END OF TRANSMISSION
    0x0005, // ENQUIRY
    0x0006, // ACKNOWLEDGE
    0x0007, // BELL
    0x0008, // BACKSPACE
    0x0009, // HORIZONTAL TABULATION
    0x000A, // LINE FEED
    0x000B, // VERTICAL TABULATION
    0x000C, // FORM FEED
    0x000D, // CARRIAGE RETURN
    0x000E, // SHIFT OUT
    0x000F, // SHIFT IN
    0x0010, // DATA LINK ESCAPE
    0x0011, // DEVICE CONTROL ONE
    0x0012, // DEVICE CONTROL TWO
    0x0013, // DEVICE CONTROL THREE
    0x0014, // DEVICE CONTROL FOUR
    0x0015, // NEGATIVE ACKNOWLEDGE
    0x0016, // SYNCHRONOUS IDLE
    0x0017, // END OF TRANSMISSION BLOCK
    0x0018, // CANCEL
    0x0019, // END OF MEDIUM
    0x001A, // SUBSTITUTE
    0x001B, // ESCAPE
    0x001C, // FILE SEPARATOR
    0x001D, // GROUP SEPARATOR
    0x001E, // RECORD SEPARATOR
    0x001F, // UNIT SEPARATOR
    0x0020, // SPACE
    0x0021, // EXCLAMATION MARK
    0x0022, // QUOTATION MARK
    0x0023, // NUMBER SIGN
    0x0024, // DOLLAR SIGN
    0x0025, // PERCENT SIGN
    0x0026, // AMPERSAND
    0x0027, // APOSTROPHE
    0x0028, // LEFT PARENTHESIS
    0x0029, // RIGHT PARENTHESIS
    0x002A, // ASTERISK
    0x002B, // PLUS SIGN
    0x002C, // COMMA
    0x002D, // HYPHEN-MINUS
    0x002E, // FULL STOP
    0x002F, // SOLIDUS
    0x0030, // DIGIT ZERO
    0x0031, // DIGIT ONE
    0x0032, // DIGIT TWO
    0x0033, // DIGIT THREE
    0x0034, // DIGIT FOUR
    0x0035, // DIGIT FIVE
    0x0036, // DIGIT SIX
    0x0037, // DIGIT SEVEN
    0x0038, // DIGIT EIGHT
    0x0039, // DIGIT NINE
    0x003A, // COLON
    0x003B, // SEMICOLON
    0x003C, // LESS-THAN SIGN
    0x003D, // EQUALS SIGN
    0x003E, // GREATER-THAN SIGN
    0x003F, // QUESTION MARK
    0x0040, // COMMERCIAL AT
    0x0041, // LATIN CAPITAL LETTER A
    0x0042, // LATIN CAPITAL LETTER B
    0x0043, // LATIN CAPITAL LETTER C
    0x0044, // LATIN CAPITAL LETTER D
    0x0045, // LATIN CAPITAL LETTER E
    0x0046, // LATIN CAPITAL LETTER F
    0x0047, // LATIN CAPITAL LETTER G
    0x0048, // LATIN CAPITAL LETTER H
    0x0049, // LATIN CAPITAL LETTER I
    0x004A, // LATIN CAPITAL LETTER J
    0x004B, // LATIN CAPITAL LETTER K
    0x004C, // LATIN CAPITAL LETTER L
    0x004D, // LATIN CAPITAL LETTER M
    0x004E, // LATIN CAPITAL LETTER N
    0x004F, // LATIN CAPITAL LETTER O
    0x0050, // LATIN CAPITAL LETTER P
    0x0051, // LATIN CAPITAL LETTER Q
    0x0052, // LATIN CAPITAL LETTER R
    0x0053, // LATIN CAPITAL LETTER S
    0x0054, // LATIN CAPITAL LETTER T
    0x0055, // LATIN CAPITAL LETTER U
    0x0056, // LATIN CAPITAL LETTER V
    0x0057, // LATIN CAPITAL LETTER W
    0x0058, // LATIN CAPITAL LETTER X
    0x0059, // LATIN CAPITAL LETTER Y
    0x005A, // LATIN CAPITAL LETTER Z
    0x005B, // LEFT SQUARE BRACKET
    0x005C, // REVERSE SOLIDUS
    0x005D, // RIGHT SQUARE BRACKET
    0x005E, // CIRCUMFLEX ACCENT
    0x005F, // LOW LINE
    0x0060, // GRAVE ACCENT
    0x0061, // LATIN SMALL LETTER A
    0x0062, // LATIN SMALL LETTER B
    0x0063, // LATIN SMALL LETTER C
    0x0064, // LATIN SMALL LETTER D
    0x0065, // LATIN SMALL LETTER E
    0x0066, // LATIN SMALL LETTER F
    0x0067, // LATIN SMALL LETTER G
    0x0068, // LATIN SMALL LETTER H
    0x0069, // LATIN SMALL LETTER I
    0x006A, // LATIN SMALL LETTER J
    0x006B, // LATIN SMALL LETTER K
    0x006C, // LATIN SMALL LETTER L
    0x006D, // LATIN SMALL LETTER M
    0x006E, // LATIN SMALL LETTER N
    0x006F, // LATIN SMALL LETTER O
    0x0070, // LATIN SMALL LETTER P
    0x0071, // LATIN SMALL LETTER Q
    0x0072, // LATIN SMALL LETTER R
    0x0073, // LATIN SMALL LETTER S
    0x0074, // LATIN SMALL LETTER T
    0x0075, // LATIN SMALL LETTER U
    0x0076, // LATIN SMALL LETTER V
    0x0077, // LATIN SMALL LETTER W
    0x0078, // LATIN SMALL LETTER X
    0x0079, // LATIN SMALL LETTER Y
    0x007A, // LATIN SMALL LETTER Z
    0x007B, // LEFT CURLY BRACKET
    0x007C, // VERTICAL LINE
    0x007D, // RIGHT CURLY BRACKET
    0x007E, // TILDE
    0x007F, // DELETE
    0x20AC, // EURO SIGN
    0x0000,
    0x201A, // SINGLE LOW-9 QUOTATION MARK
    0x0192, // LATIN SMALL LETTER F WITH HOOK
    0x201E, // DOUBLE LOW-9 QUOTATION MARK
    0x2026, // HORIZONTAL ELLIPSIS
    0x2020, // DAGGER
    0x2021, // DOUBLE DAGGER
    0x02C6, // MODIFIER LETTER CIRCUMFLEX ACCENT
    0x2030, // PER MILLE SIGN
    0x0160, // LATIN CAPITAL LETTER S WITH CARON
    0x2039, // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
    0x0152, // LATIN CAPITAL LIGATURE OE
    0x0000,
    0x017D, // LATIN CAPITAL LETTER Z WITH CARON
    0x0000, 
    0x0000,
    0x2018, // LEFT SINGLE QUOTATION MARK
    0x2019, // RIGHT SINGLE QUOTATION MARK
    0x201C, // LEFT DOUBLE QUOTATION MARK
    0x201D, // RIGHT DOUBLE QUOTATION MARK
    0x2022, // BULLET
    0x2013, // EN DASH
    0x2014, // EM DASH
    0x02DC, // SMALL TILDE
    0x2122, // TRADE MARK SIGN
    0x0161, // LATIN SMALL LETTER S WITH CARON
    0x203A, // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
    0x0153, // LATIN SMALL LIGATURE OE
    0x0000,
    0x017E, // LATIN SMALL LETTER Z WITH CARON
    0x0178, // LATIN CAPITAL LETTER Y WITH DIAERESIS
    0x00A0, // NO-BREAK SPACE
    0x00A1, // INVERTED EXCLAMATION MARK
    0x00A2, // CENT SIGN
    0x00A3, // POUND SIGN
    0x00A4, // CURRENCY SIGN
    0x00A5, // YEN SIGN
    0x00A6, // BROKEN BAR
    0x00A7, // SECTION SIGN
    0x00A8, // DIAERESIS
    0x00A9, // COPYRIGHT SIGN
    0x00AA, // FEMININE ORDINAL INDICATOR
    0x00AB, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00AC, // NOT SIGN
    0x00AD, // SOFT HYPHEN
    0x00AE, // REGISTERED SIGN
    0x00AF, // MACRON
    0x00B0, // DEGREE SIGN
    0x00B1, // PLUS-MINUS SIGN
    0x00B2, // SUPERSCRIPT TWO
    0x00B3, // SUPERSCRIPT THREE
    0x00B4, // ACUTE ACCENT
    0x00B5, // MICRO SIGN
    0x00B6, // PILCROW SIGN
    0x00B7, // MIDDLE DOT
    0x00B8, // CEDILLA
    0x00B9, // SUPERSCRIPT ONE
    0x00BA, // MASCULINE ORDINAL INDICATOR
    0x00BB, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00BC, // VULGAR FRACTION ONE QUARTER
    0x00BD, // VULGAR FRACTION ONE HALF
    0x00BE, // VULGAR FRACTION THREE QUARTERS
    0x00BF, // INVERTED QUESTION MARK
    0x00C0, // LATIN CAPITAL LETTER A WITH GRAVE
    0x00C1, // LATIN CAPITAL LETTER A WITH ACUTE
    0x00C2, // LATIN CAPITAL LETTER A WITH CIRCUMFLEX
    0x00C3, // LATIN CAPITAL LETTER A WITH TILDE
    0x00C4, // LATIN CAPITAL LETTER A WITH DIAERESIS
    0x00C5, // LATIN CAPITAL LETTER A WITH RING ABOVE
    0x00C6, // LATIN CAPITAL LETTER AE
    0x00C7, // LATIN CAPITAL LETTER C WITH CEDILLA
    0x00C8, // LATIN CAPITAL LETTER E WITH GRAVE
    0x00C9, // LATIN CAPITAL LETTER E WITH ACUTE
    0x00CA, // LATIN CAPITAL LETTER E WITH CIRCUMFLEX
    0x00CB, // LATIN CAPITAL LETTER E WITH DIAERESIS
    0x00CC, // LATIN CAPITAL LETTER I WITH GRAVE
    0x00CD, // LATIN CAPITAL LETTER I WITH ACUTE
    0x00CE, // LATIN CAPITAL LETTER I WITH CIRCUMFLEX
    0x00CF, // LATIN CAPITAL LETTER I WITH DIAERESIS
    0x00D0, // LATIN CAPITAL LETTER ETH
    0x00D1, // LATIN CAPITAL LETTER N WITH TILDE
    0x00D2, // LATIN CAPITAL LETTER O WITH GRAVE
    0x00D3, // LATIN CAPITAL LETTER O WITH ACUTE
    0x00D4, // LATIN CAPITAL LETTER O WITH CIRCUMFLEX
    0x00D5, // LATIN CAPITAL LETTER O WITH TILDE
    0x00D6, // LATIN CAPITAL LETTER O WITH DIAERESIS
    0x00D7, // MULTIPLICATION SIGN
    0x00D8, // LATIN CAPITAL LETTER O WITH STROKE
    0x00D9, // LATIN CAPITAL LETTER U WITH GRAVE
    0x00DA, // LATIN CAPITAL LETTER U WITH ACUTE
    0x00DB, // LATIN CAPITAL LETTER U WITH CIRCUMFLEX
    0x00DC, // LATIN CAPITAL LETTER U WITH DIAERESIS
    0x00DD, // LATIN CAPITAL LETTER Y WITH ACUTE
    0x00DE, // LATIN CAPITAL LETTER THORN
    0x00DF, // LATIN SMALL LETTER SHARP S
    0x00E0, // LATIN SMALL LETTER A WITH GRAVE
    0x00E1, // LATIN SMALL LETTER A WITH ACUTE
    0x00E2, // LATIN SMALL LETTER A WITH CIRCUMFLEX
    0x00E3, // LATIN SMALL LETTER A WITH TILDE
    0x00E4, // LATIN SMALL LETTER A WITH DIAERESIS
    0x00E5, // LATIN SMALL LETTER A WITH RING ABOVE
    0x00E6, // LATIN SMALL LETTER AE
    0x00E7, // LATIN SMALL LETTER C WITH CEDILLA
    0x00E8, // LATIN SMALL LETTER E WITH GRAVE
    0x00E9, // LATIN SMALL LETTER E WITH ACUTE
    0x00EA, // LATIN SMALL LETTER E WITH CIRCUMFLEX
    0x00EB, // LATIN SMALL LETTER E WITH DIAERESIS
    0x00EC, // LATIN SMALL LETTER I WITH GRAVE
    0x00ED, // LATIN SMALL LETTER I WITH ACUTE
    0x00EE, // LATIN SMALL LETTER I WITH CIRCUMFLEX
    0x00EF, // LATIN SMALL LETTER I WITH DIAERESIS
    0x00F0, // LATIN SMALL LETTER ETH
    0x00F1, // LATIN SMALL LETTER N WITH TILDE
    0x00F2, // LATIN SMALL LETTER O WITH GRAVE
    0x00F3, // LATIN SMALL LETTER O WITH ACUTE
    0x00F4, // LATIN SMALL LETTER O WITH CIRCUMFLEX
    0x00F5, // LATIN SMALL LETTER O WITH TILDE
    0x00F6, // LATIN SMALL LETTER O WITH DIAERESIS
    0x00F7, // DIVISION SIGN
    0x00F8, // LATIN SMALL LETTER O WITH STROKE
    0x00F9, // LATIN SMALL LETTER U WITH GRAVE
    0x00FA, // LATIN SMALL LETTER U WITH ACUTE
    0x00FB, // LATIN SMALL LETTER U WITH CIRCUMFLEX
    0x00FC, // LATIN SMALL LETTER U WITH DIAERESIS
    0x00FD, // LATIN SMALL LETTER Y WITH ACUTE
    0x00FE, // LATIN SMALL LETTER THORN
    0x00FF, // LATIN SMALL LETTER Y WITH DIAERESIS
};

// -----------------------------------------------------
// PdfMacRomanEncoding
// -----------------------------------------------------

// -----------------------------------------------------
// 
// -----------------------------------------------------
const pdf_utf16be* PdfMacRomanEncoding::GetToUnicodeTable() const
{
    return PdfMacRomanEncoding::s_cEncoding;
}

const pdf_utf16be PdfMacRomanEncoding::s_cEncoding[256] = {
    0x0000, // NULL
    0x0001, // START OF HEADING
    0x0002, // START OF TEXT
    0x0003, // END OF TEXT
    0x0004, // END OF TRANSMISSION
    0x0005, // ENQUIRY
    0x0006, // ACKNOWLEDGE
    0x0007, // BELL
    0x0008, // BACKSPACE
    0x0009, // HORIZONTAL TABULATION
    0x000A, // LINE FEED
    0x000B, // VERTICAL TABULATION
    0x000C, // FORM FEED
    0x000D, // CARRIAGE RETURN
    0x000E, // SHIFT OUT
    0x000F, // SHIFT IN
    0x0010, // DATA LINK ESCAPE
    0x0011, // DEVICE CONTROL ONE
    0x0012, // DEVICE CONTROL TWO
    0x0013, // DEVICE CONTROL THREE
    0x0014, // DEVICE CONTROL FOUR
    0x0015, // NEGATIVE ACKNOWLEDGE
    0x0016, // SYNCHRONOUS IDLE
    0x0017, // END OF TRANSMISSION BLOCK
    0x0018, // CANCEL
    0x0019, // END OF MEDIUM
    0x001A, // SUBSTITUTE
    0x001B, // ESCAPE
    0x001C, // FILE SEPARATOR
    0x001D, // GROUP SEPARATOR
    0x001E, // RECORD SEPARATOR
    0x001F, // UNIT SEPARATOR
    0x0020, // SPACE
    0x0021, // EXCLAMATION MARK
    0x0022, // QUOTATION MARK
    0x0023, // NUMBER SIGN
    0x0024, // DOLLAR SIGN
    0x0025, // PERCENT SIGN
    0x0026, // AMPERSAND
    0x0027, // APOSTROPHE
    0x0028, // LEFT PARENTHESIS
    0x0029, // RIGHT PARENTHESIS
    0x002A, // ASTERISK
    0x002B, // PLUS SIGN
    0x002C, // COMMA
    0x002D, // HYPHEN-MINUS
    0x002E, // FULL STOP
    0x002F, // SOLIDUS
    0x0030, // DIGIT ZERO
    0x0031, // DIGIT ONE
    0x0032, // DIGIT TWO
    0x0033, // DIGIT THREE
    0x0034, // DIGIT FOUR
    0x0035, // DIGIT FIVE
    0x0036, // DIGIT SIX
    0x0037, // DIGIT SEVEN
    0x0038, // DIGIT EIGHT
    0x0039, // DIGIT NINE
    0x003A, // COLON
    0x003B, // SEMICOLON
    0x003C, // LESS-THAN SIGN
    0x003D, // EQUALS SIGN
    0x003E, // GREATER-THAN SIGN
    0x003F, // QUESTION MARK
    0x0040, // COMMERCIAL AT
    0x0041, // LATIN CAPITAL LETTER A
    0x0042, // LATIN CAPITAL LETTER B
    0x0043, // LATIN CAPITAL LETTER C
    0x0044, // LATIN CAPITAL LETTER D
    0x0045, // LATIN CAPITAL LETTER E
    0x0046, // LATIN CAPITAL LETTER F
    0x0047, // LATIN CAPITAL LETTER G
    0x0048, // LATIN CAPITAL LETTER H
    0x0049, // LATIN CAPITAL LETTER I
    0x004A, // LATIN CAPITAL LETTER J
    0x004B, // LATIN CAPITAL LETTER K
    0x004C, // LATIN CAPITAL LETTER L
    0x004D, // LATIN CAPITAL LETTER M
    0x004E, // LATIN CAPITAL LETTER N
    0x004F, // LATIN CAPITAL LETTER O
    0x0050, // LATIN CAPITAL LETTER P
    0x0051, // LATIN CAPITAL LETTER Q
    0x0052, // LATIN CAPITAL LETTER R
    0x0053, // LATIN CAPITAL LETTER S
    0x0054, // LATIN CAPITAL LETTER T
    0x0055, // LATIN CAPITAL LETTER U
    0x0056, // LATIN CAPITAL LETTER V
    0x0057, // LATIN CAPITAL LETTER W
    0x0058, // LATIN CAPITAL LETTER X
    0x0059, // LATIN CAPITAL LETTER Y
    0x005A, // LATIN CAPITAL LETTER Z
    0x005B, // LEFT SQUARE BRACKET
    0x005C, // REVERSE SOLIDUS
    0x005D, // RIGHT SQUARE BRACKET
    0x005E, // CIRCUMFLEX ACCENT
    0x005F, // LOW LINE
    0x0060, // GRAVE ACCENT
    0x0061, // LATIN SMALL LETTER A
    0x0062, // LATIN SMALL LETTER B
    0x0063, // LATIN SMALL LETTER C
    0x0064, // LATIN SMALL LETTER D
    0x0065, // LATIN SMALL LETTER E
    0x0066, // LATIN SMALL LETTER F
    0x0067, // LATIN SMALL LETTER G
    0x0068, // LATIN SMALL LETTER H
    0x0069, // LATIN SMALL LETTER I
    0x006A, // LATIN SMALL LETTER J
    0x006B, // LATIN SMALL LETTER K
    0x006C, // LATIN SMALL LETTER L
    0x006D, // LATIN SMALL LETTER M
    0x006E, // LATIN SMALL LETTER N
    0x006F, // LATIN SMALL LETTER O
    0x0070, // LATIN SMALL LETTER P
    0x0071, // LATIN SMALL LETTER Q
    0x0072, // LATIN SMALL LETTER R
    0x0073, // LATIN SMALL LETTER S
    0x0074, // LATIN SMALL LETTER T
    0x0075, // LATIN SMALL LETTER U
    0x0076, // LATIN SMALL LETTER V
    0x0077, // LATIN SMALL LETTER W
    0x0078, // LATIN SMALL LETTER X
    0x0079, // LATIN SMALL LETTER Y
    0x007A, // LATIN SMALL LETTER Z
    0x007B, // LEFT CURLY BRACKET
    0x007C, // VERTICAL LINE
    0x007D, // RIGHT CURLY BRACKET
    0x007E, // TILDE
    0x007F, // DEL
    0x00C4, // LATIN CAPITAL LETTER A WITH DIAERESIS
    0x00C5, // LATIN CAPITAL LETTER A WITH RING ABOVE
    0x00C7, // LATIN CAPITAL LETTER C WITH CEDILLA
    0x00C9, // LATIN CAPITAL LETTER E WITH ACUTE
    0x00D1, // LATIN CAPITAL LETTER N WITH TILDE
    0x00D6, // LATIN CAPITAL LETTER O WITH DIAERESIS
    0x00DC, // LATIN CAPITAL LETTER U WITH DIAERESIS
    0x00E1, // LATIN SMALL LETTER A WITH ACUTE
    0x00E0, // LATIN SMALL LETTER A WITH GRAVE
    0x00E2, // LATIN SMALL LETTER A WITH CIRCUMFLEX
    0x00E4, // LATIN SMALL LETTER A WITH DIAERESIS
    0x00E3, // LATIN SMALL LETTER A WITH TILDE
    0x00E5, // LATIN SMALL LETTER A WITH RING ABOVE
    0x00E7, // LATIN SMALL LETTER C WITH CEDILLA
    0x00E9, // LATIN SMALL LETTER E WITH ACUTE
    0x00E8, // LATIN SMALL LETTER E WITH GRAVE
    0x00EA, // LATIN SMALL LETTER E WITH CIRCUMFLEX
    0x00EB, // LATIN SMALL LETTER E WITH DIAERESIS
    0x00ED, // LATIN SMALL LETTER I WITH ACUTE
    0x00EC, // LATIN SMALL LETTER I WITH GRAVE
    0x00EE, // LATIN SMALL LETTER I WITH CIRCUMFLEX
    0x00EF, // LATIN SMALL LETTER I WITH DIAERESIS
    0x00F1, // LATIN SMALL LETTER N WITH TILDE
    0x00F3, // LATIN SMALL LETTER O WITH ACUTE
    0x00F2, // LATIN SMALL LETTER O WITH GRAVE
    0x00F4, // LATIN SMALL LETTER O WITH CIRCUMFLEX
    0x00F6, // LATIN SMALL LETTER O WITH DIAERESIS
    0x00F5, // LATIN SMALL LETTER O WITH TILDE
    0x00FA, // LATIN SMALL LETTER U WITH ACUTE
    0x00F9, // LATIN SMALL LETTER U WITH GRAVE
    0x00FB, // LATIN SMALL LETTER U WITH CIRCUMFLEX
    0x00FC, // LATIN SMALL LETTER U WITH DIAERESIS
    0x2020, // DAGGER
    0x00B0, // DEGREE SIGN
    0x00A2, // CENT SIGN
    0x00A3, // POUND SIGN
    0x00A7, // SECTION SIGN
    0x2022, // BULLET
    0x00B6, // PILCROW SIGN
    0x00DF, // LATIN SMALL LETTER SHARP S
    0x00AE, // REGISTERED SIGN
    0x00A9, // COPYRIGHT SIGN
    0x2122, // TRADE MARK SIGN
    0x00B4, // ACUTE ACCENT
    0x00A8, // DIAERESIS
    0x2260, // NOT EQUAL TO
    0x00C6, // LATIN CAPITAL LETTER AE
    0x00D8, // LATIN CAPITAL LETTER O WITH STROKE
    0x221E, // INFINITY
    0x00B1, // PLUS-MINUS SIGN
    0x2264, // LESS-THAN OR EQUAL TO
    0x2265, // GREATER-THAN OR EQUAL TO
    0x00A5, // YEN SIGN
    0x00B5, // MICRO SIGN
    0x2202, // PARTIAL DIFFERENTIAL
    0x2211, // N-ARY SUMMATION
    0x220F, // N-ARY PRODUCT
    0x03C0, // GREEK SMALL LETTER PI
    0x222B, // INTEGRAL
    0x00AA, // FEMININE ORDINAL INDICATOR
    0x00BA, // MASCULINE ORDINAL INDICATOR
    0x03A9, // GREEK CAPITAL LETTER OMEGA
    0x00E6, // LATIN SMALL LETTER AE
    0x00F8, // LATIN SMALL LETTER O WITH STROKE
    0x00BF, // INVERTED QUESTION MARK
    0x00A1, // INVERTED EXCLAMATION MARK
    0x00AC, // NOT SIGN
    0x221A, // SQUARE ROOT
    0x0192, // LATIN SMALL LETTER F WITH HOOK
    0x2248, // ALMOST EQUAL TO
    0x2206, // INCREMENT
    0x00AB, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00BB, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x2026, // HORIZONTAL ELLIPSIS
    0x00A0, // NO-BREAK SPACE
    0x00C0, // LATIN CAPITAL LETTER A WITH GRAVE
    0x00C3, // LATIN CAPITAL LETTER A WITH TILDE
    0x00D5, // LATIN CAPITAL LETTER O WITH TILDE
    0x0152, // LATIN CAPITAL LIGATURE OE
    0x0153, // LATIN SMALL LIGATURE OE
    0x2013, // EN DASH
    0x2014, // EM DASH
    0x201C, // LEFT DOUBLE QUOTATION MARK
    0x201D, // RIGHT DOUBLE QUOTATION MARK
    0x2018, // LEFT SINGLE QUOTATION MARK
    0x2019, // RIGHT SINGLE QUOTATION MARK
    0x00F7, // DIVISION SIGN
    0x25CA, // LOZENGE
    0x00FF, // LATIN SMALL LETTER Y WITH DIAERESIS
    0x0178, // LATIN CAPITAL LETTER Y WITH DIAERESIS
    0x2044, // FRACTION SLASH
    0x20AC, // EURO SIGN
    0x2039, // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
    0x203A, // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
    0xFB01, // LATIN SMALL LIGATURE FI
    0xFB02, // LATIN SMALL LIGATURE FL
    0x2021, // DOUBLE DAGGER
    0x00B7, // MIDDLE DOT
    0x201A, // SINGLE LOW-9 QUOTATION MARK
    0x201E, // DOUBLE LOW-9 QUOTATION MARK
    0x2030, // PER MILLE SIGN
    0x00C2, // LATIN CAPITAL LETTER A WITH CIRCUMFLEX
    0x00CA, // LATIN CAPITAL LETTER E WITH CIRCUMFLEX
    0x00C1, // LATIN CAPITAL LETTER A WITH ACUTE
    0x00CB, // LATIN CAPITAL LETTER E WITH DIAERESIS
    0x00C8, // LATIN CAPITAL LETTER E WITH GRAVE
    0x00CD, // LATIN CAPITAL LETTER I WITH ACUTE
    0x00CE, // LATIN CAPITAL LETTER I WITH CIRCUMFLEX
    0x00CF, // LATIN CAPITAL LETTER I WITH DIAERESIS
    0x00CC, // LATIN CAPITAL LETTER I WITH GRAVE
    0x00D3, // LATIN CAPITAL LETTER O WITH ACUTE
    0x00D4, // LATIN CAPITAL LETTER O WITH CIRCUMFLEX
    0xF8FF, // Apple logo
    0x00D2, // LATIN CAPITAL LETTER O WITH GRAVE
    0x00DA, // LATIN CAPITAL LETTER U WITH ACUTE
    0x00DB, // LATIN CAPITAL LETTER U WITH CIRCUMFLEX
    0x00D9, // LATIN CAPITAL LETTER U WITH GRAVE
    0x0131, // LATIN SMALL LETTER DOTLESS I
    0x02C6, // MODIFIER LETTER CIRCUMFLEX ACCENT
    0x02DC, // SMALL TILDE
    0x00AF, // MACRON
    0x02D8, // BREVE
    0x02D9, // DOT ABOVE
    0x02DA, // RING ABOVE
    0x00B8, // CEDILLA
    0x02DD, // DOUBLE ACUTE ACCENT
    0x02DB, // OGONEK
    0x02C7, // CARON
};

// -----------------------------------------------------
// PdfIdentityEncoding
// -----------------------------------------------------
PdfIdentityEncoding::PdfIdentityEncoding( int nFirstChar, int nLastChar, bool bAutoDelete )
    : PdfEncoding( nFirstChar, nLastChar ), m_bAutoDelete( bAutoDelete )
{
    // create a unique ID
    std::ostringstream oss;
    oss << "/Identity-H" << nFirstChar << "_" << nLastChar;

    m_id = PdfName( oss.str() );
}

void PdfIdentityEncoding::AddToDictionary( PdfDictionary & rDictionary ) const
{
    rDictionary.AddKey( "Encoding", PdfName("Identity-H") );
}

pdf_utf16be PdfIdentityEncoding::GetCharCode( int nIndex ) const
{
    if( nIndex < this->GetFirstChar() ||
	nIndex > this->GetLastChar() )
    {
	PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

#ifdef PODOFO_IS_LITTLE_ENDIAN
    return ((nIndex & 0xff00) >> 8) | ((nIndex & 0xff) << 8);
#else
    return static_cast<pdf_utf16be>(nIndex);
#endif // PODOFO_IS_LITTLE_ENDIAN
}

PdfString PdfIdentityEncoding::ConvertToUnicode( const PdfString & rEncodedString, const PdfFont* pFont ) const
{
    if( !pFont ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // Get the string in UTF-16be format
    PdfString          sStr = rEncodedString.ToUnicode();
    const pdf_utf16be* pStr = sStr.GetUnicode();
    long               lUnicodeValue;

    std::ostringstream out;
    PdfLocaleImbue(out);

    while( *pStr ) 
    {
        lUnicodeValue = this->GetUnicodeValue( static_cast<long>(*(const_cast<pdf_utf16be*>(pStr))) );

#ifdef PODOFO_IS_LITTLE_ENDIAN
        out << static_cast<unsigned char>((lUnicodeValue & 0xff00) >> 8);
        out << static_cast<unsigned char>(lUnicodeValue & 0x00ff);
#else
        out << static_cast<unsigned char>(lUnicodeValue & 0x00ff);
        out << static_cast<unsigned char>((lUnicodeValue & 0xff00) >> 8);
#endif // PODOFO_IS_LITTLE_ENDIAN

        ++pStr;
    }

    return PdfString( out.str().c_str(), out.str().length() );;
}

PdfString PdfIdentityEncoding::ConvertToEncoding( const PdfString & rString, const PdfFont* pFont ) const
{
    if( !pFont ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // Get the string in UTF-16be format
    PdfString          sStr = rString.ToUnicode();
    const pdf_utf16be* pStr = sStr.GetUnicode();
    long               lGlyphId;

    std::ostringstream out;
    PdfLocaleImbue(out);

    while( *pStr ) 
    {
#ifdef PODOFO_IS_LITTLE_ENDIAN
        lGlyphId = pFont->GetFontMetrics()->GetGlyphId( (((*pStr & 0xff) << 8) | ((*pStr & 0xff00) >> 8)) );
#else
        lGlyphId = pFont->GetFontMetrics()->GetGlyphId( *pStr );
#endif // PODOFO_IS_LITTLE_ENDIAN

        out << static_cast<unsigned char>((lGlyphId & 0xff00) >> 8);
        out << static_cast<unsigned char>(lGlyphId & 0x00ff);

        ++pStr;
    }

    return PdfString( out.str().c_str(), out.str().length() );;
}

pdf_utf16be PdfIdentityEncoding::GetUnicodeValue( long lCharCode ) const
{
    return 0;
}
 
}; /* namespace PoDoFo */
