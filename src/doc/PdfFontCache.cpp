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

#include "PdfFontCache.h" 

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfInputDevice.h"
#include "base/PdfOutputDevice.h"

#include "PdfDifferenceEncoding.h"
#include "PdfFont.h"
#include "PdfFontFactory.h"
#include "PdfFontMetricsFreetype.h"
#include "PdfFontMetricsBase14.h"
#include "PdfFontTTFSubset.h"
#include "PdfFontType1.h"

#include <algorithm>

#ifdef _WIN32

//#include <windows.h>
// Undefined stuff which Windows does
// define that breaks our build
// e.g. GetObject is defined to either GetObjectA or GetObjectW
#ifdef GetObject
#undef GetObject
#endif // GetObject

#ifdef CreateFont
#undef CreateFont
#endif // CreateFont

#endif // _WIN32

#include <ft2build.h>
#include FT_FREETYPE_H

#if defined(PODOFO_HAVE_FONTCONFIG)
#include <fontconfig/fontconfig.h>
#include "base/util/PdfMutexWrapper.h"
#endif

using namespace std;

//us: I know the endian functions are redundant, but they are not availabe in a .h file, right?
//    Ohh, C++ should have these as intrinsic operators, since processors just need one SWAP directive.
#ifdef PODOFO_IS_LITTLE_ENDIAN
inline unsigned long FromBigEndian(unsigned long i)
{
    return ((i << 24) & 0xFF000000) |
           ((i <<  8) & 0x00FF0000) |
           ((i >>  8) & 0x0000FF00) |
           ((i >> 24) & 0x000000FF);
}
inline unsigned short ShortFromBigEndian(unsigned short i)
{
    return ((i << 8) & 0xFF00) | ((i >> 8) & 0x00FF);
}
#else
inline unsigned long FromBigEndian(unsigned long i)
{
    return i;
}
inline unsigned short ShortFromBigEndian(unsigned short i)
{
    return i;
}
#endif


namespace PoDoFo {

#if defined(_WIN32) && !defined(PODOFO_NO_FONTMANAGER)
// The function receives a buffer containing a true type collection and replaces the buffer
// by a new buffer with the extracted font.
// On error the function returns false.
static bool GetFontFromCollection(char *&buffer, unsigned int &bufferLen, unsigned int bufferOffset, const LOGFONTW* inFont)
{
    bool ok = false;

    // these properties are extracted to match the font
    wchar_t fontFamilyLocale[1024];
    // if you have a languagePack installed EnumFontFamiliesEx Call will still give you english names
    // even though the systemLCID is set to the language pack -> resulting in conflicts when comparing fontnames with 
    // the LOGFONTW->lfFaceName
    wchar_t fontFamilyEngl[1024];   
    wchar_t fontStyle[1024];
    wchar_t fontFullName[1024];
    wchar_t fontPostscriptName[1024];
    fontFamilyLocale[0]   = 0;
    fontFamilyEngl[0] = 0;
    fontStyle[0]    = 0;
    fontFullName[0] = 0;
    fontPostscriptName[0] = 0;
    unsigned int fontFileSize = 12;

    //us: see "http://www.microsoft.com/typography/otspec/otff.htm"
    USHORT numTables = ShortFromBigEndian(*(USHORT *)(buffer + bufferOffset + 4));
    char *entry = buffer + bufferOffset + 12;
    for(int i=0; i<numTables; i++)
    {
        char tag[5];
        tag[0] = entry[0];
        tag[1] = entry[1];
        tag[2] = entry[2];
        tag[3] = entry[3];
        tag[4] = 0;
        //ULONG checkSum = FromBigEndian(*(ULONG *)(entry+4));
        ULONG offset   = FromBigEndian(*(ULONG *)(entry+8));
        ULONG length   = FromBigEndian(*(ULONG *)(entry+12));
        length = (length+3) & ~3;
        if(offset+length > bufferLen)
        {
            return false; // truncated or corrupted buffer
        }

        
        if(strcmp(tag, "name") == 0)
        {
            //us: see "http://www.microsoft.com/typography/otspec/name.htm"
            char *nameTable = buffer + offset;
            //USHORT format       = ShortFromBigEndian(*(USHORT *)(nameTable));
            USHORT nameCount    = ShortFromBigEndian(*(USHORT *)(nameTable+2));
            USHORT stringOffset = ShortFromBigEndian(*(USHORT *)(nameTable+4));
            char *stringArea = nameTable + stringOffset;
            char *nameRecord = nameTable + 6;

            int systemLCID = GetUserDefaultLCID(); // systemcall to get the locale user ID

            for(int n=0; n<nameCount; n++)
            {
                USHORT platformID = ShortFromBigEndian(*(USHORT *)(nameRecord));
                USHORT encodingID = ShortFromBigEndian(*(USHORT *)(nameRecord+2));
                USHORT languageID = ShortFromBigEndian(*(USHORT *)(nameRecord+4));

                if(platformID == 0 && languageID == 0)
                {
                    // Unicode platform / unicode 1.0
                    USHORT nameID = ShortFromBigEndian(*(USHORT *)(nameRecord+6));
                    USHORT length = ShortFromBigEndian(*(USHORT *)(nameRecord+8));
                    USHORT offset = ShortFromBigEndian(*(USHORT *)(nameRecord+10));
                    wchar_t name[1024];
                    if( length >= sizeof(name) )
                        length = sizeof(name) - sizeof(wchar_t);
                    unsigned int charCount = length / sizeof(wchar_t);
                    for(unsigned int i=0; i<charCount; i++)
                    {
                        name[i] = ShortFromBigEndian(((USHORT *)(stringArea + offset))[i]);
                    }
                    name[charCount] = 0;

                    switch(nameID)
                    {
                    case 1:
                        wcscpy(fontFamilyLocale, name);
                        break;
                    case 2:
                        wcscpy(fontStyle, name);
                        break;
                    case 4:
                        wcscpy(fontFullName, name);
                        break;
                    case 6:
                        wcscpy(fontPostscriptName, name);
                        break;
                    }
                }
                // dv: see "http://www.microsoft.com/typography/otspec/name.htm"
                if (platformID == 3 && encodingID == 1) //Platform Windows -> Unicode (UCS-2)
                {
                    USHORT nameID = ShortFromBigEndian(*(USHORT *)(nameRecord + 6));
                    USHORT length = ShortFromBigEndian(*(USHORT *)(nameRecord + 8));
                    USHORT offset = ShortFromBigEndian(*(USHORT *)(nameRecord + 10));
                    wchar_t name[1024];
                    if( length >= sizeof(name) )
                        length = sizeof(name) - sizeof(wchar_t);
                    unsigned int charCount = length / sizeof(wchar_t);
                    for (unsigned int i = 0; i < charCount; i++)
                    {
                        name[i] = ShortFromBigEndian(((USHORT *)(stringArea + offset))[i]);
                    }
                    name[charCount] = 0;

                    if (languageID == systemLCID)
                    {                 
                        switch (nameID)
                        {
                        case 1:
                            wcscpy(fontFamilyLocale, name);
                            break;
                        case 2:
                            wcscpy(fontStyle, name);
                            break;
                        case 4:
                            wcscpy(fontFullName, name);
                            break;
                        case 6:
                            wcscpy(fontPostscriptName, name);
                            break;
                        }
                    }
                    else if (languageID == 1033) // English - United States
                    {
                        switch (nameID)
                        {
                        case 1:
                            wcscpy(fontFamilyEngl, name);
                            break;
                        }
                    }
                }
                nameRecord += 12;
            }
        }
        entry += 16;
        fontFileSize += 16 + length;
    }

    // check if font matches the wanted properties
    unsigned int isMatchingFont = _wcsicmp(fontFamilyLocale, inFont->lfFaceName) == 0 || _wcsicmp(fontFamilyEngl, inFont->lfFaceName) == 0;
    //..TODO.. check additional styles (check fontStyle, fontFullName, fontPostscriptName) ?

    // extract font
    if(isMatchingFont)
    {
        char *newBuffer = (char *) podofo_malloc(fontFileSize);
		if (!newBuffer)
		{
			PODOFO_RAISE_ERROR(ePdfError_OutOfMemory);
		}

        // copy font header and table index (offsets need to be still adjusted)
        memcpy(newBuffer, buffer + bufferOffset, 12+16*numTables);
        unsigned int dstDataOffset = 12+16*numTables;

        // process tables
        char *srcEntry = buffer + bufferOffset + 12;
        char *dstEntry = newBuffer + 12;
        for(int table=0; table < numTables; table++)
        {
            // read source entry
            ULONG offset   = FromBigEndian(*(ULONG *)(srcEntry+8));
            ULONG length   = FromBigEndian(*(ULONG *)(srcEntry+12));
            length = (length+3) & ~3;

            // adjust offset
            // U can use FromBigEndian() also to convert _to_ big endian 
            *(ULONG *)(dstEntry+8) = FromBigEndian(dstDataOffset);

            //copy data
            memcpy(newBuffer + dstDataOffset, buffer + offset, length);
            dstDataOffset += length;

            // adjust table entry pointers for loop
            srcEntry += 16;
            dstEntry += 16;
        }

        // replace old buffer
        //assert(dstDataOffset==fontFileSize)
        podofo_free(buffer);
        buffer = newBuffer;
        bufferLen = fontFileSize;
        ok = true;
    }

    return ok;
}

static bool GetDataFromHFONT( HFONT hf, char** outFontBuffer, unsigned int& outFontBufferLen, const LOGFONTW* inFont )
{
    HDC hdc = GetDC(0);
    if ( hdc == NULL ) return false;
    HGDIOBJ oldFont = SelectObject(hdc, hf);    // Petr Petrov (22 December 2009)

    bool ok = false;

    // try get data from true type collection
    char *buffer = NULL;
    unsigned int bufferLen = 0;
    bool hasData = false;
    const DWORD ttcf_const = 0x66637474;
    DWORD dwTable = ttcf_const;
    bufferLen = GetFontData(hdc, dwTable, 0, 0, 0);

    if (bufferLen == GDI_ERROR)
    {
        dwTable = 0;
        bufferLen = GetFontData(hdc, dwTable, 0, 0, 0);

    }
    if (bufferLen != GDI_ERROR)
    {
        buffer = (char *) podofo_malloc( bufferLen );
		if (!buffer)
		{
			PODOFO_RAISE_ERROR(ePdfError_OutOfMemory);
		}

        hasData = GetFontData( hdc, dwTable, 0, buffer, (DWORD) bufferLen ) != GDI_ERROR;
    }

    if(hasData)
    {
        if(((DWORD *)buffer)[0] == ttcf_const)
        {
            // true type collection data
            unsigned int numFonts = FromBigEndian(((unsigned int *)buffer)[2]);
            for(unsigned int i=0; i<numFonts; i++)
            {
                unsigned int offset = FromBigEndian(((unsigned int *)buffer)[3+i]);
                ok = GetFontFromCollection(buffer, bufferLen, offset, inFont);
                if(ok) break;
            }
        }
        else
        {
            // "normal" font data
            ok = true;
        }
    }

    // clean up
    SelectObject(hdc,oldFont);
    ReleaseDC(0,hdc);
    if(ok)
    {
        // on success set result buffer
        *outFontBuffer = buffer;
        outFontBufferLen = bufferLen;
    }
    else if(buffer)
    {
        // on failure free local buffer
        podofo_free(buffer);
    }
    return ok;
}

static bool GetDataFromLPFONT( const LOGFONTW* inFont, char** outFontBuffer, unsigned int& outFontBufferLen )
{
    bool ok = false;
    HFONT hf = CreateFontIndirectW(inFont);
    if(hf)
    {
        ok = GetDataFromHFONT( hf, outFontBuffer, outFontBufferLen, inFont );
        DeleteObject(hf);
    }
    return ok;
}

static bool GetDataFromLPFONT( const LOGFONTA* inFont, char** outFontBuffer, unsigned int& outFontBufferLen )
{
    bool ok = false;
    HFONT hf = CreateFontIndirectA(inFont);
    if(hf)
    {
        LOGFONTW inFontW;
        GetObjectW(hf, sizeof(LOGFONTW), &inFontW);
        ok = GetDataFromHFONT( hf, outFontBuffer, outFontBufferLen, &inFontW);
        DeleteObject(hf);
    }
    return ok;
}
#endif // _WIN32

PdfFontCache::PdfFontCache( PdfVecObjects* pParent )
    : m_pParent( pParent )
{
    Init();
}

PdfFontCache::PdfFontCache( const PdfFontConfigWrapper & rFontConfig, PdfVecObjects* pParent )
    : m_pParent( pParent ), m_fontConfig( rFontConfig )
{
    Init();
}

PdfFontCache::~PdfFontCache()
{
    this->EmptyCache();

    if( m_ftLibrary ) 
    {
        FT_Done_FreeType( m_ftLibrary );
        m_ftLibrary = NULL;
    }
}

void PdfFontCache::Init(void)
{
    m_sSubsetBasename[0] = 0;
    char *p = m_sSubsetBasename;
    int ii;
    for (ii = 0; ii < SUBSET_BASENAME_LEN; ii++, p++) {
        *p = 'A';
    }
    p[0] = '+';
    p[1] = 0;

    m_sSubsetBasename[0]--;

    // Initialize all the fonts stuff
    if( FT_Init_FreeType( &m_ftLibrary ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_FreeType );
    }
}

void PdfFontCache::EmptyCache() 
{
    TISortedFontList itFont = m_vecFonts.begin();

    while( itFont != m_vecFonts.end() )
    {
        delete (*itFont).m_pFont;
        ++itFont;
    }

    itFont = m_vecFontSubsets.begin();
    while( itFont != m_vecFontSubsets.end() )
    {
        delete (*itFont).m_pFont;
        ++itFont;
    }

    m_vecFonts.clear();
    m_vecFontSubsets.clear();
}

PdfFont* PdfFontCache::GetFont( PdfObject* pObject )
{
    TCISortedFontList it = m_vecFonts.begin();
    const PdfReference & ref = pObject->Reference(); 

    // Search if the object is a cached normal font
    while( it != m_vecFonts.end() )
    {
        if( (*it).m_pFont->GetObject()->Reference() == ref ) 
            return (*it).m_pFont;

        ++it;
    }

    // Search if the object is a cached font subset
    it = m_vecFontSubsets.begin();
    while( it != m_vecFontSubsets.end() )
    {
        if( (*it).m_pFont->GetObject()->Reference() == ref ) 
            return (*it).m_pFont;

        ++it;
    }

    // Create a new font
    PdfFont* pFont = PdfFontFactory::CreateFont( &m_ftLibrary, pObject );
    if( pFont ) 
    {
        TFontCacheElement element;
        element.m_pFont     = pFont;
        element.m_bBold     = pFont->IsBold();
        element.m_bItalic   = pFont->IsItalic();
        element.m_sFontName = pFont->GetFontMetrics()->GetFontname();
        element.m_pEncoding = NULL;
        element.m_bIsSymbolCharset = pFont->GetFontMetrics()->IsSymbol();
        m_vecFonts.push_back( element );
        
        // Now sort the font list
        std::sort( m_vecFonts.begin(), m_vecFonts.end() );
    }
    
    return pFont;
}

PdfFont* PdfFontCache::GetFont( const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                bool bEmbedd, EFontCreationFlags eFontCreationFlags,
                                const PdfEncoding * const pEncoding, 
                                const char* pszFileName)
{
    PODOFO_ASSERT( pEncoding );

    PdfFont*          pFont = NULL;
    PdfFontMetrics*   pMetrics = NULL;
    std::pair<TISortedFontList,TCISortedFontList> it;

    it = std::equal_range( m_vecFonts.begin(), m_vecFonts.end(), 
               TFontCacheElement( pszFontName, bBold, bItalic, bSymbolCharset, pEncoding ) );

        
    if( it.first == it.second )
    {
        if ( (eFontCreationFlags & eFontCreationFlags_AutoSelectBase14) 
             && PODOFO_Base14FontDef_FindBuiltinData(pszFontName) )
        {
            EPdfFontFlags eFlags = ePdfFont_Normal;
            if( bBold )
            {
                if( bItalic )
                {
                    eFlags = ePdfFont_BoldItalic;
                }
                else
                {
                    eFlags = ePdfFont_Bold;
                }
            }
            else if( bItalic )
                eFlags = ePdfFont_Italic;

            pFont = PdfFontFactory::CreateBase14Font(pszFontName, eFlags,
                        pEncoding, m_pParent);
            if( pFont ) 
            {
                TFontCacheElement element;
                element.m_pFont     = pFont;
                element.m_bBold     = pFont->IsBold();
                element.m_bItalic   = pFont->IsItalic();
                element.m_sFontName = pszFontName;
                element.m_pEncoding = pEncoding;
                element.m_bIsSymbolCharset = bSymbolCharset;

                // Do a sorted insert, so no need to sort again
                //rvecContainer.insert( itSorted, element ); 
                m_vecFonts.insert( it.first, element );
                
             }

        }

        if (!pFont)
        {
            bool bSubsetting = (eFontCreationFlags & eFontCreationFlags_Type1Subsetting) != 0;
            std::string sPath;
            if ( pszFileName == NULL )
                sPath = this->GetFontPath( pszFontName, bBold, bItalic );
            else
                sPath = pszFileName;
            
            if( sPath.empty() )
            {
#if defined(_WIN32) && !defined(PODOFO_NO_FONTMANAGER)
                pFont = GetWin32Font( it.first, m_vecFonts, pszFontName, bBold, bItalic, bSymbolCharset, bEmbedd, pEncoding, bSubsetting  );
#endif // _WIN32
            }
            else
            {
                pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, sPath.c_str(), bSymbolCharset, bSubsetting ? genSubsetBasename() : NULL );
                pFont    = this->CreateFontObject( it.first, m_vecFonts, pMetrics, 
                           bEmbedd, bBold, bItalic, pszFontName, pEncoding, bSubsetting );
            }

        }
    }
    else
        pFont = (*it.first).m_pFont;

#if !(defined(_WIN32) && !defined(PODOFO_NO_FONTMANAGER))
        if (!pFont)
            PdfError::LogMessage( eLogSeverity_Critical, "No path was found for the specified fontname: %s\n", pszFontName );
#endif             

    return pFont;
}

#if defined(_WIN32) && !defined(PODOFO_NO_FONTMANAGER)
PdfFont* PdfFontCache::GetFont( const wchar_t* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                bool bEmbedd, const PdfEncoding * const pEncoding )
{
    PODOFO_ASSERT( pEncoding );

    PdfFont*          pFont;
    std::pair<TISortedFontList,TCISortedFontList> it;

    size_t lMaxLen = wcslen(pszFontName) * 5;

    if (lMaxLen == 0) 
        PODOFO_RAISE_ERROR_INFO(ePdfError_InternalLogic, "Font name is empty");
        
    char* pmbFontName = static_cast<char*>(podofo_malloc(lMaxLen));
    if (!pmbFontName)
    {
        PODOFO_RAISE_ERROR(ePdfError_OutOfMemory);
    }
    if (wcstombs(pmbFontName, pszFontName, lMaxLen) == -1)
    {
        podofo_free(pmbFontName);
        PODOFO_RAISE_ERROR_INFO(ePdfError_InternalLogic, "Conversion to multibyte char failed");
    }

    TFontCacheElement element;
    element.m_bBold = bBold;
    element.m_bItalic = bItalic;
    element.m_pEncoding = pEncoding;
    element.m_sFontName = pmbFontName;

    it = std::equal_range( m_vecFonts.begin(), m_vecFonts.end(), element );
    
    if( it.first == it.second )
        return GetWin32Font( it.first, m_vecFonts, pszFontName, bBold, bItalic, bSymbolCharset, bEmbedd, pEncoding );
    else
        pFont = (*it.first).m_pFont;
    
    return pFont;
}

PdfFont* PdfFontCache::GetFont( const LOGFONTA &logFont, 
                                bool bEmbedd, const PdfEncoding * const pEncoding )
{
    PODOFO_ASSERT( pEncoding );

    PdfFont*          pFont;
    std::pair<TISortedFontList,TCISortedFontList> it;

    it = std::equal_range( m_vecFonts.begin(), m_vecFonts.end(), 
         TFontCacheElement( logFont.lfFaceName, logFont.lfWeight >= FW_BOLD ? true : false, logFont.lfItalic ? true : false, logFont.lfCharSet == SYMBOL_CHARSET, pEncoding ) );
    if( it.first == it.second )
        return GetWin32Font( it.first, m_vecFonts, logFont, bEmbedd, pEncoding );
    else
        pFont = (*it.first).m_pFont;
    
    return pFont;
}

PdfFont* PdfFontCache::GetFont( const LOGFONTW &logFont, 
                                bool bEmbedd, const PdfEncoding * const pEncoding )
{
    PODOFO_ASSERT( pEncoding );

    PdfFont*          pFont;
    std::pair<TISortedFontList,TCISortedFontList> it;

    it = std::equal_range( m_vecFonts.begin(), m_vecFonts.end(), 
         TFontCacheElement( logFont.lfFaceName, logFont.lfWeight >= FW_BOLD ? true : false, logFont.lfItalic ? true : false, logFont.lfCharSet == SYMBOL_CHARSET, pEncoding ) );
    if( it.first == it.second )
        return GetWin32Font( it.first, m_vecFonts, logFont, bEmbedd, pEncoding );
    else
        pFont = (*it.first).m_pFont;
    
    return pFont;
}
#endif // _WIN32

PdfFont* PdfFontCache::GetFont( FT_Face face, bool bSymbolCharset, bool bEmbedd, const PdfEncoding * const pEncoding )
{
    PdfFont*          pFont;
    PdfFontMetrics*   pMetrics;
    std::pair<TISortedFontList,TCISortedFontList> it;

    std::string sName = FT_Get_Postscript_Name( face );
    if( sName.empty() )
    {
        PdfError::LogMessage( eLogSeverity_Critical, "Could not retrieve fontname for font!\n" );
        return NULL;
    }

    bool bBold   = ((face->style_flags & FT_STYLE_FLAG_BOLD)   != 0);
    bool bItalic = ((face->style_flags & FT_STYLE_FLAG_ITALIC) != 0);

    it = std::equal_range( m_vecFonts.begin(), m_vecFonts.end(), 
               TFontCacheElement( sName.c_str(), bBold, bItalic, bSymbolCharset, pEncoding ) );
    if( it.first == it.second )
    {
        pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, face, bSymbolCharset );
        pFont    = this->CreateFontObject( it.first, m_vecFonts, pMetrics, 
                       bEmbedd, bBold, bItalic, sName.c_str(), pEncoding );
    }
    else
        pFont = (*it.first).m_pFont;

    return pFont;
}

PdfFont* PdfFontCache::GetDuplicateFontType1( PdfFont * pFont, const char* pszSuffix )
{
    TCISortedFontList it = m_vecFonts.begin();

    std::string id = pFont->GetIdentifier().GetName();
    id += pszSuffix;

    // Search if the object is a cached normal font
    while( it != m_vecFonts.end() )
    {
        if( (*it).m_pFont->GetIdentifier() == id ) 
            return (*it).m_pFont;

        ++it;
    }

    // Search if the object is a cached font subset
    it = m_vecFontSubsets.begin();
    while( it != m_vecFontSubsets.end() )
    {
        if( (*it).m_pFont->GetIdentifier() == id ) 
            return (*it).m_pFont;

        ++it;
    }

    // Create a copy of the font
    PODOFO_ASSERT( pFont->GetFontMetrics()->GetFontType() == ePdfFontType_Type1Pfb );
    PdfFontMetrics* pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, pFont->GetFontMetrics()->GetFilename(), pFont->GetFontMetrics()->IsSymbol() );
    PdfFont* newFont = new PdfFontType1( static_cast<PdfFontType1 *>(pFont), pMetrics, pszSuffix, m_pParent );
    if( newFont ) 
    {
        std::string name = newFont->GetFontMetrics()->GetFontname();
        name += pszSuffix;
        TFontCacheElement element;
        element.m_pFont     = newFont;
        element.m_bBold     = newFont->IsBold();
        element.m_bItalic   = newFont->IsItalic();
        element.m_sFontName = name;
        element.m_pEncoding = newFont->GetEncoding();
          element.m_bIsSymbolCharset = pFont->GetFontMetrics()->IsSymbol();
        m_vecFonts  .push_back( element );
        
        // Now sort the font list
        std::sort( m_vecFonts.begin(), m_vecFonts.end() );
    }

    return newFont;
}

PdfFont* PdfFontCache::GetFontSubset( const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                      const PdfEncoding * const pEncoding,
                      const char* pszFileName )
{
    PdfFont*        pFont = 0;
    PdfFontMetrics* pMetrics;
    std::pair<TISortedFontList,TCISortedFontList> it;

    // WARNING: The characters are completely ignored right now!

    it = std::equal_range( m_vecFontSubsets.begin(), m_vecFontSubsets.end(), 
               TFontCacheElement( pszFontName, bBold, bItalic, bSymbolCharset, pEncoding ) );
    if( it.first == it.second )
    {
        std::string sPath; 
        if( pszFileName == NULL || *pszFileName == 0) 
        {
            sPath = this->GetFontPath( pszFontName, bBold, bItalic );
            if( sPath.empty() )
            {
#if defined(_WIN32) && !defined(PODOFO_NO_FONTMANAGER)
                return GetWin32Font( it.first, m_vecFontSubsets, pszFontName, bBold, bItalic, bSymbolCharset, true, pEncoding, true );
#else       
                PdfError::LogMessage( eLogSeverity_Critical, "No path was found for the specified fontname: %s\n", pszFontName );
                return NULL;
#endif // _WIN32
            }
        }
        else {
            sPath = pszFileName;
        }
        
        pMetrics = PdfFontMetricsFreetype::CreateForSubsetting( &m_ftLibrary, sPath.c_str(), bSymbolCharset, genSubsetBasename() );
        pFont = this->CreateFontObject( it.first, m_vecFontSubsets, pMetrics, 
                                        true, bBold, bItalic, pszFontName, pEncoding, true );
    }
    else
        pFont = (*it.first).m_pFont;
    
    
    return pFont;
}

void PdfFontCache::EmbedSubsetFonts()
{
    TCISortedFontList it = m_vecFontSubsets.begin();

    while( it != m_vecFontSubsets.end() )
    {
        if( (*it).m_pFont->IsSubsetting() )
        {
            (*it).m_pFont->EmbedSubsetFont();
        }

        ++it;
    }
}

#if defined(_WIN32) && !defined(PODOFO_NO_FONTMANAGER)
PdfFont* PdfFontCache::GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, 
                                     const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                     bool bEmbedd, const PdfEncoding * const pEncoding, bool pSubsetting )
{
    LOGFONTW lf;
    
    lf.lfHeight         = 0;
    lf.lfWidth          = 0;
    lf.lfEscapement     = 0;
    lf.lfOrientation    = 0;
    lf.lfWeight         = bBold ? FW_BOLD : 0;
    lf.lfItalic         = bItalic;
    lf.lfUnderline      = 0;
    lf.lfStrikeOut      = 0;
    lf.lfCharSet           = bSymbolCharset ? SYMBOL_CHARSET : DEFAULT_CHARSET;
    lf.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    lf.lfQuality        = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    
    if (strlen(pszFontName) >= LF_FACESIZE)
        return NULL;
    
    memset(&(lf.lfFaceName), 0, LF_FACESIZE);
    //strcpy( lf.lfFaceName, pszFontName );
    /*int destLen =*/ MultiByteToWideChar (0, 0, pszFontName, -1, lf.lfFaceName, LF_FACESIZE);

     return GetWin32Font(itSorted, vecContainer, lf, bEmbedd, pEncoding, pSubsetting);
}

PdfFont* PdfFontCache::GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, 
                                     const wchar_t* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                     bool bEmbedd, const PdfEncoding * const pEncoding, bool pSubsetting )
{
    LOGFONTW    lf;
    
    lf.lfHeight         = 0;
    lf.lfWidth          = 0;
    lf.lfEscapement     = 0;
    lf.lfOrientation    = 0;
    lf.lfWeight         = bBold ? FW_BOLD : 0;
    lf.lfItalic         = bItalic;
    lf.lfUnderline      = 0;
    lf.lfStrikeOut      = 0;
    lf.lfCharSet           = bSymbolCharset ? SYMBOL_CHARSET : DEFAULT_CHARSET;
    lf.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    lf.lfQuality        = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    
    pdf_long lFontNameLen = wcslen(pszFontName);
    if (lFontNameLen >= LF_FACESIZE)
        return NULL;
    if (lFontNameLen == 0)
        PODOFO_RAISE_ERROR_INFO(ePdfError_InternalLogic, "Font name is empty");
    
    memset(&(lf.lfFaceName), 0, LF_FACESIZE);
    wcscpy( static_cast<wchar_t*>(lf.lfFaceName), pszFontName );
    
    return GetWin32Font(itSorted, vecContainer, lf, bEmbedd, pEncoding, pSubsetting);
}

PdfFont* PdfFontCache::GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, const LOGFONTA &logFont,
                                bool bEmbedd, const PdfEncoding * const pEncoding, bool pSubsetting)
{
    char*        pBuffer = NULL;
    unsigned int nLen;

    if( !GetDataFromLPFONT( &logFont, &pBuffer, nLen ) )
        return NULL;
    
    PdfFontMetrics* pMetrics;
    PdfFont*        pFont = NULL;
    try {
         pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, pBuffer, nLen, logFont.lfCharSet == SYMBOL_CHARSET, pSubsetting ? genSubsetBasename() : NULL );
        pFont    = this->CreateFontObject( itSorted, vecContainer, pMetrics, 
              bEmbedd, logFont.lfWeight >= FW_BOLD ? true : false, logFont.lfItalic ? true : false, logFont.lfFaceName, pEncoding, pSubsetting );
    } catch( PdfError & error ) {
        podofo_free( pBuffer );
        throw error;
    }
    
    podofo_free( pBuffer );
    return pFont;
}

PdfFont* PdfFontCache::GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, const LOGFONTW &logFont,
                                bool bEmbedd, const PdfEncoding * const pEncoding, bool pSubsetting)
{
    pdf_long lFontNameLen = wcslen(logFont.lfFaceName);
    if (lFontNameLen >= LF_FACESIZE)
        return NULL;

    pdf_long lMaxLen = lFontNameLen * 5;
    char* pmbFontName = static_cast<char*>(podofo_malloc(lMaxLen));
    if( !pmbFontName )
    {
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }

    if( wcstombs( pmbFontName, logFont.lfFaceName, lMaxLen ) == -1 )
    {
        podofo_free( pmbFontName );
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Conversion to multibyte char failed" );
    }

    char*        pBuffer = NULL;
    unsigned int nLen;
    if (!GetDataFromLPFONT(&logFont, &pBuffer, nLen))
        return NULL;

    PdfFontMetrics* pMetrics;
    PdfFont*        pFont = NULL;
    try {
        pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, pBuffer, nLen, logFont.lfCharSet == SYMBOL_CHARSET, pSubsetting ? genSubsetBasename() : NULL );
        pFont    = this->CreateFontObject( itSorted, vecContainer, pMetrics, 
              bEmbedd, logFont.lfWeight >= FW_BOLD ? true : false, logFont.lfItalic ? true : false, pmbFontName, pEncoding, pSubsetting );
        podofo_free( pmbFontName );
        pmbFontName = NULL;
    } catch( PdfError & error ) {
        podofo_free( pmbFontName );
        pmbFontName = NULL;
        podofo_free( pBuffer );
        throw error;
    }
    
    podofo_free( pBuffer );
    return pFont;
}

#endif // _WIN32

#if defined(PODOFO_HAVE_FONTCONFIG)
std::string PdfFontCache::GetFontConfigFontPath( FcConfig* pConfig, const char* pszFontName, bool bBold, bool bItalic )
{
    FcPattern*  pattern;
    FcPattern*  matched;
    FcResult    result = FcResultMatch;
    FcValue     v;
    std::string sPath;
    // Build a pattern to search using fontname, bold and italic
    pattern = FcPatternBuild (0, FC_FAMILY, FcTypeString, pszFontName, 
                              FC_WEIGHT, FcTypeInteger, (bBold ? FC_WEIGHT_BOLD : FC_WEIGHT_MEDIUM),
                              FC_SLANT, FcTypeInteger, (bItalic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN),  
                              static_cast<char*>(0));

    FcDefaultSubstitute( pattern );

    if( !FcConfigSubstitute( pConfig, pattern, FcMatchFont ) )
    {
        FcPatternDestroy( pattern );
        return sPath;
    }

    matched = FcFontMatch( pConfig, pattern, &result );
    if( result != FcResultNoMatch )
    {
        result = FcPatternGet( matched, FC_FILE, 0, &v );
        sPath = reinterpret_cast<const char*>(v.u.s);
#ifdef PODOFO_VERBOSE_DEBUG
        PdfError::LogMessage( eLogSeverity_Debug,
                              "Got Font %s for for %s\n", sPath.c_str(), pszFontName );
#endif // PODOFO_DEBUG
    }

    FcPatternDestroy( pattern );
    FcPatternDestroy( matched );
    return sPath;
}

#endif // PODOFO_HAVE_FONTCONFIG

std::string PdfFontCache::GetFontPath( const char* pszFontName, bool bBold, bool bItalic )
{
#if defined(PODOFO_HAVE_FONTCONFIG)
    Util::PdfMutexWrapper mutex(m_fontConfig.GetFontConfigMutex());
    FcConfig* pFcConfig = static_cast<FcConfig*>(m_fontConfig.GetFontConfig());
    std::string sPath   = this->GetFontConfigFontPath( pFcConfig,
                                                       pszFontName, bBold, bItalic );
#else
    std::string sPath = "";
#endif
    return sPath;
}

PdfFont* PdfFontCache::CreateFontObject( TISortedFontList itSorted, TSortedFontList & rvecContainer, 
                     PdfFontMetrics* pMetrics, bool bEmbedd, bool bBold, bool bItalic, 
                     const char* pszFontName, const PdfEncoding * const pEncoding, bool bSubsetting ) 
{
    PdfFont* pFont;

    try {
        int nFlags = ePdfFont_Normal;

        if ( bSubsetting )
            nFlags |= ePdfFont_Subsetting;
        
        if( bEmbedd )
            nFlags |= ePdfFont_Embedded;
        
        if( bBold ) 
            nFlags |= ePdfFont_Bold;

        if( bItalic )
            nFlags |= ePdfFont_Italic;
        
        pFont    = PdfFontFactory::CreateFontObject( pMetrics, nFlags, pEncoding, m_pParent );

        if( pFont ) 
        {
            TFontCacheElement element;
            element.m_pFont     = pFont;
            element.m_bBold     = pFont->IsBold();
            element.m_bItalic   = pFont->IsItalic();
            element.m_sFontName = pszFontName;
            element.m_pEncoding = pEncoding;
            element.m_bIsSymbolCharset = pMetrics->IsSymbol();
            
            // Do a sorted insert, so no need to sort again
            rvecContainer.insert( itSorted, element );
        }
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__ );
        e.PrintErrorMsg();
        PdfError::LogMessage( eLogSeverity_Error, "Cannot initialize font: %s\n", pszFontName ? pszFontName : "" );
        return NULL;
    }
    
    return pFont;
}

const char *PdfFontCache::genSubsetBasename(void)
{
    int ii = 0;
    while(ii < SUBSET_BASENAME_LEN)
    {
        m_sSubsetBasename[ii]++;
        if (m_sSubsetBasename[ii] <= 'Z')
        {
            break;
        }

        m_sSubsetBasename[ii] = 'A';
        ii++;
    }

    return m_sSubsetBasename;
}

};
