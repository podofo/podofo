/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_STRING_STREAM
#define PDF_STRING_STREAM

#include "PdfDeclarations.h"
#include <podofo/auxiliary/OutputStream.h>

namespace PoDoFo
{
    /** A specialized Pdf output string stream
     * It supplies an iostream-like operator<< interface,
     * while still inheriting OutputStream
     */
    class PODOFO_API PdfStringStream final : public OutputStream
    {
    public:
        PdfStringStream();

        template <typename T>
        inline PdfStringStream& operator<<(T const& val)
        {
            (*m_stream) << val;
            return *this;
        }

        // This is needed to allow using std::endl
        PdfStringStream& operator<<(
            std::ostream& (*pfn)(std::ostream&));

        PdfStringStream& operator<<(float val);

        PdfStringStream& operator<<(double val);

        std::string_view GetString() const;

        std::string TakeString();

        void Clear();

        void SetPrecision(unsigned short value);

        unsigned short GetPrecision() const;

        unsigned GetSize() const;

        explicit operator std::ostream& () { return *m_stream; }

    protected:
        void writeBuffer(const char* buffer, size_t size);

    private:
        using OutputStream::Flush;
        using OutputStream::Write;

    private:
        charbuff m_temp;
        std::unique_ptr<std::ostream> m_stream;
    };
}

#endif // PDF_STRING_STREAM
