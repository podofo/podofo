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

TEST_CASE("TestRebuildSkipsUnavailableObjectNumber")
{
    // The startxref offset is out of bounds, triggering a rebuild of the cross
    // reference table. The rebuild must skip the unavailable object number 0
    string_view pdf = R"(%PDF-1.7
0 5 obj<</Type/Bogus>>endobj
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/Resources<<>>/MediaBox[0 0 9 9]>>endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    REQUIRE(doc.HasBrokenXRef());
    REQUIRE(doc.GetPages().GetCount() == 1);
    REQUIRE(doc.GetObjects().GetObject(PdfReference(0, 5)) == nullptr);
}

TEST_CASE("TestRebuildSkipsUnavailableCompressedObject")
{
    // The rebuild enumerates the object stream contents, which store the
    // unavailable object number 0. It must be skipped without shifting the
    // index of the objects stored after it
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/Resources<<>>/MediaBox[0 0 9 9]/Bogus 0 0 R/Data 4 0 R>>endobj
5 0 obj<</Type/ObjStm/N 2/First 8/Length 25>>stream
0 0 4 9 <</A 1>> <</B 2>>
endstream
endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    REQUIRE(doc.HasBrokenXRef());
    REQUIRE(doc.GetObjects().GetObject(PdfReference(0, 0)) == nullptr);

    // The object stored after the skipped one still resolves through its index
    auto dataObj = doc.GetObjects().GetObject(PdfReference(4, 0));
    REQUIRE(dataObj != nullptr);
    REQUIRE(dataObj->GetDictionary().FindKeyAsSafe<int64_t>("B", 0) == 2);
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
