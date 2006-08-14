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

#ifndef _PDF_DOCUMENT_H_
#define _PDF_DOCUMENT_H_

#include "PdfDefines.h"
#include "PdfObject.h"
#include "PdfParser.h"
#include "PdfWriter.h"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace PoDoFo {

class PdfDictionary;
class PdfFont;
class PdfPage;
class PdfPagesTree;
class PdfRect;

typedef std::vector<PdfFont*>           TSortedFontList;
typedef TSortedFontList::iterator       TISortedFontList;
typedef TSortedFontList::const_iterator TCISortedFontList;

/** PdfDocument is the core class for reading and manipulating
 *  PDF files and writing them back to disk.
 *
 *  PdfDocument provides easy access to the individual pages
 *  in the PDF file and to certain special dictionaries.
 *
 *  \see PdfParser
 *  \see PdfWriter
 */
class PdfDocument {
    friend class PdfWriter;

 public:

    /** Construct a new (empty) PdfDocument
     */
    PdfDocument();
    
    /** Construct a PdfDocument from an existing PDF (on disk)
     *  \param pszFilename filename of the file which is going to be parsed/opened
     */
    PdfDocument( const char* pszFilename );

    /** Construct a PdfDocument from a PdfParser object.
     *
     *  The objects will be removed from the parser and are now
     *  owned by the PdfDocument.
     *
     *  \param pParser pointer to a PdfParser
     */
    PdfDocument( PdfParser* pParser );

    /** Close down/destruct the PdfDocument
     */
    virtual ~PdfDocument();

    /** Load a PdfDocument from a file
     *  \param pszFilename filename of the file which is going to be parsed/opened
     */
    void Load( const char* pszFilename );

    /** Writes the complete document to a file
     *
     *  \param pszFilename filename of the document 
     */
    void Write( const char* pszFilename );

    /** Set the PDF Version of the document. Has to be called before Write() to
     *  have an effect.
     *  \param eVersion  version of the pdf document
     */
    void SetPdfVersion( EPdfVersion eVersion ) { m_eVersion = eVersion;}

    /** Get the PDF version of the document
     *  \returns EPdfVersion version of the pdf document
     */
    EPdfVersion GetPdfVersion() const { return m_eVersion; }

    /** Returns wether this PDF document is linearized, aka
     *  weboptimized
     *  \returns true if the PDF document is linearized
     */
    bool IsLinearized() const { return m_bLinearized; }
    
    /** Get a reference to the sorted internal objects vector.
     *  \returns the internal objects vector.
     */
    const PdfVecObjects & GetObjects() const { return m_vecObjects; }

    /** Get a reference to the sorted internal objects vector.
     *  This is an overloaded function for your convinience.
     *  \returns the internal objects vector.
     */
    PdfVecObjects & GetObjects() { return m_vecObjects; }

    /** Get access to the internal Catalog dictionary
     *  or root object.
     *  
     *  \returns PdfObject the documents catalog or NULL 
     *                     if no catalog is available
     */
    PdfObject* GetCatalog() const { return m_pCatalog; }

    /** Get the trailer dictionary
     *  which can be written unmodified to a pdf file.
     */
    const PdfObject* GetTrailer() const { return m_pTrailer; }
    
    /** Get access to the internal Info dictionary
     *  \param bCreate if true the dictionary will
     *         be created if it does not exist
     *  \returns PdfObject the info dictionary
     */
    PdfObject* GetInfo( bool bCreate );

    /** Get access to the internal Info dictionary
     *  \returns PdfObject the info dictionary
     */
    PdfObject* GetInfo() const { return m_pTrailer->GetIndirectKey( PdfName( "Info" ) ); };

    /** Get access to the StructTreeRoot dictionary
     *  \returns PdfObject the StructTreeRoot dictionary
     */
    PdfObject* GetStructTreeRoot() const { return GetNamedObjectFromCatalog( "StructTreeRoot" ); }

    /** Get access to the Metadata stream
     *  \returns PdfObject the Metadata stream (should be in XML, using XMP grammar)
     */
    PdfObject* GetMetadata() const { return GetNamedObjectFromCatalog( "Metadata" ); }

    /** Get access to the Outlines (Bookmarks) dictionary
     *  \returns PdfObject the Outlines/Bookmarks dictionary
     */
    PdfObject* GetOutlines() const { return GetNamedObjectFromCatalog( "Outlines" ); }

    /** Get access to the AcroForm dictionary
     *  \returns PdfObject the AcroForm dictionary
     */
    PdfObject* GetAcroForm() const { return GetNamedObjectFromCatalog( "AcroForm" ); }

    /** Get the total number of pages in a document
     *  \returns int number of pages
     */
    int GetPageCount() const;

    /** Get the PdfPage for a specific page in a document
     *  The returned page is owned by the PdfDocument
     *  and will get deleted along with it!
     *
     *  \param nIndex which page (0-based)
     *  \returns a pointer to a PdfPage for the requested page
     */
    PdfPage* GetPage( int nIndex ) const;

    /** Creates a PdfFont object
     *  \param pszFontName name of the font as it is known to the system
     *  \param bEmbedd specifies whether this font should be embedded in the PDF file.
     *         Embedding fonts is usually a good idea.
     *  \returns PdfFont* a pointer to a new PdfFont object.
     */
    PdfFont* CreateFont( const char* pszFontName, bool bEmbedd = true );

    /** Creates a new page object and inserts it into the internal
     *  page tree. 
     *  The returned page is owned by the PdfDocument
     *  and will get deleted along with it!
     *
     *  \param rSize a PdfRect spezifying the size of the page (i.e the /MediaBox key) in 1/1000th mm
     *  \returns a pointer to a PdfPage object
     */
    PdfPage* CreatePage( const PdfRect & rSize );

    /** Set the author of the document.
     *  \param sAuthor author
     */
    void SetAuthor( const PdfString & sAuthor );

    /** Get the author of the document
     *  \returns the author
     */
    inline const PdfString & Author() const;

    /** Set the creator of the document.
     *  Typically the name of the application using the library.
     *  \param sCreator creator
     */
    void SetCreator( const PdfString & sCreator );

    /** Get the creator of the document
     *  \returns the creator
     */
    inline const PdfString & Creator() const;

    /** Set keywords for this document
     *  \param sKeywords a list of keywords
     */
    void SetKeywords( const PdfString & sKeywords );

    /** Get the keywords of the document
     *  \returns the keywords
     */
    inline const PdfString & Keywords() const;

    /** Set the subject of the document.
     *  \param sSubject subject
     */
    void SetSubject( const PdfString & sSubject );

    /** Get the subject of the document
     *  \returns the subject
     */
    inline const PdfString & Subject() const;

    /** Set the title of the document.
     *  \param sTitle title
     */
    void SetTitle( const PdfString & sTitle );

    /** Get the title of the document
     *  \returns the title
     */
    inline const PdfString & Title() const;

 private:
    /** Get a dictioary from the catalog dictionary by its name.
     *  \param pszName will be converted into a PdfName
     *  \returns the dictionary if it was found or NULL
     */
    PdfObject* GetNamedObjectFromCatalog( const char* pszName ) const;

    /** Internal method for initializing the pages tree for this document
     */
    void InitPagesTree();

    /** Internal method to load all objects from a PdfParser object.
     *  The objects will be removed from the parser and are now
     *  owned by the PdfDocument.
     */
    void InitFromParser( PdfParser* pParser );

    /** Initialize freetype and fontconfig
     */
    void InitFonts();

    /** Clear all internal variables
     */
    void Clear();

    /** Get a value from the info dictionary as name
     *  \para rName the key to fetch from the info dictionary
     *  \return a value from the info dictionary
     */
    const PdfString & GetStringFromInfoDict( const PdfName & rName ) const;

 private:
    bool            m_bLinearized;

    PdfVecObjects   m_vecObjects;

    PdfPagesTree*   m_pPagesTree;
    PdfObject*      m_pTrailer;
    PdfObject*      m_pCatalog;

    EPdfVersion     m_eVersion;

    // Variables for fontloading and redering
    TSortedFontList m_vecFonts;
    void*           m_pFcConfig; // (FcConfig*)
    FT_Library      m_ftLibrary;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfDocument::Author() const
{
    return this->GetStringFromInfoDict( PdfName("Author") );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfDocument::Creator() const
{
    return this->GetStringFromInfoDict( PdfName("Creator") );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfDocument::Keywords() const
{
    return this->GetStringFromInfoDict( PdfName("Keywords") );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfDocument::Subject() const
{
    return this->GetStringFromInfoDict( PdfName("Subject") );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfDocument::Title() const
{
    return this->GetStringFromInfoDict( PdfName("Title") );
}

};


#endif	// _PDF_DOCUMENT_H_
