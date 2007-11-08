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

#include "podofo.h"

#include "../PdfTest.h"
#include "ContentParser.h"
#include <iostream>
#include <stack>

using namespace PoDoFo;

void PdfContentsTokenizer::ReadNext( EPdfContentsType* peType, const char** ppszKeyword, PdfVariant & rVariant )
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
