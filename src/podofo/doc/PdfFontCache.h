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

#ifndef _PDF_FONT_CACHE_H_
#define _PDF_FONT_CACHE_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/Pdf3rdPtyForwardDecl.h"
#include "podofo/base/PdfEncoding.h"
#include "podofo/base/PdfEncodingFactory.h"

#include "PdfFont.h"
#include "PdfFontConfigWrapper.h"

#ifdef _WIN32

// to have LOGFONTA/LOGFONTW available
#include <windows.h>

// Undefined stuff which windows does
// define that breaks are build
// e.g. GetObject is defined to either GetObjectA or GetObjectW
#ifdef GetObject
#undef GetObject
#endif // GetObject

#ifdef CreateFont
#undef CreateFont
#endif // CreateFont

#ifdef DrawText
#undef DrawText
#endif // DrawText

#endif // __WIN32

namespace PoDoFo {

class PdfFontMetrics;
class PdfVecObjects;

/** A private structure,
 *  which represents a font in the cache.
 */
struct TFontCacheElement {
    TFontCacheElement() 
        : m_pFont( NULL ),
	  m_pEncoding( NULL ),
	  m_bBold( false ),
	  m_bItalic( false ),
          m_bIsSymbolCharset (false)
    {
    }

    TFontCacheElement( const char* pszFontName, bool bBold, bool bItalic, bool bIsSymbolCharset,
		               const PdfEncoding * const pEncoding )
        : m_pFont(NULL), m_pEncoding( pEncoding ), m_bBold( bBold ), m_bItalic( bItalic ),
          m_sFontName( reinterpret_cast<const pdf_utf8*>(pszFontName) ), m_bIsSymbolCharset (bIsSymbolCharset)
    {
    }

#if defined(_WIN32) && !defined(PODOFO_NO_FONTMANAGER)
    TFontCacheElement( const wchar_t* pszFontName, bool bBold, bool bItalic, bool bIsSymbolCharset,
		       const PdfEncoding * const pEncoding )
        : m_pFont(NULL), m_pEncoding( pEncoding ), m_bBold( bBold ), 
          m_bItalic( bItalic ), m_sFontName( pszFontName ), m_bIsSymbolCharset (bIsSymbolCharset)
    {
    }
#endif // _WIN32

    TFontCacheElement( const TFontCacheElement & rhs ) 
    {
        this->operator=(rhs);
    }
    
    const TFontCacheElement & operator=( const TFontCacheElement & rhs ) 
    {
        m_pFont     = rhs.m_pFont;
        m_pEncoding = rhs.m_pEncoding;
        m_bBold     = rhs.m_bBold;
        m_bItalic   = rhs.m_bItalic;
        m_sFontName = rhs.m_sFontName;
        m_bIsSymbolCharset = rhs.m_bIsSymbolCharset;
        
        return *this;
    }
    
    bool operator<( const TFontCacheElement & rhs ) const
    {
		  if (m_bIsSymbolCharset != rhs.m_bIsSymbolCharset) {
			   return m_bIsSymbolCharset < rhs.m_bIsSymbolCharset;
		  }
        if( m_sFontName == rhs.m_sFontName ) 
        {
            if( m_pEncoding == NULL  ||  rhs.m_pEncoding == NULL  ||  *m_pEncoding == *rhs.m_pEncoding ) 
            {
                if( m_bBold == rhs.m_bBold) 
                    return m_bItalic < rhs.m_bItalic;
                else
                    return m_bBold < rhs.m_bBold;
            }
            else
                return *m_pEncoding < *rhs.m_pEncoding;
        }
        else
            return (m_sFontName < rhs.m_sFontName);
    }
    
    inline bool operator()( const TFontCacheElement& r1, 
			    const TFontCacheElement& r2 ) const 
    { 
        return r1 < r2;
    }

    PdfFont*           m_pFont;
    const PdfEncoding* m_pEncoding;
    bool               m_bBold;
    bool               m_bItalic;
    PdfString          m_sFontName; ///< We use PdfString here as it can easily handle unicode on windows
    bool               m_bIsSymbolCharset;
};

/**
 * This class assists PdfDocument
 * with caching font information.
 *
 * Additional to font caching, this class is also
 * responsible for font matching.
 *
 * PdfFont is an actual font that can be used in
 * a PDF file (i.e. it does also font embedding)
 * and PdfFontMetrics provides only metrics informations.
 *
 * This class is an internal class of PoDoFo
 * and should not be used in user applications
 *
 * \see PdfDocument
 */
class PODOFO_DOC_API PdfFontCache {
    typedef std::vector<TFontCacheElement>  TSortedFontList;
    typedef TSortedFontList::iterator       TISortedFontList;
    typedef TSortedFontList::const_iterator TCISortedFontList;

 public:

    /**
     * Flags to control font creation.
     */
    enum EFontCreationFlags {
        eFontCreationFlags_None = 0,				///< No special settings
        eFontCreationFlags_AutoSelectBase14 = 1,	///< Create automatically a base14 font if the fontname matches one of them
        eFontCreationFlags_Type1Subsetting = 2		///< Create subsetted type1-font, which includes only used characters
    };

    /** Create an empty font cache 
     *
     *  \param pParent a PdfVecObjects which is required
     *                 to create new font objects
     */
    PdfFontCache( PdfVecObjects* pParent );

    /** Create an empty font cache 
     *
     *  \param rFontConfig provide a handle to fontconfig, as initializing a 
     *         new fontconfig intance might be time consuming.
     *  \param pParent a PdfVecObjects which is required
     *                 to create new font objects
     */
    PdfFontCache( const PdfFontConfigWrapper & rFontConfig, PdfVecObjects* pParent );

    /** Destroy and empty the font cache
     */
    ~PdfFontCache();

    /** 
     * Empty the internal font cache.
     * This should be done when ever a new document
     * is created or openened.
     */
    void EmptyCache();

    /** Get a font from the cache. If the font does not yet
     *  exist, add it to the cache. This font is created
     *  from an existing object.
     *
     *  \param pObject a PdfObject that is a font
     *
     *  \returns a PdfFont object or NULL if the font could
     *           not be created or found.
     */
    PdfFont* GetFont( PdfObject* pObject );

    /** Get a font from the cache. If the font does not yet
     *  exist, add it to the cache.
     *
     *  \param pszFontName a valid fontname
     *  \param bBold if true search for a bold font
     *  \param bItalic if true search for an italic font
	  *  \param bSymbolCharset whether to use symbol charset, rather than unicode charset
     *  \param bEmbedd if true a font for embedding into 
     *                 PDF will be created
     *  \param eFontCreationFlags special flag to specify how fonts should be created
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *  \param pszFileName optional path to a valid font file
     *
     *  \returns a PdfFont object or NULL if the font could
     *           not be created or found.
     */
    PdfFont* GetFont( const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                      bool bEmbedd, EFontCreationFlags eFontCreationFlags = eFontCreationFlags_AutoSelectBase14,
                      const PdfEncoding * const = PdfEncodingFactory::GlobalWinAnsiEncodingInstance(), 
                      const char* pszFileName = NULL );

#if defined(_WIN32) && !defined(PODOFO_NO_FONTMANAGER)
    /** Get a font from the cache. If the font does not yet
     *  exist, add it to the cache.
     *
     *  \param pszFontName a valid fontname
     *  \param bBold if true search for a bold font
     *  \param bItalic if true search for an italic font
	  *  \param bSymbolCharset whether to use symbol charset, rather than unicode charset
     *  \param bEmbedd if true a font for embedding into 
     *                 PDF will be created
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *
     *  \returns a PdfFont object or NULL if the font could
     *           not be created or found.
 	 *
     *  This is an overloaded member function to allow working
     *  with unicode characters. On Unix systes you can also path
     *  UTF-8 to the const char* overload.
     */
    PdfFont* GetFont( const wchar_t* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                      bool bEmbedd, const PdfEncoding * const = PdfEncodingFactory::GlobalWinAnsiEncodingInstance() );

	 PdfFont* GetFont( const LOGFONTA &logFont, bool bEmbedd, const PdfEncoding * const pEncoding );
	 PdfFont* GetFont( const LOGFONTW &logFont, bool bEmbedd, const PdfEncoding * const pEncoding );

#endif // _WIN32

    /** Get a font from the cache. If the font does not yet
     *  exist, add it to the cache.
     *
     *  \param face a valid freetype font face (will be free'd by PoDoFo)
	  *  \param bSymbolCharset whether to use symbol charset, rather than unicode charset
     *  \param bEmbedd if true a font for embedding into 
     *                 PDF will be created
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *
     *  \returns a PdfFont object or NULL if the font could
     *           not be created or found.
     */
    PdfFont* GetFont( FT_Face face, bool bSymbolCharset, bool bEmbedd, const PdfEncoding * const = PdfEncodingFactory::GlobalWinAnsiEncodingInstance() );

    /** Get a font with specific id from the cache. If the font does not yet
     *  exist, copy from existing type1-font and set id.
     *
     *  \param pFont an existing font
     *  \param pszSuffix Suffix to add to font-id 
     *
     *  \returns a PdfFont object or NULL if the font could
     *           not be created or found.
     */
	PdfFont* GetDuplicateFontType1( PdfFont* pFont, const char* pszSuffix );

    /** Get a fontsubset from the cache. If the font does not yet
     *  exist, add it to the cache.
     *
     *  \param pszFontName a valid fontname
     *  \param bBold if true search for a bold font
     *  \param bItalic if true search for an italic font
	  *  \param bSymbolCharset whether to use symbol charset, rather than unicode charset
     *  \param pEncoding the encoding of the font. All characters
     *                   of the encoding will be included in this subset.
     *                   The font will not take ownership of this object.     
     *  \param pszFileName optional path to a valid font file
     *
     *  \returns a PdfFont object or NULL if the font could
     *           not be created or found.
     */
    PdfFont* GetFontSubset( const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
			    const PdfEncoding * const = PdfEncodingFactory::GlobalWinAnsiEncodingInstance(),
			    const char* pszFileName = NULL);

    /** Embeds all pending subset-fonts
     *
     */
	void EmbedSubsetFonts();

#if defined(PODOFO_HAVE_FONTCONFIG)
    /** Get the path of a font file on a Unix system using fontconfig
     *
     *  This method is only available if PoDoFo was compiled with
     *  fontconfig support. Make sure to lock any FontConfig mutexes before
     *  calling this method by yourself!
     *
     *  \param pConfig a handle to an initialized fontconfig library
     *  \param pszFontName name of the requested font
     *  \param bBold if true find a bold font
     *  \param bItalic if true find an italic font
     *  \returns the path to the fontfile or an empty string
     */
    static std::string GetFontConfigFontPath( FcConfig* pConfig, const char* pszFontName, bool bBold, bool bItalic );
#endif // defined(PODOFO_HAVE_FONTCONFIG)

    // Peter Petrov: 26 April 2008
    /** Returns the font library from font cache
     *
     *  \returns the internal handle to the freetype library
     */
    inline FT_Library GetFontLibrary() const;

    /**
     * Set wrapper for the fontconfig library.
     * Useful to avoid initializing Fontconfig multiple times.
     *
     * This setter can be called until first use of Fontconfig
     * as the library is initialized at first use.
     */
    inline void SetFontConfigWrapper(const PdfFontConfigWrapper & rFontConfig);

 private:
    /**
     * Get the path to a font file for a certain fontname
     *
     * \param pszFontName a valid fontname
     * \param bBold if true search for a bold font
     * \param bItalic if true search for an italic font
     *
     * \returns the path to the fonts file if it was found.
     */
    std::string GetFontPath( const char* pszFontName, bool bBold, bool bItalic );

    /** Create a font and put it into the fontcache
     *
     *  \param itSorted iterator pointing to a location in vecContainer
     *                  where a sorted insert can be made
     *  \param vecContainer container where the font object should be added
     *  \param pMetrics a font metrics
     *  \param bEmbedd if true the font will be embedded in the pdf file
     *  \param bBold if true this font will be treated as bold font
     *  \param bItalic if true this font will be treated as italic font
     *  \param pszFontName a font name for debug output
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *  \param bSubsetting if true the font will be subsetted in the pdf file
     *
     *  \returns a font handle or NULL in case of error
     */
    PdfFont* CreateFontObject( TISortedFontList itSorted, TSortedFontList & vecContainer,
                               PdfFontMetrics* pMetrics, bool bEmbedd, bool bBold, 
                               bool bItalic, const char* pszFontName, const PdfEncoding * const pEncoding,
							   bool bSubsetting = false );

    /** Create a font subset.
     *  \param pMetrics a font metrics
     *  \param pszFontName a font name for debug output
     *  \param bBold if true this font will be treated as bold font
     *  \param bItalic if true this font will be treated as italic font
     *  \param vecCharacters a list of Unicode character indeces that should be embedded in the subset
     *
     *  \returns a font handle or NULL in case of error
     */
    /*
    PdfFont* CreateFontSubset( PdfFontMetrics* pMetrics, const char* pszFontName, bool bBold, 
                               bool bItalic, const std::vector<int> & vecCharacters );
    */
#if defined(_WIN32) && !defined(PODOFO_NO_FONTMANAGER)
    /** Load and create a font with windows API calls
     *
     *  This method is only available on Windows systems.
     * 
     *  \param itSorted iterator pointing to a location in vecContainer
     *                  where a sorted insert can be made
     *  \param vecContainer container where the font object should be added
     *  \param pszFontName a fontname
     *  \param bBold if true search for a bold font
     *  \param bItalic if true search for an italic font
     *  \param bEmbedd if true embedd the font
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *
     *  \returns a font handle or NULL in case of error
     */
    PdfFont* GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, const char* pszFontName, 
			               bool bBold, bool bItalic, bool bSymbolCharset, bool bEmbedd, const PdfEncoding * const pEncoding, bool pSubsetting = false );

    PdfFont* GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, const wchar_t* pszFontName, 
			               bool bBold, bool bItalic, bool bSymbolCharset, bool bEmbedd, const PdfEncoding * const pEncoding, bool pSubsetting = false );

    PdfFont* GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, const LOGFONTA &logFont,
								bool bEmbedd, const PdfEncoding * const pEncoding, bool pSubsetting = false );

    PdfFont* GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, const LOGFONTW &logFont,
								bool bEmbedd, const PdfEncoding * const pEncoding, bool pSubsetting = false );
#endif // _WIN32

	#define SUBSET_BASENAME_LEN 6 // + 2 for "+\0"

	// kind of ABCDEF+
	const char *genSubsetBasename(void);

 protected:
    void Init(void);
	
 private:
    TSortedFontList m_vecFonts;              ///< Sorted list of all fonts, currently in the cache
    TSortedFontList m_vecFontSubsets;
    FT_Library      m_ftLibrary;             ///< Handle to the freetype library

    PdfVecObjects*  m_pParent;               ///< Handle to parent for creating new fonts and objects

    PdfFontConfigWrapper m_fontConfig;       ///< Handle to the fontconfig library

    char m_sSubsetBasename[SUBSET_BASENAME_LEN + 2]; //< For genSubsetBasename()
};

// Peter Petrov: 26 April 2008
// -----------------------------------------------------
// 
// -----------------------------------------------------
FT_Library PdfFontCache::GetFontLibrary() const
{
    return this->m_ftLibrary;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfFontCache::SetFontConfigWrapper(const PdfFontConfigWrapper & rFontConfig)
{
    m_fontConfig = rFontConfig;
}

};

#endif /* _PDF_FONT_CACHE_H_ */

