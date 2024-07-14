/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_EXTGSTATE_H
#define PDF_EXTGSTATE_H

#include "PdfElement.h"

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
    PdfExtGState(PdfDocument& doc);
    PdfExtGState(const PdfExtGState&) = default;

public:
    /** Sets the opacity value to be used for fill operations
     *  \param opacity a floating point value from 0 (transparent) to 1 (opaque)
     */
    void SetFillOpacity(nullable<double> opacity);

    /** Sets the opacity value to be used for stroking operations
     *  \param opacity a floating point value from 0 (transparent) to 1 (opaque)
     */
    void SetStrokeOpacity(nullable<double> opacity);

    /** Sets the transparency blend mode
     *  \param blendMode one of the predefined blending modes (see PdfDeclarations.h)
     */
    void SetBlendMode(nullable<PdfBlendMode> blendMode);

    /** Enables/Disables overprinting for both Fill & Stroke
     *  \param enable enable or disable
     */
    void SetOverprintEnabled(nullable<bool> enabled);

    /** Enables/Disables overprinting for Fill operations
     *  \param enable enable or disable
     */
    void SetFillOverprintEnabled(nullable<bool> enabled);

    /** Enables/Disables overprinting for Stroke operations
     *  \param enable enable or disable
     */
    void SetStrokeOverprintEnabled(nullable<bool> enabled);

    /** Enables/Disables non-zero overprint mode
     *  \param enable enable or disable
     */
    void SetNonZeroOverprintEnabled(nullable<bool> enabled);

    /** Set the Rendering Intent
     *  \param intent one of the predefined intents
     */
    void SetRenderingIntent(nullable<PdfRenderingIntent> intent);

    /** Set the frequency for halftones
     *  \param frequency screen frequency, measured in halftone cells per inch in device space
     */
    void SetFrequency(double frequency);
};

};

#endif // PDF_EXTGSTATE_H

