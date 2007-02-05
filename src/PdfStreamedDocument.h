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

namespace PoDoFo {

class PdfOutputDevice;

#if 0
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
    PdfInfo* GetInfo() const { return m_pInfo; }

    /** Get access to the Outlines (Bookmarks) dictionary
     *  The returned outlines object is owned by the PdfDocument.
     * 
     *  \returns the Outlines/Bookmarks dictionary
     */
    PdfOutlines* GetOutlines( bool bCreate = ePdfCreateObject );

    /** Get the total number of pages in a document
     *  \returns int number of pages
     */
    int GetPageCount() const;

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
    /** Initialize the PdfStreamedDocument with an output device
     *  \param pDevice write to this device
     */
    void Init( PdfOutputDevice* pDevice );

 private:

};
#endif // 0
};

#endif /* _PDF_STREAMED_DOCUMENT_H_ */
