/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: MIT-0
 */

#include <PdfTest.h>
#include <podofo/private/OpenSSLInternal.h>

using namespace std;
using namespace PoDoFo;

TEST_CASE("TestFixInvalidCrossReferenceTable")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestFixInvalidCrossReferenceTable.pdf"));
    doc.Save(TestUtils::GetTestOutputFilePath("TestFixInvalidCrossReferenceTable.pdf"), PdfSaveOptions::NoMetadataUpdate);
    charbuff buff;
    utls::ReadTo(buff, TestUtils::GetTestOutputFilePath("TestFixInvalidCrossReferenceTable.pdf"));
    REQUIRE(ssl::ComputeMD5Str(buff) == "FF980936FDE894F4495DDEC7C13AF4F4");
}

TEST_CASE("TestCVE20259394InvalidNumberInContentStream")
{
    // CVE-2025-9394 regression: verifies that a PDF containing malformed
    // numeric tokens in its content stream can be loaded without crashing.
    // The tokenizer-level tests (TokenizerTest.cpp) directly exercise the
    // DetermineDataType recovery paths; this test verifies the document
    // structure parser handles the surrounding stream object gracefully.
    static const char pdfData[] =
        "%PDF-1.0\n"
        "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
        "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"
        "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Contents 4 0 R >>\nendobj\n"
        "4 0 obj\n<< /Length 8 >>\nstream\n"
        "-. +. Td"
        "\nendstream\nendobj\n"
        "xref\n0 5\n"
        "0000000000 65535 f \r\n"
        "0000000009 00000 n \r\n"
        "0000000058 00000 n \r\n"
        "0000000115 00000 n \r\n"
        "0000000202 00000 n \r\n"
        "trailer\n<< /Size 5 /Root 1 0 R >>\n"
        "startxref\n259\n%%EOF\n";

    auto device = std::make_shared<SpanStreamDevice>(pdfData, sizeof(pdfData) - 1);
    PdfMemDocument doc;
    REQUIRE_NOTHROW(doc.Load(device));
}

TEST_CASE("TestMalformedAnnotationAction")
{
    // Test that a PDF with a malformed action in a Link annotation does not
    // crash when accessing the annotation's action.
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestMalformedAnnotationAction.pdf"));

    auto& page = doc.GetPages().GetPageAt(0);
    REQUIRE(page.GetAnnotations().GetCount() == 1);

    auto& annot = page.GetAnnotations().GetAnnotAt(0);
    REQUIRE(annot.GetType() == PdfAnnotationType::Link);

    auto& linkAnnot = static_cast<PdfAnnotationLink&>(annot);
    auto action = linkAnnot.GetAction();
    REQUIRE(action == nullptr);
}
