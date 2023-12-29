/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFileSpec.h"

#include <podofo/private/outstringstream.h>

#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfObject.h"
#include "PdfObjectStream.h"
#include <podofo/auxiliary/StreamDevice.h>

using namespace std;
using namespace cmn;
using namespace PoDoFo;

PdfFileSpec::PdfFileSpec(PdfDocument& doc)
    : PdfDictionaryElement(doc, "Filespec")
{
}

PdfFileSpec::PdfFileSpec(PdfObject& obj)
    : PdfDictionaryElement(obj)
{
}

bool PdfFileSpec::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfFileSpec>& filespec)
{
    PdfDictionary* dict;
    const PdfObject* typeObj;
    const PdfString* typeStr;
    if (!obj.TryGetDictionary(dict)
        || (typeObj = dict->FindKey(PdfName::KeyType)) == nullptr
        || typeObj->TryGetString(typeStr)
        || typeStr->GetString() != "Filespec")
    {
        filespec.reset();
        return false;
    }

    filespec.reset(new PdfFileSpec(obj));
    return true;
}

nullable<const PdfString&> PdfFileSpec::GetFilename() const
{
    auto filenameObj = GetDictionary().FindKey("UF");
    if (filenameObj == nullptr)
    {
        // As a fallback try to access the non unicode one
        filenameObj = GetDictionary().FindKey("F");
        if (filenameObj == nullptr)
            return nullptr;
    }

    return filenameObj->GetString();
}

void PdfFileSpec::SetFilename(nullable<const PdfString&>& filename)
{
    auto& dict = GetDictionary();
    if (filename == nullptr)
        dict.RemoveKey("UF");
    else
        dict.AddKey("UF", *filename);
    dict.RemoveKey("F");
}

void PdfFileSpec::SetEmbeddedData(nullable<const charbuff&>& data)
{
    if (data == nullptr)
    {
        auto& dict = GetDictionary();
        dict.RemoveKey("EF");
        dict.RemoveKey("F");
    }
    else
    {
        BufferStreamDevice input(*data);
        setData(input, data->size());
    }
}

void PdfFileSpec::SetEmbeddedDataFromFile(const string_view& filepath)
{
    size_t size = utls::FileSize(filepath);
    FileStreamDevice input(filepath);
    setData(input, size);
}

nullable<charbuff> PdfFileSpec::GetEmbeddedData() const
{
    auto efObj = GetDictionary().FindKey("EF");
    const PdfDictionary* efDict;
    const PdfObject* fObj;
    const PdfObjectStream* stream;
    if (efObj == nullptr
        || !efObj->TryGetDictionary(efDict)
        || ((fObj = efDict->FindKey("UF")) == nullptr && (fObj = efDict->FindKey("F")) == nullptr)
        || (stream = fObj->GetStream()) == nullptr)
    {
        return nullptr;
    }

    charbuff ret;
    stream->CopyTo(ret);
    return std::move(ret);
}

void PdfFileSpec::setData(InputStream& input, size_t size)
{
    auto& fObj = this->GetDocument().GetObjects().CreateDictionaryObject("EmbeddedFile");
    fObj.GetOrCreateStream().SetData(input);

    // Add additional information about the embedded file to the stream
    PdfDictionary params;
    params.AddKey("Size", static_cast<int64_t>(size));
    // TODO: CreationDate and ModDate
    fObj.GetDictionary().AddKey("Params", params);
    auto& efObj = GetDictionary().AddKey("EF", PdfDictionary());
    efObj.GetDictionary().AddKeyIndirect("F", fObj);
}
