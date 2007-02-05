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
#include "PdfFontCache.h"
#include "PdfObject.h"
#include "PdfParser.h"
#include "PdfWriter.h"

namespace PoDoFo {

class PdfDestination;
class PdfDictionary;
class PdfFileSpec;
class PdfFont;
class PdfImmediateWriter;
class PdfInfo;
class PdfNamesTree;
class PdfOutlines;
class PdfPage;
class PdfPagesTree;
class PdfRect;

/** PdfDocument is the core class for reading and manipulating
 *  PDF files and writing them back to disk.
 *
 *  PdfDocument provides easy access to the individual pages
 *  in the PDF file and to certain special dictionaries.
 *
 *  PdfDocument should be used whenever you want to change
 *  the object structure of a PDF file.
 *
 *  When you are only creating PDF files, please use PdfStreamedDocument
 *  which is usually faster for creating PDFs.
 *
 *  \see PdfStreamedDocument
 *  \see PdfParser
 *  \see PdfWriter
 */
class PODOFO_API PdfDocument {
    friend class PdfWriter;

 public:

    /** Construct a new (empty) PdfDocument
     */
    PdfDocument();
    
    /** Construct a PdfDocument from an existing PDF (on disk)
     *  \param pszFilename filename of the file which is going to be parsed/opened
     */
    PdfDocument( const char* pszFilename );

    /** Close down/destruct the PdfDocument
     */
    virtual ~PdfDocument();

    /** Load a PdfDocument from a file
     *
     *  \param pszFilename filename of the file which is going to be parsed/opened
     */
    void Load( const char* pszFilename );

    /** Writes the complete document to a file
     *
     *  \param pszFilename filename of the document 
     *
     *  \see Write
     *
     *  This is an overloaded member function for your convinience.
     */
    void Write( const char* pszFilename );

    /** Writes the complete document to an output device
     *
     *  \param pDevice write to this output device
     */
    void Write( PdfOutputDevice* pDevice );

    /** Write the PDF immediately while all objects are
     *  created. This is much faster, if you are only
     *  creating PDF files and are not changing any objects
     *  after they have been created.
     *
     *  This method may only be called once and should be 
     *  the first method to call after constructing the PdfDocument.
     *
     *  \param pDevice write to this device.
     */
    void WriteImmediately( PdfOutputDevice* pDevice );

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
     *  You can set the author, title etc. of the
     *  document using the info dictionary.
     *
     *  \returns the info dictionary
     */
    PdfInfo* GetInfo() const { return m_pInfo; }

    /** Get access to the StructTreeRoot dictionary
     *  \returns PdfObject the StructTreeRoot dictionary
     */
    PdfObject* GetStructTreeRoot() const { return GetNamedObjectFromCatalog( "StructTreeRoot" ); }

    /** Get access to the Metadata stream
     *  \returns PdfObject the Metadata stream (should be in XML, using XMP grammar)
     */
    PdfObject* GetMetadata() const { return GetNamedObjectFromCatalog( "Metadata" ); }

    /** Get access to the Outlines (Bookmarks) dictionary
     *  The returned outlines object is owned by the PdfDocument.
     * 
     *  \returns the Outlines/Bookmarks dictionary
     */
    PdfOutlines* GetOutlines( bool bCreate = ePdfCreateObject );

    /** Get access to the Names dictionary (where all the named objects are stored)
     *  The returned PdfNamesTree object is owned by the PdfDocument.
     * 
     *  \returns the Names dictionary
     */
    PdfNamesTree* GetNamesTree( bool bCreate = ePdfCreateObject );

    /** Get access to the AcroForm dictionary
     *  \returns PdfObject the AcroForm dictionary
     */
    PdfObject* GetAcroForm() const { return GetNamedObjectFromCatalog( "AcroForm" ); }

    /** Get access to the pages tree.
     *  Better use GetPage and CreatePage methods.
     *  \returns the PdfPagesTree of this document.
     */
    inline PdfPagesTree* GetPagesTree() const;

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

    /** Appends another PdfDocument to this document
     *  \param rDoc the document to append
     *  \returns this document
     */
    const PdfDocument & Append( const PdfDocument & rDoc );

    /** Attach a file to the document.
     *  \param rFileSpec a file specification
     */
    void AttachFile( const PdfFileSpec & rFileSpec );

    /** Copies one or more pages from another PdfDocument to this document
     *  \param rDoc the document to append
     *  \param inFirstPage the first page number to copy (0-based)
     *  \param inNumPages the number of pages to copy
     *  \returns this document
     */
    const PdfDocument & InsertPages( const PdfDocument & rDoc, int inFirstPage, int inNumPages );

    /** Deletes one or more pages from this document
     *  \param inFirstPage the first page number to delete (0-based)
     *  \param inNumPages the number of pages to delete
     *  \returns this document
     */
    void DeletePages( int inFirstPage, int inNumPages );

    /** Adds a PdfDestination into the global Names tree
     *  with the specified name, optionally replacing one of the same name
     *  \param rDest the destination to be assigned
     *  \param rsName the name for the destination
     */
    void AddNamedDestination( const PdfDestination& rDest, const PdfString & rsName );

    /** Sets the opening mode for a document
     *  \param inMode which mode to set
     */
    void SetPageMode( EPdfPageMode inMode ) const;

    /** Gets the opening mode for a document
     *  \returns which mode is set
     */
    EPdfPageMode GetPageMode( void ) const;

    /** Sets the opening mode for a document to be in full screen
     */
    void SetUseFullScreen( void ) const;
    
    /** Sets the page layout for a document
     */
    void SetPageLayout( EPdfPageLayout inLayout );
    
    /** Set the document's Viewer Preferences:
     *  Hide the toolbar in the viewer
     */
    void SetHideToolbar( void );

    /** Set the document's Viewer Preferences:
     *  Hide the menubar in the viewer
     */
    void SetHideMenubar( void );

    /** Set the document's Viewer Preferences:
     *  Show only the documents contents and no controll
     *  elements such as buttons and scrollbars in the viewer
     */
    void SetHideWindowUI( void );

    /** Set the document's Viewer Preferences:
     *  Fit the document in the viewers window
     */
    void SetFitWindow( void );

    /** Set the document's Viewer Preferences:
     *  Center the document in the viewers window
     */
    void SetCenterWindow( void );

    /** Set the document's Viewer Preferences:
     *  Display the title from the document information
     *  in the title of the viewer.
     * 
     *  \see SetTitle
     */
    void SetDisplayDocTitle( void );

    /** Set the document's Viewer Preferences:
     *  Set the default print scaling of the document
     */   
    void SetPrintScaling( PdfName& inScalingType );

    /** Set the document's Viewer Preferences:
     *  Set the base URI of the document
     */
    void SetBaseURI( const std::string& inBaseURI );

    /** Set the document's Viewer Preferences:
     *  Set the language of the document
     */    
    void SetLanguage( const std::string& inLanguage );

    /** Set the document's Viewer Preferences:
     */    
    void SetBindingDirection( PdfName& inDirection );

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

    /** Clear all internal variables
     */
    void Clear();

    /** Recursively changes every PdfReference in the PdfObject and in any child
     *  that is either an PdfArray or a direct object.
     *  The reference is changed so that difference is added to the object number
     *  if the reference.
     *  \param pObject object to change
     *  \param difference add this value to every reference that is encountered
     */
    void FixObjectReferences( PdfObject* pObject, int difference );

    /** Low level APIs for setting a viewer preference
     *  \param whichPrefs the dictionary key to set
     *  \param the object to be set
     */
    void SetViewerPreference( const PdfName& whichPref, const PdfObject & valueObj ) const;
    void SetViewerPreference( const PdfName& whichPref, bool inValue ) const;

 private:
    bool            m_bLinearized;

    PdfVecObjects   m_vecObjects;

    PdfInfo*        m_pInfo;
    PdfOutlines*    m_pOutlines;
    PdfNamesTree*   m_pNamesTree;
    PdfPagesTree*   m_pPagesTree;

    PdfObject*      m_pTrailer;
    PdfObject*      m_pCatalog;

    EPdfVersion     m_eVersion;

    PdfFontCache    m_fontCache;

    PdfImmediateWriter* m_pImmediate;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfPagesTree* PdfDocument::GetPagesTree() const
{
    return m_pPagesTree;
}

};


#endif	// _PDF_DOCUMENT_H_
