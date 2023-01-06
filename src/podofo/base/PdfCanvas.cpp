/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCanvas.h"

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfObjectStream.h"

using namespace std;
using namespace PoDoFo;

PdfCanvas::~PdfCanvas() { }

const PdfObject* PdfCanvas::GetContentsObject() const
{
    return getContentsObject();
}

PdfObject* PdfCanvas::GetContentsObject()
{
    return getContentsObject();
}

PdfObject* PdfCanvas::GetFromResources(const string_view& type, const string_view& key)
{
    return getFromResources(type, key);
}

const PdfObject* PdfCanvas::GetFromResources(const string_view& type, const string_view& key) const
{
    return getFromResources(type, key);
}

PdfResources* PdfCanvas::GetResources()
{
    return getResources();
}

const PdfResources* PdfCanvas::GetResources() const
{
    return getResources();
}

PdfElement& PdfCanvas::GetElement()
{
    return getElement();
}

const PdfElement& PdfCanvas::GetElement() const
{
    return getElement();
}

PdfObject* PdfCanvas::getFromResources(const string_view& type, const string_view& key) const
{
    auto resources = getResources();
    if (resources == nullptr)
        return nullptr;

    return resources->GetResource(type, key);

}

PdfArray PdfCanvas::GetProcSet()
{
    PdfArray procset;
    procset.Add(PdfName("PDF"));
    procset.Add(PdfName("Text"));
    procset.Add(PdfName("ImageB"));
    procset.Add(PdfName("ImageC"));
    procset.Add(PdfName("ImageI"));
    return procset;
}
