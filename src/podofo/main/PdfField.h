/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FIELD_H
#define PDF_FIELD_H

#include "PdfDeclarations.h"

#include "PdfName.h"
#include "PdfString.h"
#include "PdfAnnotationWidget.h"
#include "PdfAction.h"
#include "PdfFieldChildrenCollection.h"

namespace PoDoFo {

class PdfPage;
class PdfAcroForm;
class PdfAnnotationWidget;
class PdfPushButton;
class PdfCheckBox;
class PdfRadioButton;
class PdfTextBox;
class PdfComboBox;
class PdfListBox;
class PdfSignature;

class PODOFO_API PdfField : public PdfDictionaryElement
{
    friend class PdfSignature;
    friend class PdfButton;
    friend class PdChoiceField;
    friend class PdfTextBox;
    friend class PdfPage;
    friend class PdfAcroForm;
    friend class PdfAnnotationWidget;
    friend class PdfFieldChildrenCollectionBase;

protected:
    enum
    {
        PdfButton_NoToggleOff = 0x0004000,
        PdfButton_Radio = 0x0008000,
        PdfButton_PushButton = 0x0010000,
        PdfButton_RadioInUnison = 0x2000000,
        PdfListField_Combo = 0x0020000,
        PdfListField_Edit = 0x0040000,
        PdfListField_Sort = 0x0080000,
        PdfListField_MultiSelect = 0x0200000,
        PdfListField_NoSpellcheck = 0x0400000,
        PdfListField_CommitOnSelChange = 0x4000000
    };

private:
    PdfField(PdfAcroForm& acroform, PdfFieldType fieldType,
        std::shared_ptr<PdfField>&& parent);

    PdfField(PdfAnnotationWidget& widget, PdfFieldType fieldType,
        std::shared_ptr<PdfField>&& parent);

    PdfField(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType);

public:

    /** Try create a field from an object, in the absence of an annotation
     */
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfField>& field);

    PdfField* GetParentSafe();
    const PdfField* GetParentSafe() const;

    /** Set the highlighting mode which should be used when the user
     *  presses the mouse button over this widget.
     *
     *  \param mode the highliting mode
     *
     *  The default value is PdfHighlightingMode::Invert
     */
    void SetHighlightingMode(PdfHighlightingMode mode);

    /**
     * \returns the highlighting mode to be used when the user
     *          presses the mouse button over this widget
     */
    PdfHighlightingMode GetHighlightingMode() const;

    /** Sets the field name of this PdfField
     *
     *  PdfFields require a field name to work correctly in acrobat reader!
     *  This name can be used to access the field in JavaScript actions.
     *
     *  \param name the field name of this pdf field
     */
    void SetName(nullable<const PdfString&> name);

    /** Returns signature object for this signature field.
     *  It can be nullptr, when the signature field was created
     *  from an existing annotation and it didn't have set it.
     *
     *  \returns associated signature object, or nullptr
     */
    PdfObject* GetValueObject();
    const PdfObject* GetValueObject() const;

    /** \returns the field name of this PdfField
     */
    nullable<const PdfString&> GetName() const;

    /** \returns the field name of this PdfField at this level of the hierarchy
     */
    nullable<const PdfString&> GetNameRaw() const;

    /** \returns the parents qualified name of this PdfField
     *
     *  \param skipEscapePartialName don't escape non compliant partial names that have dots with ESC character
     */
    std::string GetFullName(bool skipEscapePartialName = false) const;

    /**
     * Set the alternate name of this field which
     * is used to display the fields name to the user
     * (e.g. in error messages).
     *
     * \param name a name that can be displayed to the user
     */
    void SetAlternateName(nullable<const PdfString&> name);

    /** \returns the fields alternate name
     */
    nullable<const PdfString&> GetAlternateName() const;

    /**
     * Sets the fields mapping name which is used when exporting
     * the fields data
     *
     * \param name the mapping name of this PdfField
     */
    void SetMappingName(nullable<const PdfString&> name);

    /** \returns the mapping name of this field
     */
    nullable<const PdfString&> GetMappingName() const;

    /** Set this field to be readonly.
     *  I.e. it will not interact with the user
     *  and respond to mouse button events.
     *
     *  This is useful for fields that are pure calculated.
     *
     *  \param readOnly specifies if this field is read-only.
     */
    void SetReadOnly(bool readOnly);

    /**
     * \returns true if this field is read-only
     *
     * \see SetReadOnly
     */
    bool IsReadOnly() const;

    /** Required fields must have a value
     *  at the time the value is exported by a submit action
     *
     *  \param required if true this field requires a value for submit actions
     */
    void SetRequired(bool required);

    /**
     * \returns true if this field is required for submit actions
     *
     * \see SetRequired
     */
    bool IsRequired() const;

    /** Sets if this field can be exported by a submit action
     *
     *  Fields can be exported by default.
     *
     *  \param exprt if false this field cannot be exported by submit actions
     */
    void SetNoExport(bool exprt);

    /**
     * \returns true if this field can be exported by submit actions
     *
     * \see SetExport
     */
    bool IsNoExport() const;

    void SetMouseEnterAction(const PdfAction& action);
    void SetMouseLeaveAction(const PdfAction& action);
    void SetMouseDownAction(const PdfAction& action);
    void SetMouseUpAction(const PdfAction& action);

    void SetFocusEnterAction(const PdfAction& action);
    void SetFocusLeaveAction(const PdfAction& action);

    void SetPageOpenAction(const PdfAction& action);
    void SetPageCloseAction(const PdfAction& action);

    void SetPageVisibleAction(const PdfAction& action);
    void SetPageInvisibleAction(const PdfAction& action);

    void SetKeystrokeAction(const PdfAction& action);
    void SetValidateAction(const PdfAction& action);

public:
    PdfFieldType GetType() const { return m_FieldType; }

    PdfAnnotationWidget* GetWidget() { return m_Widget; }

    const PdfAnnotationWidget* GetWidget() const { return m_Widget; }

    PdfAnnotationWidget& MustGetWidget();

    const PdfAnnotationWidget& MustGetWidget() const;

    PdfFieldChildrenCollectionBase& GetChildren();

    const PdfFieldChildrenCollectionBase& GetChildren() const;

protected:
    /**
     *  Set a bit in the field flags value of the fields dictionary.
     *
     *  \param value the value specifying the bits to set
     *  \param set if true the value will be set otherwise
     *              they will be cleared.
     *
     *  \see GetFieldFlag
     */
    void SetFieldFlag(int64_t value, bool set);

    /**
     *  \param value it is checked if these bits are set
     *  \param defvalue the returned value if no field flags are specified
     *
     *  \returns true if given bits are set in the field flags
     *
     *  \see SetFieldFlag
     */
    bool GetFieldFlag(int64_t value, bool defvalue) const;

    /**
    *  \param obj the object to test for field flags
    *  \param value is set with the flag if found
    *  \returns true if flag is found
    */
    static bool GetFieldFlags(const PdfObject& obj, int64_t& value);

    virtual PdfObject* getValueObject() const;

    void AssertTerminalField() const;

    template <typename TField>
    TField* GetParentTyped(PdfFieldType type) const;

private:
    template <typename TField>
    static constexpr PdfFieldType GetFieldType();

    void SetWidget(PdfAnnotationWidget& widget) { m_Widget = &widget; }
    void SetAcroForm(PdfAcroForm& acroform) { m_AcroForm = &acroform; }

private:
    // To be called by PdfPage
    static PdfField& Create(const std::string_view& name,
        PdfAnnotationWidget& widget, PdfFieldType fieldType);

    // To be called by PdfAcroForm
    static std::unique_ptr<PdfField> Create(const std::string_view& name,
        PdfAcroForm& acroform, PdfFieldType fieldType);
    static std::unique_ptr<PdfField> Create(PdfObject& obj, PdfAcroForm& acroform,
        PdfFieldType fieldType);

    // To be called by PdfFieldChildrenCollectionBase
    std::unique_ptr<PdfField> CreateChild();
    std::unique_ptr<PdfField> CreateChild(PdfPage& page, const Rect& rect);
    void SetParent(std::shared_ptr<PdfField>&& parent);

private:
    PdfField(const PdfField& rhs) = delete;
    PdfField& operator=(const PdfField& rhs) = delete;

private:
    static std::unique_ptr<PdfField> createField(PdfAnnotationWidget& widget,
        PdfFieldType type, std::shared_ptr<PdfField>&& parent, bool insertInAcroform);

    static std::unique_ptr<PdfField> createField(PdfAcroForm& acroform, PdfFieldType type,
        std::shared_ptr<PdfField>&& parent);

    static void linkFieldObjectToParent(const std::shared_ptr<PdfField>& field, PdfField& parentField,
        const std::vector<std::string>& parentKeys, bool setParent, bool moveKeysToParent);

    void init();
    void initParent();
    void initChildren();
    void setName(const PdfString& name);
    void addAlternativeAction(const PdfName& name, const PdfAction& action);
    static bool tryCreateField(PdfObject& obj, PdfFieldType type,
        std::unique_ptr<PdfField>& field);
    std::unique_ptr<PdfField> createChildField(PdfPage* page, const Rect& rect);
    static PdfFieldType getFieldType(const PdfObject& obj);
    std::shared_ptr<PdfField> GetPtr();
    PdfField* getParentTyped(PdfFieldType type) const;

private:
    PdfAnnotationWidget* m_Widget;
    PdfAcroForm* m_AcroForm;
    PdfFieldType m_FieldType;
    nullable<std::shared_ptr<PdfField>> m_Parent;
    std::unique_ptr<PdfFieldChildrenCollectionBase> m_Children;
};

template<typename TField>
TField* PdfField::GetParentTyped(PdfFieldType type) const
{
    return static_cast<TField*>(getParentTyped(type));
}

template<typename TField>
constexpr PdfFieldType PdfField::GetFieldType()
{
    if (std::is_same_v<TField, PdfPushButton>)
        return PdfFieldType::PushButton;
    else if (std::is_same_v<TField, PdfCheckBox>)
        return PdfFieldType::CheckBox;
    else if (std::is_same_v<TField, PdfRadioButton>)
        return PdfFieldType::RadioButton;
    else if (std::is_same_v<TField, PdfTextBox>)
        return PdfFieldType::TextBox;
    else if (std::is_same_v<TField, PdfComboBox>)
        return PdfFieldType::ComboBox;
    else if (std::is_same_v<TField, PdfListBox>)
        return PdfFieldType::ListBox;
    else if (std::is_same_v<TField, PdfSignature>)
        return PdfFieldType::Signature;
    else
        return PdfFieldType::Unknown;
}

};

#endif // PDF_FIELD_H
