// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef PDF_XOBJECT_POST_SCRIPT_H
#define PDF_XOBJECT_POST_SCRIPT_H

#include "PdfXObject.h"

namespace PoDoFo {

class PODOFO_API PdfXObjectPostScript final : public PdfXObject
{
    friend class PdfXObject;

public:
    Rect GetRect() const override;

private:
    PdfXObjectPostScript(PdfObject& obj);
};

}

#endif // PDF_XOBJECT_POST_SCRIPT_H
