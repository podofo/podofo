/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_EXTGSTATE_H
#define PDF_EXTGSTATE_H

#include "PdfElement.h"
#include "PdfExtGStateDefinition.h"

namespace PoDoFo {

class PdfGraphicsStateWrapper;

/** This class wraps the ExtGState object used in the Resource
 *  Dictionary of a Content-supporting element (page, Pattern, etc.)
 *  The main usage is for transparency, but it also support a variety
 *  of prepress features.
 */
class PODOFO_API PdfExtGState final : public PdfDictionaryElement
{
    friend class PdfDocument;
    friend class PdfGraphicsStateWrapper;

private:
    /** Create a new PdfExtGState object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param parent parent document
     *
     */
    PdfExtGState(PdfDocument& doc, PdfExtGStateDefinitionPtr&& filter);

public:
    const PdfExtGStateDefinition& GetDefinition() const { return *m_Definition; }
    PdfExtGStateDefinitionPtr GetDefinitionPtr() const { return m_Definition; }

public:
    PdfExtGStateDefinitionPtr m_Definition;
};

};

#endif // PDF_EXTGSTATE_H

