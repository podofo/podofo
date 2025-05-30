/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfMetadataStore.h"

using namespace std;
using namespace PoDoFo;

PdfMetadataStore::PdfMetadataStore()
    : Version(PdfVersion::Unknown), PdfaLevel(PdfALevel::Unknown), PdfuaLevel(PdfUALevel::Unknown) { }

const PdfString* PdfMetadataStore::GetMetadata(PdfAdditionalMetadata prop) const
{
    if (m_additionalMetadata == nullptr)
        return nullptr;

    auto found = m_additionalMetadata->find(prop);
    if (found == m_additionalMetadata->end())
        return nullptr;

    return &found->second;
}

void PdfMetadataStore::SetMetadata(PdfAdditionalMetadata prop, const PdfString* value)
{
    if (value == nullptr)
    {
        if (m_additionalMetadata == nullptr)
            return;

        m_additionalMetadata->erase(prop);
    }
    else
    {
        if (m_additionalMetadata == nullptr)
            m_additionalMetadata.reset(new unordered_map<PdfAdditionalMetadata, PdfString>());

        (*m_additionalMetadata)[prop] = *value;
    }
}

void PdfMetadataStore::Reset()
{
    Title = nullptr;
    Author = nullptr;
    Subject = nullptr;
    Keywords = nullptr;
    Creator = nullptr;
    Producer = nullptr;
    CreationDate = nullptr;
    ModDate = nullptr;
    Trapped = nullptr;
    Version = PdfVersion::Unknown;
    PdfaLevel = PdfALevel::Unknown;
    PdfuaLevel = PdfUALevel::Unknown;
    m_additionalMetadata.reset();
}
