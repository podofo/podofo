/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_CHOICE_FIELD_H
#define PDF_CHOICE_FIELD_H

#include "PdfField.h"

namespace PoDoFo
{
    // TODO: Multiselect
    /** A list of items in a PDF file.
     *  You cannot create this object directly, use
     *  PdfComboBox or PdfListBox instead.
     *
     *  \see PdfComboBox
     *  \see PdfListBox
     */
    class PODOFO_API PdChoiceField : public PdfField
    {
        friend class PdfListBox;
        friend class PdfComboBox;

    private:
        PdChoiceField(PdfAcroForm& acroform, PdfFieldType fieldType,
            std::shared_ptr<PdfField>&& parent);

        PdChoiceField(PdfAnnotationWidget& widget, PdfFieldType fieldType,
            std::shared_ptr<PdfField>&& parent);

        PdChoiceField(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType);

    public:
        /**
         * Inserts a new item into the list
         *
         * \param value the value of the item
         * \param displayName an optional display string that is displayed in the viewer
         *                      instead of the value
         */
        void InsertItem(const PdfString& value, nullable<const PdfString&> displayName = { });

        /**
         * Removes an item for the list
         *
         * \param index index of the item to remove
         */
        void RemoveItem(unsigned index);

        /**
         * \param index index of the item
         * \returns the value of the item at the specified index
         */
        PdfString GetItem(unsigned index) const;

        /**
         * \param index index of the item
         * \returns the display text of the item or if it has no display text
         *          its value is returned. This call is equivalent to GetItem()
         *          in this case
         *
         * \see GetItem
         */
        nullable<const PdfString&> GetItemDisplayText(int index) const;

        /**
         * \returns the number of items in this list
         */
        unsigned GetItemCount() const;

        /** Sets the currently selected item
         *  \param index index of the currently selected item
         */
        void SetSelectedIndex(int index);

        /** Sets the currently selected item
         *
         *  \returns the selected item or -1 if no item was selected
         */
        int GetSelectedIndex() const;

        /**
         * \returns true if this PdChoiceField is a PdfComboBox and false
         *               if it is a PdfListBox
         */
        bool IsComboBox() const;

        /**
         *  Enable/disable spellchecking for this combobox
         *
         *  \param spellCheck if true spellchecking will be enabled
         *
         *  combobox are spellchecked by default
         */
        void SetSpellCheckingEnabled(bool spellCheck);

        /**
         *  \returns true if spellchecking is enabled for this combobox
         */
        bool IsSpellCheckingEnabled() const;

        /**
         * Enable or disable sorting of items.
         * The sorting does not happen in acrobat reader
         * but whenever adding items using PoDoFo or another
         * PDF editing application.
         *
         * \param sorted enable/disable sorting
         */
        void SetSorted(bool sorted);

        /**
         * \returns true if sorting is enabled
         */
        bool IsSorted() const;

        /**
         * Sets whether multiple items can be selected by the
         * user in the list.
         *
         * \param multi if true multiselect will be enabled
         *
         * By default multiselection is turned off.
         */
        void SetMultiSelect(bool multi);

        /**
         * \returns true if multi selection is enabled
         *               for this list
         */
        bool IsMultiSelect() const;

        void SetCommitOnSelectionChange(bool commit);
        bool IsCommitOnSelectionChange() const;
    };
}

#endif // PDF_CHOICE_FIELD_H
