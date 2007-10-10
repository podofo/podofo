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
     *  \param pEncrypt pointer to an encryption object or NULL. If not NULL
     *                  the PdfEncrypt object will be copied and used to encrypt the
     *                  created document.
     */
    PdfStreamedDocument( PdfOutputDevice* pDevice, EPdfVersion eVersion = ePdfVersion_1_5, PdfEncrypt* pEncrypt = NULL );

    /** Create a new PdfStreamedDocument.
     *  All data is written to a file immediately.
     *
     *  \param pszFilename resulting PDF file
     *  \param eVersion the PDF version of the document to write.
     *                  The PDF version can only be set in the constructor
     *                  as it is the first item written to the document on disk.
     *  \param pEncrypt pointer to an encryption object or NULL. If not NULL
     *                  the PdfEncrypt object will be copied and used to encrypt the
     *                  created document.
     */
    PdfStreamedDocument( const char* pszFilename, EPdfVersion eVersion = ePdfVersion_1_5, PdfEncrypt* pEncrypt = NULL );

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

    /** Checks if printing this document is allowed.
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to print this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    inline virtual bool IsPrintAllowed() const; 

    /** Checks if modifiying this document (besides annotations, form fields or changing pages) is allowed.
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to modfiy this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    inline virtual bool IsEditAllowed() const;

    /** Checks if text and graphics extraction is allowed.
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to extract text and graphics from this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    inline virtual bool IsCopyAllowed() const;

    /** Checks if it is allowed to add or modify annotations or form fields
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to add or modify annotations or form fields
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    inline virtual bool IsEditNotesAllowed() const;

    /** Checks if it is allowed to fill in existing form or signature fields
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to fill in existing form or signature fields
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    inline virtual bool IsFillAndSignAllowed() const;

    /** Checks if it is allowed to extract text and graphics to support users with disabillities
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to extract text and graphics to support users with disabillities
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    inline virtual bool IsAccessibilityAllowed() const;

    /** Checks if it is allowed to insert, create, rotate, delete pages or add bookmarks
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed  to insert, create, rotate, delete pages or add bookmarks
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    inline virtual bool IsDocAssemblyAllowed() const;

    /** Checks if it is allowed to print a high quality version of this document 
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to print a high quality version of this document 
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    inline virtual bool IsHighPrintAllowed() const;

 private:
    /** Initialize the PdfStreamedDocument with an output device
     *  \param pDevice write to this device
     *  \param eVersion the PDF version of the document to write.
     *                  The PDF version can only be set in the constructor
     *                  as it is the first item written to the document on disk.
     *  \param pEncrypt pointer to an encryption object or NULL. If not NULL
     *                  the PdfEncrypt object will be copied and used to encrypt the
     *                  created document.
     */
    void Init( PdfOutputDevice* pDevice, EPdfVersion eVersion = ePdfVersion_1_5, PdfEncrypt* pEncrypt = NULL );

 private:
    PdfImmediateWriter* m_pWriter;
    PdfOutputDevice*    m_pDevice;

    PdfEncrypt*         m_pEncrypt;

    bool                m_bOwnDevice; ///< If true m_pDevice is owned by this object and has to be deleted
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfVersion PdfStreamedDocument::GetPdfVersion() const
{
    return m_pWriter->GetPdfVersion();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfStreamedDocument::IsLinearized() const
{
    // Linearization is currently not supported by PdfStreamedDocument
    return false;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfStreamedDocument::IsPrintAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsPrintAllowed() : true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfStreamedDocument::IsEditAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsEditAllowed() : true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfStreamedDocument::IsCopyAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsCopyAllowed() : true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfStreamedDocument::IsEditNotesAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsEditNotesAllowed() : true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfStreamedDocument::IsFillAndSignAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsFillAndSignAllowed() : true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfStreamedDocument::IsAccessibilityAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsAccessibilityAllowed() : true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfStreamedDocument::IsDocAssemblyAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsDocAssemblyAllowed() : true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfStreamedDocument::IsHighPrintAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsHighPrintAllowed() : true;
}

};

#endif /* _PDF_STREAMED_DOCUMENT_H_ */
