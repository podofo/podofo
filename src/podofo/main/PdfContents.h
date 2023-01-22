/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_CONTENTS_H
#define PDF_CONTENTS_H

#include "PdfCanvas.h"

namespace PoDoFo {

class PdfPage;

/** A interface that provides a wrapper around "PDF content" -
	the instructions that are used to draw on the PDF "canvas".
 */
class PODOFO_API PdfContents
{
public:
    PdfContents(PdfPage &parent, PdfObject &obj);

    PdfContents(PdfPage &parent);

    /** Reset the contents internal object
     * \param obj the object to set as the /Contents. Must be
     * a dictionary or an array. if nullptr, a new array object
     * will be created
     */
    void Reset(PdfObject* obj = nullptr);

    /** Get access to the raw contents object.
     *  It will either be a PdfObjectStream or a PdfArray
     *  \returns a contents object
     */
    inline const PdfObject& GetObject() const { return *m_object; }

    inline PdfObject& GetObject() { return *m_object; }

    charbuff GetCopy() const;
    void CopyTo(charbuff& buffer) const;
    void CopyTo(OutputStream& stream) const;

    /** Get access to an object into which you can add contents
     *   at the end of the "stream".
     */
    PdfObjectStream & GetStreamForAppending(PdfStreamAppendFlags flags = PdfStreamAppendFlags::None);

private:
    void copyTo(OutputStream& stream, const PdfArray& arr) const;
    void reset();

private:
    PdfPage *m_parent;
    PdfObject *m_object;
};

};

#endif // PDF_CONTENTS_H
