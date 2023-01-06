/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfGraphicsState.h"

using namespace std;
using namespace PoDoFo;

PdfGraphicsState::PdfGraphicsState() :
    LineWidth(0),
    MiterLimit(10),
    LineCapStyle(PdfLineCapStyle::Square),
    LineJoinStyle(PdfLineJoinStyle::Miter)
{
}
