/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_DATATYPE_H
#define PDF_DATATYPE_H

#include "PdfDeclarations.h"
#include <podofo/auxiliary/StreamDevice.h>

namespace PoDoFo {

class OutputStream;
class PdfStatefulEncrypt;

/** An interface for data provider classes that are stored in a PdfVariant
 *  
 *  \see PdfName \see PdfArray \see PdfReference 
 *  \see PdfVariant \see PdfDictionary \see PdfString
 */
template <typename T>
class PdfDataProvider
{
    friend class PdfDataContainer;
    friend class PdfData;
    friend class PdfString;
    friend class PdfName;

private:
    PdfDataProvider() { }

public:
    /** Converts the current object into a string representation
     *  which can be written directly to a PDF file on disc.
     *  \param str the object string is returned in this object.
     */
    std::string ToString() const
    {
        std::string ret;
        ToString(ret);
        return ret;
    }

    void ToString(std::string& str) const
    {
        str.clear();
        StringStreamDevice device(str);
        charbuff buffer;
        static_cast<const T&>(*this).Write(device, PdfWriteFlags::None, nullptr, buffer);
    }
};

}

#endif // PDF_DATATYPE_H
