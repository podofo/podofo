/**
 * SPDX-FileCopyrightText: (C) 2009 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPageTreeCache.h"

#include "PdfPage.h"
#include "PdfPageCollection.h"

using namespace std;
using namespace PoDoFo;

PdfPageTreeCache::PdfPageTreeCache(unsigned initialSize)
{
    if (initialSize < (1 << 20))
        m_PageObjs.resize(initialSize);
}

PdfPage* PdfPageTreeCache::GetPage(unsigned atIndex)
{
    if (atIndex >= m_PageObjs.size())
        return nullptr;

    return m_PageObjs[atIndex];
}

void PdfPageTreeCache::SetPage(unsigned atIndex, PdfPage* page)
{
    // Delete an old page if it is at the same position
    PdfPage* oldPage = GetPage(atIndex);
    delete oldPage;

    if (atIndex >= m_PageObjs.size())
        m_PageObjs.resize((size_t)atIndex + 1);

    m_PageObjs[atIndex] = page;
}

void PdfPageTreeCache::SetPages(unsigned atIndex, const vector<PdfPage*>& pages)
{
    if ((atIndex + pages.size()) >= m_PageObjs.size())
        m_PageObjs.resize(atIndex + pages.size() + 1);

    for (unsigned i = 0; i < pages.size(); i++)
    {
        // Delete any old pages if it is at the same position
        PdfPage* oldPage = GetPage(atIndex + i);
        delete oldPage;

        // Assign the new page
        m_PageObjs[(size_t)atIndex + i] = pages.at(i);
    }
}

void PdfPageTreeCache::InsertPlaceHolders(unsigned atIndex, unsigned count)
{
    // Reserve the total space needed
    m_PageObjs.reserve((size_t)atIndex + count);
    if (atIndex >= m_PageObjs.size())
    {
        // Resize to the the insertion index
        m_PageObjs.resize(atIndex);
    }

    for (unsigned i = 0; i < count; i++)
        m_PageObjs.insert(m_PageObjs.begin() + atIndex + i, static_cast<PdfPage*>(nullptr));
}

void PdfPageTreeCache::DeletePage(unsigned atIndex)
{
    if (atIndex >= m_PageObjs.size())
        return;

    delete m_PageObjs[atIndex];
    m_PageObjs.erase(m_PageObjs.begin() + atIndex);
}

void PdfPageTreeCache::ClearCache()
{
    for (auto page : m_PageObjs)
        delete page;

    m_PageObjs.clear();
}
