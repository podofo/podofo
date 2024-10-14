/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfBaseDataTypes.h"

#include "PdfName.h"
#include "PdfString.h"
#include "PdfDictionary.h"
#include "PdfArray.h"
#include "PdfData.h"

using namespace std;
using namespace PoDoFo;

PdfDataMember::PdfDataMember(PdfDataType dataType)
    : m_DataType(dataType) { }
