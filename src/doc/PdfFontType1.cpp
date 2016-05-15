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

#include "PdfFontType1.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfName.h"
#include "base/PdfStream.h"

#include "PdfDifferenceEncoding.h"

#include <stdlib.h>

namespace PoDoFo {

PdfFontType1::PdfFontType1( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                            PdfVecObjects* pParent, bool bEmbed, bool bSubsetting )
    : PdfFontSimple( pMetrics, pEncoding, pParent )
{
	memset( m_bUsed, 0, sizeof( m_bUsed ) );
	m_bIsSubsetting = bSubsetting;
    this->Init( bEmbed, PdfName("Type1") );
}


PdfFontType1::PdfFontType1( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                            PdfObject* pObject )
    : PdfFontSimple( pMetrics, pEncoding, pObject )
{
	memset( m_bUsed, 0, sizeof( m_bUsed ) );
}

PdfFontType1::PdfFontType1( PdfFontType1* pFont, PdfFontMetrics* pMetrics, const char *pszSuffix, PdfVecObjects* pParent )
    : PdfFontSimple( pMetrics, pFont->m_pEncoding, pParent )
{
	memset( m_bUsed, 0, sizeof( m_bUsed ) );
	// don't embedd font
	Init( false, PdfName("Type1") );

	// Use identical subset-names
	if ( pFont->IsSubsetting() )
	  GetObject()->GetDictionary().AddKey( "BaseFont", pFont->GetObject()->GetDictionary().GetKey( "BaseFont" ) );

	// set identifier
	std::string id = pFont->GetIdentifier().GetName();
	id += pszSuffix;
	m_Identifier = id;

	// remove new FontDescriptor and use FontDescriptor of source font instead
	PdfObject* pObj = pParent->RemoveObject( GetObject()->GetIndirectKey( "FontDescriptor" )->Reference() );
	delete pObj;
	GetObject()->GetDictionary().AddKey( "FontDescriptor", pFont->GetObject()->GetDictionary().GetKey( "FontDescriptor" ) );
}

void PdfFontType1::AddUsedSubsettingGlyphs( const PdfString & sText, long lStringLen )
{
	if ( m_bIsSubsetting )
	{
		// TODO: Unicode and Hex not yet supported
		PODOFO_ASSERT( sText.IsUnicode() == false );
		PODOFO_ASSERT( sText.IsHex() == false );
		const unsigned char* strp = reinterpret_cast<const unsigned char *>(sText.GetString());	// must be unsigned for access to m_bUsed-array
		for ( int i = 0; i < lStringLen; i++ )
		{
			m_bUsed[strp[i] / 32] |= 1 << (strp[i] % 32 ); 
		}
	}
}

void PdfFontType1::AddUsedGlyphname( const char * sGlyphName )
{
	if ( m_bIsSubsetting )
    {
		m_sUsedGlyph.insert( sGlyphName ); 
    }
}

void PdfFontType1::EmbedSubsetFont()
{
	if ( !m_bIsSubsetting  ||  m_bWasEmbedded == true )
		return;

    pdf_long    lSize    = 0;
    pdf_int64   lLength1 = 0L;
    pdf_int64   lLength2 = 0L;
    pdf_int64   lLength3 = 0L;
    PdfObject*  pContents;
    const char* pBuffer;
    char*       pAllocated = NULL;
    int         i;

    m_bWasEmbedded = true;

    pContents = this->GetObject()->GetOwner()->CreateObject();
    if( !pContents )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
        
    m_pDescriptor->GetDictionary().AddKey( "FontFile", pContents->Reference() );

    // if the data was loaded from memory - use it from there
    // otherwise, load from disk
    if ( m_pMetrics->GetFontDataLen() && m_pMetrics->GetFontData() ) 
    {
        pBuffer = m_pMetrics->GetFontData();
        lSize   = m_pMetrics->GetFontDataLen();
    }
    else
    {
        FILE* hFile = fopen( m_pMetrics->GetFilename(), "rb" );
        if( !hFile )
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, m_pMetrics->GetFilename() );
        }

        fseek( hFile, 0L, SEEK_END );
        lSize = ftell( hFile );
        fseek( hFile, 0L, SEEK_SET );

        pAllocated = static_cast<char*>(podofo_calloc( lSize, sizeof(char) ));
        if( !pAllocated )
        {
            fclose( hFile );
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }

        fread( pAllocated, sizeof(char), lSize, hFile );
        fclose( hFile );

        pBuffer = pAllocated;
    }

	// Allocate buffer for subsetted font, worst case size is input size
	unsigned char * outBuff = new unsigned char[lSize];
	int outIndex = 0;

	// unsigned to make comparisons work
	unsigned const char * inBuff = reinterpret_cast<unsigned const char*>(pBuffer);
	int inIndex = 0;

	// 6-Byte binary header for leading ascii-part
	PODOFO_ASSERT( inBuff[inIndex + 0] == 0x80 );
	PODOFO_ASSERT( inBuff[inIndex + 1] == 0x01 );
	int length = inBuff[inIndex + 2] + 
        (inBuff[inIndex + 3] << 8) + 
        (inBuff[inIndex + 4] << 16) + 
        (inBuff[inIndex + 5] << 24);				// little endian
	inIndex += 6;
	
	PODOFO_ASSERT( memcmp( &inBuff[inIndex], "%!PS-AdobeFont-1.", 17 ) == 0 );

	// transfer ascii-part, modify encoding dictionary (dup ...), if present
	std::string line;
	bool dupFound = false;
	for ( i = 0; i < length; i++ )
	{
		line += static_cast<char>( inBuff[inIndex+i] );
		if ( inBuff[inIndex+i] == '\r' )
		{
			if ( line.find( "dup " ) != 0 )
			{
				memcpy( &outBuff[outIndex], line.c_str(), line.length() );
				outIndex += line.length();
			}
			else
			{
				if ( dupFound == false )
				{
					// if first found, replace with new dictionary according to used glyphs
					// ignore further dup's
					for ( int i = 0; i < 256; i++ )
					{
						if ( (m_bUsed[i / 32] & (1 << (i % 32 ))) != 0 )
						{
							outIndex += sprintf( reinterpret_cast<char *>( &outBuff[outIndex] ), 
												 "dup %d /%s put\r", 
												 i, 
												 PdfDifferenceEncoding::UnicodeIDToName( GetEncoding()->GetCharCode(i) ).GetName().c_str() );
						}
					}
					dupFound = true;
				}
			}
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200 // Visual Studio 6
			line.erase();
#else
			line.clear();
#endif
		}
	}
	inIndex += length;
	lLength1 = outIndex;


	// 6-Byte binary header for binary-part
	PODOFO_ASSERT( inBuff[inIndex + 0] == 0x80 );
	PODOFO_ASSERT( inBuff[inIndex + 1] == 0x02 );
	length = inBuff[inIndex + 2] + 
        (inBuff[inIndex + 3] << 8) + 
        (inBuff[inIndex + 4] << 16) + 
        (inBuff[inIndex + 5] << 24);				// little endian
	inIndex += 6;

	// copy binary using encrpytion
	int outIndexStart = outIndex;
	bool foundSeacGlyph;

	// if glyph contains seac-command, add the used base-glyphs and loop again
	do
	{
		PdfType1EncryptEexec inCrypt;
		
		outIndex = outIndexStart;
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200 // Visual Studio 6
		line.erase();
#else
		line.clear();
#endif
		foundSeacGlyph = false;
		bool inCharString = false;
		for ( int i = 0; i < length;  )
		{
			unsigned char plain = inCrypt.Decrypt( inBuff[inIndex+i] );
			i++;

			line += static_cast<char>(plain);

			// output is ssssbuild uncrypted, as parts might be skipped and cipher-engine must be unchanged
			if ( inCharString && line.find( "/" ) == 0 )
			{
				// we are now inside a glyph, copy anything until RD or -| to output,
				// in case this glyph will be skipped we go back to saveOutIndex
				int outIndexSave = outIndex;

				outBuff[outIndex++] = plain;
				while ( line.find( "RD " ) == static_cast<size_t>(-1) 
                        && line.find( "-| " ) == static_cast<size_t>(-1) )
				{
					plain = inCrypt.Decrypt( inBuff[inIndex+i] );
					outBuff[outIndex++] = plain;
					line += static_cast<char>(plain);
					i++;
				}

				// parse line for name and length of glyph
				char *glyphName = new char[line.length()];
				int glyphLen;
				int result;
				if ( line.find( "RD " ) != static_cast<size_t>(-1) ) 
                {
					result = sscanf( line.c_str(), "%s %d RD ", glyphName, &glyphLen );
                }
				else
                {
					result = sscanf( line.c_str(), "%s %d -| ", glyphName, &glyphLen );
                }
				PODOFO_ASSERT( result == 2);

				bool useGlyph = false;
				// determine if this glyph is used in normal chars
				for ( int code = 0; code <= 255; code++ )
				{
					if ( 
						(m_bUsed[code / 32] & (1 << (code % 32 ))) != 0	&&
						strcmp( glyphName+1, PdfDifferenceEncoding::UnicodeIDToName( GetEncoding()->GetCharCode(code) ).GetName().c_str() ) == 0
                        )
					{
						useGlyph = true;
						break;
					}
				}

				// determine if this glyph is used in special chars
				if ( m_sUsedGlyph.find( glyphName+1 ) != m_sUsedGlyph.end() )
					useGlyph = true;

				// always use .notdef
				if ( strcmp( glyphName, "/.notdef" ) == 0 )
					useGlyph = true;


				// transfer glyph to output
				for ( int j = 0; j < glyphLen; j++, i++ )
					outBuff[outIndex++] = inCrypt.Decrypt( inBuff[inIndex+i] );

				// check used glyph for seac-command
				if ( useGlyph  &&  FindSeac( &outBuff[outIndex - glyphLen], glyphLen ) )
					foundSeacGlyph = true;

				delete[] glyphName;

				// transfer rest until end of line to output
				do
				{
					plain = inCrypt.Decrypt( inBuff[inIndex+i] );
					outBuff[outIndex++] = plain;
					line += static_cast<char>(plain);
					i++;
				} while ( plain != '\r'  &&  plain != '\n' );

				if ( useGlyph == false )
				{
					// glyph is not used, go back to saved position
					outIndex = outIndexSave;
				}
			}
			else
			{
				// copy anything outside glyph to output
				outBuff[outIndex++] = plain;
			}

			if ( plain == '\r' ||  plain == '\n' )
			{
				// parse for /CharStrings = begin of glyphs
				if ( line.find( "/CharStrings" ) != static_cast<size_t>(-1) )
					inCharString = true;
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200 // Visual Studio 6
				line.erase();
#else
				line.clear();
#endif
			}
		}
	} while ( foundSeacGlyph );

	// now encrypt resulting output-buffer
	PdfType1EncryptEexec outCrypt;
	for ( i = outIndexStart; i < outIndex; i++ )
		outBuff[i] = outCrypt.Encrypt( outBuff[i] );

	lLength2 = outIndex - outIndexStart;
	inIndex += length;


	// 6-Byte binary header for ascii-part
	PODOFO_ASSERT( inBuff[inIndex + 0] == 0x80 );
	PODOFO_ASSERT( inBuff[inIndex + 1] == 0x01 );
	length = inBuff[inIndex + 2] + 
        (inBuff[inIndex + 3] << 8) + 
        (inBuff[inIndex + 4] << 16) + 
        (inBuff[inIndex + 5] << 24);				// little endian
	inIndex += 6;

	// copy ascii
	memcpy( &outBuff[outIndex], &inBuff[inIndex], length );
	lLength3 = length;
	inIndex += length;
	outIndex += length;

	// now embed
	pContents->GetStream()->Set( reinterpret_cast<const char *>(outBuff), outIndex );

	// cleanup memory
    if( pAllocated )
        podofo_free( pAllocated );
    delete[] outBuff;

	// enter length in dictionary
    pContents->GetDictionary().AddKey( "Length1", PdfVariant( lLength1 ) );
    pContents->GetDictionary().AddKey( "Length2", PdfVariant( lLength2 ) );
    pContents->GetDictionary().AddKey( "Length3", PdfVariant( lLength3 ) );
}

void PdfFontType1::EmbedFontFile( PdfObject* pDescriptor )
{
    pdf_long    lSize    = 0;
    pdf_int64   lLength1 = 0L;
    pdf_int64   lLength2 = 0L;
    pdf_int64   lLength3 = 0L;
    PdfObject*  pContents;
    const char* pBuffer;
    char*       pAllocated = NULL;

	if (m_isBase14) 
	{
		m_bWasEmbedded = false;
		return;
	}

    m_bWasEmbedded = true;

    pContents = this->GetObject()->GetOwner()->CreateObject();
    if( !pContents )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
        
    pDescriptor->GetDictionary().AddKey( "FontFile", pContents->Reference() );

    // if the data was loaded from memory - use it from there
    // otherwise, load from disk
    if ( m_pMetrics->GetFontDataLen() && m_pMetrics->GetFontData() ) 
    {
        pBuffer = m_pMetrics->GetFontData();
        lSize   = m_pMetrics->GetFontDataLen();
    }
    else
    {
        FILE* hFile = fopen( m_pMetrics->GetFilename(), "rb" );
        if( !hFile )
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, m_pMetrics->GetFilename() );
        }

        fseek( hFile, 0L, SEEK_END );
        lSize = ftell( hFile );
        fseek( hFile, 0L, SEEK_SET );

        pAllocated = static_cast<char*>(podofo_calloc( lSize, sizeof(char) ));
        if( !pAllocated )
        {
            fclose( hFile );
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }

        fread( pAllocated, sizeof(char), lSize, hFile );
        fclose( hFile );

        pBuffer = pAllocated;
    }

	// Remove binary segment headers from pfb
	unsigned char *pBinary = reinterpret_cast<unsigned char*>(const_cast<char*>(pBuffer));
	while( *pBinary == 0x80 )	// binary segment header
	{
		const int	cHeaderLength  = 6;
		int			iSegmentType   = pBinary[1];	// binary segment type
		long		lSegmentLength = 0L;
		long		lSegmentDelta  = static_cast<long>(&pBuffer[lSize] - reinterpret_cast<const char*>(pBinary) );

		switch( iSegmentType )
		{
			case 1:									// ASCII text
				lSegmentLength = pBinary[2] + 		// little endian
								 pBinary[3] * 256L + 
								 pBinary[4] * 65536L +
								 pBinary[5] * 16777216L;
				if( lLength1 == 0L )
					lLength1 = lSegmentLength;
				else
					lLength3 = lSegmentLength;
				lSize -= cHeaderLength;
				memmove( pBinary, &pBinary[cHeaderLength], lSegmentDelta );
				pBinary = &pBinary[lSegmentLength];
				break;
			case 2:									// binary data
				lSegmentLength = pBinary[2] + 		// little endian
								 pBinary[3] * 256L + 
								 pBinary[4] * 65536L +
								 pBinary[5] * 16777216L;
				lLength2 = lSegmentLength;
				lSize -= cHeaderLength;
				memmove( pBinary, &pBinary[cHeaderLength], lSegmentDelta );
				pBinary = &pBinary[lSegmentLength];
				break;
			case 3:									// end-of-file
			  // First set pContents keys before writing stream, so that PdfTFontType1 works with streamed document
			        pContents->GetDictionary().AddKey( "Length1", PdfVariant( lLength1 ) );
				pContents->GetDictionary().AddKey( "Length2", PdfVariant( lLength2 ) );
				pContents->GetDictionary().AddKey( "Length3", PdfVariant( lLength3 ) );

				pContents->GetStream()->Set( pBuffer, lSize - 2L );
				if( pAllocated )
					podofo_free( pAllocated );

				return;
			default:
				break;
		}
	}

	// Parse the font data buffer to get the values for length1, length2 and length3
	lLength1 = FindInBuffer( "eexec", pBuffer, lSize );
	if( lLength1 > 0 )
		lLength1 += 6; // 6 == eexec + lf
	else
		lLength1 = 0;

	if( lLength1 )
	{
		lLength2 = FindInBuffer( "cleartomark", pBuffer, lSize );
		if( lLength2 > 0 )
			lLength2 = lSize - lLength1 - 520; // 520 == 512 + strlen(cleartomark)
		else
			lLength1 = 0;
	}

	lLength3 = lSize - lLength2 - lLength1;
    
	// TODO: Pdf Supports only Type1 fonts with binary encrypted sections and not the hex format
	pContents->GetStream()->Set( pBuffer, lSize );
    if( pAllocated )
        podofo_free( pAllocated );

    pContents->GetDictionary().AddKey( "Length1", PdfVariant( lLength1 ) );
    pContents->GetDictionary().AddKey( "Length2", PdfVariant( lLength2 ) );
    pContents->GetDictionary().AddKey( "Length3", PdfVariant( lLength3 ) );
}

bool PdfFontType1::FindSeac( const unsigned char * buffer, int length )
{
	PdfType1EncryptCharstring crypt;
	const PdfEncoding * stdEncoding = PdfEncodingFactory::GlobalStandardEncodingInstance();

	bool foundNewGlyph = false;
	int code1 = 0;
	int code2 = 0;
	for ( int j = 0; j < length; )
	{
		unsigned char plain = crypt.Decrypt( buffer[j++] );

		if ( j <= 4 )
		{
			// skip first 4 bytes
		}
		else if ( plain < 32 )
		{
			// decode commands
			switch ( plain )
			{
				case 1:		// hstem
				case 3:		// vstem
				case 4:		// rmoveto
				case 5:		// rlineto
				case 6:		// hlineto
				case 7:		// vlineto
				case 8:		// rrcurveto
				case 9:		// closepath
				case 10:	// callsubr
				case 11:	// return
				break;

				case 12:	// escape
				{
					plain = crypt.Decrypt( buffer[j++] );
					switch ( plain )
					{
						case 0:		// dotsection
						case 1:		// vstem3
						case 2:		// hstem3
						break;

						case 6:		// seac
						{
							// found seac command, use acquired code1 and code2 to get glyphname in standard-encoding
							std::string name;
							name = PdfDifferenceEncoding::UnicodeIDToName( stdEncoding->GetCharCode(code1) ).GetName().c_str();
							if ( m_sUsedGlyph.find( name ) == m_sUsedGlyph.end() )
							{
								// add new glyph
								m_sUsedGlyph.insert( name );
								foundNewGlyph = true;
							}

							name = PdfDifferenceEncoding::UnicodeIDToName( stdEncoding->GetCharCode(code2) ).GetName().c_str();
							if ( m_sUsedGlyph.find( name ) == m_sUsedGlyph.end() )
							{
								// add new glyph
								m_sUsedGlyph.insert( name );
								foundNewGlyph = true;
							}
						}
						break;

						case 7:		// sbw
						case 12:	// div
						case 16:	// callothersubr
						case 17:	// pop
						case 33:	// setcurrentpoint
						break;

						default:	// ???
						break;
					}
				}
				break;

				case 13:	// hsbw
				case 14:	// endchar
				case 21:	// rmoveto
				case 22:	// hmoveto
				case 30:	// vhcurveto
				case 31:	// hcurveto
				break;

				default:	// ???
				break;
			}
		}
		else if ( plain >= 32 ) // &&  plain <= 255 )
		{
			// this is a number
			int number = 0;
			if ( plain >= 32  &&  plain <= 246 )
			{
				number = static_cast<int>(plain-139);
			}
			else if ( plain >= 247  &&  plain <= 250 )
			{
				unsigned char next = crypt.Decrypt( buffer[j++] );

				number = (static_cast<int>(plain)-247)*256 + next + 108;
			}
			else if ( plain >= 251  &&  plain <= 254 )
			{
				unsigned char next = crypt.Decrypt( buffer[j++] );

				number = -((static_cast<int>(plain)-251)*256) - next - 108;
			}
			else if ( plain == 255 )
			{
				unsigned char next1 = crypt.Decrypt( buffer[j++] );
				unsigned char next2 = crypt.Decrypt( buffer[j++] );
				unsigned char next3 = crypt.Decrypt( buffer[j++] );
				unsigned char next4 = crypt.Decrypt( buffer[j++] );

				number = (static_cast<int>(next1) << 24)
                    + (static_cast<int>(next2) << 16)
                    + (static_cast<int>(next3) << 8)
                    + next4 ;
			}

			char num[32];
			sprintf( num, "%d ", number );

			code1 = code2;
			code2 = number;
		}
	}
	return foundNewGlyph;
}

pdf_long PdfFontType1::FindInBuffer( const char* pszNeedle, const char* pszHaystack, pdf_long lLen ) const
{
    // if lNeedleLen is 0 the while loop will not be executed and we return -1
    pdf_long lNeedleLen      = pszNeedle ? strlen( pszNeedle ) : 0; 
    const char* pszEnd   = pszHaystack + lLen - lNeedleLen; 
    const char* pszStart = pszHaystack;

    if ( pszNeedle )
    {
        while( pszHaystack < pszEnd )
        {
            if( strncmp( pszHaystack, pszNeedle, lNeedleLen ) == 0 )
                return pszHaystack - pszStart;

            ++pszHaystack;
        }
    }
    
    return -1;
}

PdfType1EncryptEexec::PdfType1EncryptEexec() 
	: PdfType1Encrypt()
{
	m_r = 55665;
}
 
PdfType1EncryptCharstring::PdfType1EncryptCharstring() 
    : PdfType1Encrypt()
{
 	m_r = 4330;
}

PdfType1Encrypt::PdfType1Encrypt()
    : m_r( -1 ) // will be initialized in subclasses with real value
{
	m_c1 = 52845;
	m_c2 = 22719;
}

unsigned char PdfType1Encrypt::Encrypt( unsigned char plain )
{
	unsigned char cipher;
	cipher = (plain ^ (m_r >> 8));
	m_r = ((cipher + m_r) * m_c1 + m_c2) & ((1<<16) -1);

	return cipher;
}

unsigned char PdfType1Encrypt::Decrypt( unsigned char cipher )
{
	unsigned char plain;
	plain = (cipher ^ (m_r >> 8));
	m_r = (cipher + m_r) * m_c1 + m_c2;

	return plain;
}

};

