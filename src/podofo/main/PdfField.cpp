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

#define CHECK_FIELD_NAME(name) if (name.find('.') != string::npos)\
    throw runtime_error("Unsupported dot \".\" in field name. Use PdfField.CreateChild()");

using namespace std;
using namespace PoDoFo;

void getFullName(const PdfObject& obj, bool escapePartialNames, string& fullname);

PdfField::PdfField(PdfAnnotationWidget& widget,
        PdfFieldType fieldType, const shared_ptr<PdfField>& parent) :
    PdfDictionaryElement(widget.GetObject()),
    m_Widget(&widget),
    m_AcroForm(nullptr),
    m_FieldType(fieldType),
    m_Parent(parent),
    m_Children(*this)
{
    if (parent == nullptr)
    {
        init();
    }
    else
    {
        // Set /Parent key to newly created field
        GetDictionary().AddKey("Parent", parent->GetObject().GetIndirectReference());
    }
}

PdfField::PdfField(PdfAcroForm& acroform, PdfFieldType fieldType,
        const shared_ptr<PdfField>& parent) :
    PdfDictionaryElement(acroform.GetDocument()),
    m_Widget(nullptr),
    m_AcroForm(&acroform),
    m_FieldType(fieldType),
    m_Parent(parent),
    m_Children(*this)
{
    if (parent == nullptr)
    {
        init();
    }
    else
    {
        // Set /Parent key to newly created field
        GetDictionary().AddKey("Parent", parent->GetObject().GetIndirectReference());
    }
}

// NOTE: This constructor does not need to perform initialization
PdfField::PdfField(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType) :
    PdfDictionaryElement(obj),
    m_Widget(nullptr),
    m_AcroForm(acroform),
    m_FieldType(fieldType),
    m_Children(*this)
{
}

bool PdfField::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfField>& field)
{
    return tryCreateField(obj, getFieldType(obj), field);
}

unique_ptr<PdfField> PdfField::CreateChild()
{
    return createChildField(nullptr, PdfRect());
}

unique_ptr<PdfField> PdfField::CreateChild(PdfPage& page, const PdfRect& rect)
{
    return createChildField(&page, rect);
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

unique_ptr<PdfField> PdfField::createChildField(PdfPage* page, const PdfRect& rect)
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
    PdfAnnotationWidget& widget, const type_info& typeInfo)
{
    return Create(name, widget, getFieldType(typeInfo));
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
                    throw runtime_error("Found field with same name and different type");

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
    }
    else
    {
        // Prepare keys to remove that will stay on the parent
        const vector<string> parentKeys{ "FT", "Ff", "T", "V" };
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

    widget.SetField(newField);
    return *newField;
}

unique_ptr<PdfField> PdfField::Create(const string_view& name,
    PdfAcroForm& acroform, const type_info& typeInfo)
{
    return Create(name, acroform, getFieldType(typeInfo));
}

unique_ptr<PdfField> PdfField::Create(const string_view& name,
    PdfAcroForm& acroform, PdfFieldType type)
{
    CHECK_FIELD_NAME(name);
    auto ret = createField(acroform, type, nullptr);
    ret->setName(name);
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
    const shared_ptr<PdfField>& parent)
{
    switch (type)
    {
        case PdfFieldType::PushButton:
            return unique_ptr<PdfField>(new PdfPushButton(acroform, parent));
        case PdfFieldType::CheckBox:
            return unique_ptr<PdfField>(new PdfCheckBox(acroform, parent));
        case PdfFieldType::RadioButton:
            return unique_ptr<PdfField>(new PdfRadioButton(acroform, parent));
        case PdfFieldType::TextBox:
            return unique_ptr<PdfField>(new PdfTextBox(acroform, parent));
        case PdfFieldType::ComboBox:
            return unique_ptr<PdfField>(new PdfComboBox(acroform, parent));
        case PdfFieldType::ListBox:
            return unique_ptr<PdfField>(new PdfListBox(acroform, parent));
        case PdfFieldType::Signature:
            return unique_ptr<PdfField>(new PdfSignature(acroform, parent));
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }
}

unique_ptr<PdfField> PdfField::createField(PdfAnnotationWidget& widget,
    PdfFieldType type, const shared_ptr<PdfField>& parent, bool insertInAcroform)
{
    unique_ptr<PdfField> ret;
    switch (type)
    {
        case PdfFieldType::PushButton:
            ret.reset(new PdfPushButton(widget, parent));
            break;
        case PdfFieldType::CheckBox:
            ret.reset(new PdfCheckBox(widget, parent));
            break;
        case PdfFieldType::RadioButton:
            ret.reset(new PdfRadioButton(widget, parent));
            break;
        case PdfFieldType::TextBox:
            ret.reset(new PdfTextBox(widget, parent));
            break;
        case PdfFieldType::ComboBox:
            ret.reset(new PdfComboBox(widget, parent));
            break;
        case PdfFieldType::ListBox:
            ret.reset(new PdfListBox(widget, parent));
            break;
        case PdfFieldType::Signature:
            ret.reset(new PdfSignature(widget, parent));
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }

    if (insertInAcroform)
        (void)widget.GetDocument().GetOrCreateAcroForm().CreateField(ret->GetObject(), ret->GetType());

    return ret;
}

PdfFieldType PdfField::getFieldType(const type_info& typeInfo)
{
    if (typeInfo == typeid(PdfPushButton))
        return PdfFieldType::PushButton;
    else if (typeInfo == typeid(PdfCheckBox))
        return PdfFieldType::CheckBox;
    else if (typeInfo == typeid(PdfRadioButton))
        return PdfFieldType::RadioButton;
    else if (typeInfo == typeid(PdfTextBox))
        return PdfFieldType::TextBox;
    else if (typeInfo == typeid(PdfComboBox))
        return PdfFieldType::ComboBox;
    else if (typeInfo == typeid(PdfListBox))
        return PdfFieldType::ListBox;
    else if (typeInfo == typeid(PdfSignature))
        return PdfFieldType::Signature;
    else
        PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
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

        if ((flags & PdfButton::ePdfButton_PushButton) == PdfButton::ePdfButton_PushButton)
            ret = PdfFieldType::PushButton;
        else if ((flags & PdfButton::ePdfButton_Radio) == PdfButton::ePdfButton_Radio)
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

        if ((flags & PdChoiceField::ePdfListField_Combo) == PdChoiceField::ePdfListField_Combo)
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
            dict.AddKey("FT", PdfName("Btn"));
            break;
        case PdfFieldType::PushButton:
            dict.AddKey("FT", PdfName("Btn"));
            dict.AddKey("Ff", (int64_t)PdfButton::ePdfButton_PushButton);
            break;
        case PdfFieldType::RadioButton:
            dict.AddKey("FT", PdfName("Btn"));
            dict.AddKey("Ff", (int64_t)(PdfButton::ePdfButton_Radio | PdfButton::ePdfButton_NoToggleOff));
            break;
        case PdfFieldType::TextBox:
            dict.AddKey("FT", PdfName("Tx"));
            break;
        case PdfFieldType::ListBox:
            dict.AddKey("FT", PdfName("Ch"));
            break;
        case PdfFieldType::ComboBox:
            dict.AddKey("FT", PdfName("Ch"));
            dict.AddKey("Ff", (int64_t)PdChoiceField::ePdfListField_Combo);
            break;
        case PdfFieldType::Signature:
            dict.AddKey("FT", PdfName("Sig"));
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

    GetDictionary().AddKey("Ff", curr);
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
            value = "N";
            break;
        case PdfHighlightingMode::Invert:
            value = "I";
            break;
        case PdfHighlightingMode::InvertOutline:
            value = "O";
            break;
        case PdfHighlightingMode::Push:
            value = "P";
            break;
        case PdfHighlightingMode::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidName);
            break;
    }

    GetDictionary().AddKey("H", value);
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
    GetDictionary().AddKey("T", name);
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

string PdfField::GetFullName(bool escapePartialNames) const
{
    string fullName;
    getFullName(GetObject(), escapePartialNames, fullName);
    return fullName;
}

void PdfField::SetAlternateName(nullable<const PdfString&> name)
{
    if (name.has_value())
        GetDictionary().AddKey("TU", *name);
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
        GetDictionary().AddKey("TM", *name);
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
        aaObj = &GetDictionary().AddKey("AA", PdfDictionary());

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
    this->addAlternativeAction("E", action);
}

void PdfField::SetMouseLeaveAction(const PdfAction& action)
{
    this->addAlternativeAction("X", action);
}

void PdfField::SetMouseDownAction(const PdfAction& action)
{
    this->addAlternativeAction("D", action);
}

void PdfField::SetMouseUpAction(const PdfAction& action)
{
    this->addAlternativeAction("U", action);
}

void PdfField::SetFocusEnterAction(const PdfAction& action)
{
    this->addAlternativeAction("Fo", action);
}

void PdfField::SetFocusLeaveAction(const PdfAction& action)
{
    this->addAlternativeAction("BI", action);
}

void PdfField::SetPageOpenAction(const PdfAction& action)
{
    this->addAlternativeAction("PO", action);
}

void PdfField::SetPageCloseAction(const PdfAction& action)
{
    this->addAlternativeAction("PC", action);
}

void PdfField::SetPageVisibleAction(const PdfAction& action)
{
    this->addAlternativeAction("PV", action);
}

void PdfField::SetPageInvisibleAction(const PdfAction& action)
{
    this->addAlternativeAction("PI", action);
}

void PdfField::SetKeystrokeAction(const PdfAction& action)
{
    this->addAlternativeAction("K", action);
}

void PdfField::SetValidateAction(const PdfAction& action)
{
    this->addAlternativeAction("V", action);
}

// Link field and parent creating /P key and adding the field to /Kids
void PdfField::linkFieldObjectToParent(const shared_ptr<PdfField>& field, PdfField& parentField,
    const vector<string>& parentKeys, bool setParent, bool moveKeysToParent)
{
    auto& fieldDict = field->GetObject().GetDictionary();
    if (moveKeysToParent)
    {
        auto& parentDict = parentField.GetObject().GetDictionary();
        for (auto& pair : fieldDict)
        {
            string keyName = pair.first.GetString();
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
        fieldDict.AddKey("Parent", parentField.GetObject().GetIndirectReference());
    }
}

void getFullName(const PdfObject& obj, bool escapePartialNames, string& fullname)
{
    auto& dict = obj.GetDictionary();
    auto parent = dict.FindKey("Parent");
    if (parent != nullptr)
        getFullName(*parent, escapePartialNames, fullname);

    const PdfObject* nameObj = dict.GetKey("T");
    if (nameObj != nullptr)
    {
        string name = nameObj->GetString().GetString();
        if (escapePartialNames)
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
                name.replace(currpos, 1, "..", 2);
                currpos += 2;
            }
        }

        if (fullname.length() == 0)
            fullname = name;
        else
            fullname.append(".").append(name);
    }
}
