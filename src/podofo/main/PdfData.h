/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_DATA_H
#define PDF_DATA_H

#include "PdfDeclarations.h"

#include "PdfBaseDataTypes.h"

namespace PoDoFo {

/** A datatype that allows to write arbitrary data
 *  to a PDF file. 
 *  The user of this class has to ensure that the data
 *  written to the PDF file using this class is valid data
 *  for a PDF file!
 */
class PODOFO_API PdfData final : public PdfDataProvider<PdfData>
{
public:
    PdfData();
    PdfData(const PdfData&) = default;
    PdfData(PdfData&&) noexcept = default;

    /**
     * Create a new PdfData object with valid PdfData
     *
     * The contained data has to be a valid value in a PDF file.
     * It will be written directly to the PDF file.
     * \param writeBeacon Shared sentinel that will updated
     *                    during writing of the document with
     *                    the current position in the stream
     *
     */
    PdfData(charbuff&& data, std::shared_ptr<size_t> writeBeacon = { });

    /**
     * Create a new PdfData object with valid PdfData
     *
     * The contained data has to be a valid value in a PDF file.
     * It will be written directly to the PDF file.
     * \param writeBeacon Shared sentinel that will updated
     *                    during writing of the document with
     *                    the current position in the stream
     *
     */
    PdfData(const bufferview& data, std::shared_ptr<size_t> writeBeacon = { });

    void Write(OutputStream& stream, PdfWriteFlags writeMode,
        const PdfStatefulEncrypt* encrypt, charbuff& buffer) const;

    PdfData& operator=(const PdfData& rhs) = default;
    PdfData& operator=(PdfData&& rhs) = default;
    PdfData& operator=(const bufferview& data);

    /**
     * Access the data as a std::string
     * \returns a const reference to the contained data
     */
     inline const charbuff& GetBuffer() const { return m_data; }

private:
    charbuff m_data;
    std::shared_ptr<size_t> m_writeBeacon;
};

}

#endif // PDF_DATA_H

