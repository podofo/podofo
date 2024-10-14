/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_BASE_DATA_TYPES_H
#define PDF_BASE_DATA_TYPES_H

#include "PdfDeclarations.h"
#include <podofo/auxiliary/StreamDevice.h>

namespace PoDoFo {

class OutputStream;
class PdfStatefulEncrypt;

/** A class to inherit for classes that are stored as union members in a PdfVariant
 */
class PODOFO_API PdfDataMember
{
    friend class PdfString;
    friend class PdfName;
    friend class PdfReference;
public:
    PdfDataMember(PdfDataType type);
public:
    inline PdfDataType GetDataType() const { return m_DataType; }
private:
    PdfDataType m_DataType;
};

/** An helper class to inherit to provide common serialization methods
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
    friend class PdfReference;

private:
    PdfDataProvider() { }

public:
    /** Converts the current object into a string representation
     *  which can be written directly to a PDF file on disc.
     *  \param str the object string is returned in this object.
     */
    std::string ToString(PdfWriteFlags flags = PdfWriteFlags::None) const
    {
        std::string ret;
        ToString(ret, flags);
        return ret;
    }

    void ToString(std::string& str, PdfWriteFlags flags = PdfWriteFlags::None) const
    {
        str.clear();
        StringStreamDevice device(str);
        charbuff buffer;
        static_cast<const T&>(*this).Write(device, flags, nullptr, buffer);
    }
};

}

#endif // PDF_BASE_DATA_TYPES_H
