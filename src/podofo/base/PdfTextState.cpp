/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfTextState.h"

using namespace PoDoFo;

PdfTextState::PdfTextState() :
    Font(nullptr),
    FontSize(-1),
    FontScale(1),
    CharSpacing(0),
    WordSpacing(0),
    RenderingMode(PdfTextRenderingMode::Fill) { }
