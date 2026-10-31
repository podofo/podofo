// SPDX-FileCopyrightText: 2022 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef PDF_FIELD_CHILDREN_COLLECTION_H
#define PDF_FIELD_CHILDREN_COLLECTION_H

#include "PdfObject.h"
#include <podofo/auxiliary/Rect.h>

namespace PoDoFo
{
    class PdfField;
    class PdfPage;

    class PODOFO_API PdfFieldChildrenCollectionBase
    {
        friend class PdfField;

    private:
        PdfFieldChildrenCollectionBase(PdfField& field);

    public:
        PdfField& CreateChild();
        PdfField& CreateChild(PdfPage& page, const Rect& rect);

        PdfField& GetFieldAt(unsigned index);

        const PdfField& GetFieldAt(unsigned index) const;

        PdfField& GetField(const PdfReference& ref);

        const PdfField& GetField(const PdfReference& ref) const;

        void RemoveFieldAt(unsigned index);

        void RemoveField(const PdfReference& ref);

        unsigned GetCount() const;

        bool HasKidsArray() const;

    public:
        using FieldList = std::vector<std::shared_ptr<PdfField>>;

        template <typename TField, typename TListIterator>
        class Iterator final
        {
            friend class PdfFieldChildrenCollectionBase;
        public:
            using difference_type = void;
            using value_type = TField*;
            using pointer = void;
            using reference = void;
            using iterator_category = std::forward_iterator_tag;
        public:
            Iterator() { }
        private:
            Iterator(const TListIterator& iterator, const TListIterator& end)
                : m_iterator(iterator), m_end(end)
            {
                skipPlaceholders();
            }
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
                skipPlaceholders();
                return *this;
            }
            Iterator operator++(int)
            {
                auto copy = *this;
                m_iterator++;
                skipPlaceholders();
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
            // Invalid fields are stored as null placeholders: the iteration skips them
            void skipPlaceholders()
            {
                while (m_iterator != m_end && *m_iterator == nullptr)
                    m_iterator++;
            }
        private:
            TListIterator m_iterator;
            TListIterator m_end;
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
        PdfField& AddChild(std::shared_ptr<PdfField> field);
        std::shared_ptr<PdfField> GetFieldPtr(const PdfReference& ref) const;
    private:
        PdfArray* getKidsArray() const;
        void inlineKidsArray();
        void initFields();
        void promoteDirectEntries();
        PdfField& getFieldAt(unsigned index) const;
        PdfField& getField(const PdfReference& ref) const;
        void fixIndices(unsigned index);

    private:
        using FieldMap = std::map<PdfReference, unsigned>;

    private:
        FieldList m_Fields;
        FieldMap m_fieldMap;
        PdfField* m_field;
        PdfArray* m_kidsArray;
    };

    // TODO
    template <typename TField>
    class PdfFieldChildrenCollection final : PdfFieldChildrenCollectionBase
    {

    };
}

#endif // PDF_FIELD_CHILDREN_COLLECTION_H
