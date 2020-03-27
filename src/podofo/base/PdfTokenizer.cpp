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

#include "PdfTokenizer.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfName.h"
#include "PdfString.h"
#include "PdfReference.h"
#include "PdfVariant.h"
#include "PdfDefinesPrivate.h"

#include <limits>
#include <sstream>
#include <memory>

#include <stdlib.h>
#include <string.h>

#define PDF_BUFFER 4096

#define DICT_SEP_LENGTH 2
#define NULL_LENGTH     4
#define TRUE_LENGTH     4
#define FALSE_LENGTH    5

namespace PoDoFo {

namespace PdfTokenizerNameSpace{

static const int g_MapAllocLen = 256;
static char g_DelMap[g_MapAllocLen] = { 0 };
static char g_WsMap[g_MapAllocLen] = { 0 };
static char g_EscMap[g_MapAllocLen] = { 0 };
static char g_hexMap[g_MapAllocLen] = { 0 };

// Generate the delimiter character map at runtime
// so that it can be derived from the more easily
// maintainable structures in PdfDefines.h
const char * genDelMap()
{
    char* map = static_cast<char*>(g_DelMap);
    memset( map, 0, sizeof(char) * g_MapAllocLen );
    for (int i = 0; i < PoDoFo::s_nNumDelimiters; ++i)
    {
        map[static_cast<int>(PoDoFo::s_cDelimiters[i])] = 1;
    }

    return map;
}

// Generate the whitespace character map at runtime
// so that it can be derived from the more easily
// maintainable structures in PdfDefines.h
const char * genWsMap()
{
    char* map = static_cast<char*>(g_WsMap);
    memset( map, 0, sizeof(char) * g_MapAllocLen );
    for (int i = 0; i < PoDoFo::s_nNumWhiteSpaces; ++i)
    {
        map[static_cast<int>(PoDoFo::s_cWhiteSpaces[i])] = 1;
    }
    return map;
}

// Generate the escape character map at runtime
const char* genEscMap()
{
    char* map = static_cast<char*>(g_EscMap);
    memset( map, 0, sizeof(char) * g_MapAllocLen );

    map[static_cast<unsigned char>('n')] = '\n'; // Line feed (LF)
    map[static_cast<unsigned char>('r')] = '\r'; // Carriage return (CR)
    map[static_cast<unsigned char>('t')] = '\t'; // Horizontal tab (HT)
    map[static_cast<unsigned char>('b')] = '\b'; // Backspace (BS)
    map[static_cast<unsigned char>('f')] = '\f'; // Form feed (FF)
    map[static_cast<unsigned char>(')')] = ')';
    map[static_cast<unsigned char>('(')] = '(';
    map[static_cast<unsigned char>('\\')] = '\\';

    return map;
}

// Generate the hex character map at runtime
const char* genHexMap()
{
    char* map = static_cast<char*>(g_hexMap);
    memset( map, PdfTokenizer::HEX_NOT_FOUND, sizeof(char) * g_MapAllocLen );

    map[static_cast<unsigned char>('0')] = 0x0;
    map[static_cast<unsigned char>('1')] = 0x1;
    map[static_cast<unsigned char>('2')] = 0x2;
    map[static_cast<unsigned char>('3')] = 0x3;
    map[static_cast<unsigned char>('4')] = 0x4;
    map[static_cast<unsigned char>('5')] = 0x5;
    map[static_cast<unsigned char>('6')] = 0x6;
    map[static_cast<unsigned char>('7')] = 0x7;
    map[static_cast<unsigned char>('8')] = 0x8;
    map[static_cast<unsigned char>('9')] = 0x9;
    map[static_cast<unsigned char>('a')] = 0xA;
    map[static_cast<unsigned char>('b')] = 0xB;
    map[static_cast<unsigned char>('c')] = 0xC;
    map[static_cast<unsigned char>('d')] = 0xD;
    map[static_cast<unsigned char>('e')] = 0xE;
    map[static_cast<unsigned char>('f')] = 0xF;
    map[static_cast<unsigned char>('A')] = 0xA;
    map[static_cast<unsigned char>('B')] = 0xB;
    map[static_cast<unsigned char>('C')] = 0xC;
    map[static_cast<unsigned char>('D')] = 0xD;
    map[static_cast<unsigned char>('E')] = 0xE;
    map[static_cast<unsigned char>('F')] = 0xF;

    return map;
}

};

const unsigned int PdfTokenizer::HEX_NOT_FOUND   = std::numeric_limits<unsigned int>::max();
const char * const PdfTokenizer::s_delimiterMap  = PdfTokenizerNameSpace::genDelMap();
const char * const PdfTokenizer::s_whitespaceMap = PdfTokenizerNameSpace::genWsMap();
const char * const PdfTokenizer::s_escMap        = PdfTokenizerNameSpace::genEscMap();
const char * const PdfTokenizer::s_hexMap        = PdfTokenizerNameSpace::genHexMap();

const char PdfTokenizer::s_octMap[]        = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0
};

PdfTokenizer::PdfTokenizer()
    : m_buffer( PDF_BUFFER )
{
    PdfLocaleImbue(m_doubleParser);
}

PdfTokenizer::PdfTokenizer( const char* pBuffer, size_t lLen )
    : m_device( pBuffer, lLen ), m_buffer( PDF_BUFFER )
{
    PdfLocaleImbue(m_doubleParser);
}

PdfTokenizer::PdfTokenizer( const PdfRefCountedInputDevice & rDevice, const PdfRefCountedBuffer & rBuffer )
    : m_device( rDevice ), m_buffer( rBuffer )
{
    PdfLocaleImbue(m_doubleParser);
}

PdfTokenizer::~PdfTokenizer()
{
}

bool PdfTokenizer::GetNextToken( const char*& pszToken , EPdfTokenType* peType )
{
    int  c;
    pdf_int64  counter  = 0;

    // check first if there are queued tokens and return them first
    if( m_deqQueque.size() )
    {
        TTokenizerPair pair = m_deqQueque.front();
        m_deqQueque.pop_front();

        if( peType )
            *peType = pair.second;

        if ( !m_buffer.GetBuffer() || m_buffer.GetSize() == 0)
        {
            PODOFO_RAISE_ERROR(ePdfError_InvalidHandle);
        }

        // make sure buffer is \0 terminated
        strncpy(m_buffer.GetBuffer(), pair.first.c_str(), m_buffer.GetSize());
        m_buffer.GetBuffer()[m_buffer.GetSize() - 1] = 0;
        pszToken = m_buffer.GetBuffer();
        return true;
    }

    if( !m_device.Device() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( peType )
        *peType = ePdfTokenType_Token;

    while( (c = m_device.Device()->Look()) != EOF
           && counter + 1 < static_cast<pdf_int64>(m_buffer.GetSize()) )
    {
        // ignore leading whitespaces
        if( !counter && IsWhitespace( c ) )
        {
            // Consume the whitespace character
            c = m_device.Device()->GetChar();
            continue;
        }
        // ignore comments
        else if( c == '%' )
        {
            // Consume all characters before the next line break
			// 2011-04-19 Ulrich Arnold: accept 0x0D, 0x0A and oX0D 0x0A as one EOL
            do {
                c = m_device.Device()->GetChar();
            } while( c != EOF && c != 0x0D  && c != 0x0A );

            if ( c == 0x0D )
			{
                if ( m_device.Device()->Look() == 0x0A )
	                c = m_device.Device()->GetChar();
			}
            // If we've already read one or more chars of a token, return them, since
            // comments are treated as token-delimiting whitespace. Otherwise keep reading
            // at the start of the next line.
            if (counter)
                break;
        }
        // special handling for << and >> tokens
        else if( !counter && (c == '<' || c == '>' ) )
        {
            if( peType )
                *peType = ePdfTokenType_Delimiter;

            // retrieve c really from stream
            c = m_device.Device()->GetChar();
            m_buffer.GetBuffer()[counter] = c;
            ++counter;

            char n = m_device.Device()->Look();
            // Is n another < or > , ie are we opening/closing a dictionary?
            // If so, consume that character too.
            if( n == c )
            {
                n = m_device.Device()->GetChar();
                m_buffer.GetBuffer()[counter] = n;
                ++counter;
            }
            // `m_buffer' contains one of < , > , << or >> ; we're done .
            break;
        }
        else if( counter && (IsWhitespace( c ) || IsDelimiter( c )) )
        {
            // Next (unconsumed) character is a token-terminating char, so
            // we have a complete token and can return it.
            break;
        }
        else
        {
            // Consume the next character and add it to the token we're building.
            c = m_device.Device()->GetChar();
            m_buffer.GetBuffer()[counter] = c;
            ++counter;

            if( IsDelimiter( c ) )
            {
                // All delimeters except << and >> (handled above) are
                // one-character tokens, so if we hit one we can just return it
                // immediately.
                if( peType )
                    *peType = ePdfTokenType_Delimiter;
                break;
            }
        }
    }

    m_buffer.GetBuffer()[counter] = '\0';

    if( c == EOF && !counter )
    {
        // No characters were read before EOF, so we're out of data.
        // Ensure the buffer points to NULL in case someone fails to check the return value.
        pszToken = 0;
        return false;
    }

    pszToken = m_buffer.GetBuffer();
    return true;
}

bool PdfTokenizer::IsNextToken( const char* pszToken )
{
    if( !pszToken )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    const char* pszRead;
    bool gotToken = this->GetNextToken( pszRead, NULL );

    if (!gotToken)
    {
        PODOFO_RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    return (strcmp( pszToken, pszRead ) == 0);
}

pdf_long PdfTokenizer::GetNextNumber()
{
    EPdfTokenType eType;
    const char* pszRead;
    bool gotToken = this->GetNextToken( pszRead, &eType );

    if( !gotToken )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_UnexpectedEOF, "Expected number" );
    }

    char* end;
#ifdef _WIN64
    pdf_long l = _strtoui64( pszRead, &end, 10 );
#else
    pdf_long l = strtol( pszRead, &end, 10 );
#endif
    if( end == pszRead )
    {
        // Don't consume the token
        this->QuequeToken( pszRead, eType );
        PODOFO_RAISE_ERROR_INFO( ePdfError_NoNumber, pszRead );
    }

    return l;
}

void PdfTokenizer::GetNextVariant( PdfVariant& rVariant, PdfEncrypt* pEncrypt )
{
   EPdfTokenType eTokenType;
   const char*   pszRead;
   bool gotToken = this->GetNextToken( pszRead, &eTokenType );

   if (!gotToken)
   {
       PODOFO_RAISE_ERROR_INFO( ePdfError_UnexpectedEOF, "Expected variant." );
   }

   this->GetNextVariant( pszRead, eTokenType, rVariant, pEncrypt );
}

void PdfTokenizer::GetNextVariant( const char* pszToken, EPdfTokenType eType, PdfVariant& rVariant, PdfEncrypt* pEncrypt )
{
    EPdfDataType eDataType = this->DetermineDataType( pszToken, eType, rVariant );

    if( eDataType == ePdfDataType_Null ||
        eDataType == ePdfDataType_Bool ||
        eDataType == ePdfDataType_Number ||
        eDataType == ePdfDataType_Real ||
        eDataType == ePdfDataType_Reference )
    {
        // the data was already read into rVariant by the DetermineDataType function
        return;
    }

    this->ReadDataType( eDataType, rVariant, pEncrypt );
}

EPdfDataType PdfTokenizer::DetermineDataType( const char* pszToken, EPdfTokenType eTokenType, PdfVariant& rVariant )
{
    if( eTokenType == ePdfTokenType_Token )
    {
        // check for the two special datatypes
        // null and boolean.
        // check for numbers
        if( strncmp( "null", pszToken, NULL_LENGTH ) == 0 )
        {
            rVariant = PdfVariant();
            return ePdfDataType_Null;
        }
        else if( strncmp( "true", pszToken, TRUE_LENGTH ) == 0 )
        {
            rVariant = PdfVariant( true );
            return ePdfDataType_Bool;
        }
        else if( strncmp( "false", pszToken, FALSE_LENGTH ) == 0 )
        {
            rVariant = PdfVariant( false );
            return ePdfDataType_Bool;
        }

        EPdfDataType eDataType = ePdfDataType_Number;
        const char*  pszStart  = pszToken;

        while( *pszStart )
        {
            if( *pszStart == '.' )
                eDataType = ePdfDataType_Real;
            else if( !(isdigit( static_cast<const unsigned char>(*pszStart) ) || *pszStart == '-' || *pszStart == '+' ) )
            {
                eDataType = ePdfDataType_Unknown;
                break;
            }

            ++pszStart;
        }

        if( eDataType == ePdfDataType_Real )
        {
            // DOM: strtod is locale dependend,
            //      do not use it
            //double dVal = strtod( pszToken, NULL );
            double dVal;

            m_doubleParser.clear(); // clear error state
            m_doubleParser.str( pszToken );
            if( !(m_doubleParser >> dVal) )
            {
                m_doubleParser.clear(); // clear error state
                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, pszToken );
            }

            rVariant = PdfVariant( dVal );
            return ePdfDataType_Real;
        }
        else if( eDataType == ePdfDataType_Number )
        {
#ifdef _WIN64
            rVariant = PdfVariant( static_cast<pdf_int64>(_strtoui64( pszToken, NULL, 10 )) );
#else
            rVariant = PdfVariant( static_cast<pdf_int64>(strtol( pszToken, NULL, 10 )) );
#endif
            // read another two tokens to see if it is a reference
            // we cannot be sure that there is another token
            // on the input device, so if we hit EOF just return
            // ePdfDataType_Number .
            EPdfTokenType eSecondTokenType;
            bool gotToken = this->GetNextToken( pszToken, &eSecondTokenType );
            if (!gotToken)
                // No next token, so it can't be a reference
                return eDataType;
            if( eSecondTokenType != ePdfTokenType_Token )
            {
                this->QuequeToken( pszToken, eSecondTokenType );
                return eDataType;
            }


            pszStart = pszToken;
#ifdef _WIN64
            pdf_long  l   = _strtoui64( pszStart, const_cast<char**>(&pszToken), 10 );
#else
            long  l   = strtol( pszStart, const_cast<char**>(&pszToken), 10 );
#endif
            if( pszToken == pszStart )
            {
                this->QuequeToken( pszStart, eSecondTokenType );
                return eDataType;
            }

            std::string backup( pszStart );
            EPdfTokenType eThirdTokenType;
            gotToken = this->GetNextToken( pszToken, &eThirdTokenType );
            if (!gotToken)
                // No third token, so it can't be a reference
                return eDataType;
            if( eThirdTokenType == ePdfTokenType_Token &&
                pszToken[0] == 'R' && pszToken[1] == '\0' )
            {
                rVariant = PdfReference( static_cast<unsigned int>(rVariant.GetNumber()),
                                         static_cast<const pdf_uint16>(l) );
                return ePdfDataType_Reference;
            }
            else
            {
                this->QuequeToken( backup.c_str(), eSecondTokenType );
                this->QuequeToken( pszToken, eThirdTokenType );
                return eDataType;
            }
        }
    }
    else if( eTokenType == ePdfTokenType_Delimiter )
    {
        if( strncmp( "<<", pszToken, DICT_SEP_LENGTH ) == 0 )
            return ePdfDataType_Dictionary;
        else if( pszToken[0] == '[' )
            return ePdfDataType_Array;
        else if( pszToken[0] == '(' )
            return ePdfDataType_String;
        else if( pszToken[0] == '<' )
            return ePdfDataType_HexString;
        else if( pszToken[0] == '/' )
            return ePdfDataType_Name;
    }

    if( false )
    {
        std::ostringstream ss;
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200
        ss << "Got unexpected PDF data in" << __FILE__ << ", line " << __LINE__
#else
        ss << "Got unexpected PDF data in" << PODOFO__FUNCTION__
#endif
           << ": \""
           << pszToken
           << "\". Current read offset is "
           << m_device.Device()->Tell()
           << " which should be around the problem.\n";
        PdfError::DebugMessage(ss.str().c_str());
    }

    return ePdfDataType_Unknown;
}

void PdfTokenizer::ReadDataType( EPdfDataType eDataType, PdfVariant& rVariant, PdfEncrypt* pEncrypt )
{
    switch( eDataType )
    {
        case ePdfDataType_Dictionary:
            this->ReadDictionary( rVariant, pEncrypt );
            break;
        case ePdfDataType_Array:
            this->ReadArray( rVariant, pEncrypt );
            break;
        case ePdfDataType_String:
            this->ReadString( rVariant, pEncrypt );
            break;
        case ePdfDataType_HexString:
            this->ReadHexString( rVariant, pEncrypt );
            break;
        case ePdfDataType_Name:
            this->ReadName( rVariant );
            break;

        // The following datatypes are not handled by read datatype
        // but are already parsed by DetermineDatatype
        case ePdfDataType_Null:
        case ePdfDataType_Bool:
        case ePdfDataType_Number:
        case ePdfDataType_Real:
        case ePdfDataType_Reference:
        case ePdfDataType_Unknown:
        case ePdfDataType_RawData:

        default:
        {
            PdfError::LogMessage( eLogSeverity_Debug, "Got Datatype: %i\n", eDataType );
            PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
        }
    }
}

void PdfTokenizer::ReadDictionary( PdfVariant& rVariant, PdfEncrypt* pEncrypt )
{
    PdfVariant    val;
    PdfName       key;
    PdfDictionary dict;
    EPdfTokenType eType;
    const char *  pszToken;
    PODOFO_UNIQUEU_PTR<std::vector<char> > contentsHexBuffer;

    for( ;; )
    {
        bool gotToken = this->GetNextToken( pszToken, &eType );
        if (!gotToken)
        {
            PODOFO_RAISE_ERROR_INFO(ePdfError_UnexpectedEOF, "Expected dictionary key name or >> delim.");
        }
        if( eType == ePdfTokenType_Delimiter && strncmp( ">>", pszToken, DICT_SEP_LENGTH ) == 0 )
            break;

        this->GetNextVariant( pszToken, eType, val, pEncrypt );
        // Convert the read variant to a name; throws InvalidDataType if not a name.
        key = val.GetName();

        // Try to get the next variant
        gotToken = this->GetNextToken( pszToken, &eType );
        if ( !gotToken )
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_UnexpectedEOF, "Expected variant." );
        }

        EPdfDataType eDataType = this->DetermineDataType( pszToken, eType, val );
        if ( key == "Contents" && eDataType == ePdfDataType_HexString )
        {
            // 'Contents' key in signature dictionaries is an unencrypted Hex string:
            // save the string buffer for later check if it needed decryption
            contentsHexBuffer = PODOFO_UNIQUEU_PTR<std::vector<char> >( new std::vector<char>() );
            ReadHexString( *contentsHexBuffer );
            continue;
        }

        switch ( eDataType )
        {
            case ePdfDataType_Null:
            case ePdfDataType_Bool:
            case ePdfDataType_Number:
            case ePdfDataType_Real:
            case ePdfDataType_Reference:
            {
                // the data was already read into rVariant by the DetermineDataType function
                break;
            }
            case ePdfDataType_Name:
            case ePdfDataType_String:
            case ePdfDataType_HexString:
            case ePdfDataType_Array:
            case ePdfDataType_Dictionary:
            {
                this->ReadDataType( eDataType, val, pEncrypt );
                break;
            }
            case ePdfDataType_RawData:
            case ePdfDataType_Unknown:
            default:
            {
                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "Unexpected data type" );
            }
        }

        dict.AddKey( key, val );
    }

    if ( contentsHexBuffer.get() != NULL )
    {
        PdfObject *type = dict.GetKey( "Type" );
        // "Contents" is unencrypted in /Type/Sig and /Type/DocTimeStamp dictionaries 
        // https://issues.apache.org/jira/browse/PDFBOX-3173
        bool contentsUnencrypted = type != NULL && type->GetDataType() == ePdfDataType_Name &&
            (type->GetName() == PdfName( "Sig" ) || type->GetName() == PdfName( "DocTimeStamp" ));

        PdfEncrypt *encrypt = NULL;
        if ( !contentsUnencrypted )
            encrypt = pEncrypt;

        PdfString string;
        string.SetHexData( contentsHexBuffer->size() ? &(*contentsHexBuffer)[0] : "", contentsHexBuffer->size(), encrypt );

        val = string;
        dict.AddKey( "Contents", val );
    }

    rVariant = dict;
}

void PdfTokenizer::ReadArray( PdfVariant& rVariant, PdfEncrypt* pEncrypt )
{
    const char*   pszToken;
    EPdfTokenType eType;
    PdfVariant    var;
    PdfArray      array;

    for( ;; )
    {
        bool gotToken = this->GetNextToken( pszToken, &eType );
        if (!gotToken)
        {
            PODOFO_RAISE_ERROR_INFO(ePdfError_UnexpectedEOF, "Expected array item or ] delim.");
        }
        if( eType == ePdfTokenType_Delimiter && pszToken[0] == ']' )
            break;

        this->GetNextVariant( pszToken, eType, var, pEncrypt );
        array.push_back( var );
    }

    rVariant = array;
}

void PdfTokenizer::ReadString( PdfVariant& rVariant, PdfEncrypt* pEncrypt )
{
    int               c;

    bool              bEscape       = false;
    bool              bOctEscape    = false;
    int               nOctCount     = 0;
    char              cOctValue     = 0;
    int               nBalanceCount = 0; // Balanced parathesis do not have to be escaped in strings

    m_vecBuffer.clear();

    while( (c = m_device.Device()->Look()) != EOF )
    {
        // end of stream reached
        if( !bEscape )
        {
            // Handle raw characters
            c = m_device.Device()->GetChar();
            if( !nBalanceCount && c == ')' )
                break;

            if( c == '(' )
                ++nBalanceCount;
            else if( c == ')' )
                --nBalanceCount;

            bEscape = (c == '\\');
            if( !bEscape )
                m_vecBuffer.push_back( static_cast<char>(c) );
        }
        else
        {
            // Handle escape sequences
            if( bOctEscape || s_octMap[c & 0xff] )
                // The last character we have read was a '\\',
                // so we check now for a digit to find stuff like \005
                bOctEscape = true;

            if( bOctEscape )
            {
                // Handle octal escape sequences
                ++nOctCount;

                if( !s_octMap[c & 0xff] )
                {
                    // No octal character anymore,
                    // so the octal sequence must be ended
                    // and the character has to be treated as normal character!
                    m_vecBuffer.push_back ( cOctValue );
                    bEscape    = false;
                    bOctEscape = false;
                    nOctCount  = 0;
                    cOctValue  = 0;
                    continue;
                }

                c = m_device.Device()->GetChar();
                cOctValue <<= 3;
                cOctValue  |= ((c-'0') & 0x07);

                if( nOctCount > 2 )
                {
                    m_vecBuffer.push_back ( cOctValue );
                    bEscape    = false;
                    bOctEscape = false;
                    nOctCount  = 0;
                    cOctValue  = 0;
                }
            }
            else
            {
                // Handle plain escape sequences
                const char & code = s_escMap[m_device.Device()->GetChar() & 0xff];
                if( code )
                    m_vecBuffer.push_back( code );

                bEscape = false;
            }
        }
    }

    // In case the string ends with a octal escape sequence
    if( bOctEscape )
        m_vecBuffer.push_back ( cOctValue );

    if( m_vecBuffer.size() )
    {
        if( pEncrypt )
        {
            pdf_long outLen = m_vecBuffer.size() - pEncrypt->CalculateStreamOffset();
            char * outBuffer = new char[outLen + 16 - (outLen % 16)];
            pEncrypt->Decrypt( reinterpret_cast<unsigned char*>(&(m_vecBuffer[0])),
                              static_cast<unsigned int>(m_vecBuffer.size()),
                              reinterpret_cast<unsigned char*>(outBuffer), outLen);

            rVariant = PdfString( outBuffer, outLen );

            delete[] outBuffer;
        }
        else
        {
            rVariant = PdfString( &(m_vecBuffer[0]), m_vecBuffer.size() );
        }
    }
    else
    {
        rVariant = PdfString("");
    }
}

void PdfTokenizer::ReadHexString( PdfVariant& rVariant, PdfEncrypt* pEncrypt )
{
    ReadHexString( m_vecBuffer );

    PdfString string;
    string.SetHexData( m_vecBuffer.size() ? &(m_vecBuffer[0]) : "", m_vecBuffer.size(), pEncrypt );

    rVariant = string;
}

void PdfTokenizer::ReadHexString( std::vector<char>& rVecBuffer)
{
    rVecBuffer.clear();
    int        c;

    while( (c = m_device.Device()->GetChar()) != EOF )
    {
        // end of stream reached
        if( c == '>' )
            break;

        // only a hex digits
        if( isdigit( c ) ||
            ( c >= 'A' && c <= 'F') ||
            ( c >= 'a' && c <= 'f'))
            rVecBuffer.push_back( c );
    }

    // pad to an even length if necessary
    if(rVecBuffer.size() % 2 )
        rVecBuffer.push_back( '0' );
}

void PdfTokenizer::ReadName( PdfVariant& rVariant )
{
    EPdfTokenType eType;
    const char*   pszToken;

    // Do special checking for empty names
    // as GetNextToken will ignore white spaces
    // and we have to take care for stuff like:
    // 10 0 obj / endobj
    // which stupid but legal PDF
    int c = m_device.Device()->Look();
    if( IsWhitespace( c ) ) // Delimeters are handled correctly by GetNextToken
    {
        // We are an empty PdfName
        rVariant = PdfName();
        return;
    }

    bool gotToken = this->GetNextToken( pszToken, &eType );
    if( !gotToken || eType != ePdfTokenType_Token )
    {
        // We got an empty name which is legal according to the PDF specification
        // Some weird PDFs even use them.
        rVariant = PdfName();

        // Enqueue the token again
        if( gotToken )
            QuequeToken( pszToken, eType );
    }
    else
        rVariant = PdfName::FromEscaped( pszToken );
}

void PdfTokenizer::QuequeToken( const char* pszToken, EPdfTokenType eType )
{
    m_deqQueque.push_back( TTokenizerPair( std::string( pszToken ), eType ) );
}

};
