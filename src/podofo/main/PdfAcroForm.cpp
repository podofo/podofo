// SPDX-FileCopyrightText: 2007 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

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

    auto ret = m_Fields[index].get();
    if (ret == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The field at index {} is invalid", index);

    return *ret;
}

PdfField& PdfAcroForm::getField(const PdfReference& ref) const
{
    const_cast<PdfAcroForm&>(*this).initFields();
    auto found = m_fieldMap->find(ref);
    if (found == m_fieldMap->end())
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    unsigned index = found->second;
    PODOFO_ASSERT(index < m_Fields.size());
    auto ret = m_Fields[index].get();
    if (ret == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The field at index {} is invalid", index);

    return *ret;
}

void PdfAcroForm::RemoveFieldAt(unsigned index)
{
    initFields();
    if (index >= m_Fields.size())
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    inlineFieldsArray();
    promoteDirectEntries();
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
    PODOFO_ASSERT(index < m_Fields.size());
    inlineFieldsArray();
    promoteDirectEntries();
    m_fieldArray->RemoveAt(index);
    m_Fields.erase(m_Fields.begin() + index);
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
    return iterator(m_Fields.begin(), m_Fields.end());
}

PdfAcroForm::iterator PdfAcroForm::end()
{
    initFields();
    return iterator(m_Fields.end(), m_Fields.end());
}

PdfAcroForm::const_iterator PdfAcroForm::begin() const
{
    const_cast<PdfAcroForm&>(*this).initFields();
    return const_iterator(m_Fields.begin(), m_Fields.end());
}

PdfAcroForm::const_iterator PdfAcroForm::end() const
{
    const_cast<PdfAcroForm&>(*this).initFields();
    return const_iterator(m_Fields.end(), m_Fields.end());
}

PdfField& PdfAcroForm::CreateField(PdfObject& obj, PdfFieldType type)
{
    return AddField(PdfField::Create(obj, *this, type));
}

PdfField& PdfAcroForm::AddField(unique_ptr<PdfField>&& field)
{
    initFields();
    if (m_fieldArray == nullptr)
    {
        m_fieldArray = &GetDictionary().AddKey("Fields"_n, PdfArray()).GetArray();
    }
    else
    {
        inlineFieldsArray();
        promoteDirectEntries();
    }

    (*m_fieldMap)[field->GetObject().GetIndirectReference()] = (unsigned)m_Fields.size();
    m_fieldArray->AddIndirectSafe(field->GetObject());
    m_Fields.push_back(std::move(field));
    return *m_Fields.back();
}

shared_ptr<PdfField> PdfAcroForm::GetFieldPtr(const PdfReference& ref)
{
    PODOFO_INVARIANT(m_fieldMap != nullptr);
    auto found = m_fieldMap->find(ref);
    if (found == m_fieldMap->end())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The field is not listed in the AcroForm");

    unsigned index = found->second;
    PODOFO_ASSERT(index < m_Fields.size());
    return m_Fields[index];
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

// A /Fields array stored in its own object may be shared with the /Kids of
// a field: it's copied inline in the form before a modification
void PdfAcroForm::inlineFieldsArray()
{
    if (!m_fieldArray->GetOwner()->IsIndirect())
        return;

    m_fieldArray = &GetDictionary().AddKey("Fields"_n, PdfArray(*m_fieldArray)).GetArray();
}

void PdfAcroForm::initFields()
{
    if (m_fieldMap != nullptr)
        return;

    // The state is built in locals and committed at the end, so a throwing
    // entry doesn't leave the form half initialized
    unique_ptr<FieldMap> fieldMap(new FieldMap());
    FieldList fields;
    auto fieldArray = getFieldArray();
    if (fieldArray != nullptr)
    {
        fields.reserve(fieldArray->size());
        unique_ptr<PdfField> field;
        unsigned i = 0;
        for (auto obj : fieldArray->GetIndirectIterator())
        {
            auto& ref = obj->GetIndirectReference();
            // NOTE: Direct fields all share the 0 0 R reference
            if (!fieldMap->try_emplace(ref, i).second && ref.IsIndirect())
            {
                // A repeated reference is wrapped only once: the duplicate is a placeholder
                fields.push_back(nullptr);
            }
            else if (PdfField::TryCreateFromObject(*obj, field))
            {
                field->SetAcroForm(*this);
                fields.push_back(std::move(field));
            }
            else
            {
                // The field may be invalid. In that case we push a placeholder
                fields.push_back(nullptr);
            }

            i++;
        }
    }

    m_fieldMap = std::move(fieldMap);
    m_Fields = std::move(fields);
    m_fieldArray = fieldArray;
}

// Fields from direct dictionaries point into the array storage, which a
// modification can relocate or free: such entries are first moved to indirect
// objects, as the specification requires, and the elements are rebound to them
void PdfAcroForm::promoteDirectEntries()
{
    PODOFO_ASSERT(m_fieldArray->GetSize() == m_Fields.size());
    bool promoted = false;
    for (unsigned i = 0; i < m_fieldArray->GetSize(); i++)
    {
        auto& elem = (*m_fieldArray)[i];
        if (elem.IsDictionary())
        {
            // It's a field stored directly in the array
            auto& obj = PoDoFo::MoveToIndirect(elem);
            if (m_Fields[i] != nullptr)
                m_Fields[i]->SetObject(obj);

            promoted = true;
        }
    }

    if (!promoted)
        return;

    // Key the map again, as the promoted entries have their own references now
    m_fieldMap->clear();
    unsigned i = 0;
    for (auto obj : m_fieldArray->GetIndirectIterator())
    {
        (void)m_fieldMap->try_emplace(obj->GetIndirectReference(), i);
        i++;
    }
}

void PdfAcroForm::fixIndices(unsigned index)
{
    for (auto it = m_fieldMap->begin(); it != m_fieldMap->end(); )
    {
        if (it->second == index)
        {
            // The entry of the removed slot: it exists also for placeholders
            it = m_fieldMap->erase(it);
            continue;
        }

        // Decrement indices where needed
        if (it->second > index)
            it->second--;

        it++;
    }
}
