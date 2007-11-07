/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "../../src/podofo.h"

#include "../PdfTest.h"

#include <iostream>
#include <stack>

using namespace PoDoFo;

enum EPdfContentsType {
    ePdfContentsType_Keyword,
    ePdfContentsType_Variant
};

class PdfContentsTokenizer : public PdfTokenizer {
public:
    PdfContentsTokenizer( const char* pBuffer, long lLen )
        : PdfTokenizer( pBuffer, lLen )
    {
        
    }
    
    /** Read the next keyword or variant
     *
     *  \param peType will be set to either keyword or variant
     *  \param ppszKeyword if pType is set to ePdfContentsType_Keyword this will point to the keyword
     *  \param rVariant if pType is set to ePdfContentsType_Variant this will be set to the read variant
     *
     */
    void ReadNext( EPdfContentsType* peType, const char** ppszKeyword, PdfVariant & rVariant )
    {
        EPdfTokenType eTokenType;
        EPdfDataType  eDataType;
        const char*   pszToken;

        pszToken  = this->GetNextToken( &eTokenType );
        eDataType = this->DetermineDataType( pszToken, eTokenType, rVariant );
    
        // asume we read a variant
        *peType = ePdfContentsType_Variant;

        switch( eDataType ) 
        {
            case ePdfDataType_Null:
            case ePdfDataType_Bool:
            case ePdfDataType_Number:
            case ePdfDataType_Real:
                // the data was already read into rVariant by the DetermineDataType function
                break;

            case ePdfDataType_Reference:
            {
                // references are invalid in content streams
                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "references are invalid in content streams" );
                break;
            }

            case ePdfDataType_Dictionary:
                this->ReadDictionary( rVariant, NULL );
                break;
            case ePdfDataType_Array:
                this->ReadArray( rVariant, NULL );
                break;
            case ePdfDataType_String:
                this->ReadString( rVariant, NULL );
                break;
            case ePdfDataType_HexString:
                this->ReadHexString( rVariant, NULL );
                break;
            case ePdfDataType_Name:
                this->ReadName( rVariant );
                break;

            case ePdfDataType_Unknown:
            case ePdfDataType_RawData:
            default:
                // Assume we have a keyword
                *peType      = ePdfContentsType_Keyword;
                *ppszKeyword = pszToken;
                break;
        }
    }
};

void parse_contents( PdfContentsTokenizer* pTokenizer ) 
{
    const char*      pszToken = NULL;
    PdfVariant       var;
    EPdfContentsType eType;
    std::string      str;

    std::stack<PdfVariant> stack;
    std::cout << std::endl << "Parsing a page:" << std::endl;

    try 
    {
        while( true )
        {
            pTokenizer->ReadNext( &eType, &pszToken, var );
            if( eType == ePdfContentsType_Keyword )
            {
                std::cout << "Keyword: " << pszToken << std::endl;

                // support 'l' and 'm' tokens
                if( strcmp( pszToken, "l" ) == 0 ) 
                {
                    double dPosY = stack.top().GetReal();
                    stack.pop();
                    double dPosX = stack.top().GetReal();
                    stack.pop();

                    std::cout << "LineTo: " << dPosX << " " << dPosY << std::endl;
                }
                else if( strcmp( pszToken, "m" ) == 0 ) 
                {
                    double dPosY = stack.top().GetReal();
                    stack.pop();
                    double dPosX = stack.top().GetReal();
                    stack.pop();

                    std::cout << "MoveTo: " << dPosX << " " << dPosY << std::endl;
                }

            }
            else
            {
                var.ToString( str );
                std::cout << "Variant: " << str << std::endl;
                stack.push( var );
            }
        }
    }
    catch( const PdfError & e )
    {
        if( e.GetError() == ePdfError_UnexpectedEOF )
            return; // done with the stream
        else 
            throw e;
    }
}

void parse_page( PdfPage* pPage )
{
    PdfObject* pContents = pPage->GetContents();
    if( pContents  ) 
    {
        PdfStream* pStream = pContents->GetStream();
        char*      pBuffer;
        long       lLen;

        pStream->GetFilteredCopy( &pBuffer, &lLen );

        try 
        {
            PdfContentsTokenizer tokenizer( pBuffer, lLen );
            parse_contents( &tokenizer );
        } 
        catch( const PdfError & e )
        {
            free( pBuffer );
            throw e;
        }
        
        free( pBuffer );
    }
}

int main( int argc, char* argv[] ) 
{
    if( argc != 2 )
    {
        printf("Usage: ContentParser [input_filename]\n");
        return 0;
    }

    try 
    {
        PdfMemDocument doc( argv[1] );
        if( !doc.GetPageCount() )
        {
            std::cerr << "This document contains no page!" << std::endl;
            return 1;
        }

        PdfPage* pFirst = doc.GetPage( 0 );
        
        parse_page( pFirst );
    } 
    catch( const PdfError & e )
    {
        e.PrintErrorMsg();
        return e.GetError();
    }

    return 0;
}
