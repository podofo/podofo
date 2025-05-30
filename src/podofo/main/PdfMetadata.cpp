/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfMetadata.h"

#include <podofo/private/XMPUtils.h>

#include "PdfDocument.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdfMetadata::PdfMetadata(PdfDocument& doc)
    : m_doc(&doc), m_metadata(nullptr), m_xmpSynced(false)
{
}

PdfMetadata::~PdfMetadata()
{
    delete m_metadata;
}

void PdfMetadata::SetTitle(nullable<const PdfString&> title)
{
    ensureInitialized();
    if (m_metadata->Title == title)
        return;

    m_doc->GetOrCreateInfo().SetTitle(title);
    if (title == nullptr)
        m_metadata->Title = nullptr;
    else
        m_metadata->Title = *title;

    m_xmpSynced = false;
}

nullable<const PdfString&> PdfMetadata::GetTitle() const
{
    const_cast<PdfMetadata&>(*this).ensureInitialized();
    return m_metadata->Title;
}

void PdfMetadata::SetAuthor(nullable<const PdfString&> author)
{
    ensureInitialized();
    if (m_metadata->Author == author)
        return;

    m_doc->GetOrCreateInfo().SetAuthor(author);
    if (author == nullptr)
        m_metadata->Author = nullptr;
    else
        m_metadata->Author = *author;

    m_xmpSynced = false;
}

nullable<const PdfString&> PdfMetadata::GetAuthor() const
{
    const_cast<PdfMetadata&>(*this).ensureInitialized();
    return m_metadata->Author;
}

void PdfMetadata::SetSubject(nullable<const PdfString&> subject)
{
    ensureInitialized();
    if (m_metadata->Subject == subject)
        return;

    m_doc->GetOrCreateInfo().SetSubject(subject);
    if (subject == nullptr)
        m_metadata->Subject = nullptr;
    else
        m_metadata->Subject = *subject;

    m_xmpSynced = false;
}

nullable<const PdfString&> PdfMetadata::GetSubject() const
{
    const_cast<PdfMetadata&>(*this).ensureInitialized();
    return m_metadata->Subject;
}

nullable<const PdfString&> PdfMetadata::GetKeywordsRaw() const
{
    const_cast<PdfMetadata&>(*this).ensureInitialized();
    return m_metadata->Keywords;
}

void PdfMetadata::SetKeywords(vector<string> keywords)
{
    if (keywords.size() == 0)
        setKeywords(nullptr);
    else
        setKeywords(PdfString(PoDoFo::ToPdfKeywordsString(keywords)));
}

void PdfMetadata::setKeywords(nullable<const PdfString&> keywords)
{
    ensureInitialized();
    if (m_metadata->Keywords == keywords)
        return;

    m_doc->GetOrCreateInfo().SetKeywords(keywords);
    if (keywords == nullptr)
        m_metadata->Keywords = nullptr;
    else
        m_metadata->Keywords = *keywords;

    m_xmpSynced = false;
}

vector<string> PdfMetadata::GetKeywords() const
{
    const_cast<PdfMetadata&>(*this).ensureInitialized();
    if (m_metadata->Keywords == nullptr)
        return vector<string>();
    else
        return PoDoFo::ToPdfKeywordsList(*m_metadata->Keywords);
}

void PdfMetadata::SetCreator(nullable<const PdfString&> creator)
{
    ensureInitialized();
    if (m_metadata->Creator == creator)
        return;

    m_doc->GetOrCreateInfo().SetCreator(creator);
    if (creator == nullptr)
        m_metadata->Creator = nullptr;
    else
        m_metadata->Creator = *creator;

    m_xmpSynced = false;
}

nullable<const PdfString&> PdfMetadata::GetCreator() const
{
    const_cast<PdfMetadata&>(*this).ensureInitialized();
    return m_metadata->Creator;
}

void PdfMetadata::SetProducer(nullable<const PdfString&> producer)
{
    ensureInitialized();
    if (m_metadata->Producer == producer)
        return;

    m_doc->GetOrCreateInfo().SetProducer(producer);
    if (producer == nullptr)
        m_metadata->Producer = nullptr;
    else
        m_metadata->Producer = *producer;

    m_xmpSynced = false;
}

nullable<const PdfString&> PdfMetadata::GetProducer() const
{
    const_cast<PdfMetadata&>(*this).ensureInitialized();
    return m_metadata->Producer;
}

void PdfMetadata::SetCreationDate(nullable<PdfDate> date)
{
    ensureInitialized();
    if (m_metadata->CreationDate == date)
        return;

    m_doc->GetOrCreateInfo().SetCreationDate(date);
    m_metadata->CreationDate = date;
    m_xmpSynced = false;
}

nullable<const PdfDate&> PdfMetadata::GetCreationDate() const
{
    const_cast<PdfMetadata&>(*this).ensureInitialized();
    return m_metadata->CreationDate;
}

void PdfMetadata::SetModifyDate(nullable<PdfDate> date)
{
    ensureInitialized();
    if (m_metadata->ModDate == date)
        return;

    m_doc->GetOrCreateInfo().SetModDate(date);
    m_metadata->ModDate = date;
    m_xmpSynced = false;
}

nullable<const PdfDate&> PdfMetadata::GetModifyDate() const
{
    const_cast<PdfMetadata&>(*this).ensureInitialized();
    return m_metadata->ModDate;
}

void PdfMetadata::SetTrapped(nullable<bool> trapped)
{
    ensureInitialized();
    if (m_metadata->Trapped == trapped)
        return;

    if (trapped == nullptr)
        m_doc->GetOrCreateInfo().SetTrapped(nullptr);
    else
        m_doc->GetOrCreateInfo().SetTrapped(*trapped ? PdfName("True") : PdfName("False"));

    m_metadata->Trapped = trapped;
    m_xmpSynced = false;
}

nullable<bool> PdfMetadata::GetTrapped() const
{
    const_cast<PdfMetadata&>(*this).ensureInitialized();
    return m_metadata->Trapped;
}

void PdfMetadata::SetPdfVersion(PdfVersion version)
{
    m_doc->SetPdfVersion(version);
}

PdfVersion PdfMetadata::GetPdfVersion() const
{
    return m_doc->GetPdfVersion();
}

PdfALevel PdfMetadata::GetPdfALevel() const
{
    const_cast<PdfMetadata&>(*this).ensureInitialized();
    return m_metadata->PdfaLevel;
}

void PdfMetadata::SetPdfALevel(PdfALevel level)
{
    ensureInitialized();
    if (m_metadata->PdfaLevel == level)
        return;

    if (level != PdfALevel::Unknown)
    {
        // The PDF/A level can be set only in XMP,
        // metadata let's ensure it exists
        if (m_packet == nullptr)
            m_packet.reset(new PdfXMPPacket());
    }

    m_metadata->PdfaLevel = level;
    m_xmpSynced = false;
}

PdfUALevel PdfMetadata::GetPdfUALevel() const
{
    const_cast<PdfMetadata&>(*this).ensureInitialized();
    return m_metadata->PdfuaLevel;
}

void PdfMetadata::SetPdfUALevel(PdfUALevel level)
{
    ensureInitialized();
    if (m_metadata->PdfuaLevel == level)
        return;

    if (level != PdfUALevel::Unknown)
    {
        // The PDF/UA level can be set only in XMP,
        // metadata let's ensure it exists
        if (m_packet == nullptr)
            m_packet.reset(new PdfXMPPacket());
    }

    m_metadata->PdfuaLevel = level;
    m_xmpSynced = false;
}

nullable<const PdfString&> PdfMetadata::GetProperty(PdfAdditionalMetadata prop) const
{
    if (m_metadata == nullptr)
        return nullptr;

    return m_metadata->GetMetadata(prop);
}

void PdfMetadata::SetProperty(PdfAdditionalMetadata prop, nullable<const PdfString&> value)
{
    ensureInitialized();
    m_metadata->SetMetadata(prop, (const PdfString*)value);
}

void PdfMetadata::SyncXMPMetadata(bool resetXMPPacket)
{
    ensureInitialized();
    if (m_xmpSynced)
        return;

    syncXMPMetadata(resetXMPPacket);
}

bool PdfMetadata::TrySyncXMPMetadata()
{
    ensureInitialized();
    if (m_packet == nullptr)
        return true;

    if (m_xmpSynced)
        return true;

    syncXMPMetadata(false);
    return true;
}

unique_ptr<PdfXMPPacket> PdfMetadata::TakeXMPPacket()
{
    ensureInitialized();
    if (m_packet == nullptr)
        return nullptr;

    if (!m_xmpSynced)
    {
        // If the XMP packet is not synced, do it now
        m_packet->SetMetadata(*m_metadata);
    }

    invalidate();
    return std::move(m_packet);
}

void PdfMetadata::Invalidate()
{
    invalidate();
    m_packet = nullptr;
}

void PdfMetadata::invalidate()
{
    m_metadata = nullptr;
    m_xmpSynced = false;
    m_metadata = { };
}

void PdfMetadata::ensureInitialized()
{
    if (m_metadata != nullptr)
        return;

    m_metadata = new PdfMetadataStore();
    auto info = m_doc->GetInfo();
    if (info != nullptr)
    {
        auto title = info->GetTitle();
        if (title != nullptr)
            m_metadata->Title = *title;

        auto author = info->GetAuthor();
        if (author != nullptr)
            m_metadata->Author = *author;

        auto subject = info->GetSubject();
        if (subject != nullptr)
            m_metadata->Subject = *subject;

        auto keywords = info->GetKeywords();
        if (keywords != nullptr)
            m_metadata->Keywords = *keywords;

        auto creator = info->GetCreator();
        if (creator != nullptr)
            m_metadata->Creator = *creator;

        auto producer = info->GetProducer();
        if (producer != nullptr)
            m_metadata->Producer = *producer;

        auto trapped = info->GetTrapped();
        if (trapped != nullptr)
        {
            if (*trapped == "True")
                m_metadata->Trapped = true;
            else if (*trapped == "False")
                m_metadata->Trapped = false;
        }

        m_metadata->CreationDate = info->GetCreationDate();
        m_metadata->ModDate = info->GetModDate();
    }
    auto metadataValue = m_doc->GetCatalog().GetMetadataStreamValue();
    m_packet = PdfXMPPacket::Create(metadataValue);
    if (m_packet != nullptr)
    {
        auto xmpMetadata = m_packet->GetMetadata();
        if (m_metadata->Title == nullptr)
            m_metadata->Title = xmpMetadata.Title;
        if (m_metadata->Author == nullptr)
            m_metadata->Author = xmpMetadata.Author;
        if (m_metadata->Subject == nullptr)
            m_metadata->Subject = xmpMetadata.Subject;
        if (m_metadata->Keywords == nullptr)
            m_metadata->Keywords = xmpMetadata.Keywords;
        if (m_metadata->Creator == nullptr)
            m_metadata->Creator = xmpMetadata.Creator;
        if (m_metadata->Producer == nullptr)
            m_metadata->Producer = xmpMetadata.Producer;
        if (m_metadata->CreationDate == nullptr)
            m_metadata->CreationDate = xmpMetadata.CreationDate;
        if (m_metadata->ModDate == nullptr)
            m_metadata->ModDate = xmpMetadata.ModDate;
        if (m_metadata->Trapped == nullptr)
            m_metadata->Trapped = xmpMetadata.Trapped;
        m_metadata->PdfaLevel = xmpMetadata.PdfaLevel;
        m_metadata->PdfuaLevel = xmpMetadata.PdfuaLevel;
        m_xmpSynced = true;
    }
}

void PdfMetadata::syncXMPMetadata(bool resetXMPPacket)
{
    if (m_packet == nullptr || resetXMPPacket)
        m_packet.reset(new PdfXMPPacket());

    m_packet->SetMetadata(*m_metadata);
    m_doc->GetCatalog().SetMetadataStreamValue(m_packet->ToString());
    m_xmpSynced = true;
}

