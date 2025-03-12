/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_ACRO_FORM_H
#define PDF_ACRO_FORM_H

#include "PdfField.h"

namespace PoDoFo {

class PdfDocument;

enum class PdfAcroFormDefaulAppearance : uint8_t
{
    None = 0, ///< Do not add a default appearrance
    ArialBlack ///< Add a default appearance with Arial embedded and black text if no other DA key is present
};

enum class PdfAcroFormSigFlags
{
    None = 0,
    SignaturesExist = 1,
    AppendOnly = 2,
};

class PODOFO_API PdfAcroForm final : public PdfDictionaryElement
{
    friend class PdfField;
    friend class PdfDocument;
    friend class PdfSigningContext;
    friend class PdfSignature;

private:
    /** Create a new PdfAcroForm dictionary object
     *  \param doc parent of this action
     *  \param defaultAppearance specifies if a default appearance should be added
     */
    PdfAcroForm(PdfDocument & doc,
                 PdfAcroFormDefaulAppearance defaultAppearance = PdfAcroFormDefaulAppearance::ArialBlack);

    /** Create a PdfAcroForm dictionary object from an existing PdfObject
     *	\param obj the object to create from
     */
    PdfAcroForm(PdfObject& obj);

public:
    /** Set the value of the NeedAppearances key in the interactive forms
     *  dictionary.
     *
     *  \param bNeedAppearances A flag specifying whether to construct appearance streams
     *                          and appearance dictionaries for all widget annotations in
     *                          the document. Default value is false.
     */
    void SetNeedAppearances(bool needAppearances);

    /** Retrieve the value of the NeedAppearances key in the interactive forms
     *  dictionary.
     *
     *  \returns value of the NeedAppearances key
     *
     *  \see SetNeedAppearances
     */
    bool GetNeedAppearances() const;

    /** Get the value of the /SigFlags document-level characteristics related to signature fields
     */
    PdfAcroFormSigFlags GetSigFlags() const;

    template <typename TField>
    TField& CreateField(const std::string_view& name);

    PdfField& CreateField(const std::string_view& name, PdfFieldType fieldType);

    /** Get the field with index index of the form.
     *  \param index the index of the field to retrieve
     *
     *  \returns a field object. The field object is owned by the PdfAcroForm.
     *
     *  \see GetAnnotationCount
     */
    PdfField& GetFieldAt(unsigned index);

    const PdfField& GetFieldAt(unsigned index) const;

    PdfField& GetField(const PdfReference& ref);

    const PdfField& GetField(const PdfReference& ref) const;

    /** Delete the field with index index from this page.
     *  \param index the index of the field to delete
     */
    void RemoveFieldAt(unsigned index);

    /** Delete the field with the given object reference
     *  \param ref the object reference
     */
    void RemoveField(const PdfReference& ref);

    unsigned GetFieldCount() const;

public:
    using FieldList = std::vector<std::shared_ptr<PdfField>>;

    template <typename TObject, typename TListIterator>
    class Iterator final
    {
        friend class PdfAcroForm;
    public:
        using difference_type = void;
        using value_type = TObject*;
        using pointer = void;
        using reference = void;
        using iterator_category = std::forward_iterator_tag;
    public:
        Iterator() { }
    private:
        Iterator(const TListIterator& iterator) : m_iterator(iterator) { }
    public:
        Iterator(const Iterator&) = default;
        Iterator& operator=(const Iterator&) = default;
        bool operator==(const Iterator& rhs) const
        {
            return m_iterator == rhs.m_iterator;
        }
        bool operator!=(const Iterator& rhs) const
        {
            return m_iterator != rhs.m_iterator;
        }
        Iterator& operator++()
        {
            m_iterator++;
            return *this;
        }
        Iterator operator++(int)
        {
            auto copy = *this;
            m_iterator++;
            return copy;
        }
        value_type operator*()
        {
            return (*m_iterator).get();
        }
        value_type operator->()
        {
            return (*m_iterator).get();
        }
    private:
        TListIterator m_iterator;
    };

    using iterator = Iterator<PdfField, FieldList::iterator>;
    using const_iterator = Iterator<const PdfField, FieldList::const_iterator>;

public:
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

private:
    // To be called by PdfField
    PdfField& CreateField(PdfObject& obj, PdfFieldType type);
    PdfField& AddField(std::unique_ptr<PdfField>&& field);
    std::shared_ptr<PdfField> GetFieldPtr(const PdfReference& ref);

    // To be called by PdfSignature/PdfSigningContext
    void SetSigFlags(PdfAcroFormSigFlags flags);

private:
    /** Initialize this object
     *  with a default appearance
     *  \param defaultAppearance specifies if a default appearance should be added
     */
    void init(PdfAcroFormDefaulAppearance defaultAppearance);

    PdfArray* getFieldArray() const;

    void initFields();

    PdfField& getField(unsigned index) const;
    PdfField& getField(const PdfReference& ref) const;

    void fixIndices(unsigned index);

private:
    using FieldMap = std::map<PdfReference, unsigned>;

private:
    FieldList m_Fields;
    std::unique_ptr<FieldMap> m_fieldMap;
    PdfArray* m_fieldArray;
};

template<typename TField>
TField& PdfAcroForm::CreateField(const std::string_view& name)
{
    return static_cast<TField&>(CreateField(name, PdfField::GetFieldType<TField>()));
}

};

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfAcroFormSigFlags);

#endif // PDF_ACRO_FORM_H
