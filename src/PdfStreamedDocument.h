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
#include "PdfImmediateWriter.h"

namespace PoDoFo {

class PdfOutputDevice;

/** PdfStreamedDocument is the preferred class for 
 *  creating new PDF documents.
 * 
 *  Page contents, fonts and images are written to disk
 *  as soon as possible and are not kept in memory.
 *  This results in faster document generation and 
 *  less memory being used.
 *
 *  Please use PdfMemDocument if you intend to work
 *  on the object structure of a PDF file.
 *
 *  One of the design goals of PdfStreamedDocument was
 *  to hide the underlying object structure of a PDF 
 *  file as far as possible.
 *
 *  \see PdfDocument
 *  \see PdfMemDocument
 *
 *  Example of using PdfStreamedDocument:
 *
 *  PdfStreamedDocument document( "outputfile.pdf" );
 *  PdfPage* pPage = document.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
 *  PdfFont* pFont = document.CreateFont( "Arial" );
 *
 *  PdfPainter painter;
 *  painter.SetPage( pPage );
 *  painter.SetFont( pFont );
 *  painter.DrawText( 56.69, pPage->GetPageSize().GetHeight() - 56.69, "Hello World!" );
 *  painter.FinishPage();
 *
 *  document.Close();
 */
class PODOFO_API PdfStreamedDocument : public PdfDocument {
    friend class PdfImage;
    friend class PdfElement;

 public:
    /** Create a new PdfStreamedDocument.
     *  All data is written to an output device
     *  immediately.
     *
     *  \param pDevice an output device
     *  \param eVersion the PDF version of the document to write.
     *                  The PDF version can only be set in the constructor
     *                  as it is the first item written to the document on disk.
     */
    PdfStreamedDocument( PdfOutputDevice* pDevice, EPdfVersion eVersion = ePdfVersion_1_5 );

    /** Create a new PdfStreamedDocument.
     *  All data is written to a file immediately.
     *
     *  \param pszFilename resulting PDF file
     *  \param eVersion the PDF version of the document to write.
     *                  The PDF version can only be set in the constructor
     *                  as it is the first item written to the document on disk.
     */
    PdfStreamedDocument( const char* pszFilename, EPdfVersion eVersion = ePdfVersion_1_5 );

    ~PdfStreamedDocument();

    /** Close the document. The PDF file on disk is finished.
     *  No other member function of this class maybe called
     *  after calling this function.
     */
    void Close();


    /** Get the PDF version of the document
     *  \returns EPdfVersion version of the pdf document
     */
    inline virtual EPdfVersion GetPdfVersion() const;

    /** Returns wether this PDF document is linearized, aka
     *  weboptimized
     *  \returns true if the PDF document is linearized
     */
    inline virtual bool IsLinearized() const;

 private:
    /** Initialize the PdfStreamedDocument with an output device
     *  \param pDevice write to this device
     *  \param eVersion the PDF version of the document to write.
     *                  The PDF version can only be set in the constructor
     *                  as it is the first item written to the document on disk.
     */
    void Init( PdfOutputDevice* pDevice, EPdfVersion eVersion = ePdfVersion_1_5 );

 private:
    PdfImmediateWriter* m_pWriter;
    PdfOutputDevice*    m_pDevice;

};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline EPdfVersion PdfStreamedDocument::GetPdfVersion() const
{
    return m_pWriter->GetPdfVersion();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfStreamedDocument::IsLinearized() const
{
    // Linearization is currently not supported by PdfStreamedDocument
    return false;
}

};

#endif /* _PDF_STREAMED_DOCUMENT_H_ */
