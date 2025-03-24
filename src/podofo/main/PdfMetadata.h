/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_METADATA
#define PDF_METADATA

#include "PdfInfo.h"
#include "PdfXMPPacket.h"

namespace PoDoFo
{
    class PdfMetadataStore;
    class PdfDocument;

    class PODOFO_API PdfMetadata final
    {
        friend class PdfDocument;
        friend class PdfCatalog;

    private:
        PdfMetadata(PdfDocument& doc);
        PdfMetadata(PdfMetadata&) = delete;

    public:
        ~PdfMetadata();

        /** Set the title of the document.
         * \param title title
         */
        void SetTitle(nullable<const PdfString&> title);

        /** Get the title of the document
         *  \returns the title
         */
        nullable<const PdfString&> GetTitle() const;

        /** Set the author of the document.
         * \param author author
         */
        void SetAuthor(nullable<const PdfString&> author);

        /** Get the author of the document
         *  \returns the author
         */
        nullable<const PdfString&> GetAuthor() const;

        /** Set the subject of the document.
         * \param subject subject
         */
        void SetSubject(nullable<const PdfString&> subject);

        /** Get the subject of the document
         *  \returns the subject
         */
        nullable<const PdfString&> GetSubject() const;

        /** Get the raw keywords of the document
         *  \returns the subject
         */
        nullable<const PdfString&> GetKeywordsRaw() const;

        /** Set keywords for this document
         * \param keywords a list of keywords
         */
        void SetKeywords(std::vector<std::string> keywords);

        /** Get the keywords of the document
         *  \returns the keywords
         */
        std::vector<std::string> GetKeywords() const;

        /** Set the creator of the document.
         * Typically the name of the application using the library.
         * \param creator creator
         */
        void SetCreator(nullable<const PdfString&> creator);

        /** Get the creator of the document
         *  \returns the creator
         */
        nullable<const PdfString&> GetCreator() const;

        /** Set the producer of the document.
         * \param producer producer
         */
        void SetProducer(nullable<const PdfString&> producer);

        /** Get the producer of the document
         *  \returns the producer
         */
        nullable<const PdfString&> GetProducer() const;

        /** Set the document creation date
         * \param date the creation date
         */
        void SetCreationDate(nullable<PdfDate> date);

        /** Get creation date of document
         *  \return creation date
         */
        nullable<const PdfDate&> GetCreationDate() const;

        /** Set the document modification date
         * \param date the modification date
         */
        void SetModifyDate(nullable<PdfDate> date);

        /** Get modification date of document
         *  \return modification date
         */
        nullable<const PdfDate&> GetModifyDate() const;

        /** Set the trapping state of the document.
         * \param trapped trapped
         */
        void SetTrapped(nullable<bool> trapped);

        /** Get the trapping state of the document
         */
        nullable<bool>GetTrapped() const;

        /** Set the PDF Version of the document. Has to be called before Write() to
         *  have an effect.
         *  \param version  version of the pdf document
         */
        void SetPdfVersion(PdfVersion version);

        PdfVersion GetPdfVersion() const;

        PdfALevel GetPdfALevel() const;

        /** Set the document PDF/A level
         * \param level the PDF/A level
         */
        void SetPdfALevel(PdfALevel level);

        PdfUALevel GetPdfUALevel() const;

        void SetPdfUALevel(PdfUALevel version);

        nullable<const PdfString&> GetProperty(PdfAdditionalMetadata prop) const;

        void SetProperty(PdfAdditionalMetadata prop, nullable<const PdfString&> value);

        /** Ensure the XMP metadata is created
         * Also, ensure some /Info metadata is normalized
         * so it will be compatible with the correspondent XMP
         * \param resetXMPPacket true if the XMP packet should be reset unconditionally.
         *      This will loose custom entities
         */
        void SyncXMPMetadata(bool resetXMPPacket = false);

        /** Try to sync an XMP packet, if present
         * \returns true if the XMP packet was present and synced
         */
        bool TrySyncXMPMetadata();

        /** Take the XMP packet, if available, and invalidate the data
         */
        std::unique_ptr<PdfXMPPacket> TakeXMPPacket();

    private:
        void Invalidate();

    private:
        PdfMetadata& operator=(PdfMetadata&) = delete;

        void setKeywords(nullable<const PdfString&> keywords);
        void ensureInitialized();
        void syncXMPMetadata(bool resetXMPPacket);
        void invalidate();

    private:
        PdfDocument* m_doc;
        PdfMetadataStore* m_metadata;
        bool m_xmpSynced;
        std::unique_ptr<PdfXMPPacket> m_packet;
    };
}

#endif // PDF_METADATA
