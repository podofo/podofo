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
#include "PdfStreamDevice.h"

using namespace std;
using namespace cmn;
using namespace PoDoFo;

PdfFileSpec::PdfFileSpec(PdfDocument& doc, const string_view& filename, bool embed, bool striPath)
    : PdfDictionaryElement(doc, "Filespec")
{
    Init(filename, embed, striPath);
}

PdfFileSpec::PdfFileSpec(PdfDocument& doc, const string_view& filename, const char* data, size_t size, bool striPath)
    : PdfDictionaryElement(doc, "Filespec")
{
    Init(filename, data, size, striPath);
}

PdfFileSpec::PdfFileSpec(PdfObject& obj)
    : PdfDictionaryElement(obj)
{
}

void PdfFileSpec::Init(const string_view& filename, bool embed, bool striPath)
{
    this->GetObject().GetDictionary().AddKey("F", this->CreateFileSpecification(MaybeStripPath(filename, striPath)));
    this->GetObject().GetDictionary().AddKey("UF", PdfString(MaybeStripPath(filename, true)));

    if (embed)
    {
        PdfDictionary ef;

        auto& embeddedStream = this->GetDocument().GetObjects().CreateDictionaryObject("EmbeddedFile");
        this->EmbeddFile(embeddedStream, filename);

        ef.AddKey("F", embeddedStream.GetIndirectReference());

        this->GetObject().GetDictionary().AddKey("EF", ef);
    }
}

void PdfFileSpec::Init(const string_view& filename, const char* data, size_t size, bool striPath)
{
    this->GetObject().GetDictionary().AddKey("F", this->CreateFileSpecification(MaybeStripPath(filename, striPath)));
    this->GetObject().GetDictionary().AddKey("UF", PdfString(MaybeStripPath(filename, true)));

    PdfDictionary ef;

    auto& embeddedStream = this->GetDocument().GetObjects().CreateDictionaryObject("EmbeddedFile");
    this->EmbeddFileFromMem(embeddedStream, data, size);

    ef.AddKey("F", embeddedStream.GetIndirectReference());

    this->GetObject().GetDictionary().AddKey("EF", ef);
}

PdfString PdfFileSpec::CreateFileSpecification(const string_view& filename) const
{
    // FIX-ME: The following is not Unicode compliant

    outstringstream str;
    char buff[5];

    // Construct a platform independent file specifier

    for (size_t i = 0; i < filename.size(); i++)
    {
        char ch = filename[i];
        if (ch == ':' || ch == '\\')
            ch = '/';
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_')
        {
            str.put(ch & 0xFF);
        }
        else if (ch == '/')
        {
            str.put('\\');
            str.put('\\');
            str.put('/');
        }
        else
        {
            utls::FormatTo(buff, 5, "{:02X}", ch & 0xFF);
            str << buff;
        }
    }

    return PdfString(str.str());
}

void PdfFileSpec::EmbeddFile(PdfObject& obj, const string_view& filename) const
{
    size_t size = utls::FileSize(filename);

    FileStreamDevice input(filename);
    obj.GetOrCreateStream().SetData(input);

    // Add additional information about the embedded file to the stream
    PdfDictionary params;
    params.AddKey("Size", static_cast<int64_t>(size));
    // TODO: CreationDate and ModDate
    obj.GetDictionary().AddKey("Params", params);
}

string PdfFileSpec::MaybeStripPath(const string_view& filename, bool stripPath) const
{
    // FIX-ME: The following is not Unicode compliant
    if (!stripPath)
        return (string)filename;

    string_view lastFrom = filename;
    for (size_t i = 0; i < filename.size(); i++)
    {
        char ch = filename[i];
        if (
#ifdef _WIN32
            ch == ':' || ch == '\\' ||
#endif // _WIN32
            ch == '/')
        {
            lastFrom = ((string_view)filename).substr(i + 1);
        }
    }

    return (string)lastFrom;
}

void PdfFileSpec::EmbeddFileFromMem(PdfObject& obj, const char* data, size_t size) const
{
    obj.GetOrCreateStream().SetData(bufferview(data, size));

    // Add additional information about the embedded file to the stream
    PdfDictionary params;
    params.AddKey("Size", static_cast<int64_t>(size));
    obj.GetDictionary().AddKey("Params", params);
}

const PdfString& PdfFileSpec::GetFilename(bool canUnicode) const
{
    if (canUnicode && this->GetObject().GetDictionary().HasKey("UF"))
        return this->GetObject().GetDictionary().MustFindKey("UF").GetString();

    if (this->GetObject().GetDictionary().HasKey("F"))
        return this->GetObject().GetDictionary().MustFindKey("F").GetString();

    PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);
}
