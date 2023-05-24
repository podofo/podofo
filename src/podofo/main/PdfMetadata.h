/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_METADATA
#define PDF_METADATA

#include "PdfInfo.h"
#include "PdfXMPMetadata.h"
#include "PdfXMPPacket.h"

namespace PoDoFo
{
    class PdfDocument;

    class PODOFO_API PdfMetadata final
    {
        friend class PdfDocument;
        friend class PdfCatalog;

    private:
        PdfMetadata(PdfDocument& doc);
        PdfMetadata(PdfMetadata&) = delete;

    public:
        /** Set the title of the document.
         * \param title title
         * \param trySyncXMP if a XMP packet was found, immediately synchronize it to "/Metadata"
         */
        void SetTitle(nullable<const PdfString&> title, bool trySyncXMP = false);

        /** Get the title of the document
         *  \returns the title
         */
        const nullable<PdfString>& GetTitle() const;

        /** Set the author of the document.
         * \param author author
         * \param trySyncXMP if a XMP packet was found, immediately synchronize it to "/Metadata"
         */
        void SetAuthor(nullable<const PdfString&> author, bool trySyncXMP = false);

        /** Get the author of the document
         *  \returns the author
         */
        const nullable<PdfString>& GetAuthor() const;

        /** Set the subject of the document.
         * \param subject subject
         * \param trySyncXMP if a XMP packet was found, immediately synchronize it to "/Metadata"
         */
        void SetSubject(nullable<const PdfString&> subject, bool trySyncXMP = false);

        /** Get the subject of the document
         *  \returns the subject
         */
        const nullable<PdfString>& GetSubject() const;

        /** Get the raw keywords of the document
         *  \returns the subject
         */
        const nullable<PdfString>& GetKeywordsRaw() const;

        /** Set keywords for this document
         * \param keywords a list of keywords
         * \param trySyncXMP if a XMP packet was found, immediately synchronize it to "/Metadata"
         */
        void SetKeywords(std::vector<std::string> keywords, bool trySyncXMP = false);

        /** Get the keywords of the document
         *  \returns the keywords
         */
        std::vector<std::string> GetKeywords() const;

        /** Set the creator of the document.
         * Typically the name of the application using the library.
         * \param creator creator
         * \param trySyncXMP if a XMP packet was found, immediately synchronize it to "/Metadata"
         */
        void SetCreator(nullable<const PdfString&> creator, bool trySyncXMP = false);

        /** Get the creator of the document
         *  \returns the creator
         */
        const nullable<PdfString>& GetCreator() const;

        /** Set the producer of the document.
         * \param producer producer
         * \param trySyncXMP if a XMP packet was found, immediately synchronize it to "/Metadata"
         */
        void SetProducer(nullable<const PdfString&> producer, bool trySyncXMP = false);

        /** Get the producer of the document
         *  \returns the producer
         */
        const nullable<PdfString>& GetProducer() const;

        /** Set the document creation date
         * \param date the creation date
         * \param trySyncXMP if a XMP packet was found, immediately synchronize it to "/Metadata"
         */
        void SetCreationDate(nullable<PdfDate> date, bool trySyncXMP = false);

        /** Get creation date of document
         *  \return creation date
         */
        const nullable<PdfDate>& GetCreationDate() const;

        /** Set the document modification date
         * \param date the modification date
         * \param trySyncXMP if a XMP packet was found, immediately synchronize it to "/Metadata"
         */
        void SetModifyDate(nullable<PdfDate> date, bool trySyncXMP = false);

        /** Get modification date of document
         *  \return modification date
         */
        const nullable<PdfDate>& GetModifyDate() const;

        /** Set the trapping state of the document.
         * \param trapped trapped
         * \param trySyncXMP if a XMP packet was found, immediately synchronize it to "/Metadata"
         */
        void SetTrapped(nullable<const PdfName&> trapped);

        /** Get the trapping state of the document
         *  \returns the title
         */
        std::string GetTrapped() const;

        nullable<const PdfName&> GetTrappedRaw() const;

        /** Set the PDF Version of the document. Has to be called before Write() to
         *  have an effect.
         *  \param version  version of the pdf document
         */
        void SetPdfVersion(PdfVersion version);

        PdfVersion GetPdfVersion() const;

        PdfALevel GetPdfALevel() const;

        /** Set the document PDF/A level
         * \param level the PDF/A level
         * \param trySyncXMP if a XMP packet was found, immediately synchronize it to "/Metadata"
         */
        void SetPdfALevel(PdfALevel level, bool trySyncXMP = false);

        /** Ensure the XMP metadata is created
         * Also, ensure some /Info metadata is normalized
         * so it will be compatible with the corrispondent XMP
         */
        void EnsureXMPMetadata();

        void SyncXMPMetadata(bool forceCreationXMP);

        /** Take the XMP packet, if available, and invalidate the data
         */
        std::unique_ptr<PdfXMPPacket> TakeXMPPacket();

    private:
        void Invalidate();

    private:
        PdfMetadata& operator=(PdfMetadata&) = delete;

        void setKeywords(nullable<const PdfString&> keywords, bool trySyncXMP = false);
        void ensureInitialized();
        void trySyncXMPMetadata(bool forceCreationXMP);
        void invalidate();

    private:
        PdfDocument* m_doc;
        PdfXMPMetadata m_metadata;
        bool m_initialized;
        bool m_xmpSynced;
        std::unique_ptr<PdfXMPPacket> m_packet;
    };
}

#endif // PDF_METADATA
