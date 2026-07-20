// SPDX-FileCopyrightText: 2023 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

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

TEST_CASE("TestXRefSectionShifted")
{
    // The xref subsection header starts at object 1 instead of 0, shifting
    // every entry by one object so /Root resolves to a free entry. The catalog
    // validation performed while parsing fails and triggers a rebuild of the
    // cross reference table, after which the document loads correctly
    // This issue was discussed in https://github.com/podofo/podofo/issues/357
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("Corrupted", "XRefSectionShifted.pdf"));
    REQUIRE(doc.HasBrokenXRef());
    REQUIRE(doc.GetPages().GetCount() == 1);
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
