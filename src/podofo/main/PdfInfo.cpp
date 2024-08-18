/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfInfo.h"

#include "PdfDate.h"
#include "PdfDictionary.h"
#include "PdfString.h"

#define PRODUCER_STRING "PoDoFo - https://github.com/podofo/podofo"

using namespace std;
using namespace PoDoFo;

PdfInfo::PdfInfo(PdfObject& obj)
    : PdfDictionaryElement(obj)
{
}

PdfInfo::PdfInfo(PdfObject& obj, PdfInfoInitial initial)
    : PdfInfo(obj)
{
    init(initial);
}

void PdfInfo::init(PdfInfoInitial initial)
{
    auto now = PdfDate::LocalNow();
    PdfString str = now.ToString();

    if ((initial & PdfInfoInitial::WriteCreationTime) == PdfInfoInitial::WriteCreationTime)
        this->GetDictionary().AddKey("CreationDate"_n, str);

    if ((initial & PdfInfoInitial::WriteModificationTime) == PdfInfoInitial::WriteModificationTime)
        this->GetDictionary().AddKey("ModDate"_n, str);

    if ((initial & PdfInfoInitial::WriteProducer) == PdfInfoInitial::WriteProducer)
        this->GetDictionary().AddKey("Producer"_n, PdfString(PRODUCER_STRING));
}

nullable<const PdfString&> PdfInfo::getStringFromInfoDict(const string_view& name) const
{
    auto obj = this->GetDictionary().FindKey(name);
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return nullptr;
    else
        return *str;
}

void PdfInfo::SetAuthor(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetDictionary().AddKey("Author"_n, *value);
    else
        this->GetDictionary().RemoveKey("Author");
}

void PdfInfo::SetCreator(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetDictionary().AddKey("Creator"_n, *value);
    else
        this->GetDictionary().RemoveKey("Creator");
}

void PdfInfo::SetKeywords(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetDictionary().AddKey("Keywords"_n, *value);
    else
        this->GetDictionary().RemoveKey("Keywords");
}

void PdfInfo::SetSubject(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetDictionary().AddKey("Subject"_n, *value);
    else
        this->GetDictionary().RemoveKey("Subject");
}

void PdfInfo::SetTitle(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetDictionary().AddKey("Title"_n, *value);
    else
        this->GetDictionary().RemoveKey("Title");
}

void PdfInfo::SetProducer(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetDictionary().AddKey("Producer"_n, *value);
    else
        this->GetDictionary().RemoveKey("Producer");
}

void PdfInfo::SetTrapped(nullable<const PdfName&> trapped)
{
    if (trapped.has_value())
    {
        if (*trapped == "True" || *trapped == "False")
            this->GetDictionary().AddKey("Trapped"_n, *trapped);
        else
            this->GetDictionary().AddKey("Trapped"_n, "Unknown"_n);
    }
    else
    {
        this->GetDictionary().RemoveKey("Trapped");
    }
}

nullable<const PdfString&> PdfInfo::GetAuthor() const
{
    return this->getStringFromInfoDict("Author");
}

nullable<const PdfString&> PdfInfo::GetCreator() const
{
    return this->getStringFromInfoDict("Creator");
}

nullable<const PdfString&> PdfInfo::GetKeywords() const
{
    return this->getStringFromInfoDict("Keywords");
}

nullable<const PdfString&> PdfInfo::GetSubject() const
{
    return this->getStringFromInfoDict("Subject");
}

nullable<const PdfString&> PdfInfo::GetTitle() const
{
    return this->getStringFromInfoDict("Title");
}

nullable<const PdfString&> PdfInfo::GetProducer() const
{
    return this->getStringFromInfoDict("Producer");
}

nullable<PdfDate> PdfInfo::GetCreationDate() const
{
    auto datestr = this->getStringFromInfoDict("CreationDate");
    PdfDate date;
    if (datestr == nullptr || ! PdfDate::TryParse(*datestr, date))
        return nullptr;

    return date;
}

nullable<PdfDate> PdfInfo::GetModDate() const
{
    auto datestr = this->getStringFromInfoDict("ModDate");
    PdfDate date;
    if (datestr == nullptr || !PdfDate::TryParse(*datestr, date))
        return nullptr;

    return date;
}

nullable<const PdfName&> PdfInfo::GetTrapped() const
{
    auto obj = this->GetDictionary().FindKey("Trapped");
    const PdfName* name;
    if (obj == nullptr || !obj->TryGetName(name))
        return nullptr;
    else
        return *name;
}

void PdfInfo::SetCreationDate(nullable<PdfDate> value)
{
    if (value.has_value())
        this->GetDictionary().AddKey("CreationDate"_n, value->ToString());
    else
        this->GetDictionary().RemoveKey("CreationDate");
}

void PdfInfo::SetModDate(nullable<PdfDate> value)
{
    if (value.has_value())
        this->GetDictionary().AddKey("ModDate"_n, value->ToString());
    else
        this->GetDictionary().RemoveKey("ModDate");
}
