/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_EXTGSTATE_DEFINITION_H
#define PDF_EXTGSTATE_DEFINITION_H

#include "PdfDeclarations.h"

namespace PoDoFo
{
    enum class PdfOverprintEnablement : uint8_t
    {
        None = 0,
        Stroking = 1,
        NonStroking = 2
    };

    // TODO: Add missing properties ISO 32000-2:2020 8.4.5 "Graphics state parameter dictionaries"
    struct PODOFO_API PdfExtGStateDefinition final
    {
        nullable<double> StrokingAlpha;
        nullable<double> NonStrokingAlpha;
        nullable<PdfBlendMode> BlendMode;
        PdfOverprintEnablement OverprintControl = PdfOverprintEnablement::None;
        nullable<bool> NonZeroOverprintMode;
        nullable<PdfRenderingIntent> RenderingIntent;
    };

    /** Convenience alias for a constant PdfExtGStateDefinition shared ptr
     */
    using PdfExtGStateDefinitionPtr = std::shared_ptr<const PdfExtGStateDefinition>;
}

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfOverprintEnablement);

#endif // PDF_EXTGSTATE_DEFINITION_H
