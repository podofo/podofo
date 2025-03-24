/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

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

    if (m_Annots[index] != nullptr)
    {
        // It may be null if the annotation is invalid
        m_annotMap->erase(m_annotMap->find(m_Annots[index]->GetObject().GetIndirectReference()));
    }

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
    m_Annots.erase(m_Annots.begin() + index);
    m_annotArray->RemoveAt(index);
    m_annotMap->erase(found);
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
    return m_Annots.begin();
}

PdfAnnotationCollection::iterator PdfAnnotationCollection::end()
{
    initAnnotations();
    return m_Annots.end();
}

PdfAnnotationCollection::const_iterator PdfAnnotationCollection::begin() const
{
    const_cast<PdfAnnotationCollection&>(*this).initAnnotations();
    return m_Annots.begin();
}

PdfAnnotationCollection::const_iterator PdfAnnotationCollection::end() const
{
    const_cast<PdfAnnotationCollection&>(*this).initAnnotations();
    return m_Annots.end();
}

PdfAnnotation& PdfAnnotationCollection::addAnnotation(unique_ptr<PdfAnnotation>&& annot)
{
    initAnnotations();
    if (m_annotArray == nullptr)
        m_annotArray = &m_Page->GetDictionary().AddKey("Annots"_n, PdfArray()).GetArray();

    (*m_annotMap)[annot->GetObject().GetIndirectReference()] = m_annotArray->GetSize();
    m_annotArray->AddIndirectSafe(annot->GetObject());
    auto ret = annot.get();
    m_Annots.push_back(std::move(annot));
    return *ret;
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
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The exception at index {} is invalid", index);

    return *ret;
}

PdfAnnotation& PdfAnnotationCollection::getAnnot(const PdfReference& ref) const
{
    const_cast<PdfAnnotationCollection&>(*this).initAnnotations();
    unsigned index = (*m_annotMap).at(ref);
    auto ret = m_Annots[index].get();
    if (ret == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The exception at index {} is invalid", index);

    return *ret;
}

void PdfAnnotationCollection::initAnnotations()
{
    if (m_annotMap != nullptr)
        return;

    m_annotMap.reset(new AnnotationMap());
    m_annotArray = getAnnotationsArray();
    if (m_annotArray == nullptr)
        return;

    m_Annots.reserve(m_annotArray->size());
    unique_ptr<PdfAnnotation> annot;
    unsigned i = 0;
    for (auto obj : m_annotArray->GetIndirectIterator())
    {
        (*m_annotMap)[obj->GetIndirectReference()] = i;
        // The annotation may be invalid. In that case we push a placeholder
        if (PdfAnnotation::TryCreateFromObject(*obj, annot))
        {
            annot->SetPage(*m_Page);
            m_Annots.push_back(std::move(annot));
        }
        else
        {
            m_Annots.push_back(nullptr);
        }

        i++;
    }
}

void PdfAnnotationCollection::fixIndices(unsigned index)
{
    for (auto& pair : *m_annotMap)
    {
        // Decrement indices where needed
        if (pair.second > index)
            pair.second--;
    }
}
