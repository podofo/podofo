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

#include "PdfTokenizer.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfInputDevice.h"
#include "PdfName.h"
#include "PdfString.h"
#include "PdfReference.h"
#include "PdfVariant.h"

#define PDF_BUFFER 4096

#define DICT_SEP_LENGTH 2
#define NULL_LENGTH     4
#define TRUE_LENGTH     4
#define FALSE_LENGTH    5
#define REF_LENGTH      1

namespace PoDoFo {

namespace PdfTokenizerNameSpace{

// Generate the delimiter character map at runtime
// so that it can be derived from the more easily
// maintainable structures in PdfDefines.h
const char * genDelMap()
{
    int    i;
    char* map = static_cast<char*>(malloc(256));
    for (i = 0; i < 256; i++)
        map[i] = '\0';
    for (i = 0; i < PoDoFo::s_nNumDelimiters; ++i)
        map[PoDoFo::s_cDelimiters[i]] = 1;
    return map;
}

// Generate the whitespace character map at runtime
// so that it can be derived from the more easily
// maintainable structures in PdfDefines.h
const char * genWsMap()
{
    int   i;
    char* map = static_cast<char*>(malloc(256));
    for (i = 0; i < 256; i++)
        map[i] = '\0';
    for (i = 0; i < PoDoFo::s_nNumWhiteSpaces; ++i)
        map[PoDoFo::s_cWhiteSpaces[i]] = 1;
    return map;
}


};

const char * const PdfTokenizer::m_delimiterMap = PdfTokenizerNameSpace::genDelMap();
const char * const PdfTokenizer::m_whitespaceMap = PdfTokenizerNameSpace::genWsMap();

PdfTokenizer::PdfTokenizer()
    : m_buffer( PDF_BUFFER )
{

}

PdfTokenizer::PdfTokenizer( const char* pBuffer, long lLen )
    : m_device( pBuffer, lLen ), m_buffer( PDF_BUFFER )
{

}

PdfTokenizer::PdfTokenizer( const PdfRefCountedInputDevice & rDevice, const PdfRefCountedBuffer & rBuffer )
    : m_device( rDevice ), m_buffer( rBuffer )
{
}

PdfTokenizer::~PdfTokenizer()
{
}

const char* PdfTokenizer::GetNextToken( EPdfTokenType* peType )
{
    int  c; 
    int  counter  = 0;

    // check first if there are quequed tokens and return them first
    if( m_deqQueque.size() )
    {
        TTokenizerPair pair = m_deqQueque.front();
        m_deqQueque.pop_front();

        if( peType )
            *peType = pair.second;

        strcpy( m_buffer.GetBuffer(), pair.first.c_str() );
        return m_buffer.GetBuffer();
    }
    
    if( !m_device.Device() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( peType )
        *peType = ePdfTokenType_Token;

    while( (c = m_device.Device()->Look()) != EOF && counter < m_buffer.GetSize() )
    {
        // ignore leading whitespaces
        if( !counter && IsWhitespace( c ) )
        {
            // retrieve c really from stream
            c = m_device.Device()->GetChar();
            continue;
        }
        // ignore comments
        else if( c == '%' ) 
        {
            // remove all characters before the next line break
            do {
                c = m_device.Device()->GetChar();
            } while( c != EOF && c != 0x0A );
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
            if( n == c ) 
            {
                n = m_device.Device()->GetChar();
                m_buffer.GetBuffer()[counter] = n;
                ++counter;
            }
            break;
        }
        else if( counter && (IsWhitespace( c ) || IsDelimiter( c )) )
        {
            // do nothing
            break;
        }
        else
        {
            // retrieve c really from stream
            c = m_device.Device()->GetChar();
            m_buffer.GetBuffer()[counter] = c;
            ++counter;

            if( IsDelimiter( c ) )
            {
                if( peType )
                    *peType = ePdfTokenType_Delimiter;

                break;
            }
        }
    }

    m_buffer.GetBuffer()[counter] = '\0';

    if( c == EOF && !counter )
    {
        PODOFO_RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    return m_buffer.GetBuffer();
}

bool PdfTokenizer::IsNextToken( const char* pszToken )
{
    const char* pszRead = this->GetNextToken( NULL );

    if( !pszRead || !pszToken )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    return (strcmp( pszToken, pszRead ) == 0);
}

long PdfTokenizer::GetNextNumber()
{
    EPdfTokenType eType;
    const char*   pszRead = this->GetNextToken( &eType );
    long          l;
    char*         end;

    if( !pszRead )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    l = strtol( pszRead, &end, 10 );
    if( end == pszRead )
    {
        this->QuequeToken( pszRead, eType );
        PODOFO_RAISE_ERROR( ePdfError_NoNumber );
    }

    return l;
}

void PdfTokenizer::GetNextVariant( PdfVariant& rVariant )
{
   EPdfTokenType eTokenType;
   const char*   pszRead = this->GetNextToken( &eTokenType );

   this->GetNextVariant( pszRead, eTokenType, rVariant );
}

void PdfTokenizer::GetNextVariant( const char* pszToken, EPdfTokenType eType, PdfVariant& rVariant )
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

    this->ReadDataType( eDataType, rVariant );

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
            else if( !(isdigit( *pszStart ) || *pszStart == '-' || *pszStart == '+' ) )
            {
                eDataType = ePdfDataType_Unknown;
                break;
            }
            
            ++pszStart;
        }

        if( eDataType == ePdfDataType_Real ) 
        {
            rVariant = PdfVariant( strtod( pszToken, NULL ) );
            return ePdfDataType_Real;
        }
        else if( eDataType == ePdfDataType_Number ) 
        {
            rVariant = PdfVariant( strtol( pszToken, NULL, 10 ) );

            // we cannot be sure that there is another datatype 
            // on the input device. So ignore EOF exceptions
            try {
                // read another two tokens to see if it is a reference
                EPdfTokenType eSecondTokenType;
                pszToken = this->GetNextToken( &eSecondTokenType );
                pszStart = pszToken;
                if( eSecondTokenType != ePdfTokenType_Token ) 
                {
                    this->QuequeToken( pszToken, eSecondTokenType );
                    return eDataType;
                }
                
                long  l   = strtol( pszStart, const_cast<char**>(&pszToken), 10 );
                if( pszToken == pszStart ) 
                {
                    this->QuequeToken( pszStart, eSecondTokenType );
                    return eDataType;
                }

                std::string backup( pszStart );
                EPdfTokenType eThirdTokenType;
                pszToken = this->GetNextToken( &eThirdTokenType );
                if( eThirdTokenType == ePdfTokenType_Token &&
                    pszToken[0] == 'R' )
                {
                    rVariant = PdfReference( rVariant.GetNumber(), l );
                    return ePdfDataType_Reference;
                }
                else
                {
                    this->QuequeToken( backup.c_str(), eSecondTokenType );
                    this->QuequeToken( pszToken, eThirdTokenType );
                    return eDataType;
                }

            } catch( const PdfError & e ) {
                if( e.GetError() == ePdfError_UnexpectedEOF ) 
                    return ePdfDataType_Number;
                else
                    throw e;
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

    return ePdfDataType_Unknown;
}

void PdfTokenizer::ReadDataType( EPdfDataType eDataType, PdfVariant& rVariant )
{
    switch( eDataType ) 
    {
        case ePdfDataType_Dictionary:
            this->ReadDictionary( rVariant );
            break;
        case ePdfDataType_Array:
            this->ReadArray( rVariant );
            break;
        case ePdfDataType_String:
            this->ReadString( rVariant );
            break;
        case ePdfDataType_HexString:
            this->ReadHexString( rVariant );
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

void PdfTokenizer::ReadDictionary( PdfVariant& rVariant )
{
    const char*   pszToken;
    EPdfTokenType eType;
    PdfVariant    val;
    PdfName       key;
    PdfDictionary dict;

    for( ;; ) 
    {
        pszToken = this->GetNextToken( &eType );
        if( eType == ePdfTokenType_Delimiter && strncmp( ">>", pszToken, DICT_SEP_LENGTH ) == 0 )
            break;

        this->GetNextVariant( pszToken, eType, val );
        key = val.GetName(); // will raise error ePdfError_InvalidDataType if val is no name!
        this->GetNextVariant( val );

        dict.AddKey( key, val );
    }

    rVariant = dict;
}

void PdfTokenizer::ReadArray( PdfVariant& rVariant )
{
    const char*   pszToken;
    EPdfTokenType eType;
    PdfVariant    var;
    PdfArray      array;

    for( ;; ) 
    {
        pszToken = this->GetNextToken( &eType );
        if( eType == ePdfTokenType_Delimiter && pszToken[0] == ']' )
            break;

        this->GetNextVariant( pszToken, eType, var );
        array.push_back( var );
    }

    rVariant = array;
}

void PdfTokenizer::ReadString( PdfVariant& rVariant )
{
    int               c;
    std::vector<char> vec; // we use a vector instead of a string
                           // because we might read a unicode
                           // string which is allowed to contain 0 bytes.
    bool              bIgnore = false;

    while( (c = m_device.Device()->GetChar()) != EOF )
    {
        // end of stream reached
        if( !bIgnore && c == ')' )
            break;
        
        bIgnore = (c == '\\') && !bIgnore;
        vec.push_back( static_cast<char>(c) );
    }

    rVariant = PdfString( &(vec[0]), vec.size() );
}

void PdfTokenizer::ReadHexString( PdfVariant& rVariant )
{
    int        c;
    std::string str; 

    while( (c = m_device.Device()->GetChar()) != EOF )
    {
        // end of stream reached
        if( c == '>' )
            break;

        // only a hex digits
        if( isdigit( c ) || 
            ( c >= 'A' && c <= 'F') ||
            ( c >= 'a' && c <= 'f'))
#ifdef _MSC_VER
            str += c;
#else
            str.push_back( c );
#endif
    }

    // pad to an even length if necessary
    if( str.length() % 2 )
#ifdef _MSC_VER
        str += c;
#else
        str.push_back( c );
#endif

    PdfString string;
    string.SetHexData( str.c_str(), str.length() );

    rVariant = string;
}

void PdfTokenizer::ReadName( PdfVariant& rVariant )
{
    EPdfTokenType eType;
    const char*   pszToken = this->GetNextToken( &eType );

    if( eType != ePdfTokenType_Token )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    rVariant = PdfName::FromEscaped( pszToken );
}

void PdfTokenizer::QuequeToken( const char* pszToken, EPdfTokenType eType )
{
    m_deqQueque.push_back( TTokenizerPair( std::string( pszToken ), eType ) );
}

};
