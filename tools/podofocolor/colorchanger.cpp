/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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
x1 ***************************************************************************/

#include "colorchanger.h"

#include <podofo.h>

#include <iostream>
#include <cstdlib>
#include <iomanip>

#include "graphicsstack.h"
#include "iconverter.h"

using namespace PoDoFo;

static const ColorChanger::KWInfo kwInfo[] = {
    { ColorChanger::eKeyword_GraphicsStack_Push,     "q", 0,    "Save state" },
    { ColorChanger::eKeyword_GraphicsStack_Pop,      "Q", 0,    "Restore state" },

    { ColorChanger::eKeyword_SelectGray_Stroking,    "G", 1,    "Select gray stroking color" },
    { ColorChanger::eKeyword_SelectRGB_Stroking,     "RG", 3,   "Select RGB stroking color" },
    { ColorChanger::eKeyword_SelectCMYK_Stroking,    "K", 4,    "Select CMYK stroking color" },

    { ColorChanger::eKeyword_SelectGray_NonStroking,    "g", 1,    "Select gray non-stroking color" },
    { ColorChanger::eKeyword_SelectRGB_NonStroking,     "rg", 3,   "Select RGB non-stroking color" },
    { ColorChanger::eKeyword_SelectCMYK_NonStroking,    "k", 4,    "Select CMYK non-stroking color" },

    { ColorChanger::eKeyword_SelectColorSpace_Stroking,    "CS", 1,    "Select colorspace non-stroking color" },
    { ColorChanger::eKeyword_SelectColorSpace_NonStroking,    "cs", 1,    "Select colorspace non-stroking color" },

    { ColorChanger::eKeyword_SelectColor_Stroking,    "SC", 1,    "Select depending on current colorspace" },
    { ColorChanger::eKeyword_SelectColor_NonStroking,    "sc", 1,    "Select depending on current colorspace" },
    { ColorChanger::eKeyword_SelectColor_Stroking2,    "SCN", 1,    "Select depending on current colorspace (extended)" },
    { ColorChanger::eKeyword_SelectColor_NonStroking2,    "scn", 1,    "Select depending on current colorspace (extended)" },

    // Sentinel
    { ColorChanger::eKeyword_Undefined,              "\0", 0,   NULL }
};


// PDF Commands, which modify colors according to PDFReference 1.7
// CS - select colorspace stroking (May need lookup in Colorspace key of resource directory)
// cs - select colorspace non-stroking (May need lookup in Colorspace key of resource directory)
// SC - select stroking color depending on colorspace
// SCN - select stroking color for colorspaces including Separation, DeviceN, ICCBased
// sc - select non-stroking color depending on colorspace
// scn - select non-stroking color for colorspaces including Separation, DeviceN, ICCBased
// G - select gray colorspace and gray stroking color
// g - select gray colorspace and gray non stroking color
// RG - select RGB colorspace and RGB stroking color
// rg - select RGB colorspace and RGB non stroking color
// K - select CMYK colorspace and CMYK stroking color
// k - select CMYK colorspace and CMYK non stroking color

// TODO: Allow to set default color and colorspace when starting a page

// ColorSpaces and their default colors
//  DeviceColorSpaces
//   DeviceGray 0.0
//   DeviceRGB 0.0
//   DeviceCMYK 0.0 0.0 0.0 1.0
//  CIE Based ColorSpaces
//   CalGray 0.0
//   CalRGB 0.0
//   Lab - all values 0.0 or closest according to range
//   ICCBased - all values 0.0 or closest according to range
//  Special ColorSpaces
//   Pattern - the value that causes nothing to be painted
//   Indexed 0
//   Separation - all values 1.0
//   DeviceN  - all values 1.0

// GraphicsState entries and their default values
//  ColorSpace - DeviceGray
//  color stroking - black (see ColorSpace default values)
//  color non stroking - black (see ColorSpace default values)
// Operations
//  q Push
//  Q Pop

ColorChanger::ColorChanger( IConverter* pConvert, const std::string & sInput, const std::string & sOutput )
    : m_pConverter( pConvert ), m_sInput( sInput ), m_sOutput( sOutput )
{
    if( !m_pConverter ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    } 
}

void ColorChanger::start()
{
    PdfMemDocument input( m_sInput.c_str() );

    for( int i = 0; i < input.GetPageCount(); i++ )
    {
        std::cout << "Processing page " << std::setw(6) << (i+1) << "..." << std::endl << std::flush;

        PdfPage* pPage = input.GetPage( i );
        PODOFO_RAISE_LOGIC_IF( !pPage, "Got null page pointer within valid page range" );

        m_pConverter->StartPage( pPage, i );
        this->ReplaceColorsInPage( pPage );
        m_pConverter->EndPage( pPage, i );
    }

    // Go through all XObjects
    PdfVecObjects::iterator it = input.GetObjects().begin();
    while( it != input.GetObjects().end() )
    {
        if( (*it)->IsDictionary() && (*it)->GetDictionary().HasKey( "Type") ) 
        {
            if( PdfName("XObject") == (*it)->GetDictionary().GetKey("Type")->GetName() 
                && (*it)->GetDictionary().HasKey("Subtype") 
                && PdfName("Image") != (*it)->GetDictionary().GetKey("Subtype")->GetName() )
            {
                std::cout << "Processing XObject " << (*it)->Reference().ObjectNumber() << " " 
                          << (*it)->Reference().GenerationNumber() << std::endl;
                
                PdfXObject xObject( *it );
                m_pConverter->StartXObject( &xObject );
                this->ReplaceColorsInPage( &xObject ); 
                m_pConverter->EndXObject( &xObject );
            }
        }
        ++it;
    }


    input.Write( m_sOutput.c_str() );
}

void ColorChanger::ReplaceColorsInPage( PdfCanvas* pPage )
{
    EPdfContentsType t;
    const char* pszKeyword;
    PdfVariant var;
    bool bReadToken;

    GraphicsStack graphicsStack;
    PdfContentsTokenizer tokenizer( pPage );
    std::vector<PdfVariant> args;

    PdfRefCountedBuffer buffer;
    PdfOutputDevice device( &buffer );

    while( (bReadToken = tokenizer.ReadNext(t, pszKeyword, var)) )
    {
        if (t == ePdfContentsType_Variant)
        {
            // arguments come before operators, but we want to group them up before
            // their operator.
            args.push_back(var);
        }
        else if (t == ePdfContentsType_ImageData) 
        {
            // Handle inline images (Internally using PdfData)
            args.push_back(var);
        }
        else if (t == ePdfContentsType_Keyword)
        {
            const KWInfo* pInfo = FindKeyWordByName(pszKeyword);
            PdfColor color, newColor;
            int nNumArgs = pInfo->nNumArguments;
            EPdfColorSpace eColorSpace;

            if( pInfo->nNumArguments > 0 && args.size() != static_cast<size_t>( pInfo->nNumArguments ) )
            {
                std::ostringstream oss;
                oss << "Expected " << pInfo->nNumArguments << " argument(s) for keyword '" << pszKeyword << "', but " << args.size() << " given instead.";
                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidContentStream, oss.str().c_str() );
            }

            switch( pInfo->eKeywordType )
            {
                case eKeyword_GraphicsStack_Push:
                    graphicsStack.Push();
                    break;
                case eKeyword_GraphicsStack_Pop:
                    graphicsStack.Pop();
                    break;

                case eKeyword_SelectColorSpace_Stroking:
                    eColorSpace = this->GetColorSpaceForName( args.back().GetName(), pPage );
                    eColorSpace = PdfColor::GetColorSpaceForName( args.back().GetName() );
                    args.pop_back();
                    graphicsStack.SetStrokingColorSpace( eColorSpace );
                    break;

                case eKeyword_SelectColorSpace_NonStroking:
                    eColorSpace = PdfColor::GetColorSpaceForName( args.back().GetName() );
                    args.pop_back();
                    graphicsStack.SetNonStrokingColorSpace( eColorSpace );
                    break;

                case eKeyword_SelectGray_Stroking:
                case eKeyword_SelectRGB_Stroking:
                case eKeyword_SelectCMYK_Stroking:
                case eKeyword_SelectGray_NonStroking:
                case eKeyword_SelectRGB_NonStroking:
                case eKeyword_SelectCMYK_NonStroking:
                    
                    pszKeyword = 
                        this->ProcessColor( pInfo->eKeywordType, nNumArgs, args, graphicsStack );
                    
                    break;

                case eKeyword_SelectColor_Stroking:
                case eKeyword_SelectColor_Stroking2:
                {
                    /*
                    PdfError::LogMessage( eLogSeverity_Information, "SCN called for colorspace: %s\n",
                                          PdfColor::GetNameForColorSpace( 
                                              graphicsStack.GetStrokingColorSpace() ).GetName().c_str() );
                    */
                    int nTmpArgs;
                    EKeywordType eTempKeyword;

                    switch( graphicsStack.GetStrokingColorSpace() )
                    {
                        case ePdfColorSpace_DeviceGray:
                            nTmpArgs = 1;
                            eTempKeyword = eKeyword_SelectGray_Stroking;
                            break;
                        case ePdfColorSpace_DeviceRGB:
                            nTmpArgs = 3;
                            eTempKeyword = eKeyword_SelectRGB_Stroking;
                            break;
                        case ePdfColorSpace_DeviceCMYK:
                            nTmpArgs = 4;
                            eTempKeyword = eKeyword_SelectCMYK_Stroking;
                            break;

                        case ePdfColorSpace_Separation:
                        {
                            PdfError::LogMessage( eLogSeverity_Error, "Separation color space not supported.\n" );                
                            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
                            break;
                        }
                        case ePdfColorSpace_CieLab:
                        {
                            PdfError::LogMessage( eLogSeverity_Error, "CieLab color space not supported.\n" );                
                            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
                            break;
                        }
                        case ePdfColorSpace_Indexed:
                        {
                            PdfError::LogMessage( eLogSeverity_Error, "Indexed color space not supported.\n" );                
                            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
                            break;
                        }
                        case ePdfColorSpace_Unknown:

                        default:
                        {
                            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
                        }
                    }

                    pszKeyword = 
                        this->ProcessColor( eTempKeyword, nTmpArgs, args, graphicsStack );
                    break;
                }

                case eKeyword_SelectColor_NonStroking:
                case eKeyword_SelectColor_NonStroking2:
                {
                    /*
                    PdfError::LogMessage( eLogSeverity_Information, 
                                          "scn called for colorspace: %s\n",
                                          PdfColor::GetNameForColorSpace( 
                                          graphicsStack.GetNonStrokingColorSpace() ).GetName().c_str() );*/

                    int nTmpArgs;
                    EKeywordType eTempKeyword;

                    switch( graphicsStack.GetNonStrokingColorSpace() )
                    {
                        case ePdfColorSpace_DeviceGray:
                            nTmpArgs = 1;
                            eTempKeyword = eKeyword_SelectGray_NonStroking;
                            break;
                        case ePdfColorSpace_DeviceRGB:
                            nTmpArgs = 3;
                            eTempKeyword = eKeyword_SelectRGB_NonStroking;
                            break;
                        case ePdfColorSpace_DeviceCMYK:
                            nTmpArgs = 4;
                            eTempKeyword = eKeyword_SelectCMYK_NonStroking;
                            break;

                        case ePdfColorSpace_Separation:
                        case ePdfColorSpace_CieLab:
                        case ePdfColorSpace_Indexed:
                        case ePdfColorSpace_Unknown:

                        default:
                        {
                            PdfError::LogMessage( eLogSeverity_Error, "Unknown color space %i type.\n", graphicsStack.GetNonStrokingColorSpace() );
                            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
                        }
                    }

                    pszKeyword = 
                        this->ProcessColor( eTempKeyword, nTmpArgs, args, graphicsStack );
                    break;
                }
                case eKeyword_Undefined:
                    //PdfError::LogMessage( eLogSeverity_Error, "Unknown keyword type.\n" );
                    break;
                default:
                    break;
            }

            WriteArgumentsAndKeyword( args, pszKeyword, device );
        }
    }

    // Write arguments if there are any left
    WriteArgumentsAndKeyword( args, NULL, device );
    // Set new contents stream
    pPage->GetContentsForAppending()->GetStream()->Set( buffer.GetBuffer(), buffer.GetSize() );
}

void ColorChanger::WriteArgumentsAndKeyword( std::vector<PdfVariant> & rArgs, const char* pszKeyword, PdfOutputDevice & rDevice )
{
    std::vector<PdfVariant>::const_iterator it = rArgs.begin();
    while( it != rArgs.end() )
    {
        (*it).Write( &rDevice, ePdfWriteMode_Compact );
        ++it;
    }
    
    rArgs.clear();
    
    if( pszKeyword ) 
    {
        rDevice.Write( " ", 1 );
        rDevice.Write( pszKeyword, strlen( pszKeyword ) );
        rDevice.Write( "\n", 1 );
    }
}

const ColorChanger::KWInfo* ColorChanger::FindKeyWordByName(const char* pszKeyword)
{
    PODOFO_RAISE_LOGIC_IF( !pszKeyword, "Keyword cannot be NULL.");
    
    const KWInfo* pInfo = &(kwInfo[0]);
    while( pInfo->eKeywordType != eKeyword_Undefined )
    {
        if( strcmp( pInfo->pszText, pszKeyword ) == 0 )
        {
            return pInfo;
        }

        ++pInfo;
    }


    return pInfo;
}

void ColorChanger::PutColorOnStack( const PdfColor & rColor, std::vector<PdfVariant> & args )
{
    switch( rColor.GetColorSpace() )
    {
        case ePdfColorSpace_DeviceGray:
            args.push_back( rColor.GetGrayScale() );
            break;

        case ePdfColorSpace_DeviceRGB:
            args.push_back( rColor.GetRed() );
            args.push_back( rColor.GetGreen() );
            args.push_back( rColor.GetBlue() );
            break;

        case ePdfColorSpace_DeviceCMYK:
            args.push_back( rColor.GetCyan() );
            args.push_back( rColor.GetMagenta() );
            args.push_back( rColor.GetYellow() );
            args.push_back( rColor.GetBlack() );
            break;
    
        case ePdfColorSpace_Separation:
        case ePdfColorSpace_CieLab:
        case ePdfColorSpace_Indexed:
        case ePdfColorSpace_Unknown:

        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
        }
    }
}

PdfColor ColorChanger::GetColorFromStack( int nArgs, std::vector<PdfVariant> & args )
{
    PdfColor color;

    double gray = -1.0;
    double red = -1.0, green = -1.0, blue = -1.0;
    double cyan = -1.0, magenta = -1.0, yellow = -1.0, black = -1.0;
    switch( nArgs ) 
    {
        case 1:
            gray = args.back().GetReal();
            args.pop_back();
            color = PdfColor( gray );
            break;
        case 3:
            blue = args.back().GetReal();
            args.pop_back();
            green = args.back().GetReal();
            args.pop_back();
            red = args.back().GetReal();
            args.pop_back();
            color = PdfColor( red, green, blue );
            break;
        case 4:
            black = args.back().GetReal();
            args.pop_back();
            yellow = args.back().GetReal();
            args.pop_back();
            magenta = args.back().GetReal();
            args.pop_back();
            cyan = args.back().GetReal();
            args.pop_back();
            color = PdfColor( cyan, magenta, yellow, black );
            break;
    }

    return color;
}

const char* ColorChanger::ProcessColor( EKeywordType eKeywordType, int nNumArgs, std::vector<PdfVariant> & args, GraphicsStack & rGraphicsStack )
{
    PdfColor newColor;
    bool bStroking = false;
    PdfColor color = this->GetColorFromStack( nNumArgs, args ); 

    switch( eKeywordType )
    {

        case eKeyword_SelectGray_Stroking:
            bStroking = true;
            rGraphicsStack.SetStrokingColorSpace( ePdfColorSpace_DeviceGray );
            newColor = m_pConverter->SetStrokingColorGray( color );
            break;

        case eKeyword_SelectRGB_Stroking:
            bStroking = true;
            rGraphicsStack.SetStrokingColorSpace( ePdfColorSpace_DeviceRGB );
            newColor = m_pConverter->SetStrokingColorRGB( color );
            break;

        case eKeyword_SelectCMYK_Stroking:
            bStroking = true;
            rGraphicsStack.SetStrokingColorSpace( ePdfColorSpace_DeviceCMYK );
            newColor = m_pConverter->SetStrokingColorCMYK( color );
            break;
 
        case eKeyword_SelectGray_NonStroking:
            rGraphicsStack.SetNonStrokingColorSpace( ePdfColorSpace_DeviceGray );
            newColor = m_pConverter->SetNonStrokingColorGray( color );
            break;

        case eKeyword_SelectRGB_NonStroking:
            rGraphicsStack.SetNonStrokingColorSpace( ePdfColorSpace_DeviceRGB );
            newColor = m_pConverter->SetNonStrokingColorRGB( color );
            break;

        case eKeyword_SelectCMYK_NonStroking:
            rGraphicsStack.SetNonStrokingColorSpace( ePdfColorSpace_DeviceCMYK );
            newColor = m_pConverter->SetNonStrokingColorCMYK( color );
            break;

        case eKeyword_GraphicsStack_Push:
        case eKeyword_GraphicsStack_Pop:
        case eKeyword_SelectColorSpace_Stroking:
        case eKeyword_SelectColorSpace_NonStroking:
        case eKeyword_SelectColor_Stroking:
        case eKeyword_SelectColor_Stroking2:
        case eKeyword_SelectColor_NonStroking:
        case eKeyword_SelectColor_NonStroking2:
        case eKeyword_Undefined:

        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
        }
    }


    this->PutColorOnStack( newColor, args );
    if( bStroking ) 
    {
        rGraphicsStack.SetStrokingColor( newColor );
    }
    else
    {
        rGraphicsStack.SetNonStrokingColor( newColor );
    }

    return this->GetKeywordForColor( newColor, bStroking );
}

const char* ColorChanger::GetKeywordForColor( const PdfColor & rColor, bool bIsStroking )
{
    const char* pszKeyword = NULL;

    switch( rColor.GetColorSpace() )
    {
        case ePdfColorSpace_DeviceGray:
            pszKeyword = ( bIsStroking ? "G" : "g" );
            break;

        case ePdfColorSpace_DeviceRGB:
            pszKeyword = ( bIsStroking ? "RG" : "rg" );
            break;

        case ePdfColorSpace_DeviceCMYK:
            pszKeyword = ( bIsStroking ? "K" : "k" );
            break;

        case ePdfColorSpace_Separation:
        case ePdfColorSpace_CieLab:
        case ePdfColorSpace_Indexed:
        case ePdfColorSpace_Unknown:
        
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
        }
    }

    return pszKeyword;
}

EPdfColorSpace ColorChanger::GetColorSpaceForName( const PdfName & rName, PdfCanvas* pPage ) 
{
    EPdfColorSpace eColorSpace = PdfColor::GetColorSpaceForName( rName );

    if( eColorSpace == ePdfColorSpace_Unknown ) 
    {
        // See if we can find it in the resource dictionary of the current page
        PdfObject* pResources = pPage->GetResources();
        if( pResources != NULL
            && pResources->GetDictionary().HasKey( PdfName("ColorSpace") ) )
        {
            PdfObject* pColorSpaces = pResources->GetIndirectKey( PdfName("ColorSpace") );
            if( pColorSpaces != NULL
                && pColorSpaces->GetDictionary().HasKey( rName ) )
            {
                PdfObject* pCS = pColorSpaces->GetIndirectKey( rName );
                if( !pCS )
                {
                    PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
                }
                else if( pCS->IsName() )
                {
                    return this->GetColorSpaceForName( pCS->GetName(), pPage ); 
                }
                else if( pCS->IsArray() )
                {
                    return this->GetColorSpaceForArray( pCS->GetArray(), pPage );
                }
            }
        }
    }

    return eColorSpace;
}

EPdfColorSpace ColorChanger::GetColorSpaceForArray( const PdfArray &, PdfCanvas* )
{
    EPdfColorSpace eColorSpace = ePdfColorSpace_Unknown;

    // CIE Based: [name dictionary]
    //     CalGray
    //     CalRGB
    //     CalLab
    //     ICCBased [name stream]
    // Special:
    //     Pattern
    //     Indexed [/Indexed base hival lookup]
    //     Separation [/Separation name alternateSpace tintTransform]
    //     DeviceN [/DeviceN names alternateSpace tintTransform] or
    //             [/DeviceN names alternateSpace tintTransform attributes]
    // 

    return eColorSpace;
}
