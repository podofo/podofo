/**
 * SPDX-FileCopyrightText: (C) 2026 PoDoFo Project
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFastExtractor.h"

#include <podofo/private/PdfParser.h>
#include <podofo/private/PdfWriter.h>
#include <podofo/auxiliary/StreamDevice.h>
#include <podofo/auxiliary/InputDevice.h>
#include <podofo/main/PdfMemDocument.h>
#include <podofo/main/PdfPage.h>
#include <podofo/main/PdfXObjectForm.h>
#include <podofo/main/PdfContentStreamReader.h>
#include <podofo/main/PdfFont.h>
#include <podofo/main/PdfTextExtract.h>
#include <podofo/main/PdfPageCollection.h>

#include <thread>
#include <future>
#include <queue>
#include <unordered_map>
#include <atomic>

using namespace std;
using namespace PoDoFo;

// Private implementation class
class PdfFastExtractor::Impl
{
public:
    Impl(shared_ptr<InputStreamDevice> device);
    ~Impl();

    void SetOptions(const PdfFastExtractOptions& options);
    const PdfFastExtractOptions& GetOptions() const;

    void ExtractText(PdfTextExtractCallback callback);
    vector<PdfTextEntry> ExtractText(int pageNum);
    void ExtractImages(PdfImageExtractCallback callback);
    void ExtractImages(int pageNum, PdfImageExtractCallback callback);
    void Extract(PdfTextExtractCallback textCallback,
                 PdfImageExtractCallback imageCallback);

    int GetPageCount() const;
    PdfVersion GetPdfVersion() const;
    PdfDictionary GetDocumentInfo() const;
    bool IsEncrypted() const;
    void SetPassword(const string_view& password);

private:
    void init();
    void parseDocumentStructure();
    vector<PdfTextEntry> extractTextFromPage(const PdfObject& pageObj, int pageNum);
    void extractImagesFromPage(const PdfObject& pageObj, int pageNum, PdfImageExtractCallback callback);
    void processPage(int pageNum, const PdfObject& pageObj,
                     PdfTextExtractCallback* textCallback,
                     PdfImageExtractCallback* imageCallback);
    const PdfObject& loadPage(int pageNum) const;
    void checkMemoryUsage();

    // Helper methods
    void loadPageTree(const PdfObject& pagesObj);
    void collectPageObjects(const PdfObject& pagesObj, vector<PdfReference>& pageRefs);
    const PdfObject& getPageObject(int pageNum) const;

private:
    shared_ptr<InputStreamDevice> m_device;
    PdfFastExtractOptions m_options;
    unique_ptr<PdfParser> m_parser;
    unique_ptr<PdfMemDocument> m_document; // Used for non-streaming mode

    // Document structure
    PdfVersion m_pdfVersion;
    PdfReference m_rootRef;
    PdfReference m_pagesRef;
    vector<PdfReference> m_pageRefs; // Page object references
    mutable unordered_map<int, shared_ptr<PdfObject>> m_pageCache;

    // State
    string m_password;
    bool m_initialized;
    atomic<size_t> m_memoryUsage;

    // Font cache
    mutable unordered_map<PdfReference, shared_ptr<PdfFont>> m_fontCache;
};

// PdfFastExtractor::Impl implementation
PdfFastExtractor::Impl::Impl(shared_ptr<InputStreamDevice> device)
    : m_device(move(device))
    , m_options()
    , m_pdfVersion(PdfVersion::Unknown)
    , m_initialized(false)
    , m_memoryUsage(0)
{
    if (!m_device)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);
}

PdfFastExtractor::Impl::~Impl() = default;

void PdfFastExtractor::Impl::SetOptions(const PdfFastExtractOptions& options)
{
    m_options = options;
}

const PdfFastExtractOptions& PdfFastExtractor::Impl::GetOptions() const
{
    return m_options;
}

void PdfFastExtractor::Impl::init()
{
    if (m_initialized)
        return;

    // Create parser with lazy loading
    auto objects = make_shared<PdfIndirectObjectList>();
    m_parser = make_unique<PdfParser>(*objects);
    m_parser->SetLoadStreamsEagerly(false); // Crucial for memory efficiency

    // Parse the document structure
    m_parser->Parse(*m_device);

    // Get basic document info
    m_pdfVersion = m_parser->GetPdfVersion();

    // Get root and pages references from trailer
    auto trailer = m_parser->GetTrailer();
    auto rootObj = trailer.GetDictionary().MustFindKey("Root"_n).GetReference();
    m_rootRef = rootObj;

    // We need to load the root object to get the pages tree
    // For now, we'll use a simpler approach
    m_initialized = true;
}

void PdfFastExtractor::Impl::parseDocumentStructure()
{
    // This is a simplified implementation
    // In a full implementation, we would parse the pages tree
    // and collect all page references without loading page contents

    // For now, we'll use PdfMemDocument to get page count
    // TODO: Implement proper streaming pages tree parsing
    if (!m_document)
    {
        m_document = make_unique<PdfMemDocument>(m_device, m_password);
    }
}

int PdfFastExtractor::Impl::GetPageCount() const
{
    if (!m_initialized)
        const_cast<Impl*>(this)->init();

    if (m_document)
        return static_cast<int>(m_document->GetPages().GetCount());

    // Fallback: parse document if not already done
    if (m_pageRefs.empty())
    {
        // TODO: Parse pages tree
        return 0;
    }

    return static_cast<int>(m_pageRefs.size());
}

PdfVersion PdfFastExtractor::Impl::GetPdfVersion() const
{
    if (!m_initialized)
        const_cast<Impl*>(this)->init();

    return m_pdfVersion;
}

PdfDictionary PdfFastExtractor::Impl::GetDocumentInfo() const
{
    if (!m_initialized)
        const_cast<Impl*>(this)->init();

    if (m_document)
        return m_document->GetTrailer()->GetDictionary();

    // TODO: Return info from parser
    return PdfDictionary();
}

bool PdfFastExtractor::Impl::IsEncrypted() const
{
    if (!m_initialized)
        const_cast<Impl*>(this)->init();

    return m_parser->GetEncrypt() != nullptr;
}

void PdfFastExtractor::Impl::SetPassword(const string_view& password)
{
    m_password = password;
    if (m_parser)
        m_parser->SetPassword(password);

    // Reset document if password changes
    if (m_document)
        m_document.reset();
}

void PdfFastExtractor::Impl::ExtractText(PdfTextExtractCallback callback)
{
    init();

    // Ensure document is loaded
    if (!m_document)
    {
        parseDocumentStructure();
        if (!m_document)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Document not loaded");
    }

    int pageCount = GetPageCount();
    if (pageCount == 0)
        return;

    if (m_options.ParallelProcessing && pageCount > 1)
    {
        // Parallel processing
        vector<future<void>> futures;
        int numThreads = min(m_options.MaxThreads, pageCount);

        for (int i = 0; i < pageCount; i++)
        {
            futures.push_back(async(launch::async, [this, i, &callback]() {
                try
                {
                    auto entries = ExtractText(i);
                    if (!entries.empty())
                        callback(i, entries);
                }
                catch (const PdfError&)
                {
                    // Log error but continue
                }
            }));

            // Limit concurrent threads
            if (futures.size() >= static_cast<size_t>(numThreads))
            {
                for (auto& f : futures)
                    f.get();
                futures.clear();
            }
        }

        for (auto& f : futures)
            f.get();
    }
    else
    {
        // Sequential processing
        for (int i = 0; i < pageCount; i++)
        {
            if (m_options.ProgressCallback &&
                m_options.ProgressCallback(i, 0))
                break;

            try
            {
                auto entries = ExtractText(i);
                if (!entries.empty())
                    callback(i, entries);
            }
            catch (const PdfError& e)
            {
                if (m_options.SkipInvalidPages)
                    continue;
                else
                    throw;
            }
        }
    }
}

vector<PdfTextEntry> PdfFastExtractor::Impl::ExtractText(int pageNum)
{
    init();

    if (pageNum < 0 || pageNum >= GetPageCount())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Invalid page number");

    // Ensure document is loaded
    if (!m_document)
    {
        parseDocumentStructure();
        if (!m_document)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Document not loaded");
    }

    // Get a dummy page object reference (not used in current implementation)
    auto& page = m_document->GetPages().GetPageAt(pageNum);
    const PdfObject& pageObj = page.GetObject();

    return extractTextFromPage(pageObj, pageNum);
}

vector<PdfTextEntry> PdfFastExtractor::Impl::extractTextFromPage(const PdfObject& pageObj, int pageNum)
{
    vector<PdfTextEntry> entries;

    // Ensure document is loaded
    if (!m_document)
    {
        parseDocumentStructure();
        if (!m_document)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Document not loaded");
    }

    // Get the page from document
    auto& page = m_document->GetPages().GetPageAt(pageNum);

    // Prepare text extraction parameters
    PdfTextExtractParams params;
    params.Flags = m_options.TextFlags;
    params.ClipRect = m_options.ClipRect;

    // Extract text
    page.ExtractTextTo(entries, params);

    return entries;
}

void PdfFastExtractor::Impl::ExtractImages(PdfImageExtractCallback callback)
{
    init();

    // Ensure document is loaded
    if (!m_document)
    {
        parseDocumentStructure();
        if (!m_document)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Document not loaded");
    }

    int pageCount = GetPageCount();
    if (pageCount == 0)
        return;

    if (m_options.ParallelProcessing && pageCount > 1)
    {
        // Similar parallel implementation as ExtractText
        vector<future<void>> futures;
        int numThreads = min(m_options.MaxThreads, pageCount);

        for (int i = 0; i < pageCount; i++)
        {
            futures.push_back(async(launch::async, [this, i, &callback]() {
                try
                {
                    ExtractImages(i, callback);
                }
                catch (const PdfError&)
                {
                    // Log error but continue
                }
            }));

            if (futures.size() >= static_cast<size_t>(numThreads))
            {
                for (auto& f : futures)
                    f.get();
                futures.clear();
            }
        }

        for (auto& f : futures)
            f.get();
    }
    else
    {
        for (int i = 0; i < pageCount; i++)
        {
            if (m_options.ProgressCallback &&
                m_options.ProgressCallback(i, 0))
                break;

            try
            {
                ExtractImages(i, callback);
            }
            catch (const PdfError& e)
            {
                if (m_options.SkipInvalidPages)
                    continue;
                else
                    throw;
            }
        }
    }
}

void PdfFastExtractor::Impl::ExtractImages(int pageNum, PdfImageExtractCallback callback)
{
    init();

    if (pageNum < 0 || pageNum >= GetPageCount())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Invalid page number");

    // Ensure document is loaded
    if (!m_document)
    {
        parseDocumentStructure();
        if (!m_document)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Document not loaded");
    }

    // Get page from document
    auto& page = m_document->GetPages().GetPageAt(pageNum);
    const PdfObject& pageObj = page.GetObject();
    extractImagesFromPage(pageObj, pageNum, callback);
}

void PdfFastExtractor::Impl::extractImagesFromPage(const PdfObject& pageObj, int pageNum,
                                                    PdfImageExtractCallback callback)
{
    // Ensure document is loaded
    if (!m_document)
    {
        parseDocumentStructure();
        if (!m_document)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Document not loaded");
    }

    // Get page resources
    auto resourcesObj = pageObj.GetDictionary().FindKey("Resources"_n);
    if (resourcesObj == nullptr)
        return; // No resources, no images

    // Resolve reference if needed
    const PdfObject* actualResourcesObj = resourcesObj;
    if (resourcesObj->IsReference())
        actualResourcesObj = m_document->GetObjects().GetObject(resourcesObj->GetReference());

    if (actualResourcesObj == nullptr || !actualResourcesObj->IsDictionary())
        return;

    // Get XObject dictionary
    auto xobjectObj = actualResourcesObj->GetDictionary().FindKey("XObject"_n);
    if (xobjectObj == nullptr)
        return; // No XObjects

    // Resolve reference if needed
    const PdfObject* actualXObjectObj = xobjectObj;
    if (xobjectObj->IsReference())
        actualXObjectObj = m_document->GetObjects().GetObject(xobjectObj->GetReference());

    if (actualXObjectObj == nullptr || !actualXObjectObj->IsDictionary())
        return;

    // Iterate through XObjects
    auto& xobjectDict = actualXObjectObj->GetDictionary();
    for (auto& pair : xobjectDict)
    {
        const PdfObject* xobj = nullptr;
        auto& xobjValue = pair.second;

        if (xobjValue.IsReference())
        {
            xobj = m_document->GetObjects().GetObject(xobjValue.GetReference());
        }
        else if (xobjValue.IsDictionary())
        {
            // Direct embedded XObject (uncommon but possible)
            xobj = &xobjValue;
        }

        if (xobj == nullptr)
            continue;

        // Check if it's an image
        auto subtypeObj = xobj->GetDictionary().FindKey("Subtype"_n);
        if (subtypeObj == nullptr || !subtypeObj->IsName() || subtypeObj->GetName() != "Image"_n)
            continue;

        // Extract image info
        PdfImageInfo info;
        info.Width = static_cast<unsigned>(xobj->GetDictionary().MustFindKey("Width"_n).GetNumber());
        info.Height = static_cast<unsigned>(xobj->GetDictionary().MustFindKey("Height"_n).GetNumber());

        // Get filters if present
        auto filterObj = xobj->GetDictionary().FindKey("Filter"_n);
        if (filterObj != nullptr)
        {
            if (filterObj->IsName())
            {
                info.Filters = PdfFilterList();
                info.Filters->push_back(filterObj->GetName());
            }
            else if (filterObj->IsArray())
            {
                info.Filters = PdfFilterList();
                auto& filterArray = filterObj->GetArray();
                for (unsigned i = 0; i < filterArray.GetSize(); i++)
                {
                    if (filterArray[i].IsName())
                        info.Filters->push_back(filterArray[i].GetName());
                }
            }
        }

        // Get bits per component
        auto bitsObj = xobj->GetDictionary().FindKey("BitsPerComponent"_n);
        if (bitsObj != nullptr && bitsObj->IsNumber())
            info.BitsPerComponent = static_cast<unsigned char>(bitsObj->GetNumber());

        // Get color space if present
        auto colorSpaceObj = xobj->GetDictionary().FindKey("ColorSpace"_n);
        if (colorSpaceObj != nullptr)
        {
            // TODO: Parse color space
        }

        // Get image data
        if (xobj->HasStream())
        {
            auto stream = xobj->GetStream();
            if (stream != nullptr)
            {
                charbuff data = stream->GetCopy();
                // Call callback with image info and data
                if (callback)
                    callback(pageNum, info, data);
            }
        }
    }
}

void PdfFastExtractor::Impl::Extract(PdfTextExtractCallback textCallback,
                                      PdfImageExtractCallback imageCallback)
{
    init();

    // Ensure document is loaded
    if (!m_document)
    {
        parseDocumentStructure();
        if (!m_document)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Document not loaded");
    }

    int pageCount = GetPageCount();
    if (pageCount == 0)
        return;

    if (m_options.ParallelProcessing && pageCount > 1)
    {
        // Process pages in parallel
        vector<future<void>> futures;
        int numThreads = min(m_options.MaxThreads, pageCount);

        for (int i = 0; i < pageCount; i++)
        {
            futures.push_back(async(launch::async, [this, i, &textCallback, &imageCallback]() {
                try
                {
                    // Get page from document
                    auto& page = m_document->GetPages().GetPageAt(i);
                    const PdfObject& pageObj = page.GetObject();

                    vector<PdfTextEntry> textEntries;
                    if (m_options.ExtractText)
                        textEntries = extractTextFromPage(pageObj, i);

                    if (m_options.ExtractImages)
                        extractImagesFromPage(pageObj, i, imageCallback);

                    if (!textEntries.empty() && textCallback)
                        textCallback(i, textEntries);
                }
                catch (const PdfError&)
                {
                    // Log error but continue
                }
            }));

            if (futures.size() >= static_cast<size_t>(numThreads))
            {
                for (auto& f : futures)
                    f.get();
                futures.clear();
            }
        }

        for (auto& f : futures)
            f.get();
    }
    else
    {
        // Sequential processing
        for (int i = 0; i < pageCount; i++)
        {
            if (m_options.ProgressCallback &&
                m_options.ProgressCallback(i, 0))
                break;

            try
            {
                // Get page from document
                auto& page = m_document->GetPages().GetPageAt(i);
                const PdfObject& pageObj = page.GetObject();

                vector<PdfTextEntry> textEntries;
                if (m_options.ExtractText)
                    textEntries = extractTextFromPage(pageObj, i);

                if (m_options.ExtractImages)
                    extractImagesFromPage(pageObj, i, imageCallback);

                if (!textEntries.empty() && textCallback)
                    textCallback(i, textEntries);
            }
            catch (const PdfError& e)
            {
                if (m_options.SkipInvalidPages)
                    continue;
                else
                    throw;
            }
        }
    }
}

const PdfObject& PdfFastExtractor::Impl::loadPage(int pageNum) const
{
    // Check cache first
    auto it = m_pageCache.find(pageNum);
    if (it != m_pageCache.end())
        return *it->second;

    // Load page object
    // TODO: Implement proper page loading from parser
    // For now, use document if available
    if (m_document)
    {
        auto& page = m_document->GetPages().GetPageAt(pageNum);
        auto pageObj = make_shared<PdfObject>(page.GetObject());
        m_pageCache[pageNum] = pageObj;
        return *pageObj;
    }

    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Page loading not implemented");
}

void PdfFastExtractor::Impl::checkMemoryUsage()
{
    if (m_options.MaxMemoryUsage == 0)
        return;

    if (m_memoryUsage > m_options.MaxMemoryUsage)
    {
        // Clear some caches
        m_pageCache.clear();
        m_fontCache.clear();
        m_memoryUsage = 0;
    }
}

// PdfFastExtractor public interface implementation
PdfFastExtractor::PdfFastExtractor(const string_view& filepath)
    : m_impl(make_unique<Impl>(make_shared<FileStreamDevice>(string(filepath))))
{
}

PdfFastExtractor::PdfFastExtractor(shared_ptr<InputStreamDevice> device)
    : m_impl(make_unique<Impl>(move(device)))
{
}

PdfFastExtractor::~PdfFastExtractor() = default;

PdfFastExtractor::PdfFastExtractor(PdfFastExtractor&&) noexcept = default;
PdfFastExtractor& PdfFastExtractor::operator=(PdfFastExtractor&&) noexcept = default;

void PdfFastExtractor::SetOptions(const PdfFastExtractOptions& options)
{
    m_impl->SetOptions(options);
}

const PdfFastExtractOptions& PdfFastExtractor::GetOptions() const
{
    return m_impl->GetOptions();
}

void PdfFastExtractor::ExtractText(PdfTextExtractCallback callback)
{
    m_impl->ExtractText(callback);
}

vector<PdfTextEntry> PdfFastExtractor::ExtractText(int pageNum)
{
    return m_impl->ExtractText(pageNum);
}

void PdfFastExtractor::ExtractImages(PdfImageExtractCallback callback)
{
    m_impl->ExtractImages(callback);
}

void PdfFastExtractor::ExtractImages(int pageNum, PdfImageExtractCallback callback)
{
    m_impl->ExtractImages(pageNum, callback);
}

void PdfFastExtractor::Extract(PdfTextExtractCallback textCallback,
                               PdfImageExtractCallback imageCallback)
{
    m_impl->Extract(textCallback, imageCallback);
}

int PdfFastExtractor::GetPageCount() const
{
    return m_impl->GetPageCount();
}

PdfVersion PdfFastExtractor::GetPdfVersion() const
{
    return m_impl->GetPdfVersion();
}

PdfDictionary PdfFastExtractor::GetDocumentInfo() const
{
    return m_impl->GetDocumentInfo();
}

bool PdfFastExtractor::IsEncrypted() const
{
    return m_impl->IsEncrypted();
}

void PdfFastExtractor::SetPassword(const string_view& password)
{
    m_impl->SetPassword(password);
}