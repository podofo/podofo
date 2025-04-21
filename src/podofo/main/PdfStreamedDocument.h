/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_STREAMED_DOCUMENT_H
#define PDF_STREAMED_DOCUMENT_H

#include "PdfDeclarations.h"

#include "PdfDocument.h"
#include <podofo/auxiliary/OutputDevice.h>

namespace PoDoFo {

class PdfImmediateWriter;

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
 *  PdfStreamedDocument document("outputfile.pdf");
 *  auto& page = document.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
 *  auto* font = document.GetFonts().SearchFont("Arial");
 *
 *  PdfPainter painter;
 *  painter.SetCanvas(page);
 *  painter.TextState.SetFont(*font, 18);
 *  painter.DrawText("Hello World!", 56.69, page.GetRect().Height - 56.69);
 *  painter.FinishDrawing();
 */
class PODOFO_API PdfStreamedDocument final : public PdfDocument
{
    friend class PdfImage;

public:
    /** Create a new PdfStreamedDocument.
     *  All data is written to an output device
     *  immediately.
     *
     *  \param device an output device
     *  \param version the PDF version of the document to write.
     *                  The PDF version can only be set in the constructor
     *                  as it is the first item written to the document on disk.
     *  \param encrypt pointer to an encryption object or nullptr. If not nullptr
     *                  the PdfEncrypt object will be copied and used to encrypt the
     *                  created document.
     *  \param opts additional save options for writing the pdf
     */
    PdfStreamedDocument(std::shared_ptr<OutputStreamDevice> device, PdfVersion version = PdfVersionDefault,
        std::shared_ptr<PdfEncrypt> encrypt = nullptr, PdfSaveOptions opts = PdfSaveOptions::None);

    /** Create a new PdfStreamedDocument.
     *  All data is written to a file immediately.
     *
     *  \param filename resulting PDF file
     *  \param version the PDF version of the document to write.
     *                  The PDF version can only be set in the constructor
     *                  as it is the first item written to the document on disk.
     *  \param encrypt pointer to an encryption object or nullptr. If not nullptr
     *                  the PdfEncrypt object will be copied and used to encrypt the
     *                  created document.
     *  \param opts additional options for writing the pdf
     */
    PdfStreamedDocument(const std::string_view& filename, PdfVersion version = PdfVersionDefault,
        std::shared_ptr<PdfEncrypt> encrypt = nullptr, PdfSaveOptions opts = PdfSaveOptions::None);

    ~PdfStreamedDocument();

public:
    const PdfEncrypt* GetEncrypt() const override;

protected:
    PdfVersion GetPdfVersion() const override;

    void SetPdfVersion(PdfVersion version) override;

private:
    /** Initialize the PdfStreamedDocument with an output device
     *  \param device write to this device
     *  \param version the PDF version of the document to write.
     *                  The PDF version can only be set in the constructor
     *                  as it is the first item written to the document on disk.
     *  \param encrypt pointer to an encryption object or nullptr. If not nullptr
     *                  the PdfEncrypt object will be copied and used to encrypt the
     *                  created document.
     *  \param opts additional options for writing the pdf
     */
    void init(PdfVersion version, PdfSaveOptions opts);

private:
    std::shared_ptr<OutputStreamDevice> m_Device;
    std::unique_ptr<PdfImmediateWriter> m_Writer;
    std::shared_ptr<PdfEncrypt> m_Encrypt;
};

};

#endif // PDF_STREAMED_DOCUMENT_H
