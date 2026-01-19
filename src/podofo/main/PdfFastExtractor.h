/**
 * SPDX-FileCopyrightText: (C) 2026 PoDoFo Project
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FAST_EXTRACTOR_H
#define PDF_FAST_EXTRACTOR_H

#include <podofo/main/PdfDeclarations.h>
#include <podofo/main/PdfFastExtractOptions.h>
#include <podofo/main/PdfTextExtract.h>
#include <podofo/main/PdfImage.h>

namespace PoDoFo {

// Forward declarations
class PdfParser;
class PdfMemDocument;
class InputStreamDevice;

/**
 * Callback for text extraction results
 */
using PdfTextExtractCallback = std::function<void(int pageNum, const std::vector<PdfTextEntry>&)>;

/**
 * Callback for image extraction results
 */
using PdfImageExtractCallback = std::function<void(int pageNum, const PdfImageInfo&, const charbuff&)>;

/**
 * Fast extractor for large PDF files
 *
 * This class provides optimized text and image extraction for large PDF files
 * (100MB+). It uses streaming and selective loading to minimize memory usage
 * and improve performance.
 */
class PODOFO_API PdfFastExtractor final
{
public:
    /**
     * Construct a fast extractor from a file path
     *
     * @param filepath Path to the PDF file
     */
    PdfFastExtractor(const std::string_view& filepath);

    /**
     * Construct a fast extractor from an input device
     *
     * @param device Input device containing PDF data
     */
    PdfFastExtractor(std::shared_ptr<InputStreamDevice> device);

    /**
     * Destructor
     */
    ~PdfFastExtractor();

    // Disable copy and assignment
    PdfFastExtractor(const PdfFastExtractor&) = delete;
    PdfFastExtractor& operator=(const PdfFastExtractor&) = delete;

    // Enable move semantics
    PdfFastExtractor(PdfFastExtractor&&) noexcept;
    PdfFastExtractor& operator=(PdfFastExtractor&&) noexcept;

    /**
     * Set extraction options
     *
     * @param options Extraction options
     */
    void SetOptions(const PdfFastExtractOptions& options);

    /**
     * Get current extraction options
     *
     * @return Current options
     */
    const PdfFastExtractOptions& GetOptions() const;

    /**
     * Extract text from all pages
     *
     * @param callback Callback for text extraction results
     * @throws PdfError on extraction failure
     */
    void ExtractText(PdfTextExtractCallback callback);

    /**
     * Extract text from a specific page
     *
     * @param pageNum Page number (0-based)
     * @return Vector of text entries
     * @throws PdfError on extraction failure
     */
    std::vector<PdfTextEntry> ExtractText(int pageNum);

    /**
     * Extract images from all pages
     *
     * @param callback Callback for image extraction results
     * @throws PdfError on extraction failure
     */
    void ExtractImages(PdfImageExtractCallback callback);

    /**
     * Extract images from a specific page
     *
     * @param pageNum Page number (0-based)
     * @param callback Callback for image extraction results
     * @throws PdfError on extraction failure
     */
    void ExtractImages(int pageNum, PdfImageExtractCallback callback);

    /**
     * Extract both text and images from all pages
     *
     * @param textCallback Callback for text extraction results
     * @param imageCallback Callback for image extraction results
     * @throws PdfError on extraction failure
     */
    void Extract(PdfTextExtractCallback textCallback,
                 PdfImageExtractCallback imageCallback);

    /**
     * Get the number of pages in the document
     *
     * @return Page count
     * @throws PdfError if document not loaded
     */
    int GetPageCount() const;

    /**
     * Get the PDF version of the document
     *
     * @return PDF version
     * @throws PdfError if document not loaded
     */
    PdfVersion GetPdfVersion() const;

    /**
     * Get document information (title, author, etc.)
     *
     * @return Document information dictionary
     * @throws PdfError if document not loaded
     */
    PdfDictionary GetDocumentInfo() const;

    /**
     * Check if the document is encrypted
     *
     * @return true if encrypted, false otherwise
     */
    bool IsEncrypted() const;

    /**
     * Set password for encrypted documents
     *
     * @param password Password for decryption
     */
    void SetPassword(const std::string_view& password);

private:
    /**
     * Initialize the extractor
     */
    void init();

    /**
     * Parse document structure without loading all objects
     */
    void parseDocumentStructure();

    /**
     * Extract text from a page using the page object
     */
    std::vector<PdfTextEntry> extractTextFromPage(const PdfObject& pageObj, int pageNum);

    /**
     * Extract images from a page using the page object
     */
    void extractImagesFromPage(const PdfObject& pageObj, int pageNum, PdfImageExtractCallback callback);

    /**
     * Process a page for text and/or image extraction
     */
    void processPage(int pageNum, const PdfObject& pageObj,
                     PdfTextExtractCallback* textCallback,
                     PdfImageExtractCallback* imageCallback);

    /**
     * Load a page object by page number
     */
    const PdfObject& loadPage(int pageNum) const;

    /**
     * Check memory usage and enforce limits
     */
    void checkMemoryUsage();

private:
    // Private implementation
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace PoDoFo

#endif // PDF_FAST_EXTRACTOR_H