/**
 * Copyright (C) 2026 PoDoFo Project
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

TEST_CASE("FastExtractorBasic")
{
    // Test basic functionality with a small PDF
    PdfFastExtractor extractor(TestUtils::GetTestInputFilePath("TextExtraction1.pdf"));

    REQUIRE(extractor.GetPageCount() == 1);
    REQUIRE(extractor.GetPdfVersion() == PdfVersion::V1_4);

    // Test text extraction
    vector<PdfTextEntry> entries = extractor.ExtractText(0);
    REQUIRE(entries.size() >= 4);
    REQUIRE(entries[0].Text.find("MATLAB") != string::npos);

    // Test options
    PdfFastExtractOptions options;
    options.ExtractImages = false;
    options.ParallelProcessing = false;
    extractor.SetOptions(options);

    REQUIRE(extractor.GetOptions().ExtractText == true);
    REQUIRE(extractor.GetOptions().ExtractImages == false);
}

TEST_CASE("FastExtractorMultiplePages")
{
    // Test with a multi-page PDF if available
    // For now, use single page test
    PdfFastExtractor extractor(TestUtils::GetTestInputFilePath("TextExtraction1.pdf"));

    // Test callback interface
    int pageCount = 0;
    extractor.ExtractText([&pageCount](int pageNum, const vector<PdfTextEntry>& entries) {
        pageCount++;
        REQUIRE(pageNum == 0);
        REQUIRE(entries.size() > 0);
    });

    REQUIRE(pageCount == 1);
}

TEST_CASE("FastExtractorOptions")
{
    PdfFastExtractor extractor(TestUtils::GetTestInputFilePath("TextExtraction1.pdf"));

    PdfFastExtractOptions options;
    options.ExtractText = true;
    options.ExtractImages = false;
    options.ParallelProcessing = true;
    options.MaxThreads = 2;
    options.StreamProcessing = true;
    options.TextFlags = PdfTextExtractFlags::None;
    options.MaxMemoryUsage = 100 * 1024 * 1024; // 100MB
    options.CacheFonts = true;
    options.SkipInvalidPages = true;

    extractor.SetOptions(options);

    const auto& retrievedOptions = extractor.GetOptions();
    REQUIRE(retrievedOptions.ExtractText == true);
    REQUIRE(retrievedOptions.ExtractImages == false);
    REQUIRE(retrievedOptions.ParallelProcessing == true);
    REQUIRE(retrievedOptions.MaxThreads == 2);
    REQUIRE(retrievedOptions.StreamProcessing == true);
    REQUIRE(retrievedOptions.MaxMemoryUsage == 100 * 1024 * 1024);
    REQUIRE(retrievedOptions.CacheFonts == true);
    REQUIRE(retrievedOptions.SkipInvalidPages == true);
}

TEST_CASE("FastExtractorEmpty")
{
    // Test with invalid file should throw
    REQUIRE_THROWS(PdfFastExtractor("nonexistent.pdf"));
}

TEST_CASE("FastExtractorPassword")
{
    // Test encrypted PDF if available
    // For now, just test the method exists
    PdfFastExtractor extractor(TestUtils::GetTestInputFilePath("TextExtraction1.pdf"));

    // Should not throw for unencrypted PDF
    REQUIRE_NOTHROW(extractor.SetPassword(""));
    REQUIRE(extractor.IsEncrypted() == false);
}

TEST_CASE("FastExtractorDocumentInfo")
{
    PdfFastExtractor extractor(TestUtils::GetTestInputFilePath("TextExtraction1.pdf"));

    auto info = extractor.GetDocumentInfo();
    // Should at least not throw
    REQUIRE_NOTHROW(extractor.GetDocumentInfo());
}

TEST_CASE("FastExtractorImageExtraction")
{
    // Test image extraction interface (even if no images in test PDF)
    PdfFastExtractor extractor(TestUtils::GetTestInputFilePath("TextExtraction1.pdf"));

    int imageCount = 0;
    extractor.ExtractImages([&imageCount](int pageNum, const PdfImageInfo& info, const charbuff& data) {
        imageCount++;
        REQUIRE(info.Width > 0);
        REQUIRE(info.Height > 0);
        REQUIRE(data.size() > 0);
    });

    // The test PDF may or may not have images
    // Just ensure the method doesn't crash
}

TEST_CASE("FastExtractorCombinedExtraction")
{
    PdfFastExtractor extractor(TestUtils::GetTestInputFilePath("TextExtraction1.pdf"));

    int textPageCount = 0;
    int imagePageCount = 0;

    extractor.Extract(
        [&textPageCount](int pageNum, const vector<PdfTextEntry>& entries) {
            textPageCount++;
            REQUIRE(entries.size() > 0);
        },
        [&imagePageCount](int pageNum, const PdfImageInfo& info, const charbuff& data) {
            imagePageCount++;
        }
    );

    REQUIRE(textPageCount == 1);
    // Image count depends on test PDF
}

// TODO: Add large file tests (100MB+) when test data available
// TODO: Add performance tests comparing with PdfMemDocument