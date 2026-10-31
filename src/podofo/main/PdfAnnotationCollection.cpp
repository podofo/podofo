// SPDX-FileCopyrightText: 2022 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfAnnotationCollection.h"
#include "PdfPage.h"
#include "PdfMath.h"

using namespace std;
using namespace PoDoFo;

PdfAnnotationCollection::PdfAnnotationCollection(PdfPage& page)
    : m_Page(&page), m_annotArray(nullptr)
{
}

PdfAnnotation& PdfAnnotationCollection::CreateAnnot(PdfAnnotationType annotType, const Rect& rect)
{
    Rect actualRect = PoDoFo::TransformRectPage(rect, *m_Page);
    return addAnnotation(PdfAnnotation::Create(*m_Page, annotType, actualRect));
}

PdfAnnotation& PdfAnnotationCollection::GetAnnotAt(unsigned index)
{
    return getAnnotAt(index);
}

const PdfAnnotation& PdfAnnotationCollection::GetAnnotAt(unsigned index) const
{
    return getAnnotAt(index);
}

PdfAnnotation& PdfAnnotationCollection::GetAnnot(const PdfReference& ref)
{
    return getAnnot(ref);
}

const PdfAnnotation& PdfAnnotationCollection::GetAnnot(const PdfReference& ref) const
{
    return getAnnot(ref);
}

void PdfAnnotationCollection::RemoveAnnotAt(unsigned index)
{
    initAnnotations();
    if (index >= m_Annots.size())
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    inlineAnnotationArray();
    promoteDirectEntries();
    m_annotArray->RemoveAt(index);
    m_Annots.erase(m_Annots.begin() + index);
    fixIndices(index);

    // NOTE: No need to remove the object from the document
    // indirect object list: it will be garbage collected
}

void PdfAnnotationCollection::RemoveAnnot(const PdfReference& ref)
{
    initAnnotations();
    auto found = m_annotMap->find(ref);
    if (found == m_annotMap->end())
        return;

    unsigned index = found->second;
    PODOFO_ASSERT(index < m_Annots.size());
    inlineAnnotationArray();
    promoteDirectEntries();
    m_annotArray->RemoveAt(index);
    m_Annots.erase(m_Annots.begin() + index);
    fixIndices(index);

    // NOTE: No need to remove the object from the document
    // indirect object list: it will be garbage collected
}

unsigned PdfAnnotationCollection::GetCount() const
{
    const_cast<PdfAnnotationCollection&>(*this).initAnnotations();
    return (unsigned)m_Annots.size();
}

PdfAnnotationCollection::iterator PdfAnnotationCollection::begin()
{
    initAnnotations();
    return iterator(m_Annots.begin(), m_Annots.end());
}

PdfAnnotationCollection::iterator PdfAnnotationCollection::end()
{
    initAnnotations();
    return iterator(m_Annots.end(), m_Annots.end());
}

PdfAnnotationCollection::const_iterator PdfAnnotationCollection::begin() const
{
    const_cast<PdfAnnotationCollection&>(*this).initAnnotations();
    return const_iterator(m_Annots.begin(), m_Annots.end());
}

PdfAnnotationCollection::const_iterator PdfAnnotationCollection::end() const
{
    const_cast<PdfAnnotationCollection&>(*this).initAnnotations();
    return const_iterator(m_Annots.end(), m_Annots.end());
}

PdfAnnotation& PdfAnnotationCollection::addAnnotation(unique_ptr<PdfAnnotation>&& annot)
{
    initAnnotations();
    if (m_annotArray == nullptr)
    {
        m_annotArray = &m_Page->GetDictionary().AddKey("Annots"_n, PdfArray()).GetArray();
    }
    else
    {
        inlineAnnotationArray();
        promoteDirectEntries();
    }

    (*m_annotMap)[annot->GetObject().GetIndirectReference()] = (unsigned)m_Annots.size();
    m_annotArray->AddIndirectSafe(annot->GetObject());
    auto ret = annot.get();
    m_Annots.push_back(std::move(annot));
    return *ret;
}

// An /Annots array stored in its own object may be shared with other pages, and
// Adobe Acrobat reports its change on an incremental update as a generic DocMDP
// change: it's copied inline in the page before a modification
void PdfAnnotationCollection::inlineAnnotationArray()
{
    if (!m_annotArray->GetOwner()->IsIndirect())
        return;

    m_annotArray = &m_Page->GetDictionary().AddKey("Annots"_n, PdfArray(*m_annotArray)).GetArray();
}

// Annotations from direct dictionaries point into the array storage, which a
// modification can relocate or free: such entries are first moved to indirect
// objects, as the specification requires, and the elements are rebound to them
void PdfAnnotationCollection::promoteDirectEntries()
{
    PODOFO_ASSERT(m_annotArray->GetSize() == m_Annots.size());
    bool promoted = false;
    for (unsigned i = 0; i < m_annotArray->GetSize(); i++)
    {
        auto& elem = (*m_annotArray)[i];
        if (elem.IsDictionary())
        {
            // It's an annotation stored directly in the array
            auto& obj = PoDoFo::MoveToIndirect(elem);
            if (m_Annots[i] != nullptr)
                m_Annots[i]->SetObject(obj);

            promoted = true;
        }
    }

    if (!promoted)
        return;

    // Key the map again, as the promoted entries have their own references now
    m_annotMap->clear();
    unsigned i = 0;
    for (auto obj : m_annotArray->GetIndirectIterator())
    {
        (void)m_annotMap->try_emplace(obj->GetIndirectReference(), i);
        i++;
    }
}

PdfArray* PdfAnnotationCollection::getAnnotationsArray() const
{
    auto obj = const_cast<PdfAnnotationCollection&>(*this).m_Page->GetDictionary().FindKey("Annots");
    if (obj == nullptr)
        return nullptr;

    return &obj->GetArray();
}

PdfAnnotation& PdfAnnotationCollection::getAnnotAt(unsigned index) const
{
    const_cast<PdfAnnotationCollection&>(*this).initAnnotations();
    if (index >= m_Annots.size())
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    auto ret = m_Annots[index].get();
    if (ret == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The annotation at index {} is invalid", index);

    return *ret;
}

PdfAnnotation& PdfAnnotationCollection::getAnnot(const PdfReference& ref) const
{
    const_cast<PdfAnnotationCollection&>(*this).initAnnotations();
    auto found = m_annotMap->find(ref);
    if (found == m_annotMap->end())
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    unsigned index = found->second;
    PODOFO_ASSERT(index < m_Annots.size());
    auto ret = m_Annots[index].get();
    if (ret == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The annotation at index {} is invalid", index);

    return *ret;
}

void PdfAnnotationCollection::initAnnotations()
{
    if (m_annotMap != nullptr)
        return;

    // The state is built in locals and committed at the end, so a throwing
    // entry doesn't leave the collection half initialized
    unique_ptr<AnnotationMap> annotMap(new AnnotationMap());
    AnnotationList annots;
    auto annotArray = getAnnotationsArray();
    if (annotArray != nullptr)
    {
        annots.reserve(annotArray->size());
        unique_ptr<PdfAnnotation> annot;
        unsigned i = 0;
        for (auto obj : annotArray->GetIndirectIterator())
        {
            auto& ref = obj->GetIndirectReference();
            // NOTE: Direct annotations all share the 0 0 R reference
            if (!annotMap->try_emplace(ref, i).second && ref.IsIndirect())
            {
                // A repeated reference is wrapped only once: the duplicate is a placeholder
                annots.push_back(nullptr);
            }
            else if (PdfAnnotation::TryCreateFromObject(*obj, annot))
            {
                annot->SetPage(*m_Page);
                annots.push_back(std::move(annot));
            }
            else
            {
                // The annotation may be invalid. In that case we push a placeholder
                annots.push_back(nullptr);
            }

            i++;
        }
    }

    m_annotMap = std::move(annotMap);
    m_Annots = std::move(annots);
    m_annotArray = annotArray;
}

void PdfAnnotationCollection::fixIndices(unsigned index)
{
    for (auto it = m_annotMap->begin(); it != m_annotMap->end(); )
    {
        if (it->second == index)
        {
            // The entry of the removed slot: it exists also for placeholders
            it = m_annotMap->erase(it);
            continue;
        }

        // Decrement indices where needed
        if (it->second > index)
            it->second--;

        it++;
    }
}
