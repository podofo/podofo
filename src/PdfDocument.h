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

namespace PoDoFo {

class PdfDictionary;
class PdfPage;
class PdfPagesTree;

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

    /** Set the PDF Version of the document. Has to be called before Write() to
     *  have an effect.
     *  \param eVersion  version of the pdf document
     */
    void SetPdfVersion( EPdfVersion eVersion ) { m_eVersion = eVersion;}

    /** Get the PDF version of the document
     *  \returns EPdfVersion version of the pdf document
     */
    EPdfVersion GetPdfVersion() const { return m_eVersion; }
    
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
     *  \returns PdfObject the info dictionary
     */
    PdfObject* GetInfo() const { return m_pCatalog->GetIndirectKey( PdfName( "Info" ) ); }

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

    /** Get the PdfObject for a specific page in a document
     *  \param nIndex which page (0-based)
     *  \returns PdfObject* for the Page
     */
    PdfPage* GetPage( int nIndex ) const;

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

 private:
    bool            m_bLinearized;

    PdfVecObjects   m_vecObjects;

    PdfPagesTree*   m_pPagesTree;
    PdfObject*      m_pTrailer;
    PdfObject*      m_pCatalog;

    EPdfVersion     m_eVersion;
};

};


#endif	// _PDF_DOCUMENT_H_
