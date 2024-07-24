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
#include <podofo/auxiliary/StreamDevice.h>

using namespace std;
using namespace PoDoFo;

PdfCanvas::~PdfCanvas() { }

const PdfObject* PdfCanvas::GetContentsObject() const
{
    return const_cast<PdfCanvas&>(*this).getContentsObject();
}

PdfObject* PdfCanvas::GetContentsObject()
{
    return getContentsObject();
}

charbuff PdfCanvas::GetContentsCopy() const
{
    charbuff ret;
    CopyContentsTo(ret);
    return ret;
}

void PdfCanvas::CopyContentsTo(charbuff& buffer) const
{
    buffer.clear();
    BufferStreamDevice stream(buffer);
    CopyContentsTo(stream);
}

PdfObject* PdfCanvas::GetFromResources(PdfResourceType type, const string_view& key)
{
    return getFromResources(type, key);
}

const PdfObject* PdfCanvas::GetFromResources(PdfResourceType type, const string_view& key) const
{
    return const_cast<PdfCanvas&>(*this).getFromResources(type, key);
}

PdfResources* PdfCanvas::GetResources()
{
    return getResources();
}

const PdfResources* PdfCanvas::GetResources() const
{
    return const_cast<PdfCanvas&>(*this).getResources();
}

PdfDictionaryElement& PdfCanvas::GetElement()
{
    return getElement();
}

const PdfDictionaryElement& PdfCanvas::GetElement() const
{
    return const_cast<PdfCanvas&>(*this).getElement();
}

PdfObject* PdfCanvas::getFromResources(PdfResourceType type, const string_view& key)
{
    auto resources = getResources();
    if (resources == nullptr)
        return nullptr;

    return resources->GetResource(type, key);

}
