/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FILE_SPEC_H
#define PDF_FILE_SPEC_H

#include "PdfDeclarations.h"

#include "PdfElement.h"
#include "PdfString.h"

namespace PoDoFo {

class PdfDocument;

/**
 *  A file specification is used in the PDF file to refer to another file.
 *  The other file can be a file outside of the PDF or can be embedded into
 *  the PDF file itself.
 */
class PODOFO_API PdfFileSpec final : public PdfDictionaryElement
{
    friend class PdfDocument;
    friend class PdfAnnotationFileAttachment;
    friend class PdfNameTreeBase;

private:
    PdfFileSpec(PdfDocument& doc);

    PdfFileSpec(PdfObject& obj);

    PdfFileSpec(const PdfFileSpec& fileSpec) = default;

public:
    bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfFileSpec>& filespec);

    /** Gets file name for the FileSpec
     *  \returns the filename of this file specification
     */
    nullable<const PdfString&> GetFilename() const;

    void SetFilename(nullable<const PdfString&> filename);

    void SetEmbeddedData(nullable<const charbuff&> data);

    void SetEmbeddedDataFromFile(const std::string_view& filepath);

    nullable<charbuff> GetEmbeddedData() const;

private:
    void setData(InputStream& stream, size_t size);
};

};

#endif // PDF_FILE_SPEC_H

