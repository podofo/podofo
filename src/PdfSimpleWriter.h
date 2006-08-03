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
 ***************************************************************************/

#ifndef _PDF_SIMPLE_WRITER_H_
#define _PDF_SIMPLE_WRITER_H_

#include "PdfDefines.h"
#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfVariant.h"
#include "PdfWriter.h"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace PoDoFo {

class PdfObject;
class PdfPage;
class PdfFont;
class PdfImage;
class PdfRect;
class PdfString;

typedef std::vector<PdfFont*>           TSortedFontList;
typedef TSortedFontList::iterator       TISortedFontList;
typedef TSortedFontList::const_iterator TCISortedFontList;

/** PdfSimpleWriter is more convinient to use than
 *  PdfWriter. When using PdfSimpleWriter one has almost never
 *  to create PDF dictionary entries by hand. 
 *  
 *  Most of the time it is just creating pages and drawing onto
 *  them using PdfPainter
 *
 *  \see CreatePage
 */
class PdfSimpleWriter : public PdfWriter {
 public:
    /** Creates a new PDF file from scratch
     */
    PdfSimpleWriter();

    /** Deletes the PdfSimpleWriter object
     */
    virtual ~PdfSimpleWriter();
    
    /** Creates a new page object and inserts it into the internal
     *  object tree. 
     *
     *  The returned PdfPage object has to be deleted by the caller.
     *  TODO: DS: Do not require the user to delete page objects.
     *
     *  \param rSize a PdfRect spezifying the size of the page (i.e the /MediaBox key) in 1/1000th mm
     *  \returns PdfObject pointer to the new PdfObject
     */
    PdfPage* CreatePage( const PdfRect & rSize );

    /** Creates a PdfFont object
     *  \param pszFontName name of the font as it is known to the system
     *  \param bEmbedd specifies whether this font should be embedded in the PDF file.
     *         Embedding fonts is usually a good idea.
     *  \returns PdfFont* a pointer to a new PdfFont object.
     */
    PdfFont* CreateFont( const char* pszFontName, bool bEmbedd = true );

    /** Set the author of the document.
     *  \param sAuthor author
     */
    void SetDocumentAuthor( const PdfString & sAuthor );

    /** Set the creator of the document.
     *  Typically the name of the application using the library.
     *  \param sCreator creator
     */
    void SetDocumentCreator( const PdfString & sCreator );

    /** Set keywords for this document
     *  \param sKeywords a list of keywords
     */
    void SetDocumentKeywords( const PdfString & sKeywords );

    /** Set the subject of the document.
     *  \param sSubject subject
     */
    void SetDocumentSubject( const PdfString & sSubject );

    /** Set the title of the document.
     *  \param sTitle title
     */
    void SetDocumentTitle( const PdfString & sTitle );

 private:
    /** Create a new pdf file from scratch.
     */
    void Init();

 private:
    PdfObject*      m_pPageTree;
    unsigned int    m_nPageTreeSize;

    PdfArray        m_vecPageReferences;
    TSortedFontList m_vecFonts;

    void*           m_pFcConfig; // (FcConfig*)
    FT_Library      m_ftLibrary;

    bool            m_bInitDone;
};

};

#endif // _PDF_SIMPLE_WRITER_H_
