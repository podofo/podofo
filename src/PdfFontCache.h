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

#ifndef _PDF_FONT_CACHE_H_
#define _PDF_FONT_CACHE_H_

#include "PdfDefines.h"
#include "Pdf3rdPtyForwardDecl.h"
#include "PdfFont.h"

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
    m_bItalic( false )
    {
    }
    
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
        
        return *this;
    }
    
    bool operator<( const TFontCacheElement & rhs ) const
    {
        if( m_sFontName == rhs.m_sFontName ) 
        {
            if( m_pEncoding == rhs.m_pEncoding ) 
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

    PdfFont*           m_pFont;
    const PdfEncoding* m_pEncoding;
    bool               m_bBold;
    bool               m_bItalic;
    std::string        m_sFontName;
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
class PODOFO_API PdfFontCache {
 public:
    /** Create an empty font cache 
     *
     *  \param pParent a PdfVecObjects which is required
     *                 to create new font objects
     */
    PdfFontCache( PdfVecObjects* pParent );

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
     *  exist, add it to the cache.
     *
     *  \param pszFontName a valid fontname
     *  \param bBold if true search for a bold font
     *  \param bItalic if true search for an italic font
     *  \param bEmbedd if true a font for embedding into 
     *                 PDF will be created
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *  \param optional: pszFileName path to a valid font file
     *
     *  \returns a PdfFont object or NULL if the font could
     *           not be created or found.
     */
    PdfFont* GetFont( const char* pszFontName, bool bBold, bool bItalic, 
                      bool bEmbedd, const PdfEncoding * const = &PdfFont::WinAnsiEncoding, 
                      const char* pszFileName = NULL );

    /** Get a font from the cache. If the font does not yet
     *  exist, add it to the cache.
     *
     *  \param face a valid freetype font face (will be free'd by PoDoFo)
     *  \param bEmbedd if true a font for embedding into 
     *                 PDF will be created
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *
     *  \returns a PdfFont object or NULL if the font could
     *           not be created or found.
     */
    PdfFont* GetFont( FT_Face face, bool bEmbedd, const PdfEncoding * const = &PdfFont::WinAnsiEncoding );

    /** Get a fontsubset from the cache. If the font does not yet
     *  exist, add it to the cache.
     *
     *  \param pszFontName a valid fontname
     *  \param bBold if true search for a bold font
     *  \param bItalic if true search for an italic font
     *  \param vecGlyphs a list of Unicode glyph indeces that should be embedded in the subset
     *
     *  \returns a PdfFont object or NULL if the font could
     *           not be created or found.
     */
    /*
    PdfFont* GetFontSubset( const char* pszFontName, bool bBold, 
                            bool bItalic, const std::vector<int> & vecGlyphs );
    */
    
#if defined(HAVE_FONTCONFIG)
    /** Get the path of a font file on a Unix system using fontconfig
     *
     *  This method is only available if PoDoFo was compiled with
     *  fontconfig support.
     *
     *  \param pConfig a handle to an initialized fontconfig library
     *  \param pszFontName name of the requested font
     *  \param bBold if true find a bold font
     *  \param bItalic if true find an italic font
     *  \returns the path to the fontfile or an empty string
     */
    static std::string GetFontConfigFontPath( FcConfig* pConfig, const char* pszFontName, bool bBold, bool bItalic );
#endif // (HAVE_FONTCONFIG)

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
     *  \param pMetrics a font metrics
     *  \param bEmbedd if true the font will be embedded in the pdf file
     *  \param bBold if true this font will be treated as bold font
     *  \param bItalic if true this font will be treated as italic font
     *  \param pszFontName a font name for debug output
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *
     *  \returns a font handle or NULL in case of error
     */
    PdfFont* CreateFontObject( PdfFontMetrics* pMetrics, bool bEmbedd, bool bBold, 
                               bool bItalic, const char* pszFontName, const PdfEncoding * const pEncoding );

    /** Create a font subset.
     *  \param pMetrics a font metrics
     *  \param pszFontName a font name for debug output
     *  \param bBold if true this font will be treated as bold font
     *  \param bItalic if true this font will be treated as italic font
     *  \param vecGlyphs the list of all glyphs that should get embedded into this subset
     *
     *  \returns a font handle or NULL in case of error
     */
    /*
    PdfFont* CreateFontSubset( PdfFontMetrics* pMetrics, const char* pszFontName, bool bBold, 
                               bool bItalic, const std::vector<int> & vecGlyphs );
    */
#ifdef _WIN32
    /** Load and create a font with windows API calls
     *
     *  This method is only available on Windows systems.
     * 
     *  \param pszFontName a fontnanme
     *  \param bBold if true search for a bold font
     *  \param bItalic if true search for an italic font
     *  \param bEmbedd if true embedd the font
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *
     *  \returns a font handle or NULL in case of error
     */
    PdfFont* GetWin32Font( const char* pszFontName, bool bBold, bool bItalic, bool bEmbedd, const PdfEncoding * const pEncoding );
#endif // _WIN32

 private:
    typedef std::vector<TFontCacheElement>  TSortedFontList;
    typedef TSortedFontList::iterator       TISortedFontList;
    typedef TSortedFontList::const_iterator TCISortedFontList;

    TSortedFontList m_vecFonts;              ///< Sorted list of all fonts, currently in the cache
    TSortedFontList m_vecFontSubsets;
    FT_Library      m_ftLibrary;             ///< Handle to the freetype library

    void*           m_pFcConfig;             ///< Handle to fontconfig on unix systems

    PdfVecObjects*  m_pParent;               ///< Handle to parent for creating new fonts and objects
};

};

#endif /* _PDF_FONT_CACHE_H_ */

