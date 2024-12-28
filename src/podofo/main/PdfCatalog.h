/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_CATALOG
#define PDF_CATALOG

#include "PdfElement.h"

namespace PoDoFo
{
    class PODOFO_API PdfCatalog final : public PdfDictionaryElement
    {
        friend class PdfDocument;

    private:
        PdfCatalog(PdfObject& obj);

    public:
        /** Get access to the StructTreeRoot dictionary
         *  \returns PdfObject the StructTreeRoot dictionary
         */
        PdfObject* GetStructTreeRootObject();
        const PdfObject* GetStructTreeRootObject() const;

        /** Get access to the MarkInfo dictionary (ISO 32000-1:2008 14.7.1)
         *  \returns PdfObject the MarkInfo dictionary
         */
        PdfObject* GetMarkInfoObject();
        const PdfObject* GetMarkInfoObject() const;

        /** Get access to the RFC 3066 natural language id for the document (ISO 32000-1:2008 14.9.2.1)
         *  \returns PdfObject the language ID string
         */
        PdfObject* GetLangObject();
        const PdfObject* GetLangObject() const;

        /** Sets the opening mode for a document.
         *  \param mode which mode to set
         */
        void SetPageMode(nullable<PdfPageMode> mode);

        /** Gets the opening mode for a document.
         *  \returns which mode is set
         */
        PdfPageMode GetPageMode() const;

        /** Sets the opening mode for a document to be in full screen.
         */
        void SetUseFullScreen();

        /** Sets the page layout for a document.
         */
        void SetPageLayout(nullable<PdfPageLayout> layout);

        /** Set the document's Viewer Preferences:
         *  Hide the toolbar in the viewer.
         */
        void SetHideToolbar();

        /** Set the document's Viewer Preferences:
         *  Hide the menubar in the viewer.
         */
        void SetHideMenubar();

        /** Set the document's Viewer Preferences:
         *  Show only the documents contents and no control
         *  elements such as buttons and scrollbars in the viewer.
         */
        void SetHideWindowUI();

        /** Set the document's Viewer Preferences:
         *  Fit the document in the viewer's window.
         */
        void SetFitWindow();

        /** Set the document's Viewer Preferences:
         *  Center the document in the viewer's window.
         */
        void SetCenterWindow();

        /** Set the document's Viewer Preferences:
         *  Display the title from the document information
         *  in the title of the viewer.
         *
         *  \see SetTitle
         */
        void SetDisplayDocTitle();

        /** Set the document's Viewer Preferences:
         *  Set the default print scaling of the document.
         *
         *  TODO: DS use an enum here!
         */
        void SetPrintScaling(const PdfName& scalingType);

        /** Set the document's Viewer Preferences:
         *  Set the base URI of the document.
         *
         *  TODO: DS document value!
         */
        void SetBaseURI(const std::string_view& baseURI);

        /** Set the document's Viewer Preferences:
         *  Set the language of the document.
         */
        void SetLanguage(const std::string_view& language);

        /** Set the document's Viewer Preferences:
            Set the document's binding direction.
         */
        void SetBindingDirection(const PdfName& direction);

        /** Get access to the Metadata stream
         *  \returns PdfObject the Metadata stream (should be in XML, using XMP grammar)
         */
        PdfObject* GetMetadataObject();
        const PdfObject* GetMetadataObject() const;
        PdfObject& GetOrCreateMetadataObject();

        std::string GetMetadataStreamValue() const;
        void SetMetadataStreamValue(const std::string_view& value);

    private:
        /** Low-level APIs for setting a viewer preference.
         *  \param whichPref the dictionary key to set
         *  \param valueObj the object to be set
         */
        void setViewerPreference(const PdfName& whichPref, const PdfObject& valueObj);

        /** Low-level APIs for setting a viewer preference.
         *  Convenience overload.
         *  \param whichPref the dictionary key to set
         *  \param inValue the object to be set
         */
        void setViewerPreference(const PdfName& whichPref, bool inValue);
    };
}

#endif // PDF_CATALOG
