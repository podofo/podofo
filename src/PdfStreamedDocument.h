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

#ifndef _PDF_STREAMED_DOCUMENT_H_
#define _PDF_STREAMED_DOCUMENT_H_

#include "PdfDefines.h"

#include "PdfDocument.h"

namespace PoDoFo {

class PdfImmediateWriter;
class PdfOutputDevice;

/** PdfStreamedDocument is the preferred class for 
 *  creating new PDF documents.
 * 
 *  Page contents, fonts and images are written to disk
 *  as soon as possible and are not kept in memory.
 *  This results in faster document generation and 
 *  less memory being used.
 *
 *  Please use PdfDocument if you intend to work
 *  on the object structure of a PDF file.
 *
 *  \see PdfDocument
 */
class PODOFO_API PdfStreamedDocument {
    friend class PdfImage;
    friend class PdfElement;

 public:
    /** Create a new PdfStreamedDocument.
     *  All data is written to an output device
     *  immediately.
     *
     *  \param pDevice an output device
     */
    PdfStreamedDocument( PdfOutputDevice* pDevice );

    /** Create a new PdfStreamedDocument.
     *  All data is written to a file immediately.
     *
     *  \param pszFilename resulting PDF file
     */
    PdfStreamedDocument( const char* pszFilename );

    ~PdfStreamedDocument();

    /** Close the document. The PDF file on disk is finished.
     *  No other member function of this class maybe called
     *  after calling this function.
     */
    void Close();

    /** Get access to the internal Info dictionary
     *  You can set the author, title etc. of the
     *  document using the info dictionary.
     *
     *  \returns the info dictionary
     */
    inline PdfInfo* GetInfo() const;

    /** Get access to the Outlines (Bookmarks) dictionary
     *  The returned outlines object is owned by the PdfDocument.
     * 
     *  \returns the Outlines/Bookmarks dictionary
     */
    inline PdfOutlines* GetOutlines( bool bCreate = ePdfCreateObject );

    /** Get the total number of pages in a document
     *  \returns int number of pages
     */
    inline int GetPageCount() const;

    /** Creates a PdfFont object
     *  \param pszFontName name of the font as it is known to the system
     *  \param bEmbedd specifies whether this font should be embedded in the PDF file.
     *         Embedding fonts is usually a good idea.
     *  \returns PdfFont* a pointer to a new PdfFont object.
     */
    inline PdfFont* CreateFont( const char* pszFontName, bool bEmbedd = true );

    /** Creates a PdfFont object
     *  \param face a valid freetype font handle
     *  \param bEmbedd specifies whether this font should be embedded in the PDF file.
     *         Embedding fonts is usually a good idea.
     *  \returns PdfFont* a pointer to a new PdfFont object.
     */
    inline PdfFont* CreateFont( FT_Face face, bool bEmbedd = true );

    /** Creates a new page object and inserts it into the internal
     *  page tree. 
     *  The returned page is owned by the PdfDocument
     *  and will get deleted along with it!
     *
     *  \param rSize a PdfRect spezifying the size of the page (i.e the /MediaBox key) in 1/1000th mm
     *  \returns a pointer to a PdfPage object
     */
    inline PdfPage* CreatePage( const PdfRect & rSize );

    /** Appends another PdfDocument to this document
     *  \param rDoc the document to append
     *  \returns this document
     */
    inline const PdfDocument & Append( const PdfDocument & rDoc );

    /** Attach a file to the document.
     *  \param rFileSpec a file specification
     */
    inline void AttachFile( const PdfFileSpec & rFileSpec );

    /** Sets the opening mode for a document
     *  \param inMode which mode to set
     */
    inline void SetPageMode( EPdfPageMode inMode ) const;

    /** Gets the opening mode for a document
     *  \returns which mode is set
     */
    inline EPdfPageMode GetPageMode() const;

    /** Sets the opening mode for a document to be in full screen
     */
    inline void SetUseFullScreen( void ) const;
    
    /** Sets the page layout for a document
     */
    inline void SetPageLayout( EPdfPageLayout inLayout );
    
    /** Set the document's Viewer Preferences:
     *  Hide the toolbar in the viewer
     */
    inline void SetHideToolbar( void );

    /** Set the document's Viewer Preferences:
     *  Hide the menubar in the viewer
     */
    inline void SetHideMenubar( void );

    /** Set the document's Viewer Preferences:
     *  Show only the documents contents and no controll
     *  elements such as buttons and scrollbars in the viewer
     */
    inline void SetHideWindowUI( void );

    /** Set the document's Viewer Preferences:
     *  Fit the document in the viewers window
     */
    inline void SetFitWindow( void );

    /** Set the document's Viewer Preferences:
     *  Center the document in the viewers window
     */
    inline void SetCenterWindow( void );

    /** Set the document's Viewer Preferences:
     *  Display the title from the document information
     *  in the title of the viewer.
     * 
     *  \see SetTitle
     */
    inline void SetDisplayDocTitle( void );

    /** Set the document's Viewer Preferences:
     *  Set the default print scaling of the document
     */   
    inline void SetPrintScaling( PdfName& inScalingType );

    /** Set the document's Viewer Preferences:
     *  Set the base URI of the document
     */
    inline void SetBaseURI( const std::string& inBaseURI );

    /** Set the document's Viewer Preferences:
     *  Set the language of the document
     */    
    inline void SetLanguage( const std::string& inLanguage );

    /** Set the document's Viewer Preferences:
     */    
    inline void SetBindingDirection( PdfName& inDirection );

    /** Get access to the AcroForm dictionary
     *  \returns PdfObject the AcroForm dictionary
     */
    inline PdfAcroForm* GetAcroForm( bool bCreate = ePdfCreateObject );

 private:
    /** Initialize the PdfStreamedDocument with an output device
     *  \param pDevice write to this device
     */
    void Init( PdfOutputDevice* pDevice );

 private:
    PdfDocument         m_doc;
    PdfImmediateWriter* m_pWriter;
    PdfOutputDevice*    m_pDevice;

};

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfInfo* PdfStreamedDocument::GetInfo() const
{
    return m_doc.GetInfo();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfOutlines* PdfStreamedDocument::GetOutlines( bool bCreate )
{
    return m_doc.GetOutlines( bCreate );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
int PdfStreamedDocument::GetPageCount() const
{
    return m_doc.GetPageCount();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfFont* PdfStreamedDocument::CreateFont( const char* pszFontName, bool bEmbedd )
{
    return m_doc.CreateFont( pszFontName, bEmbedd );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfFont* PdfStreamedDocument::CreateFont( FT_Face face, bool bEmbedd )
{
    return m_doc.CreateFont( face, bEmbedd );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfPage* PdfStreamedDocument::CreatePage( const PdfRect & rSize )
{
    return m_doc.CreatePage( rSize );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfDocument & PdfStreamedDocument::Append( const PdfDocument & rDoc )
{
    return m_doc.Append( rDoc );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::AttachFile( const PdfFileSpec & rFileSpec )
{
    m_doc.AttachFile( rFileSpec );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::SetPageMode( EPdfPageMode inMode ) const
{
    m_doc.SetPageMode( inMode );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfPageMode PdfStreamedDocument::GetPageMode() const
{
    return m_doc.GetPageMode();
}


// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::SetUseFullScreen( void ) const
{
    m_doc.SetUseFullScreen();
}
    
// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::SetPageLayout( EPdfPageLayout inLayout )
{
    m_doc.SetPageLayout( inLayout );
}
    
// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::SetHideToolbar( void )
{
    m_doc.SetHideToolbar();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::SetHideMenubar( void )
{
    m_doc.SetHideMenubar();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::SetHideWindowUI( void )
{
    m_doc.SetHideWindowUI();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::SetFitWindow( void )
{
    m_doc.SetFitWindow();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::SetCenterWindow( void )
{
    m_doc.SetCenterWindow();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::SetDisplayDocTitle( void )
{
    m_doc.SetDisplayDocTitle();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::SetPrintScaling( PdfName& inScalingType )
{
    m_doc.SetPrintScaling( inScalingType );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::SetBaseURI( const std::string& inBaseURI )
{
    m_doc.SetBaseURI( inBaseURI );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::SetLanguage( const std::string& inLanguage )
{
    m_doc.SetLanguage( inLanguage );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStreamedDocument::SetBindingDirection( PdfName& inDirection )
{
    m_doc.SetBindingDirection( inDirection );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfAcroForm* PdfStreamedDocument::GetAcroForm( bool bCreate )
{
    return m_doc.GetAcroForm( bCreate );
}

};

#endif /* _PDF_STREAMED_DOCUMENT_H_ */
