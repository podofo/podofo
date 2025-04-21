/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_TEXTBOX_H
#define PDF_TEXTBOX_H

#include "PdfField.h"

namespace PoDoFo
{
    /** A text field in a PDF file.
     *
     *  Users can enter text into a text field.
     *  Single and multi line text is possible,
     *  as well as richtext. The text can be interpreted
     *  as path to a file which is going to be submitted.
     */
    class PODOFO_API PdfTextBox final : public PdfField
    {
        friend class PdfField;
    private:
        enum
        {
            PdfTextBox_MultiLine = 0x0001000,
            PdfTextBox_Password = 0x0002000,
            PdfTextBox_FileSelect = 0x0100000,
            PdfTextBox_NoSpellcheck = 0x0400000,
            PdfTextBox_NoScroll = 0x0800000,
            PdfTextBox_Comb = 0x1000000,
            PdfTextBox_RichText = 0x2000000
        };

    private:
        PdfTextBox(PdfAcroForm& acroform, std::shared_ptr<PdfField>&& parent);

        PdfTextBox(PdfAnnotationWidget& widget, std::shared_ptr<PdfField>&& parent);

        PdfTextBox(PdfObject& obj, PdfAcroForm* acroform);

    public:
        /** Sets the text contents of this text field.
         *
         *  \param text the text of this field
         */
        void SetText(nullable<const PdfString&> text);

        /**
         *  \returns the text contents of this text field
         */
        nullable<const PdfString&> GetText() const;

        /** Sets the max length in characters of this textfield
         *  \param maxLen the max length of this textfields in characters
         */
        void SetMaxLen(int64_t maxLen);

        /**
         * \returns the max length of this textfield in characters or -1
         *          if no max length was specified
         */
        int64_t GetMaxLen() const;

        /**
         *  Create a multi-line text field that can contains multiple lines of text.
         *  \param multiLine if true a multi line field is generated, otherwise
         *                    the text field can contain only a single line of text.
         *
         *  The default is to create a single line text field.
         */
        void SetMultiLine(bool multiLine);

        /**
         * \returns true if this text field can contain multiple lines of text
         */
        bool IsMultiLine() const;

        /**
         *  Create a password text field that should not echo entered
         *  characters visibly to the screen.
         *
         *  \param password if true a password field is created
         *
         *  The default is to create no password field
         */
        void SetPasswordField(bool password);

        /**
         * \returns true if this field is a password field that does
         *               not echo entered characters on the screen
         */
        bool IsPasswordField() const;

        /**
         *  Create a file selection field.
         *  The entered contents are treated as filename to a file
         *  whose contents are submitted as the value of the field.
         *
         *  \param file if true the contents are treated as a pathname
         *               to a file to submit
         */
        void SetFileField(bool file);

        /**
         * \returns true if the contents are treated as filename
         */
        bool IsFileField() const;

        /**
         *  Enable/disable spellchecking for this text field
         *
         *  \param spellCheck if true spellchecking will be enabled
         *
         *  Text fields are spellchecked by default
         */
        void SetSpellCheckingEnabled(bool spellcheck);

        /**
         *  \returns true if spellchecking is enabled for this text field
         */
        bool IsSpellCheckingEnabled() const;

        /**
         *  Enable/disable scrollbars for this text field
         *
         *  \param scroll if true scrollbars will be enabled
         *
         *  Text fields have scrollbars by default
         */
        void SetScrollBarsEnabled(bool scroll);

        /**
         *  \returns true if scrollbars are enabled for this text field
         */
        bool IsScrollBarsEnabled() const;

        /**
         *  Divide the text field into max-len equal
         *  combs.
         *
         *  \param combs if true enable division into combs
         *
         *  By default combs are disabled. Requires the max-len
         *  property to be set.
         *
         *  \see SetMaxLen
         */
        void SetCombs(bool combs);

        /**
         * \returns true if the text field has a division into equal combs set on it
         */
        bool IsCombs() const;

        /**
         * Creates a richtext field.
         *
         * \param richText if true creates a richtext field
         *
         * By default richtext is disabled.
         */
        void SetRichText(bool richText);

        /**
         * \returns true if this is a richtext text field
         */
        bool IsRichText() const;

        PdfTextBox* GetParent();
        const PdfTextBox* GetParent() const;

    private:
        void init();
    };
}

#endif // PDF_TEXTBOX_H
