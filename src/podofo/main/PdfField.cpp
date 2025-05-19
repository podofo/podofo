/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfField.h"

#include "PdfDocument.h"
#include "PdfPainter.h"
#include "PdfStreamedDocument.h"
#include "PdfXObject.h"
#include "PdfSignature.h"
#include "PdfPushButton.h"
#include "PdfRadioButton.h"
#include "PdfCheckBox.h"
#include "PdfTextBox.h"
#include "PdfComboBox.h"
#include "PdfListBox.h"

 // https://en.wikipedia.org/wiki/Escape_character#ASCII_escape_character
#define ESCAPE_CHARACTER "\033"
#define CHECK_FIELD_NAME(name) if (name.find('.') != string::npos)\
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidKey, "Unsupported dot \".\" in field name. Use PdfField.CreateChild()");

using namespace std;
using namespace PoDoFo;

void getFullName(const PdfObject& obj, bool skipEscapePartialName, string& fullname);

PdfField::PdfField(PdfAnnotationWidget& widget,
        PdfFieldType fieldType, shared_ptr<PdfField>&& parent) :
    PdfDictionaryElement(widget.GetObject()),
    m_Widget(&widget),
    m_AcroForm(nullptr),
    m_FieldType(fieldType),
    m_Parent(std::move(parent))
{
    if (*m_Parent == nullptr)
    {
        init();
    }
    else
    {
        // Set /Parent key to newly created field
        GetDictionary().AddKey("Parent"_n, (*m_Parent)->GetObject().GetIndirectReference());
    }
}

PdfField::PdfField(PdfAcroForm& acroform, PdfFieldType fieldType,
        shared_ptr<PdfField>&& parent) :
    PdfDictionaryElement(acroform.GetDocument()),
    m_Widget(nullptr),
    m_AcroForm(&acroform),
    m_FieldType(fieldType),
    m_Parent(std::move(parent))
{
    if (*m_Parent == nullptr)
    {
        init();
    }
    else
    {
        // Set /Parent key to newly created field
        GetDictionary().AddKey("Parent"_n, (*m_Parent)->GetObject().GetIndirectReference());
    }
}

// NOTE: This constructor does not need to perform initialization
PdfField::PdfField(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType) :
    PdfDictionaryElement(obj),
    m_Widget(nullptr),
    m_AcroForm(acroform),
    m_FieldType(fieldType)
{
}

bool PdfField::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfField>& field)
{
    return tryCreateField(obj, getFieldType(obj), field);
}

unique_ptr<PdfField> PdfField::CreateChild()
{
    return createChildField(nullptr, Rect());
}

unique_ptr<PdfField> PdfField::CreateChild(PdfPage& page, const Rect& rect)
{
    return createChildField(&page, rect);
}

void PdfField::SetParent(shared_ptr<PdfField>&& parent)
{
    m_Parent = std::move(parent);
}

PdfField* PdfField::GetParentSafe()
{
    initParent();
    return m_Parent->get();
}

const PdfField* PdfField::GetParentSafe() const
{
    const_cast<PdfField&>(*this).initParent();
    return m_Parent->get();
}

void PdfField::initParent()
{
    if (m_Parent.has_value())
        return;

    auto parent = GetDictionary().FindKey("Parent");
    if (parent == nullptr)
    {
        m_Parent = nullptr;
        return;
    }

    unique_ptr<PdfField> field;
    (void)TryCreateFromObject(*parent, field);
    m_Parent = shared_ptr<PdfField>(std::move(field));
}

void PdfField::initChildren()
{
    if (m_Children == nullptr)
        m_Children.reset(new PdfFieldChildrenCollectionBase(*this));
}

void PdfField::ensureAccessibilityIfNeeded(const string_view& fieldName)
{
    if (GetDocument().GetMetadata().GetPdfUALevel() == PdfUALevel::Unknown
        && !PoDoFo::IsAccessibiltyProfile(GetDocument().GetMetadata().GetPdfALevel()))
    {
        return;
    }

    // Set the /TU key
    SetAlternateName(PdfString(string(getFieldTypeDisplayName()).append(" ").append(fieldName)));
    auto widget = GetWidget();
    if (widget != nullptr)
        PoDoFo::CreateObjectStructElement(*this, widget->MustGetPage(), "Form"_n);
}

unique_ptr<PdfField> PdfField::createChildField(PdfPage* page, const Rect& rect)
{
    if (m_Widget == nullptr && m_AcroForm == nullptr)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic,
            "Unsupported creating a child from a field not bound to an annotation or AcroForm");
    }

    unique_ptr<PdfField> ret;
    if (page == nullptr)
    {
        if (m_AcroForm == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The field is not bound to an acroform");

        ret = createField(GetDocument().GetOrCreateAcroForm(), m_FieldType, GetPtr());
    }
    else
    {
        if (m_Widget != nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The field is already bound to a widget");

        // NOTE: Don't create a field from the page here. It's
        // enough to create a widget annotation. It will be
        // linked to the parent field below
        auto& widget = static_cast<PdfAnnotationWidget&>(page->GetAnnotations().CreateAnnot(PdfAnnotationType::Widget, rect));
        ret = createField(widget, m_FieldType, GetPtr(), false);
    }

    return ret;
}

PdfField& PdfField::Create(const string_view& name,
    PdfAnnotationWidget& widget, PdfFieldType type)
{
    CHECK_FIELD_NAME(name);
    auto& doc = widget.GetDocument();
    PdfField* candidateParent = nullptr;
    auto acroForm = doc.GetAcroForm();
    if (acroForm != nullptr)
    {
        for (auto field : *acroForm)
        {
            // Find a candidate parent with same name and type
            auto fieldName = field->GetNameRaw();
            if (fieldName.has_value() && *fieldName == name)
            {
                if (field->GetType() != type)
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidKey, "Found field with same name and different type");

                candidateParent = field;
                break;
            }
        }
    }

    shared_ptr<PdfField> newField;
    if (candidateParent == nullptr)
    {
        newField = createField(widget, type, nullptr, true);
        newField->setName(name);
        newField->ensureAccessibilityIfNeeded(name);
    }
    else
    {
        // Prepare keys to remove that will stay on the parent
        constexpr string_view parentKeys[] = { "FT"sv, "Ff"sv, "T"sv, "TU"sv, "V"sv, "Opt"sv };
        if (!candidateParent->GetChildren().HasKidsArray())
        {
            PODOFO_INVARIANT(acroForm != null);
            // The candidate parent doesn't have kids, we create an
            // actual parent in the acroform and move relevant
            // keys there
            auto& actualParent = acroForm->AddField(createField(*acroForm, type, nullptr));
            linkFieldObjectToParent(candidateParent->GetPtr(), actualParent, parentKeys, true, true);

            // Remove the old parent from AcroForm /Fields,
            // we just keep the new one
            acroForm->RemoveField(candidateParent->GetObject().GetIndirectReference());

            // We finally set the candidate parent as the actual newly created parent
            candidateParent = &actualParent;
        }

        newField = createField(widget, type, candidateParent->GetPtr(), false);
        linkFieldObjectToParent(newField, *candidateParent, parentKeys, false, false);
    }

    auto ret = newField.get();
    widget.SetField(std::move(newField));
    return *ret;
}

unique_ptr<PdfField> PdfField::Create(const string_view& name,
    PdfAcroForm& acroform, PdfFieldType type)
{
    CHECK_FIELD_NAME(name);
    auto ret = createField(acroform, type, nullptr);
    ret->setName(name);
    ret->ensureAccessibilityIfNeeded(name);
    return ret;
}

unique_ptr<PdfField> PdfField::Create(PdfObject& obj, PdfAcroForm& acroForm, PdfFieldType type)
{
    // NOTE: The following use the non initializing constructor
    unique_ptr<PdfField> ret;
    switch (type)
    {
        case PdfFieldType::PushButton:
            ret.reset(new PdfPushButton(obj, &acroForm));
            break;
        case PdfFieldType::CheckBox:
            ret.reset(new PdfCheckBox(obj, &acroForm));
            break;
        case PdfFieldType::RadioButton:
            ret.reset(new PdfRadioButton(obj, &acroForm));
            break;
        case PdfFieldType::TextBox:
            ret.reset(new PdfTextBox(obj, &acroForm));
            break;
        case PdfFieldType::ComboBox:
            ret.reset(new PdfComboBox(obj, &acroForm));
            break;
        case PdfFieldType::ListBox:
            ret.reset(new PdfListBox(obj, &acroForm));
            break;
        case PdfFieldType::Signature:
            ret.reset(new PdfSignature(obj, &acroForm));
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }

    return ret;
}

unique_ptr<PdfField> PdfField::createField(PdfAcroForm& acroform, PdfFieldType type,
    shared_ptr<PdfField>&& parent)
{
    switch (type)
    {
        case PdfFieldType::PushButton:
            return unique_ptr<PdfField>(new PdfPushButton(acroform, std::move(parent)));
        case PdfFieldType::CheckBox:
            return unique_ptr<PdfField>(new PdfCheckBox(acroform, std::move(parent)));
        case PdfFieldType::RadioButton:
            return unique_ptr<PdfField>(new PdfRadioButton(acroform, std::move(parent)));
        case PdfFieldType::TextBox:
            return unique_ptr<PdfField>(new PdfTextBox(acroform, std::move(parent)));
        case PdfFieldType::ComboBox:
            return unique_ptr<PdfField>(new PdfComboBox(acroform, std::move(parent)));
        case PdfFieldType::ListBox:
            return unique_ptr<PdfField>(new PdfListBox(acroform, std::move(parent)));
        case PdfFieldType::Signature:
            return unique_ptr<PdfField>(new PdfSignature(acroform, std::move(parent)));
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }
}

unique_ptr<PdfField> PdfField::createField(PdfAnnotationWidget& widget,
    PdfFieldType type, shared_ptr<PdfField>&& parent, bool insertInAcroform)
{
    unique_ptr<PdfField> ret;
    switch (type)
    {
        case PdfFieldType::PushButton:
            ret.reset(new PdfPushButton(widget, std::move(parent)));
            break;
        case PdfFieldType::CheckBox:
            ret.reset(new PdfCheckBox(widget, std::move(parent)));
            break;
        case PdfFieldType::RadioButton:
            ret.reset(new PdfRadioButton(widget, std::move(parent)));
            break;
        case PdfFieldType::TextBox:
            ret.reset(new PdfTextBox(widget, std::move(parent)));
            break;
        case PdfFieldType::ComboBox:
            ret.reset(new PdfComboBox(widget, std::move(parent)));
            break;
        case PdfFieldType::ListBox:
            ret.reset(new PdfListBox(widget, std::move(parent)));
            break;
        case PdfFieldType::Signature:
            ret.reset(new PdfSignature(widget, std::move(parent)));
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }

    if (insertInAcroform)
        (void)widget.GetDocument().GetOrCreateAcroForm().CreateField(ret->GetObject(), ret->GetType());

    return ret;
}

shared_ptr<PdfField> PdfField::GetPtr()
{
    if (m_AcroForm != nullptr)
        return m_AcroForm->GetFieldPtr(GetObject().GetIndirectReference());
    else if (m_Widget != nullptr)
        return m_Widget->GetFieldPtr();
    else
        return nullptr;
}

PdfField* PdfField::getParentTyped(PdfFieldType type) const
{
    auto parent = const_cast<PdfField&>(*this).GetParentSafe();
    if (parent == nullptr)
        return nullptr;

    if (parent->GetType() != type)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType,
            "The requested parent has a different type than requested");
    }

    return parent;
}

string_view PdfField::getFieldTypeDisplayName() const
{
    switch (m_FieldType)
    {
        case PdfFieldType::PushButton:
            return "Push-button"sv;
        case PdfFieldType::CheckBox:
            return "Check box"sv;
        case PdfFieldType::RadioButton:
            return "Radio button"sv;
        case PdfFieldType::TextBox:
            return "Text box"sv;
        case PdfFieldType::ComboBox:
            return "Combo box"sv;
        case PdfFieldType::ListBox:
            return "List box"sv;
        case PdfFieldType::Signature:
            return "Signature"sv;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

bool PdfField::tryCreateField(PdfObject& obj, PdfFieldType type,
    unique_ptr<PdfField>& field)
{
    switch (type)
    {
        case PdfFieldType::PushButton:
            field.reset(new PdfPushButton(obj, nullptr));
            return true;
        case PdfFieldType::CheckBox:
            field.reset(new PdfCheckBox(obj, nullptr));
            return true;
        case PdfFieldType::RadioButton:
            field.reset(new PdfRadioButton(obj, nullptr));
            return true;
        case PdfFieldType::TextBox:
            field.reset(new PdfTextBox(obj, nullptr));
            return true;
        case PdfFieldType::ComboBox:
            field.reset(new PdfComboBox(obj, nullptr));
            return true;
        case PdfFieldType::ListBox:
            field.reset(new PdfListBox(obj, nullptr));
            return true;
        case PdfFieldType::Signature:
            field.reset(new PdfSignature(obj, nullptr));
            return true;
            // We allow creation of unknown type field
        case PdfFieldType::Unknown:
            field.reset(new PdfField(obj, nullptr, PdfFieldType::Unknown));
            return true;
        default:
            field.reset();
            return false;
    }
}

PdfFieldType PdfField::getFieldType(const PdfObject& obj)
{
    PdfFieldType ret = PdfFieldType::Unknown;

    // ISO 32000:2008, Section 12.7.3.1, Table 220, Page #432.
    auto ftObj = obj.GetDictionary().FindKeyParent("FT");
    if (ftObj == nullptr)
        return PdfFieldType::Unknown;

    auto& fieldType = ftObj->GetName();
    if (fieldType == "Btn")
    {
        int64_t flags;
        PdfField::GetFieldFlags(obj, flags);

        if ((flags & PdfButton::PdfButton_PushButton) == PdfButton::PdfButton_PushButton)
            ret = PdfFieldType::PushButton;
        else if ((flags & PdfButton::PdfButton_Radio) == PdfButton::PdfButton_Radio)
            ret = PdfFieldType::RadioButton;
        else
            ret = PdfFieldType::CheckBox;
    }
    else if (fieldType == "Tx")
    {
        ret = PdfFieldType::TextBox;
    }
    else if (fieldType == "Ch")
    {
        int64_t flags;
        PdfField::GetFieldFlags(obj, flags);

        if ((flags & PdChoiceField::PdfListField_Combo) == PdChoiceField::PdfListField_Combo)
            ret = PdfFieldType::ComboBox;
        else
            ret = PdfFieldType::ListBox;
    }
    else if (fieldType == "Sig")
    {
        ret = PdfFieldType::Signature;
    }

    return ret;
}

void PdfField::init()
{
    PdfDictionary& dict = GetDictionary();
    switch (m_FieldType)
    {
        case PdfFieldType::CheckBox:
            dict.AddKey("FT"_n, "Btn"_n);
            break;
        case PdfFieldType::PushButton:
            dict.AddKey("FT"_n, "Btn"_n);
            dict.AddKey("Ff"_n, (int64_t)PdfButton::PdfButton_PushButton);
            break;
        case PdfFieldType::RadioButton:
            dict.AddKey("FT"_n, "Btn"_n);
            dict.AddKey("Ff"_n, (int64_t)(PdfButton::PdfButton_Radio | PdfButton::PdfButton_NoToggleOff));
            break;
        case PdfFieldType::TextBox:
            dict.AddKey("FT"_n, "Tx"_n);
            break;
        case PdfFieldType::ListBox:
            dict.AddKey("FT"_n, "Ch"_n);
            break;
        case PdfFieldType::ComboBox:
            dict.AddKey("FT"_n, "Ch"_n);
            dict.AddKey("Ff"_n, (int64_t)PdChoiceField::PdfListField_Combo);
            break;
        case PdfFieldType::Signature:
            dict.AddKey("FT"_n, "Sig"_n);
            break;

        case PdfFieldType::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
            break;
    }
}

PdfObject* PdfField::getValueObject() const
{
    return const_cast<PdfField&>(*this).GetDictionary().FindKey("V");
}

void PdfField::AssertTerminalField() const
{
    if (GetDictionary().HasKey("Kids"))
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "This method can be called only on terminal field. Ensure this field has "
            "not been retrieved from AcroFormFields collection or it's not a parent of terminal fields");
    }
}

PdfAnnotationWidget& PdfField::MustGetWidget()
{
    if (m_Widget == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Expected to retrieve a field with a linked widget annotation");

    return *m_Widget;
}

const PdfAnnotationWidget& PdfField::MustGetWidget() const
{
    if (m_Widget == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Expected to retrieve a field with a linked widget annotation");

    return *m_Widget;
}

PdfFieldChildrenCollectionBase& PdfField::GetChildren()
{
    initChildren();
    return *m_Children;
}

const PdfFieldChildrenCollectionBase& PdfField::GetChildren() const
{
    const_cast<PdfField&>(*this).initChildren();
    return *m_Children;
}

void PdfField::SetFieldFlag(int64_t value, bool set)
{
    // Retrieve parent field flags
    // CHECK-ME: It seems this semantics is not honored in all cases,
    // example for CheckBoxesRadioButton (s)
    int64_t curr = 0;
    auto fieldFlagsObj = GetDictionary().FindKeyParent("Ff");
    if (fieldFlagsObj != nullptr)
        curr = fieldFlagsObj->GetNumber();

    if (set)
    {
        curr |= value;
    }
    else
    {
        if ((curr & value) == value)
            curr ^= value;
    }

    GetDictionary().AddKey("Ff"_n, curr);
}

bool PdfField::GetFieldFlag(int64_t value, bool defvalue) const
{
    int64_t flag;
    if (!GetFieldFlags(GetObject(), flag))
        return defvalue;

    return (flag & value) == value;
}


bool PdfField::GetFieldFlags(const PdfObject& obj, int64_t& value)
{
    auto flagsObject = obj.GetDictionary().FindKeyParent("Ff");
    if (flagsObject == nullptr)
    {
        value = 0;
        return false;
    }

    value = flagsObject->GetNumber();
    return true;
}

void PdfField::SetHighlightingMode(PdfHighlightingMode mode)
{
    PdfName value;

    switch (mode)
    {
        case PdfHighlightingMode::None:
            value = "N"_n;
            break;
        case PdfHighlightingMode::Invert:
            value = "I"_n;
            break;
        case PdfHighlightingMode::InvertOutline:
            value = "O"_n;
            break;
        case PdfHighlightingMode::Push:
            value = "P"_n;
            break;
        case PdfHighlightingMode::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidName);
            break;
    }

    GetDictionary().AddKey("H"_n, value);
}

PdfHighlightingMode PdfField::GetHighlightingMode() const
{
    PdfHighlightingMode mode = PdfHighlightingMode::Invert;

    if (GetDictionary().HasKey("H"))
    {
        auto& value = GetDictionary().MustFindKey("H").GetName();
        if (value == "N")
            return PdfHighlightingMode::None;
        else if (value == "I")
            return PdfHighlightingMode::Invert;
        else if (value == "O")
            return PdfHighlightingMode::InvertOutline;
        else if (value == "P")
            return PdfHighlightingMode::Push;
    }

    return mode;
}

void PdfField::SetName(nullable<const PdfString&> name)
{
    if (name.has_value())
    {
        CHECK_FIELD_NAME(name->GetString());
        setName(*name);
    }
    else
    {
        GetDictionary().RemoveKey("T");
    }
}

void PdfField::setName(const PdfString& name)
{
    GetDictionary().AddKey("T"_n, name);
}

PdfObject* PdfField::GetValueObject()
{
    return getValueObject();
}

const PdfObject* PdfField::GetValueObject() const
{
    return getValueObject();
}

nullable<const PdfString&> PdfField::GetName() const
{
    auto obj = GetDictionary().FindKeyParent("T");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return { };

    return *str;
}

nullable<const PdfString&> PdfField::GetNameRaw() const
{
    auto obj = GetDictionary().GetKey("T");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return { };

    return *str;
}

string PdfField::GetFullName(bool skipEscapePartialName) const
{
    string fullName;
    getFullName(GetObject(), skipEscapePartialName, fullName);
    return fullName;
}

void PdfField::SetAlternateName(nullable<const PdfString&> name)
{
    if (name.has_value())
        GetDictionary().AddKey("TU"_n, *name);
    else
        GetDictionary().RemoveKey("TU");
}

nullable<const PdfString&> PdfField::GetAlternateName() const
{
    auto obj = GetDictionary().FindKeyParent("TU");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return { };

    return *str;
}

void PdfField::SetMappingName(nullable<const PdfString&> name)
{
    if (name.has_value())
        GetDictionary().AddKey("TM"_n, *name);
    else
        GetDictionary().RemoveKey("TM");
}

nullable<const PdfString&> PdfField::GetMappingName() const
{
    auto obj = GetDictionary().FindKeyParent("TM");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return { };

    return *str;
}

void PdfField::addAlternativeAction(const PdfName& name, const PdfAction& action)
{
    auto aaObj = GetDictionary().FindKey("AA");
    if (aaObj == nullptr)
        aaObj = &GetDictionary().AddKey("AA"_n, PdfDictionary());

    aaObj->GetDictionary().AddKey(name, action.GetObject().GetIndirectReference());
}

void PdfField::SetReadOnly(bool readOnly)
{
    this->SetFieldFlag(static_cast<int64_t>(PdfFieldFlags::ReadOnly), readOnly);
}

bool PdfField::IsReadOnly() const
{
    return this->GetFieldFlag(static_cast<int64_t>(PdfFieldFlags::ReadOnly), false);
}

void PdfField::SetRequired(bool required)
{
    this->SetFieldFlag(static_cast<int64_t>(PdfFieldFlags::Required), required);
}

bool PdfField::IsRequired() const
{
    return this->GetFieldFlag(static_cast<int64_t>(PdfFieldFlags::Required), false);
}

void PdfField::SetNoExport(bool exprt)
{
    this->SetFieldFlag(static_cast<int64_t>(PdfFieldFlags::NoExport), exprt);
}

bool PdfField::IsNoExport() const
{
    return this->GetFieldFlag(static_cast<int64_t>(PdfFieldFlags::NoExport), false);
}

void PdfField::SetMouseEnterAction(const PdfAction& action)
{
    this->addAlternativeAction("E"_n, action);
}

void PdfField::SetMouseLeaveAction(const PdfAction& action)
{
    this->addAlternativeAction("X"_n, action);
}

void PdfField::SetMouseDownAction(const PdfAction& action)
{
    this->addAlternativeAction("D"_n, action);
}

void PdfField::SetMouseUpAction(const PdfAction& action)
{
    this->addAlternativeAction("U"_n, action);
}

void PdfField::SetFocusEnterAction(const PdfAction& action)
{
    this->addAlternativeAction("Fo"_n, action);
}

void PdfField::SetFocusLeaveAction(const PdfAction& action)
{
    this->addAlternativeAction("BI"_n, action);
}

void PdfField::SetPageOpenAction(const PdfAction& action)
{
    this->addAlternativeAction("PO"_n, action);
}

void PdfField::SetPageCloseAction(const PdfAction& action)
{
    this->addAlternativeAction("PC"_n, action);
}

void PdfField::SetPageVisibleAction(const PdfAction& action)
{
    this->addAlternativeAction("PV"_n, action);
}

void PdfField::SetPageInvisibleAction(const PdfAction& action)
{
    this->addAlternativeAction("PI"_n, action);
}

void PdfField::SetKeystrokeAction(const PdfAction& action)
{
    this->addAlternativeAction("K"_n, action);
}

void PdfField::SetValidateAction(const PdfAction& action)
{
    this->addAlternativeAction("V"_n, action);
}

// Link field and parent creating /P key and adding the field to /Kids
void PdfField::linkFieldObjectToParent(const shared_ptr<PdfField>& field, PdfField& parentField,
    cspan<std::string_view> parentKeys, bool setParent, bool moveKeysToParent)
{
    auto& fieldDict = field->GetDictionary();
    if (moveKeysToParent)
    {
        auto& parentDict = parentField.GetDictionary();
        for (auto& pair : fieldDict)
        {
            auto keyName = pair.first.GetString();
            auto found = std::find(parentKeys.begin(), parentKeys.end(), keyName);
            if (found != parentKeys.end())
            {
                // Put the field key in the parent
                parentDict.AddKey(keyName, pair.second);
            }
        }
    }

    // Finally remove the parent keys only
    // CHECK-ME: If keyes were not moved to parent,
    // should we check theys actually exists in the parent
    // before moving them?
    for (auto& key : parentKeys)
        fieldDict.RemoveKey(key);

    parentField.GetChildren().AddChild(field);

    if (setParent)
    {
        // Set /Parent key to existing field
        field->SetParent(parentField.GetPtr());
        fieldDict.AddKey("Parent"_n, parentField.GetObject().GetIndirectReference());
    }
}

void getFullName(const PdfObject& obj, bool skipEscapePartialName, string& fullname)
{
    auto& dict = obj.GetDictionary();
    auto parent = dict.FindKey("Parent");
    if (parent != nullptr)
        getFullName(*parent, skipEscapePartialName, fullname);

    const PdfObject* nameObj = dict.GetKey("T");
    if (nameObj != nullptr)
    {
        string name = (string)nameObj->GetString().GetString();
        if (!skipEscapePartialName)
        {
            // According to ISO 32000-1:2008, "12.7.3.2 Field Names":
            // "Because the PERIOD is used as a separator for fully
            // qualified names, a partial name shall not contain a
            // PERIOD character."
            // In case the partial name still has periods (effectively
            // violating the standard and Pdf Reference) the fullname
            // would be unintelligible, let's escape them with double
            // dots "..", example "parent.partial..name"
            size_t currpos = 0;
            while ((currpos = name.find('.', currpos)) != std::string::npos)
            {
                name.replace(currpos, 1, ESCAPE_CHARACTER ".", 2);
                currpos += 2;
            }
        }

        if (fullname.length() == 0)
            fullname = name;
        else
            fullname.append(".").append(name);
    }
}
