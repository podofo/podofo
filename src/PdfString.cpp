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

#include "PdfString.h"

#include "PdfFilter.h"
#include "PdfOutputDevice.h"

namespace PoDoFo {

const PdfString PdfString::StringNull = PdfString();
const char PdfString::s_pszUnicodeMarker[PdfString::s_nUnicodeMarkerLen] = { 0xFE, 0xFF };

PdfString::PdfString()
    : m_bHex( false ), m_bUnicode( false )
{
}

PdfString::PdfString( const std::string& sString )
    : m_bHex( false ), m_bUnicode( false )
{
    if( sString.length() )
        Init( sString.c_str(), sString.length() );
}

PdfString::PdfString( const char* pszString )
    : m_bHex( false ), m_bUnicode( false )
{
    if( pszString )
        Init( pszString, strlen( pszString ) );
}

PdfString::PdfString( const char* pszString, long lLen, bool bHex )
    : m_bHex( bHex ), m_bUnicode( false )
{
    Init( pszString, lLen );
}

PdfString::PdfString( const PdfString & rhs )
    : PdfDataType(), m_bHex( false ), m_bUnicode( false )
{
    this->operator=( rhs );
}

PdfString::~PdfString()
{
}

void PdfString::SetHexData( const char* pszHex, long lLen )
{
    if( !pszHex ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( lLen == -1 )
        lLen = strlen( pszHex );

    m_bHex   = true;
    m_buffer = PdfRefCountedBuffer( lLen + 1);

    memcpy( m_buffer.GetBuffer(), pszHex, lLen );
    m_buffer.GetBuffer()[lLen] = '\0';
}

void PdfString::Write ( PdfOutputDevice* pDevice ) const
{
    // Strings in PDF documents may contain \0 especially if they are encrypted
    // this case has to be handled!

    pDevice->Print( m_bHex ? "<" : "(" );
    if( m_bUnicode ) 
        pDevice->Write( PdfString::s_pszUnicodeMarker, PdfString::s_nUnicodeMarkerLen );

    pDevice->Write( m_buffer.GetBuffer(), m_buffer.GetSize()-1 );
    pDevice->Print( m_bHex ? ">" : ")" );
}

bool PdfString::operator>( const PdfString & rhs ) const
{
    char*            pBuffer;
    long             lLen;
    bool             bGreater   = false; // avoid a compiler warning
#ifdef _MSC_VER
    std::auto_ptr<PdfFilter> pFilter;
#else
    std::auto_ptr<const PdfFilter> pFilter;
#endif

    if( m_bHex == rhs.m_bHex ) 
    {
        bGreater = (m_buffer > rhs.m_buffer);
    }
    else if( !m_bHex ) 
    {
        pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );
        pFilter->Encode( m_buffer.GetBuffer(), m_buffer.GetSize(), &pBuffer, &lLen );

        bGreater = ( PdfRefCountedBuffer( pBuffer, lLen ) > rhs.m_buffer );
    }
    else if( !rhs.m_bHex ) 
    {
        pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );
        pFilter->Encode( rhs.m_buffer.GetBuffer(), rhs.m_buffer.GetSize(), &pBuffer, &lLen );

        bGreater = ( m_buffer > PdfRefCountedBuffer( pBuffer, lLen ) );
    }

    return bGreater;
}

bool PdfString::operator<( const PdfString & rhs ) const
{
    char*            pBuffer;
    long             lLen;
    bool             bLittle   = false; // avoid a compiler warning
#ifdef _MSC_VER
    std::auto_ptr<PdfFilter> pFilter;
#else
    std::auto_ptr<const PdfFilter> pFilter;
#endif

    if( m_bHex == rhs.m_bHex ) 
    {
        bLittle = (m_buffer < rhs.m_buffer);
    }
    else if( !m_bHex ) 
    {
        pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );
        pFilter->Encode( m_buffer.GetBuffer(), m_buffer.GetSize(), &pBuffer, &lLen );

        bLittle = ( PdfRefCountedBuffer( pBuffer, lLen ) < rhs.m_buffer );
    }
    else if( !rhs.m_bHex )
    {
        pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );
        pFilter->Encode( rhs.m_buffer.GetBuffer(), rhs.m_buffer.GetSize(), &pBuffer, &lLen );

        bLittle = ( m_buffer < PdfRefCountedBuffer( pBuffer, lLen ) );
    }
        
    return bLittle;
}

const PdfString & PdfString::operator=( const PdfString & rhs )
{
    this->m_bHex     = rhs.m_bHex;
    this->m_bUnicode = rhs.m_bUnicode;
    this->m_buffer   = rhs.m_buffer;

    return *this;
}

bool PdfString::operator==( const PdfString & rhs ) const
{
    char*            pBuffer;
    long             lLen;
    bool             bEqual   = false; // avoid a compiler warning
#ifdef _MSC_VER
    std::auto_ptr<PdfFilter> pFilter;
#else
    std::auto_ptr<const PdfFilter> pFilter;
#endif

    if( m_bHex == rhs.m_bHex ) 
    {
        bEqual = (m_buffer == rhs.m_buffer);
    }
    else if( !m_bHex ) 
    {
        pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );
        pFilter->Encode( m_buffer.GetBuffer(), m_buffer.GetSize(), &pBuffer, &lLen );

        bEqual = ( PdfRefCountedBuffer( pBuffer, lLen ) == rhs.m_buffer );
    }
    else if( !rhs.m_bHex ) 
    {
        pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );
        pFilter->Encode( rhs.m_buffer.GetBuffer(), rhs.m_buffer.GetSize(), &pBuffer, &lLen );

        bEqual = ( m_buffer == PdfRefCountedBuffer( pBuffer, lLen ) );
    }
        
    return bEqual;
}

void PdfString::Init( const char* pszString, long lLen )
{
#ifdef _MSC_VER
    std::auto_ptr<PdfFilter> pFilter;
#else
    std::auto_ptr<const PdfFilter> pFilter;
#endif
    char* pBuf;
    long  lBufLen;

    // TODO: escape characters inside of strings!

    if( pszString ) 
    {
        if( m_bHex ) 
        {
            pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );
            if( pFilter.get() ) 
            {
                pFilter->Encode( pszString, lLen, &pBuf, &lBufLen );
                m_buffer = PdfRefCountedBuffer( pBuf, lBufLen );
            }
            else
            {
                PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
            }
        }
        else
        {
            bool bUft16LE = false;
            // check if it is a unicode string (UTF-16BE)
            // UTF-16BE strings have to start with 0xFE 0xFF
            if( lLen >= 2 ) 
            {
                m_bUnicode = (pszString[0] == PdfString::s_pszUnicodeMarker[0] && 
                              pszString[1] == PdfString::s_pszUnicodeMarker[1]);

                // Check also for UTF-16LE
                if( !m_bUnicode && (pszString[0] == PdfString::s_pszUnicodeMarker[1] && 
                                    pszString[1] == PdfString::s_pszUnicodeMarker[0]) )
                {
                    bUft16LE = true;
                }
            }

            // skip the first two bytes 
            if( m_bUnicode )
            {
                lLen      -= 2;
                pszString += 2;
            }

            m_buffer = PdfRefCountedBuffer( m_bUnicode ? lLen + 2 : lLen + 1);
            memcpy( m_buffer.GetBuffer(), pszString, lLen );
            m_buffer.GetBuffer()[lLen] = '\0';
            // terminate unicode strings with \0\0
            if( m_bUnicode )
                m_buffer.GetBuffer()[lLen+1] = '\0';
                

            // if the buffer is a UTF-16LE string
            // convert it to UTF-16BE
            if( bUft16LE ) 
            {
                char  cSwap;
                char* pBuf = m_buffer.GetBuffer();
                while( lLen > 1 )
                {
                    cSwap     = *pBuf;
                    *pBuf     = *(pBuf+1);
                    *(++pBuf) = cSwap;

                    ++pBuf;
                    lLen -= 2;
                }
            }
        }
    }
}


/*
 * The disclaimer below applies to the Unicode conversion
 * functions below. The functions where adapted for use in PoDoFo.
 *
 * The original can be found at:
 * http://www.unicode.org/Public/PROGRAMS/CVTUTF/
 */

/*
 * Copyright 2001-2004 Unicode, Inc.
 * 
 * Disclaimer
 * 
 * This source code is provided as is by Unicode, Inc. No claims are
 * made as to fitness for any particular purpose. No warranties of any
 * kind are expressed or implied. The recipient agrees to determine
 * applicability of information provided. If this file has been
 * purchased on magnetic or optical media from Unicode, Inc., the
 * sole remedy for any claim will be exchange of defective media
 * within 90 days of receipt.
 * 
 * Limitations on Rights to Redistribute This Code
 * 
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
 */

/* ---------------------------------------------------------------------

    Conversions between UTF32, UTF-16, and UTF-8. Source code file.
    Author: Mark E. Davis, 1994.
    Rev History: Rick McGowan, fixes & updates May 2001.
    Sept 2001: fixed const & error conditions per
	mods suggested by S. Parent & A. Lillich.
    June 2002: Tim Dodd added detection and handling of incomplete
	source sequences, enhanced error detection, added casts
	to eliminate compiler warnings.
    July 2003: slight mods to back out aggressive FFFE detection.
    Jan 2004: updated switches in from-UTF8 conversions.
    Oct 2004: updated to use UNI_MAX_LEGAL_UTF32 in UTF-32 conversions.

    See the header file "ConvertUTF.h" for complete documentation.

------------------------------------------------------------------------ */

/* --------------------------------------------------------------------- */
/* Some fundamental constants */
#define UNI_REPLACEMENT_CHAR static_cast<unsigned long>(0x0000FFFD)
#define UNI_MAX_BMP          static_cast<unsigned long>(0x0000FFFF)
#define UNI_MAX_UTF16        static_cast<unsigned long>(0x0010FFFF)
#define UNI_MAX_UTF32        static_cast<unsigned long>(0x7FFFFFFF)
#define UNI_MAX_LEGAL_UTF32  static_cast<unsigned long>(0x0010FFFF)

#define UNI_SUR_HIGH_START   static_cast<unsigned long>(0xD800)
#define UNI_SUR_HIGH_END     static_cast<unsigned long>(0xDBFF)
#define UNI_SUR_LOW_START    static_cast<unsigned long>(0xDC00)
#define UNI_SUR_LOW_END      static_cast<unsigned long>(0xDFFF)

static const int halfShift  = 10; /* used for shifting by 10 bits */

static const unsigned long halfBase = 0x0010000UL;
static const unsigned long halfMask = 0x3FFUL;

/* --------------------------------------------------------------------- */
/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
 * left as-is for anyone who may want to do such conversion, which was
 * allowed in earlier algorithms.
 */
static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/*
 * Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
static const unsigned long offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 
                                                  0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/*
 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
 * into the first byte, depending on how many bytes follow.  There are
 * as many entries in this table as there are UTF-8 sequence types.
 * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
 * for *legal* UTF-8 will be 4 or fewer bytes total.
 */
static const pdf_utf8 firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

/* --------------------------------------------------------------------- */

/*
 * Utility routine to tell whether a sequence of bytes is legal UTF-8.
 * This must be called with the length pre-determined by the first byte.
 * If not calling this from ConvertUTF8to*, then the length can be set by:
 *  length = trailingBytesForUTF8[*source]+1;
 * and the sequence is illegal right away if there aren't that many bytes
 * available.
 * If presented with a length > 4, this returns false.  The Unicode
 * definition of UTF-8 goes up to 4-byte sequences.
 */

static bool isLegalUTF8(const pdf_utf8 *source, int length) {
    pdf_utf8 a;
    const pdf_utf8 *srcptr = source+length;
    switch (length) {
    default: return false;
	/* Everything else falls through when "true"... */
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 2: if ((a = (*--srcptr)) > 0xBF) return false;

	switch (*source) {
	    /* no fall-through in this inner switch */
	    case 0xE0: if (a < 0xA0) return false; break;
	    case 0xED: if (a > 0x9F) return false; break;
	    case 0xF0: if (a < 0x90) return false; break;
	    case 0xF4: if (a > 0x8F) return false; break;
	    default:   if (a < 0x80) return false;
	}

    case 1: if (*source >= 0x80 && *source < 0xC2) return false;
    }
    if (*source > 0xF4) return false;
    return true;
}

/* --------------------------------------------------------------------- */

/*
 * Exported function to return whether a UTF-8 sequence is legal or not.
 * This is not used here; it's just exported.
 */
bool isLegalUTF8Sequence(const pdf_utf8 *source, const pdf_utf8 *sourceEnd) {
    int length = trailingBytesForUTF8[*source]+1;
    if (source+length > sourceEnd) {
	return false;
    }
    return isLegalUTF8(source, length);
}


long PdfString::ConvertUTF8toUTF16( const pdf_utf8* pszUtf8, pdf_utf16be* pszUtf16, long lLenUtf16 )
{
    return pszUtf8 ? 
        PdfString::ConvertUTF8toUTF16( pszUtf8, strlen( reinterpret_cast<const char*>(pszUtf8) ), pszUtf16, lLenUtf16 ) : 0;
}

long PdfString::ConvertUTF8toUTF16( const pdf_utf8* pszUtf8, long lLenUtf8, 
                                    pdf_utf16be* pszUtf16, long lLenUtf16,
                                    EPdfStringConversion eConversion )
{
    const pdf_utf8* source    = pszUtf8;
    const pdf_utf8* sourceEnd = pszUtf8 + lLenUtf8;
    pdf_utf16be*    target    = pszUtf16;
    pdf_utf16be*    targetEnd = pszUtf16 + lLenUtf16;

    while (source < sourceEnd) 
    {
	unsigned long ch = 0;
	unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
	if (source + extraBytesToRead >= sourceEnd) {
	    PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "The UTF8 string was to short while converting from UTF8 to UTF16." );
	}

	// Do this check whether lenient or strict
	if (! isLegalUTF8(source, extraBytesToRead+1)) {
	    PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "The UTF8 string was invalid while from UTF8 to UTF16." );
	}

	/*
	 * The cases all fall through. See "Note A" below.
	 */
	switch (extraBytesToRead) {
	    case 5: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
	    case 4: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
	    case 3: ch += *source++; ch <<= 6;
	    case 2: ch += *source++; ch <<= 6;
	    case 1: ch += *source++; ch <<= 6;
	    case 0: ch += *source++;
	}
	ch -= offsetsFromUTF8[extraBytesToRead];

	if (target >= targetEnd) {
	    source -= (extraBytesToRead+1); /* Back up source pointer! */
	    PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
	}

	if (ch <= UNI_MAX_BMP) { /* Target is a character <= 0xFFFF */
	    /* UTF-16 surrogate values are illegal in UTF-32 */
	    if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
		if (eConversion == ePdfStringConversion_Strict) {
		    source -= (extraBytesToRead+1); /* return to the illegal value itself */
                    PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
		    break;
		} else {
		    *target++ = UNI_REPLACEMENT_CHAR;
		}
	    } else {
		*target++ = static_cast<pdf_utf16be>(ch); /* normal case */
	    }
	} else if (ch > UNI_MAX_UTF16) {
	    if (eConversion == ePdfStringConversion_Strict) {
                PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
		source -= (extraBytesToRead+1); /* return to the start */
		break; /* Bail out; shouldn't continue */
	    } else {
		*target++ = UNI_REPLACEMENT_CHAR;
	    }
	} else {
	    /* target is a character in range 0xFFFF - 0x10FFFF. */
	    if (target + 1 >= targetEnd) {
		source -= (extraBytesToRead+1); /* Back up source pointer! */
                PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
	    }
            
	    ch -= halfBase;
	    *target++ = static_cast<pdf_utf16be>((ch >> halfShift) + UNI_SUR_HIGH_START);
	    *target++ = static_cast<pdf_utf16be>((ch & halfMask) + UNI_SUR_LOW_START);
	}
    }

    // return characters written
    return target - pszUtf16;
}

long PdfString::ConvertUTF16toUTF8( const pdf_utf16be* pszUtf16, pdf_utf8* pszUtf8, long lLenUtf8 )
{
    // TODO: not yet implemented
    return -1;
}

long PdfString::ConvertUTF16toUTF8( const pdf_utf16be* pszUtf16, long lLenUtf16, 
                                    pdf_utf8* pszUtf8, long lLenUtf8, 
                                    EPdfStringConversion eConversion  )
{
    const pdf_utf16be* source    = pszUtf16;
    const pdf_utf16be* sourceEnd = pszUtf16 + lLenUtf16;
    
    pdf_utf8* target    = pszUtf8;
    pdf_utf8* targetEnd = pszUtf8 + lLenUtf8;

    while (source < sourceEnd) {
	unsigned long  ch;
	unsigned short bytesToWrite = 0;
	const unsigned long byteMask = 0xBF;
	const unsigned long byteMark = 0x80; 
	const pdf_utf16be* oldSource = source; /* In case we have to back up because of target overflow. */
	ch = *source++;

	/* If we have a surrogate pair, convert to UTF32 first. */
	if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) {
	    /* If the 16 bits following the high surrogate are in the source buffer... */
	    if (source < sourceEnd) {
		unsigned long ch2 = *source;
		/* If it's a low surrogate, convert to UTF32. */
		if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) {
		    ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
			+ (ch2 - UNI_SUR_LOW_START) + halfBase;
		    ++source;
		} else if (eConversion == ePdfStringConversion_Strict) { /* it's an unpaired high surrogate */
		    --source; /* return to the illegal value itself */
                    PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
		    break;
		}
	    } else { /* We don't have the 16 bits following the high surrogate. */
		--source; /* return to the high surrogate */
                PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
		break;
	    }
	} else if (eConversion == ePdfStringConversion_Strict) {
	    /* UTF-16 surrogate values are illegal in UTF-32 */
	    if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END) {
		--source; /* return to the illegal value itself */
                PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
		break;
	    }
	}
	/* Figure out how many bytes the result will require */
	if (ch < static_cast<unsigned long>(0x80)) {	     
            bytesToWrite = 1;
	} else if (ch < static_cast<unsigned long>(0x800)) {     
            bytesToWrite = 2;
	} else if (ch < static_cast<unsigned long>(0x10000)) {   
            bytesToWrite = 3;
	} else if (ch < static_cast<unsigned long>(0x110000)) {  
            bytesToWrite = 4;
	} else {			    
            bytesToWrite = 3;
            ch = UNI_REPLACEMENT_CHAR;
	}

	target += bytesToWrite;
	if (target > targetEnd) {
	    source = oldSource; /* Back up source pointer! */
	    target -= bytesToWrite; 
            printf("target - targetEnd=%i\n", target - targetEnd );
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
            break;
	}
        
	switch (bytesToWrite) { /* note: everything falls through. */
	    case 4: *--target = static_cast<pdf_utf8>((ch | byteMark) & byteMask); ch >>= 6;
	    case 3: *--target = static_cast<pdf_utf8>((ch | byteMark) & byteMask); ch >>= 6;
	    case 2: *--target = static_cast<pdf_utf8>((ch | byteMark) & byteMask); ch >>= 6;
	    case 1: *--target = static_cast<pdf_utf8>(ch | firstByteMark[bytesToWrite]);
	}
	target += bytesToWrite;
    }

    // return bytes written
    return target - pszUtf8;
}

/* ---------------------------------------------------------------------

    Note A.
    The fall-through switches in UTF-8 reading code save a
    temp variable, some decrements & conditionals.  The switches
    are equivalent to the following loop:
	{
	    int tmpBytesToRead = extraBytesToRead+1;
	    do {
		ch += *source++;
		--tmpBytesToRead;
		if (tmpBytesToRead) ch <<= 6;
	    } while (tmpBytesToRead > 0);
	}
    In UTF-8 writing code, the switches on "bytesToWrite" are
    similarly unrolled loops.

   --------------------------------------------------------------------- */


};


