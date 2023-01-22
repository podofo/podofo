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
 *  A file specification is used in the PDF file to referr to another file.
 *  The other file can be a file outside of the PDF or can be embedded into
 *  the PDF file itself.
 */
class PODOFO_API PdfFileSpec final : public PdfDictionaryElement
{
public:
    PdfFileSpec(PdfDocument& doc, const std::string_view& filename, bool embed, bool striPath = false);

    PdfFileSpec(PdfDocument& doc, const std::string_view& filename, const char* data, size_t size, bool striPath = false);

    PdfFileSpec(PdfObject& obj);

    /** Gets file name for the FileSpec
     *  \param canUnicode Whether can return file name in unicode (/UF)
     *  \returns the filename of this file specification.
     *           if no general name is available
     *           it will try the Unix, Mac and DOS keys too.
     */
    const PdfString& GetFilename(bool canUnicode) const;

private:

    /** Initialize a filespecification from a filename
     *  \param filename filename
     *  \param embed embedd the file data into the PDF file
     *  \param striPath whether to strip path from the file name string
     */
    void Init(const std::string_view& filename, bool embed, bool striPath);

    /** Initialize a filespecification from an in-memory buffer
     *  \param filename filename
     *  \param data Data of the file
     *  \param size size of the data buffer
     *  \param striPath whether to strip path from the file name string
     */
    void Init(const std::string_view& filename, const char* data, size_t size, bool striPath);

    /** Create a file specification string from a filename
     *  \param filename filename
     *  \returns a file specification string
     */
    PdfString CreateFileSpecification(const std::string_view& filename) const;

    /** Embedd a file into a stream object
     *  \param obj write the file to this object stream
     *  \param filename the file to embedd
     */
    void EmbeddFile(PdfObject& obj, const std::string_view& filename) const;

    /** Strips path from a file, according to \a striPath
     *  \param filename a file name string
     *  \param stripPath whether to strip path from the file name string
     *  \returns Either unchanged \a filename, if \a striPath is false;
     *     or \a filename without a path part, if \a striPath is true
     */
    std::string MaybeStripPath(const std::string_view& filename, bool stripPath) const;

    /* Petr P. Petrov 17 September 2009*/
    /** Embeds the file from memory
      */
    void EmbeddFileFromMem(PdfObject& obj, const char* data, size_t size) const;
};

};

#endif // PDF_FILE_SPEC_H

