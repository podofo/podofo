/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>

#include "PdfAcroForm.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfFont.h"
#include "PdfStringStream.h"

using namespace std;
using namespace PoDoFo;

// The AcroForm dict does NOT have a /Type key!
PdfAcroForm::PdfAcroForm(PdfDocument& doc, PdfAcroFormDefaulAppearance defaultAppearance)
    : PdfDictionaryElement(doc), m_fieldArray(nullptr)
{
    // Initialize with an empty fields array
    this->GetDictionary().AddKey("Fields"_n, PdfArray());
    init(defaultAppearance);
}

PdfAcroForm::PdfAcroForm(PdfObject& obj)
    : PdfDictionaryElement(obj), m_fieldArray(nullptr)
{
}

void PdfAcroForm::init(PdfAcroFormDefaulAppearance defaultAppearance)
{
    // Add default appearance: black text, 12pt times 
    // -> only if we do not have a DA key yet

    if (defaultAppearance == PdfAcroFormDefaulAppearance::ArialBlack)
    {
        PdfFontCreateParams createParams;
        PdfFontSearchParams searchParams;
        searchParams.AutoSelect = PdfFontAutoSelectBehavior::Standard14;
        auto font = GetDocument().GetFonts().SearchFont("Helvetica", searchParams, createParams);

        // Create DR key
        auto drObj = GetDictionary().FindKey("DR");
        unique_ptr<PdfResources> resx;
        if (drObj == nullptr || !PdfResources::TryCreateFromObject(*drObj, resx))
            resx.reset(new PdfResources(GetDocument()));

        // Create DA key
        PdfStringStream ss;
        ss << "0 0 0 rg 0 g /" << resx->AddResource(PdfResourceType::Font, font->GetObject()).GetString() << " 0 Tf";
        this->GetDictionary().AddKey("DA"_n, PdfString(ss.GetString()));
    }
}

PdfField& PdfAcroForm::CreateField(const string_view& name, PdfFieldType fieldType)
{
    return AddField(PdfField::Create(name, *this, fieldType));
}

PdfField& PdfAcroForm::GetFieldAt(unsigned index)
{
    return getField(index);
}

const PdfField& PdfAcroForm::GetFieldAt(unsigned index) const
{
    return getField(index);
}

PdfField& PdfAcroForm::GetField(const PdfReference& ref)
{
    return getField(ref);
}

const PdfField& PdfAcroForm::GetField(const PdfReference& ref) const
{
    return getField(ref);
}

PdfField& PdfAcroForm::getField(unsigned index) const
{
    const_cast<PdfAcroForm&>(*this).initFields();
    if (index >= m_Fields.size())
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    return *m_Fields[index];
}

PdfField& PdfAcroForm::getField(const PdfReference& ref) const
{
    const_cast<PdfAcroForm&>(*this).initFields();
    return *m_Fields[(*m_fieldMap).at(ref)];
}

void PdfAcroForm::RemoveFieldAt(unsigned index)
{
    initFields();
    if (index >= m_Fields.size())
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    if (m_Fields[index] != nullptr)
    {
        // It may be null if the field is invalid
        m_fieldMap->erase(m_fieldMap->find(m_Fields[index]->GetObject().GetIndirectReference()));
    }

    m_fieldArray->RemoveAt(index);
    m_Fields.erase(m_Fields.begin() + index);
    fixIndices(index);

    // NOTE: No need to remove the object from the document
    // indirect object list: it will be garbage collected
}

void PdfAcroForm::RemoveField(const PdfReference& ref)
{
    initFields();
    auto found = m_fieldMap->find(ref);
    if (found == m_fieldMap->end())
        return;

    unsigned index = found->second;
    m_Fields.erase(m_Fields.begin() + index);
    m_fieldArray->RemoveAt(index);
    m_fieldMap->erase(found);
    fixIndices(index);

    // NOTE: No need to remove the object from the document
    // indirect object list: it will be garbage collected
}

unsigned PdfAcroForm::GetFieldCount() const
{
    const_cast<PdfAcroForm&>(*this).initFields();
    return (unsigned)m_Fields.size();
}

PdfAcroForm::iterator PdfAcroForm::begin()
{
    initFields();
    return iterator(m_Fields.begin());
}

PdfAcroForm::iterator PdfAcroForm::end()
{
    initFields();
    return iterator(m_Fields.end());
}

PdfAcroForm::const_iterator PdfAcroForm::begin() const
{
    const_cast<PdfAcroForm&>(*this).initFields();
    return const_iterator(m_Fields.begin());
}

PdfAcroForm::const_iterator PdfAcroForm::end() const
{
    const_cast<PdfAcroForm&>(*this).initFields();
    return const_iterator(m_Fields.end());
}

PdfField& PdfAcroForm::CreateField(PdfObject& obj, PdfFieldType type)
{
    return AddField(PdfField::Create(obj, *this, type));
}

PdfField& PdfAcroForm::AddField(unique_ptr<PdfField>&& field)
{
    initFields();
    if (m_fieldArray == nullptr)
        m_fieldArray = &GetDictionary().AddKey("Fields"_n, PdfArray()).GetArray();

    (*m_fieldMap)[field->GetObject().GetIndirectReference()] = m_fieldArray->GetSize();
    m_fieldArray->AddIndirectSafe(field->GetObject());
    m_Fields.push_back(std::move(field));
    return *m_Fields.back();
}

shared_ptr<PdfField> PdfAcroForm::GetFieldPtr(const PdfReference& ref)
{
    PODOFO_INVARIANT(m_fieldMap != nullptr);
    return m_Fields[(*m_fieldMap)[ref]];
}

void PdfAcroForm::SetNeedAppearances(bool needAppearances)
{
    this->GetDictionary().AddKey("NeedAppearances"_n, PdfVariant(needAppearances));
}

bool PdfAcroForm::GetNeedAppearances() const
{
    return this->GetDictionary().FindKeyAsSafe<bool>("NeedAppearances", false);
}

PdfAcroFormSigFlags PdfAcroForm::GetSigFlags() const
{
    int64_t num;
    if (!GetDictionary().TryFindKeyAs("SigFlags", num))
        return PdfAcroFormSigFlags::None;

    return (PdfAcroFormSigFlags)num;
}

void PdfAcroForm::SetSigFlags(PdfAcroFormSigFlags flags)
{
    GetDictionary().AddKey("SigFlags"_n, (int64_t)flags);
}

PdfArray* PdfAcroForm::getFieldArray() const
{
    auto obj = const_cast<PdfAcroForm&>(*this).GetDictionary().FindKey("Fields");
    if (obj == nullptr)
        return nullptr;

    return &obj->GetArray();
}

void PdfAcroForm::initFields()
{
    if (m_fieldMap != nullptr)
        return;

    m_fieldMap.reset(new FieldMap());
    m_fieldArray = getFieldArray();
    if (m_fieldArray == nullptr)
        return;

    m_Fields.reserve(m_fieldArray->size());
    unique_ptr<PdfField> field;
    unsigned i = 0;
    for (auto obj : m_fieldArray->GetIndirectIterator())
    {
        (*m_fieldMap)[obj->GetIndirectReference()] = i;
        // The field may be invalid. In that case we push a placeholder
        if (PdfField::TryCreateFromObject(*obj, field))
        {
            field->SetAcroForm(*this);
            m_Fields.push_back(std::move(field));
        }
        else
        {
            m_Fields.push_back(nullptr);
        }

        i++;
    }
}

void PdfAcroForm::fixIndices(unsigned index)
{
    for (auto& pair : *m_fieldMap)
    {
        // Decrement indices where needed
        if (pair.second > index)
            pair.second--;
    }
}
