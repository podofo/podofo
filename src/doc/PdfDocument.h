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

#ifndef _PDF_DOCUMENT_H_
#define _PDF_DOCUMENT_H_

#include "podofo/base/PdfDefines.h"

#include "podofo/base/PdfObject.h"
#include "podofo/base/PdfParser.h"
#include "podofo/base/PdfWriter.h"

#include "PdfAcroForm.h"
#include "PdfFontCache.h"
#include "PdfInfo.h"

namespace PoDoFo {

class PdfDestination;
class PdfDictionary;
class PdfFileSpec;
class PdfFont;
class PdfFontConfigWrapper;
class PdfInfo;
class PdfMemDocument;
class PdfNamesTree;
class PdfOutlines;
class PdfPage;
class PdfPagesTree;
class PdfRect;
class PdfXObject;

/** PdfDocument is the core interface for working with PDF documents.
 *
 *  PdfDocument provides easy access to the individual pages
 *  in the PDF file and to certain special dictionaries.
 *
 *  PdfDocument cannot be used directly.
 *  Use PdfMemDocument whenever you want to change the object structure
 *  of a PDF file. 
 *
 *  When you are only creating PDF files, please use PdfStreamedDocument
 *  which is usually faster for creating PDFs.
 *
 *  \see PdfStreamedDocument
 *  \see PdfMemDocument
 */
class PODOFO_DOC_API PdfDocument {
    friend class PdfElement;

 public:
    /** Close down/destruct the PdfDocument
     */
    virtual ~PdfDocument();

    /** Get the write mode used for wirting the PDF
     *  \returns the write mode
     */
    virtual EPdfWriteMode GetWriteMode() const = 0;

    /** Get the PDF version of the document
     *  \returns EPdfVersion version of the pdf document
     */
    virtual EPdfVersion GetPdfVersion() const = 0;

    /** Returns wether this PDF document is linearized, aka
     *  weboptimized
     *  \returns true if the PDF document is linearized
     */
    virtual bool IsLinearized() const = 0;
    
    /** Get access to the internal Info dictionary
     *  You can set the author, title etc. of the
     *  document using the info dictionary.
     *
     *  \returns the info dictionary
     */
    PdfInfo* GetInfo() const { return m_pInfo; }

    /** Get access to the Outlines (Bookmarks) dictionary
     *  The returned outlines object is owned by the PdfDocument.
     * 
     *  \param bCreate create the object if it does not exist (ePdfCreateObject) 
     *                 or return NULL if it does not exist
     *  \returns the Outlines/Bookmarks dictionary
     */
    PdfOutlines* GetOutlines( bool bCreate = ePdfCreateObject );

    /** Get access to the Names dictionary (where all the named objects are stored)
     *  The returned PdfNamesTree object is owned by the PdfDocument.
     * 
     *  \param bCreate create the object if it does not exist (ePdfCreateObject) 
     *                 or return NULL if it does not exist
     *  \returns the Names dictionary
     */
    PdfNamesTree* GetNamesTree( bool bCreate = ePdfCreateObject );

    /** Get access to the AcroForm dictionary
     *  
     *  \param bCreate create the object if it does not exist (ePdfCreateObject) 
     *                 or return NULL if it does not exist
     *  \param eDefaultAppearance specifies if a default appearence shall be created
     *
     *  \returns PdfObject the AcroForm dictionary
     */
    PdfAcroForm* GetAcroForm( bool bCreate = ePdfCreateObject,
                              EPdfAcroFormDefaulAppearance eDefaultAppearance = ePdfAcroFormDefaultAppearance_BlackText12pt);

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
     *  \returns a pointer to a PdfPage for the requested page.
     *           The returned object is owned by the PdfDocument.
     */
    PdfPage* GetPage( int nIndex ) const;

    /** Creates a PdfFont object
     *  \param pszFontName name of the font as it is known to the system
     *  \param bSymbolCharset whether to use symbol charset, rather than unicode charset
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *  \param eFontCreationFlags special flag to specify how fonts should be created
     *  \param bEmbedd specifies whether this font should be embedded in the PDF file.
     *         Embedding fonts is usually a good idea.
     *
     *  \returns PdfFont* a pointer to a new PdfFont object.
     *           The returned object is owned by the PdfDocument.
     */
    PdfFont* CreateFont( const char* pszFontName, bool bSymbolCharset = false, 
                         const PdfEncoding * const pEncoding = PdfEncodingFactory::GlobalWinAnsiEncodingInstance(), 
                         PdfFontCache::EFontCreationFlags eFontCreationFlags = PdfFontCache::eFontCreationFlags_AutoSelectBase14,
                         bool bEmbedd = true );

    /** Creates a PdfFont object
     *  \param pszFontName name of the font as it is known to the system
     *  \param bBold if true search for a bold font
     *  \param bItalic if true search for an italic font
     *  \param bSymbolCharset whether to use symbol charset, rather than unicode charset
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *  \param eFontCreationFlags special flag to specify how fonts should be created
     *  \param bEmbedd specifies whether this font should be embedded in the PDF file.
     *         Embedding fonts is usually a good idea.
     *  \param pszFileName path to a valid font file
     *
     *  \returns PdfFont* a pointer to a new PdfFont object.
     */
    PdfFont* CreateFont( const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset = false,
                         const PdfEncoding * const pEncoding = PdfEncodingFactory::GlobalWinAnsiEncodingInstance(), 
                         PdfFontCache::EFontCreationFlags eFontCreationFlags = PdfFontCache::eFontCreationFlags_AutoSelectBase14,
                         bool bEmbedd = true, const char* pszFileName = NULL );

#ifdef _WIN32
    /** Creates a PdfFont object
     *  \param pszFontName name of the font as it is known to the system
     *  \param bSymbolCharset whether to use symbol charset, rather than unicode charset
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *  \param bEmbedd specifies whether this font should be embedded in the PDF file.
     *         Embedding fonts is usually a good idea.
     *
     *  \returns PdfFont* a pointer to a new PdfFont object.
     *           The returned object is owned by the PdfDocument.
     *
     *  This is an overloaded member function to allow working
     *  with unicode characters. On Unix systes you can also path
     *  UTF-8 to the const char* overload.
     */
    PdfFont* CreateFont( const wchar_t* pszFontName, bool bSymbolCharset = false, const PdfEncoding * const pEncoding = PdfEncodingFactory::GlobalWinAnsiEncodingInstance(), 
                         bool bEmbedd = true );

    /** Creates a PdfFont object
     *  \param pszFontName name of the font as it is known to the system
     *  \param bBold if true search for a bold font
     *  \param bItalic if true search for an italic font
     *  \param bSymbolCharset whether to use symbol charset, rather than unicode charset
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *  \param bEmbedd specifies whether this font should be embedded in the PDF file.
     *         Embedding fonts is usually a good idea.
     *  \param optional: pszFileName path to a valid font file
     *
     *  \returns PdfFont* a pointer to a new PdfFont object.
     *
     *  This is an overloaded member function to allow working
     *  with unicode characters. On Unix systes you can also path
     *  UTF-8 to the const char* overload.
     */
    PdfFont* CreateFont( const wchar_t* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset = false,
                         const PdfEncoding * const pEncoding = PdfEncodingFactory::GlobalWinAnsiEncodingInstance(), 
                         bool bEmbedd = true);

	 PdfFont* CreateFont( const LOGFONTA &logFont, const PdfEncoding * const pEncoding = PdfEncodingFactory::GlobalWinAnsiEncodingInstance(),
								 bool bEmbedd = true );

	 PdfFont* CreateFont( const LOGFONTW &logFont, const PdfEncoding * const pEncoding = PdfEncodingFactory::GlobalWinAnsiEncodingInstance(),
								 bool bEmbedd = true );

#endif // _WIN32

    /** Creates a PdfFont object
     *  \param face a valid freetype font handle (will be free'd by PoDoFo)
	  *  \param bSymbolCharset whether to use symbol charset, rather than unicode charset
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *  \param bEmbedd specifies whether this font should be embedded in the PDF file.
     *         Embedding fonts is usually a good idea.
     *  \returns PdfFont* a pointer to a new PdfFont object.
     *           The returned object is owned by the PdfDocument.
     */
    PdfFont* CreateFont( FT_Face face, bool bSymbolCharset = false, const PdfEncoding * const pEncoding = PdfEncodingFactory::GlobalWinAnsiEncodingInstance(), bool bEmbedd = true );

    /** Creates a duplicate Type1-PdfFont with a new Id
     *  \param pFont is the existing font 
     *  \param pszSuffix Suffix to add to font-id 
     *           The returned object is owned by the PdfDocument.
     *
     *  TODO: DS: Make this generic so that it will work 
     *            for any font type!
     */
    PdfFont* CreateDuplicateFontType1( PdfFont * pFont, const char * pszSuffix );

	/** Creates a font subset which contains only a few characters and is embedded.
     *
     *  THIS WORKS ONLY FOR TTF FONTS!
     *
     *  \param pszFontName name of the font as it is known to the system
     *  \param bBold if true search for a bold font
     *  \param bItalic if true search for an italic font
	  *  \param bSymbolCharset whether to use symbol charset, rather than unicode charset
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *  \param pszFileName optional path of a fontfile which should be used
     *
     *  \returns PdfFont* a pointer to a new PdfFont object.
     */
    PdfFont* CreateFontSubset( const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset = false,
			       const PdfEncoding * const pEncoding = PdfEncodingFactory::GlobalWinAnsiEncodingInstance(),
			       const char* pszFileName = NULL);

#ifdef _WIN32
    /** Creates a font subset which contains only a few characters and is embedded.
     *
     *  THIS WORKS ONLY FOR TTF FONTS!
     *
     *  \param pszFontName name of the font as it is known to the system
     *  \param bBold if true search for a bold font
     *  \param bItalic if true search for an italic font
	  *  \param bSymbolCharset whether to use symbol charset, rather than unicode charset
     *  \param pEncoding the encoding of the font. The font will not take ownership of this object.     
     *
     *  \returns PdfFont* a pointer to a new PdfFont object.
     *
     *  This is an overloaded member function to allow working
     *  with unicode characters. On Unix systes you can also path
     *  UTF-8 to the const char* overload.
     */
    PdfFont* CreateFontSubset( const wchar_t* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset = false,
			       const PdfEncoding * const = PdfEncodingFactory::GlobalWinAnsiEncodingInstance() );
#endif // _WIN32

    // Peter Petrov 26 April 2008
    /** Returns the font library from font cache
     *
     *  \returns the internal handle to the freetype library
     */
    inline FT_Library GetFontLibrary() const;
	
    /** Embeds all pending subset-fonts, is automatically done on Write().
     *  Just call explicit in case PdfDocument is needed as XObject
     *
     */
    void EmbedSubsetFonts();

    /** Creates a new page object and inserts it into the internal
     *  page tree. 
     *  The returned page is owned by the PdfDocument
     *  and will get deleted along with it!
     *
     *  \param rSize a PdfRect spezifying the size of the page (i.e the /MediaBox key) in 1/1000th mm
     *  \returns a pointer to a PdfPage object
     */
    PdfPage* CreatePage( const PdfRect & rSize );


    /** Creates several new page objects and inserts them into the internal
     *  page tree. 
     *  The created pages are owned by the PdfDocument
     *  and will get deleted along with it!
     *
     *  \param vecSizes a vector PdfRect's specifying the size of the pages (i.e the /MediaBox key) in PDF Units
     */
    void CreatePages( const std::vector<PdfRect>& vecSizes );

    /** Creates a new page object and inserts it at index atIndex.
     *  The returned page is owned by the pages tree and will get deleted along
     *  with it!
     *
     *  \param rSize a PdfRect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \param atIndex index where to insert the new page (0-based)
     *  \returns a pointer to a PdfPage object
     */
    PdfPage* InsertPage( const PdfRect & rSize, int atIndex);

    /** Appends another PdfDocument to this document
     *  \param rDoc the document to append
     *  \param bAppendAll specifies whether pages and outlines are appended too
     *  \returns this document
     */
    const PdfDocument & Append( const PdfMemDocument & rDoc, bool bAppendAll = true  );

    /** Inserts existing page from another PdfMemDocument to this document
     *  \param rDoc the document to append from
     *  \param nPageIndex Page index to append (0-based), from rDoc
     *  \param nAtIndex Index at which add the page in this document
     *  \returns this document
     */
    const PdfDocument &InsertExistingPageAt( const PdfMemDocument & rDoc, int nPageIndex, int nAtIndex);

    /** Fill an existing empty XObject from a page of another document
     *  This will append the other document with this one
     *  \param pXObj pointer to the XOject
     *  \param rDoc the document to embedd into XObject
     *  \param nPage page-number to embedd into XObject
     *  \param bUseTrimBox if true try to use trimbox for size of xobject
     *  \returns the bounding box
     */
    PdfRect FillXObjectFromDocumentPage( PdfXObject * pXObj, const PdfMemDocument & rDoc, int nPage, bool bUseTrimBox );

    /** Fill an existing empty XObject from an existing page from the current document
     *  If you need a page from another document use FillXObjectFromDocumentPage, or append the documents manually
     *  \param pXObj pointer to the XOject
     *  \param nPage page-number to embedd into XObject
     *  \param bUseTrimBox if true try to use trimbox for size of xobject
     *  \returns the bounding box
     */
    PdfRect FillXObjectFromExistingPage( PdfXObject * pXObj, int nPage, bool bUseTrimBox );

    /** Fill an existing empty XObject from an existing page pointer from the current document
     *  This is the implementation for FillXObjectFromDocumentPage and FillXObjectFromExistingPage, you should use those directly instead of this
     *  \param pXObj pointer to the XOject
     *  \param pPage pointer to the page to embedd into XObject
     *  \param bUseTrimBox if true try to use trimbox for size of xobject
     *  \returns the bounding box
     */
    PdfRect FillXObjectFromPage( PdfXObject * pXObj, const PdfPage * pPage, bool bUseTrimBox, unsigned int difference );

    /** Attach a file to the document.
     *  \param rFileSpec a file specification
     */
    void AttachFile( const PdfFileSpec & rFileSpec );

    /** Get an attached file's filespec
     *  \param rName the name of the attachment
     *  \return the file specification object if the file exists, NULL otherwise
     *          The file specification object is not owned by the document and must be deleted by the caller
     */
    PdfFileSpec* GetAttachment( const PdfString & rName );
    
    /** Adds a PdfDestination into the global Names tree
     *  with the specified name, optionally replacing one of the same name
     *  \param rDest the destination to be assigned
     *  \param rsName the name for the destination
     */
    void AddNamedDestination( const PdfDestination& rDest, const PdfString & rsName );

    /** Sets the opening mode for a document
     *  \param inMode which mode to set
     */
    void SetPageMode( EPdfPageMode inMode );

    /** Gets the opening mode for a document
     *  \returns which mode is set
     */
    EPdfPageMode GetPageMode( void ) const;

    /** Sets the opening mode for a document to be in full screen
     */
    void SetUseFullScreen( void );
    
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
     *
     *  TODO: DS use an enum here!
     */   
    void SetPrintScaling( PdfName& inScalingType );

    /** Set the document's Viewer Preferences:
     *  Set the base URI of the document
     *
     *  TODO: DS document value!
     */
    void SetBaseURI( const std::string& inBaseURI );

    /** Set the document's Viewer Preferences:
     *  Set the language of the document
     */    
    void SetLanguage( const std::string& inLanguage );

    /** Set the document's Viewer Preferences:
     */    
    void SetBindingDirection( PdfName& inDirection );

    /** Checks if printing this document is allowed.
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to print this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsPrintAllowed() const = 0; 

    /** Checks if modifiying this document (besides annotations, form fields or changing pages) is allowed.
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to modfiy this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsEditAllowed() const = 0;

    /** Checks if text and graphics extraction is allowed.
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to extract text and graphics from this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsCopyAllowed() const = 0;

    /** Checks if it is allowed to add or modify annotations or form fields
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to add or modify annotations or form fields
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsEditNotesAllowed() const = 0;

    /** Checks if it is allowed to fill in existing form or signature fields
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to fill in existing form or signature fields
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsFillAndSignAllowed() const = 0;

    /** Checks if it is allowed to extract text and graphics to support users with disabillities
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to extract text and graphics to support users with disabillities
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsAccessibilityAllowed() const = 0;

    /** Checks if it is allowed to insert, create, rotate, delete pages or add bookmarks
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed  to insert, create, rotate, delete pages or add bookmarks
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsDocAssemblyAllowed() const = 0;

    /** Checks if it is allowed to print a high quality version of this document 
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to print a high quality version of this document 
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsHighPrintAllowed() const = 0;

    // Peter Petrov 26 April 2008    
    /** Get access to the internal vector of objects
     *  or root object.
     *  
     *  \returns the vector of objects
     */
    inline PdfVecObjects* GetObjects();

    // Peter Petrov 26 April 2008
    /** Get access to the internal vector of objects
     *  or root object.
     *  
     *  \returns the vector of objects
     */
    inline const PdfVecObjects* GetObjects() const;

    /**
     * Set wrapper for the fontconfig library.
     * Useful to avoid initializing Fontconfig multiple times.
     *
     * This setter can be called until first use of Fontconfig
     * as the library is initialized at first use.
     */
    inline void SetFontConfigWrapper(const PdfFontConfigWrapper & rFontConfig);

 protected:
    /** Construct a new (empty) PdfDocument
     *  \param bEmtpy if true NO default objects (such as catalog) are created.
     */
    PdfDocument( bool bEmpty = false );

    /** Set the info object containing meta information.
     *  Deletes any old info object.
     *
     *  @param pInfo the new info object (will be owned by PdfDocument)
     */
    void SetInfo( PdfInfo* pInfo );

    /** Get access to the internal Catalog dictionary
     *  or root object.
     *  
     *  \returns PdfObject the documents catalog
     */
    inline PdfObject* GetCatalog();

    /** Get access to the internal Catalog dictionary
     *  or root object.
     *  
     *  \returns PdfObject the documents catalog
     */
    inline const PdfObject* GetCatalog() const;

    /** Set the catalog of this PdfDocument
     *  deleting the old one.
     *
     *  @param pObject the new catalog object
     *         It will be owned by PdfDocument.
     */
    inline void SetCatalog( PdfObject* pObject );

    /** Get access to the internal trailer dictionary
     *  or root object.
     *  
     *  \returns PdfObject the documents catalog
     */
    inline PdfObject* GetTrailer();

    /** Get access to the internal trailer dictionary
     *  or root object.
     *  
     *  \returns PdfObject the documents catalog
     */
    inline const PdfObject* GetTrailer() const;

    /** Set the trailer of this PdfDocument
     *  deleting the old one.
     *
     *  @param pObject the new trailer object
     *         It will be owned by PdfDocument.
     */
    void SetTrailer( PdfObject* pObject );

    /** Get a dictioary from the catalog dictionary by its name.
     *  \param pszName will be converted into a PdfName
     *  \returns the dictionary if it was found or NULL
     */
    PdfObject* GetNamedObjectFromCatalog( const char* pszName ) const;

    /** Internal method for initializing the pages tree for this document
     */
    void InitPagesTree();

    /** Recursively changes every PdfReference in the PdfObject and in any child
     *  that is either an PdfArray or a direct object.
     *  The reference is changed so that difference is added to the object number
     *  if the reference.
     *  \param pObject object to change
     *  \param difference add this value to every reference that is encountered
     */
    void FixObjectReferences( PdfObject* pObject, int difference );

    /** Low level APIs for setting a viewer preference
     *  \param whichPref the dictionary key to set
     *  \param valueObj the object to be set
     */
    void SetViewerPreference( const PdfName& whichPref, const PdfObject & valueObj );

    /** Low level APIs for setting a viewer preference
     *  Convinience overload.
     *  \param whichPref the dictionary key to set
     *  \param inValue the object to be set
     */
    void SetViewerPreference( const PdfName& whichPref, bool inValue );

    /** Clear all internal variables
     *  And reset PdfDocument to an intial state
     */
    void Clear();

 protected:
    PdfFontCache    m_fontCache;
    PdfObject*      m_pTrailer;
    PdfObject*      m_pCatalog;

    PdfInfo*        m_pInfo;
    PdfPagesTree*   m_pPagesTree;
    PdfAcroForm*    m_pAcroForms;

 private:
    // Prevent use of copy constructor and assignment operator.  These methods
    // should never be referenced (given that code referencing them outside
    // PdfDocument won't compile), and calling them will result in a link error
    // as they're not defined.
    explicit PdfDocument(const PdfDocument&);
    PdfDocument& operator=(const PdfDocument&);

    PdfVecObjects   m_vecObjects;
   

    PdfOutlines*    m_pOutlines;
    PdfNamesTree*   m_pNamesTree;

    EPdfVersion     m_eVersion;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfPagesTree* PdfDocument::GetPagesTree() const
{
    return m_pPagesTree;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfObject* PdfDocument::GetCatalog()
{
    return m_pCatalog;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfObject* PdfDocument::GetCatalog() const
{
    return m_pCatalog;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfDocument::SetCatalog( PdfObject* pObject ) 
{
    m_pCatalog = pObject; // m_pCatalog does not need to 
                          // be reowned as it should
                          // alread by part of m_vecObjects
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfObject* PdfDocument::GetTrailer()
{
    return m_pTrailer;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfObject* PdfDocument::GetTrailer() const
{
    return m_pTrailer;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfVecObjects* PdfDocument::GetObjects()
{
    return &m_vecObjects;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfVecObjects* PdfDocument::GetObjects() const
{
    return &m_vecObjects;
}

// Peter Petrov 26 April 2008
// -----------------------------------------------------
// 
// -----------------------------------------------------
inline FT_Library PdfDocument::GetFontLibrary() const
{
    return this->m_fontCache.GetFontLibrary();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfDocument::SetFontConfigWrapper(const PdfFontConfigWrapper & rFontConfig)
{
    m_fontCache.SetFontConfigWrapper(rFontConfig);
}

};


#endif	// _PDF_DOCUMENT_H_
