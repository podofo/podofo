/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfTrailer.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdfTrailer::PdfTrailer(PdfObject& obj)
    : PdfDictionaryElement(obj) { }
