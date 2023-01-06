/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_EXTGSTATE_H
#define PDF_EXTGSTATE_H

#include "PdfDeclarations.h"

#include "PdfElement.h"
#include "PdfName.h"

namespace PoDoFo {

class PdfObject;
class PdfPage;
class PdfWriter;

/** This class wraps the ExtGState object used in the Resource
 *  Dictionary of a Content-supporting element (page, Pattern, etc.)
 *  The main usage is for transparency, but it also support a variety
 *  of prepress features.
 */
class PODOFO_API PdfExtGState final : public PdfDictionaryElement
{
public:
    /** Create a new PdfExtGState object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param parent parent document
     *
     */
    PdfExtGState(PdfDocument& doc);

    /** Sets the opacity value to be used for fill operations
     *  \param opac a floating point value from 0 (transparent) to 1 (opaque)
     */
    void SetFillOpacity(double opac);

    /** Sets the opacity value to be used for stroking operations
     *  \param opac a floating point value from 0 (transparent) to 1 (opaque)
     */
    void SetStrokeOpacity(double opac);

    /** Sets the transparency blend mode
     *  \param blendMode one of the predefined blending modes (see PdfDeclarations.h)
     */
    void SetBlendMode(const std::string_view& blendMode);

    /** Enables/Disables overprinting for both Fill & Stroke
     *  \param enable enable or disable
     */
    void SetOverprint(bool enable = true);

    /** Enables/Disables overprinting for Fill operations
     *  \param enable enable or disable
     */
    void SetFillOverprint(bool enable = true);

    /** Enables/Disables overprinting for Stroke operations
     *  \param enable enable or disable
     */
    void SetStrokeOverprint(bool enable = true);

    /** Enables/Disables non-zero overprint mode
     *  \param enable enable or disable
     */
    void SetNonZeroOverprint(bool enable = true);

    /** Set the Rendering Intent
     *  \param intent one of the predefined intents
     */
    void SetRenderingIntent(const std::string_view& intent);

    /** Set the frequency for halftones
     *  \param frequency screen frequency, measured in halftone cells per inch in device space
     */
    void SetFrequency(double frequency);

    /** Returns the identifier of this ExtGState how it is known
     *  in the pages resource dictionary.
     *  \returns PdfName containing the identifier (e.g. /ExtGS13)
     */
    inline const PdfName& GetIdentifier() const { return m_Identifier; }

private:
    void Init();

private:
    PdfName m_Identifier;
};

};

#endif // PDF_EXTGSTATE_H

