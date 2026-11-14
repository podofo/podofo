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

TEST_CASE("TestNonDictionaryAnnotationAction")
{
    // The /A entry of the link is a number
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Annots[4 0 R]>>endobj
4 0 obj<</Type/Annot/Subtype/Link/Rect[0 0 9 9]/A 42>>endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& link = static_cast<PdfAnnotationLink&>(doc.GetPages().GetPageAt(0).GetAnnotations().GetAnnotAt(0));
    REQUIRE(link.GetAction() == nullptr);
}

TEST_CASE("TestDecodeParmsArrayLongerThanFilter")
{
    // The /DecodeParms array has more entries than the /Filter array. The
    // surplus entries must be ignored instead of being stored past the end
    // of the decode parameters list sized by the filter count
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/Data 3 0 R>>endobj
2 0 obj<</Type/Pages/Kids[]/Count 0>>endobj
3 0 obj<</Filter/ASCIIHexDecode/DecodeParms[<<>><<>><<>><<>><<>><<>><<>><<>>]/Length 10>>stream
48656C6C6F
endstream
endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto obj = doc.GetObjects().GetObject(PdfReference(3, 0));
    REQUIRE(obj != nullptr);
    REQUIRE(obj->MustGetStream().GetCopy() == "Hello");
}

TEST_CASE("TestRebuildSkipsMaxGenerationObject")
{
    // The rebuild finds "2 65535 obj" after the referenced "2 1 obj". It must
    // be skipped like the regular xref path does: collecting it as garbage
    // wrapped the incremented generation to 0, so 2 0 R entered the free list
    // and the next created object replaced the live 2 1 R
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 1 R>>endobj
2 1 obj<</Type/Pages/Kids[]/Count 0>>endobj
2 65535 obj<</Type/Bogus>>endobj
trailer<</Root 1 0 R/Size 3>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    REQUIRE(doc.HasBrokenXRef());
    REQUIRE(doc.GetObjects().GetObject(PdfReference(2, 65535)) == nullptr);
    REQUIRE(doc.GetObjects().GetFreeObjects().empty());
    doc.GetPages().CreatePage(PdfPageSize::A4);
    REQUIRE(doc.GetPages().GetCount() == 1);
    REQUIRE(doc.GetObjects().GetObject(PdfReference(2, 1)) != nullptr);
}

TEST_CASE("TestRebuildSkipsObjectNumberAboveMaximum")
{
    // The rebuild finds "4294967295 0 obj": it must be skipped, otherwise
    // the next object number wraps to 0 and every object created after
    // the first one replaces it
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/Data 4294967295 0 R>>endobj
2 0 obj<</Type/Pages/Kids[]/Count 0>>endobj
4294967295 0 obj<</Type/Bogus>>endobj
trailer<</Root 1 0 R/Size 3>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    REQUIRE(doc.HasBrokenXRef());
    REQUIRE(doc.GetObjects().GetObject(PdfReference(4294967295, 0)) == nullptr);
    auto& obj1 = doc.GetObjects().CreateDictionaryObject();
    auto& obj2 = doc.GetObjects().CreateDictionaryObject();
    REQUIRE(obj1.GetIndirectReference() == PdfReference(3, 0));
    REQUIRE(obj2.GetIndirectReference() == PdfReference(4, 0));
}

TEST_CASE("TestRebuildSkipsObjectNumberAboveHighestMaximum")
{
    // The highest maximum object count still keeps "4294967295 0 obj" out
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/Data 4294967295 0 R>>endobj
2 0 obj<</Type/Pages/Kids[]/Count 0>>endobj
4294967295 0 obj<</Type/Bogus>>endobj
trailer<</Root 1 0 R/Size 3>>
startxref
999999
%%EOF)";

    unsigned maxObjectCount = PdfCommon::GetMaxObjectCount();
    PdfCommon::SetMaxObjectCount(numeric_limits<unsigned>::max());
    PdfMemDocument doc;
    try
    {
        doc.LoadFromBuffer(pdf);
    }
    catch (...)
    {
        PdfCommon::SetMaxObjectCount(maxObjectCount);
        throw;
    }

    PdfCommon::SetMaxObjectCount(maxObjectCount);
    REQUIRE(doc.GetObjects().GetObject(PdfReference(4294967295, 0)) == nullptr);
    auto& obj = doc.GetObjects().CreateDictionaryObject();
    REQUIRE(obj.GetIndirectReference() == PdfReference(3, 0));
}

TEST_CASE("TestRebuildRejectsCompressedObjectNumberAboveMaximum")
{
    // The object stream stores the object number 4294967295, which
    // must be rejected like the other invalid entries of the stream
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[]/Count 0>>endobj
3 0 obj<</Type/ObjStm/N 1/First 13/Length 21>>stream
4294967295 0 <</A 1>>
endstream
endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    REQUIRE_THROWS_AS(doc.LoadFromBuffer(pdf), PdfError);
}

TEST_CASE("TestXRefStreamRejectsCompressedEntryInvalidStreamNumber")
{
    // The compressed entry of object 4 refers to the object stream
    // 4294967299 (0x100000003), which must not be truncated to 3
    string_view pdf = R"(%PDF-1.5
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[]/Count 0>>endobj
3 0 obj<</Type/ObjStm/N 1/First 4/Length 12>>stream
4 0 <</A 1>>
endstream
endobj
5 0 obj<</Type/XRef/Size 6/Index[1 5]/W[1 5 1]/Root 1 0 R/Length 35>>stream
)" "\x01\x00\x00\x00\x00\x09\x00\x01\x00\x00\x00\x00\x34\x00\x01\x00\x00\x00\x00\x60\x00"
    "\x02\x01\x00\x00\x00\x03\x00\x01\x00\x00\x00\x00\xB2\x00" R"(
endstream
endobj
startxref
178
%%EOF)"sv;

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    REQUIRE(!doc.HasBrokenXRef());
    REQUIRE(doc.GetObjects().GetObject(PdfReference(4, 0)) == nullptr);
}

TEST_CASE("TestRebuildKeepsLastGenerationOfObjectNumber")
{
    // The rebuild finds the unreferenced "1 0 obj" before the catalog
    // "1 2 obj". Only the latter must be kept: collecting the former as
    // garbage put 1 1 R in the free list while the catalog was live, so
    // the next created object replaced the catalog
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Junk true>>endobj
1 2 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[]/Count 0>>endobj
trailer<</Root 1 2 R/Size 3>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    REQUIRE(doc.HasBrokenXRef());
    REQUIRE(doc.GetObjects().GetObject(PdfReference(1, 0)) == nullptr);
    REQUIRE(doc.GetObjects().GetFreeObjects().empty());
    auto& acroForm = doc.GetOrCreateAcroForm();
    REQUIRE(acroForm.GetObject().GetIndirectReference() == PdfReference(3, 0));
    REQUIRE(doc.GetCatalog().GetDictionary().HasKey("AcroForm"));
}

TEST_CASE("TestRebuildReplacesLowerGenerationOfObjectNumber")
{
    // Both "3 0 obj" and the later "3 2 obj" are referenced, so the
    // garbage collection keeps them: the rebuild itself must keep
    // only the last one found
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/A 3 0 R/B 3 2 R>>endobj
2 0 obj<</Type/Pages/Kids[]/Count 0>>endobj
3 0 obj<</Old true>>endobj
3 2 obj<</New true>>endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    REQUIRE(doc.HasBrokenXRef());
    REQUIRE(doc.GetObjects().GetObject(PdfReference(3, 0)) == nullptr);
    REQUIRE(doc.GetObjects().GetObject(PdfReference(3, 2)) != nullptr);
}

TEST_CASE("TestRebuildXRefStreamTakesFreedObjectNumber")
{
    // The rebuild finds two unreferenced generations of object 5. Freeing
    // both put 5 1 R and 5 3 R in the free list: the XRef stream object
    // took the first, then the second replaced its entry on writing
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[]/Count 0>>endobj
5 0 obj<</Old true>>endobj
5 2 obj<</New true>>endobj
trailer<</Root 1 0 R/Size 3>>
startxref
999999
%%EOF)";

    charbuff buff;
    {
        PdfMemDocument doc;
        doc.LoadFromBuffer(pdf);
        REQUIRE(doc.HasBrokenXRef());
        BufferStreamDevice device(buff);
        doc.Save(device, PdfSaveOptions::ForceXRefStream | PdfSaveOptions::NoMetadataUpdate);
    }

    PdfMemDocument doc;
    doc.LoadFromBuffer(buff);
    REQUIRE(!doc.HasBrokenXRef());
    auto xrefObj = doc.GetObjects().GetObject(PdfReference(5, 3));
    REQUIRE(xrefObj != nullptr);
    REQUIRE(xrefObj->GetDictionary().FindKeyAs<PdfName>("Type") == "XRef");
}
