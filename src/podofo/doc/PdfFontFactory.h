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

#ifndef _PDF_FONT_FACTORY_H_
#define _PDF_FONT_FACTORY_H_

#include "podofo/base/PdfDefines.h"
#include "PdfFont.h"

namespace PoDoFo {

class PdfFontMetrics;
class PdfVecObjects;

enum EPdfFontFlags {
    ePdfFont_Normal     = 0x00,
    ePdfFont_Embedded   = 0x01,
    ePdfFont_Bold       = 0x02,
    ePdfFont_Italic     = 0x04,
    ePdfFont_BoldItalic = ePdfFont_Bold | ePdfFont_Italic,
    ePdfFont_Subsetting = 0x08
};

/** This is a factory class which knows
 *  which implementation of PdfFont is required
 *  for a certain font type with certain features (like encoding).
 */
class PODOFO_DOC_API PdfFontFactory {
 public:

    /** Create a new PdfFont object.
     *
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is 
     *         deleted along with the created font. In case of an error, it is deleted
     *         here.
     *  \param nFlags font flags or'ed together, specifying the font style and if it should be embedded
     *  \param pEncoding the encoding of this font.
     *  \param pParent the parent of the created font.
     *
     *  \returns a new PdfFont object or NULL
     */
    static PdfFont* CreateFontObject( PdfFontMetrics* pMetrics, int nFlags, 
                                      const PdfEncoding* pEncoding, PdfVecObjects* pParent );

    /** Create a new PdfFont from an existing
     *  font in a PDF file.
     *
     *  \param pLibrary handle to the FreeType library, so that a PdfFontMetrics
     *         can be constructed for this font
     *  \param pObject a PDF font object
     */
    static PdfFont* CreateFont( FT_Library* pLibrary, PdfObject* pObject );

    /**
     *    Creates a new base-14 font object (of class PdfFontType1Base14) if
     *    the font name (has to include variant) is one of the base 14 fonts.
     *    The font name is to be given as specified (with an ASCII hyphen).
     *
     *    \param pszFontName ASCII C string (zero-terminated) of the font name
     *    \param eFlags one flag for font variant (Bold, Italic or BoldItalic)
     *    \param pEncoding an encoding compatible with the font
     *    \param pParent a vector of PDF objects to be the font object's owner
     */
    static PdfFont* CreateBase14Font( const char* pszFontName,
                                    EPdfFontFlags eFlags,
                                    const PdfEncoding * const pEncoding,
                                    PdfVecObjects *pParent );

    /** Try to guess the fonttype from a the filename of a font file.
     * 
     *  \param pszFilename filename of a fontfile
     *  \returns the font type
     */
    static EPdfFontType GetFontType( const char* pszFilename );

 private:
    // prohibit instantiation of all-methods-static factory from outside 
    PdfFontFactory();

    /** Actually creates the font object for the requested type.
     *  Throws an exception in case of an error.
     *
     *  \returns a new PdfFont object or NULL
     */
    static PdfFont* CreateFontForType( EPdfFontType eType, PdfFontMetrics* pMetrics, 
                                       const PdfEncoding* const pEncoding, 
                                       bool bEmbed, bool bSubsetting, PdfVecObjects* pParent );
};

};

#endif /* _PDF_FONT_FACTORY_H_ */


