// SPDX-FileCopyrightText: 2008 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>
#include <podofo/optional/PdfNames.h>

using namespace std;
using namespace PoDoFo;

/** This class tests the basic integer and other types PoDoFo uses
 *  to make sure they satisfy its requirements for behaviour, size, etc.
 */

TEST_CASE("NullableTest")
{
    int intval = 15;
    nullable<int&> nullint1(intval);
    nullable<int&> nullint2(&intval);
    nullable<int&> nullint3;
    REQUIRE(nullint1 == &intval);
    REQUIRE(&intval == nullint2);
    REQUIRE(nullint1 != nullptr);
    REQUIRE(nullptr != nullint2);
    REQUIRE(nullint1 == 15);
    REQUIRE(16 != nullint1);
    REQUIRE(nullint3 == nullptr);
    REQUIRE(nullptr == nullint3);
    REQUIRE(nullint3 != &intval);
    REQUIRE(&intval != nullint3);
}

TEST_CASE("BasicTypeTest")
{
    REQUIRE(std::numeric_limits<uint64_t>::max() >= 9999999999);
}

TEST_CASE("TestIterations")
{
    PdfMemDocument doc;
    PdfPage* pages[] = {
        &doc.GetPages().CreatePage(PdfPageSize::A4),
        &doc.GetPages().CreatePage(PdfPageSize::A4),
        &doc.GetPages().CreatePage(PdfPageSize::A4),
    };

    unsigned i = 0;
    for (auto page : doc.GetPages())
    {
        REQUIRE(page == pages[i]);
        i++;
    }

    auto& page1 = *pages[0];
    PdfAnnotation* annots[] = {
        &page1.GetAnnotations().CreateAnnot<PdfAnnotationWatermark>(Rect()),
        &page1.GetAnnotations().CreateAnnot<PdfAnnotationWatermark>(Rect()),
        &page1.GetAnnotations().CreateAnnot<PdfAnnotationWatermark>(Rect()),
    };

    i = 0;
    for (auto annot : page1.GetAnnotations())
    {
        REQUIRE(annot == annots[i]);
        i++;
    }
}

TEST_CASE("TestIterations2")
{
    PdfMemDocument doc;
    vector<PdfField*> fields;
    for (auto field : doc.GetFieldsIterator())
    {
        fields.push_back(field);
    }

    REQUIRE(fields.size() == 0);

    doc.GetPages().CreatePage(Rect(0, 0, 300, 300));
    for (auto field : doc.GetPages().GetPageAt(0).GetFieldsIterator())
    {
        fields.push_back(field);
    }
    REQUIRE(fields.size() == 0);

    doc.Load(TestUtils::GetTestInputFilePath("Hierarchies1.pdf"));
    for (auto field : doc.GetFieldsIterator())
    {
        fields.push_back(field);
    }

    REQUIRE(fields.size() == 25);

    fields.clear();
    for (auto field : doc.GetPages().GetPageAt(0).GetFieldsIterator())
    {
        fields.push_back(field);
    }

    REQUIRE(fields.size() == 23);
}

TEST_CASE("ErrorFilePath")
{
    try
    {
        PdfObject test;
        test.GetString();
    }
    catch (const PdfError& err)
    {
        auto path = err.GetCallStack().front().GetFilePath();
        REQUIRE(fs::u8path(path) == fs::u8path("main") / "PdfVariant.cpp");
    }
}

TEST_CASE("TestMetadataSet")
{
    PdfMemDocument doc;
    auto& metadata = doc.GetMetadata();
    metadata.SetTitle(PdfString("TestTitle"));
    REQUIRE(metadata.GetTitle()->GetString() == "TestTitle");
    metadata.SetTitle(PdfString("TestTitle2"));
    REQUIRE(metadata.GetTitle()->GetString() == "TestTitle2");
    metadata.SetTitle(nullptr);
    REQUIRE(metadata.GetTitle() == nullptr);
}

TEST_CASE("TestNormalizeRangeRotations")
{
    ASSERT_EQUAL(utls::NormalizeCircularRange(370, 0, 360), 10);
    ASSERT_EQUAL(utls::NormalizeCircularRange(-370, 0, 360), 350);
    ASSERT_EQUAL(utls::NormalizeCircularRange(360, 0, 360), 0);
    ASSERT_EQUAL(utls::NormalizeCircularRange(0, 0, 360), 0);
    ASSERT_EQUAL(utls::NormalizeCircularRange(10, 0, 360), 10);
    ASSERT_EQUAL(utls::NormalizeCircularRange(-190, -180, 180), 170);
    ASSERT_EQUAL(utls::NormalizeCircularRange(190, -180, 180), -170);
    ASSERT_EQUAL(utls::NormalizeCircularRange(180, -180, 180), -180);
    ASSERT_EQUAL(utls::NormalizeCircularRange(0, -180, 180), 0);
    ASSERT_EQUAL(utls::NormalizeCircularRange(10, -180, 180), 10);
    ASSERT_EQUAL(utls::NormalizeCircularRange(-10, -180, 180), -10);

    // The following page rotation normalizations have
    // been verified in Adobe Reader 2024.002.20759
    ASSERT_EQUAL(utls::NormalizePageRotation(0), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(90), 90);
    ASSERT_EQUAL(utls::NormalizePageRotation(180), 180);
    ASSERT_EQUAL(utls::NormalizePageRotation(270), 270);
    ASSERT_EQUAL(utls::NormalizePageRotation(360), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(0.1), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(0.499999999), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(0.5), 90);
    ASSERT_EQUAL(utls::NormalizePageRotation(360.00000001), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(360.499999999), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(360.5), 90);
    ASSERT_EQUAL(utls::NormalizePageRotation(359.499999999), 270);
    ASSERT_EQUAL(utls::NormalizePageRotation(359.5), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(179.499999999), 90);
    ASSERT_EQUAL(utls::NormalizePageRotation(179.5), 180);
    ASSERT_EQUAL(utls::NormalizePageRotation(180.499999999), 180);
    ASSERT_EQUAL(utls::NormalizePageRotation(180.5), 270);
}

TEST_CASE("TestFileSpecAttachment")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage();
    shared_ptr<PdfFileSpec> fs = doc.CreateFileSpec();
    fs->SetFilename(PdfString("Test.xml"));
    fs->SetEmbeddedData(charbuff(string("<?xml version=\"1.0\"?><catalog></catalog>")));
    auto& names = doc.GetOrCreateNames();
    auto& embeddedFiles = names.GetOrCreateTree<PdfEmbeddedFiles>();
    REQUIRE(embeddedFiles.GetValue(*fs->GetFilename()) == nullptr);
    REQUIRE(!embeddedFiles.HasKey(*fs->GetFilename()));
    embeddedFiles.AddValue(*fs->GetFilename(), fs);
    REQUIRE(embeddedFiles.HasKey(*fs->GetFilename()));
    REQUIRE(embeddedFiles.GetValue(*fs->GetFilename()) == fs.get());

    PdfNameTree<PdfFileSpec>::Map map;
    embeddedFiles.ToDictionary(map);
    REQUIRE(map.size() == 1);
    REQUIRE(map[*fs->GetFilename()] == fs);

    doc.Save(TestUtils::GetTestOutputFilePath("TestFileSpecAttachment.pdf"));
}

// A /Dests leaf whose /Names array has a lone trailing key
static constexpr string_view s_oddNamesTreePdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/Names 5 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</Names[(A)[3 0 R/Fit](B)]>>endobj
5 0 obj<</Dests 4 0 R>>endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

TEST_CASE("TestNameTreeOddNamesLookup")
{
    // The lookup must not step past the end of the array, and the lone key has no value
    PdfMemDocument doc;
    doc.LoadFromBuffer(s_oddNamesTreePdf);
    auto names = doc.GetNames();
    REQUIRE(names != nullptr);
    REQUIRE(!names->HasKey(PdfKnownNameTree::Dests, "M"));
    REQUIRE(names->GetValue(PdfKnownNameTree::Dests, "B") == nullptr);
    REQUIRE(names->GetValue(PdfKnownNameTree::Dests, "A") != nullptr);
}

TEST_CASE("TestNameTreeOddNamesAddValue")
{
    // Adding the lone trailing key must not write its value past the end of the array:
    // the key is dropped and added again with the value
    PdfMemDocument doc;
    doc.LoadFromBuffer(s_oddNamesTreePdf);
    shared_ptr<PdfDestination> dest = doc.CreateDestination();
    doc.GetNames()->MustGetTree<PdfDestinations>().AddValue(PdfString("B"), dest);
    REQUIRE(doc.GetObjects().MustGetObject(PdfReference(4, 0)).GetDictionary().MustFindKey("Names").GetArray().GetSize() == 4);
    REQUIRE(doc.GetNames()->GetValue(PdfKnownNameTree::Dests, "B") == &dest->GetObject());
}

TEST_CASE("TestNameTreeAddValueEmptyLimits")
{
    // The key is inserted in the first kid and the limits are recomputed
    // from the children: the last kid has an empty /Limits array
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/Names 5 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</Kids[6 0 R 7 0 R]>>endobj
5 0 obj<</Dests 4 0 R>>endobj
6 0 obj<</Limits[(B)(B)]/Names[(B)[3 0 R/Fit]]>>endobj
7 0 obj<</Limits[]/Names[]>>endobj
trailer<</Root 1 0 R/Size 8>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    shared_ptr<PdfDestination> dest = doc.CreateDestination();
    doc.GetNames()->MustGetTree<PdfDestinations>().AddValue(PdfString("A"), dest);
    REQUIRE(doc.GetNames()->HasKey(PdfKnownNameTree::Dests, "A"));
    REQUIRE(doc.GetNames()->HasKey(PdfKnownNameTree::Dests, "B"));
}

TEST_CASE("TestNameTreeKidsLoopLookup")
{
    // The /Dests root /Kids references the root itself
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/Names 5 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</Kids[4 0 R]>>endobj
5 0 obj<</Dests 4 0 R>>endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto names = doc.GetNames();
    ASSERT_THROW_WITH_ERROR_CODE(names->GetValue(PdfKnownNameTree::Dests, "A"), PdfErrorCode::MaxRecursionReached);
    shared_ptr<PdfDestination> dest = doc.CreateDestination();
    ASSERT_THROW_WITH_ERROR_CODE(names->MustGetTree<PdfDestinations>().AddValue(PdfString("A"), dest),
        PdfErrorCode::MaxRecursionReached);
}

TEST_CASE("TestNameTreeMalformedLookup")
{
    // Malformed kids, limits and keys are skipped: a number and a non dictionary
    // kid, a short and a non string /Limits, a number key. /EmbeddedFiles is no tree
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/Names 5 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</Kids[1 6 0 R 7 0 R 8 0 R]>>endobj
5 0 obj<</Dests 4 0 R/EmbeddedFiles 6 0 R>>endobj
6 0 obj 42 endobj
7 0 obj<</Limits[(X)]/Names[1 2(X)[3 0 R/Fit]]>>endobj
8 0 obj<</Limits[1 2]/Names[(A)[3 0 R/FitH 0]]>>endobj
trailer<</Root 1 0 R/Size 9>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto names = doc.GetNames();
    REQUIRE(names->GetValue(PdfKnownNameTree::Dests, "X") != nullptr);
    REQUIRE(names->GetValue(PdfKnownNameTree::Dests, "A") != nullptr);
    REQUIRE(names->GetValue(PdfKnownNameTree::Dests, "M") == nullptr);
    REQUIRE(names->GetValue(PdfKnownNameTree::EmbeddedFiles, "X") == nullptr);
}

TEST_CASE("TestNameTreeCacheAfterAddValue")
{
    // The destination of Foo is stored directly in the /Names array, which the
    // insertion relocates: it must be moved to an indirect object and its wrapper,
    // also when obtained from ToDictionary, rebound to it. Zed is already indirect
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/Names 5 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</Names[(Foo)[3 0 R/Fit](Zed)6 0 R]>>endobj
5 0 obj<</Dests 4 0 R>>endobj
6 0 obj[3 0 R/Fit]endobj
trailer<</Root 1 0 R/Size 7>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& dests = doc.GetNames()->MustGetTree<PdfDestinations>();
    PdfDestinations::Map map;
    dests.ToDictionary(map);
    auto foo = map.at(PdfString("Foo"));
    auto zed = map.at(PdfString("Zed"));
    REQUIRE(dests.GetValue("Foo") == foo.get());

    shared_ptr<PdfDestination> bar = doc.CreateDestination();
    bar->SetDestination(doc.GetPages().GetPageAt(0));
    dests.AddValue(PdfString("Bar"), bar);

    REQUIRE(foo->GetObject().IsIndirect());
    REQUIRE(foo->GetType() == PdfDestinationType::Fit);
    REQUIRE(doc.GetNames()->GetValue(PdfKnownNameTree::Dests, "Foo") == &foo->GetObject());
    REQUIRE(dests.GetValue("Foo") == foo.get());
    REQUIRE(dests.GetValue("Zed") == zed.get());
    REQUIRE(dests.GetValue("Bar") == bar.get());
}

TEST_CASE("TestPdfNames")
{
    // Just include and use "optional/PdfNames.h"
    PdfDictionary dict;
    dict.AddKey(PdfNames::Length, PdfObject(static_cast<int64_t >(100)));
}

TEST_CASE("TestMoveSemantics")
{
    string_view refs1 = "S1";
    string_view refs2 = "S2";
    PdfString s1 = refs1;
    REQUIRE(s1 == refs1);
    PdfString s2 = refs2;
    REQUIRE(s2 == refs2);
    s2 = std::move(s1);
    REQUIRE(s2 == refs1);
    REQUIRE(s1 == "");

    string_view refn1 = "N1";
    string_view refn2 = "N2";
    PdfName n1 = refn1;
    REQUIRE(n1 == refn1);
    PdfName n2 = refn2;
    REQUIRE(n2 == refn2);
    n2 = std::move(n1);
    REQUIRE(n2 == refn1);
    REQUIRE(n1 == "");
}

TEST_CASE("TestAssignObjects")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("Hierarchies1.pdf"));

    {
        auto& annotsObj = doc.GetObjects().MustGetObject(PdfReference(100, 0));
        REQUIRE(!annotsObj.IsDirty());
        annotsObj.GetArray()[15] = PdfObject(nullptr);
        REQUIRE(annotsObj.IsDirty());

        auto& pageObj = doc.GetObjects().MustGetObject(PdfReference(39, 0));
        REQUIRE(!pageObj.IsDirty());
        auto& resx = pageObj.GetDictionary().MustGetKey("Contents");
        resx = PdfObject(nullptr);
        REQUIRE(pageObj.IsDirty());
    }

    auto outputFilePath = TestUtils::GetTestOutputFilePath("TestAssignObjects.pdf");
    doc.Save(outputFilePath);

    doc.Load(outputFilePath);
    {
        auto& annotsObj = doc.GetObjects().MustGetObject(PdfReference(100, 0));
        REQUIRE(annotsObj.GetArray()[15].IsNull());

        auto& pageObj = doc.GetObjects().MustGetObject(PdfReference(39, 0));
        auto& resx = pageObj.GetDictionary().MustGetKey("Contents");
        REQUIRE(resx.IsNull());
    }
}

TEST_CASE("TestObjectAdapter")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("blank.pdf"));
    const auto& info = doc.GetTrailer().GetDictionary().FindKeyAs<PdfDictionary>("Info");
    REQUIRE(info.GetKeyAs<PdfString>("Producer") == "PoDoFo - https://podofo.github.io/");
    REQUIRE(info.GetKeyAsSafe<PdfString>("Prod", "fallback") == "fallback");
    REQUIRE(info.FindKeyAs<PdfString>("Producer") == "PoDoFo - https://podofo.github.io/");
    REQUIRE(info.FindKeyAsSafe<PdfString>("Prod", "fallback") == "fallback");
    REQUIRE(info.FindKeyParentAs<PdfString>("Producer") == "PoDoFo - https://podofo.github.io/");
    REQUIRE(info.FindKeyParentAsSafe<PdfString>("Prod", "fallback") == "fallback");
}

TEST_CASE("TestGetDocKeywords")
{
    PdfMemDocument doc;
    vector<string> keywords = { "one", "two", "three" };
    doc.GetMetadata().SetKeywords(keywords);
    REQUIRE(keywords == doc.GetMetadata().GetKeywords());
}

TEST_CASE("DocumentMagicOffsetReset")
{
    // Normal PDF
    string_view pdf = R"(%PDF-2.0
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/Resources<<>>/MediaBox[0 0 9 9]>>endobj
xref
0 4
0000000000 65535 f 
0000000009 00000 n 
0000000052 00000 n 
0000000101 00000 n 
trailer<</Root 1 0 R/Size 4/ID[(1234567890123456)(1234567890123456)]>>
startxref
174
%%EOF)";
    
    // PDF with a 10-byte prefix
    string pdfWithOffset = string("OFFSET1234").append(pdf);

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    REQUIRE(doc.GetMagicOffset() == 0);

    doc.LoadFromBuffer(pdfWithOffset);
    REQUIRE(doc.GetMagicOffset() == 10);
    doc.Reset();
    REQUIRE(doc.GetMagicOffset() == 0);
}

TEST_CASE("TestIsDebugBuild")
{
    // The unit tests are always built with the same configuration of the library
#ifdef NDEBUG
    REQUIRE(!PdfCommon::IsDebugBuild());
#else // !NDEBUG
    REQUIRE(PdfCommon::IsDebugBuild());
#endif // NDEBUG
}

TEST_CASE("TestOutlineItemSettersSelf")
{
    // Setting the cached destination and action instances back
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    auto& item = doc.GetOrCreateOutlines().CreateRoot(PdfString("Root"));
    auto dest = doc.CreateDestination();
    dest->SetDestination(page);
    item.SetDestination(*dest);
    item.SetDestination(*item.GetDestination());
    REQUIRE(item.GetDestination()->GetPage() == &page);

    auto action = doc.CreateAction<PdfActionURI>();
    item.SetAction(*action);
    item.SetAction(*item.GetAction());
    REQUIRE(&item.GetAction()->GetObject() == &action->GetObject());
    REQUIRE(item.GetDestination() == nullptr);
}

TEST_CASE("TestOutlineEraseCreatedItems")
{
    // Created items know their parent, so erasing them unlinks them from it
    PdfMemDocument doc;
    auto& root = doc.GetOrCreateOutlines();
    auto& a = root.CreateChild(PdfString("A"));
    auto& b = a.CreateNext(PdfString("B"));
    REQUIRE(a.GetParentOutline() == &root);
    REQUIRE(b.GetParentOutline() == &root);
    REQUIRE(root.Last() == &b);
    a.Erase();
    REQUIRE(root.First() == &b);
    b.Erase();
    REQUIRE(root.First() == nullptr);
    REQUIRE(root.Last() == nullptr);
    auto& c = root.CreateChild(PdfString("C"));
    REQUIRE(root.First() == &c);
}

TEST_CASE("TestOutlineRootEraseRejected")
{
    // The outlines root is owned by the document and has no siblings
    PdfMemDocument doc;
    auto& root = doc.GetOrCreateOutlines();
    root.CreateChild(PdfString("A"));
    ASSERT_THROW_WITH_ERROR_CODE(root.Erase(), PdfErrorCode::UnsupportedOperation);
    ASSERT_THROW_WITH_ERROR_CODE(root.CreateNext(PdfString("B")), PdfErrorCode::UnsupportedOperation);
    REQUIRE(doc.GetOutlines() == &root);
    REQUIRE(root.First() != nullptr);
    REQUIRE(root.First()->Next() == nullptr);
}

TEST_CASE("TestOutlineManySiblings")
{
    // The siblings are more than the default recursion limit
    PdfMemDocument doc;
    doc.GetPages().CreatePage(PdfPageSize::A4);
    auto& root = doc.GetOrCreateOutlines();
    for (unsigned i = 0; i < 300; i++)
        root.CreateChild(PdfString(std::to_string(i)));

    charbuff buffer;
    StringStreamDevice device(buffer);
    doc.Save(device);

    PdfMemDocument loaded;
    loaded.LoadFromBuffer(buffer);
    auto& loadedRoot = loaded.MustGetOutlines();
    unsigned count = 0;
    for (auto item = loadedRoot.First(); item != nullptr; item = item->Next())
        count++;

    REQUIRE(count == 300);
    REQUIRE(loadedRoot.Last()->GetTitle().GetString() == "299");
}

TEST_CASE("TestOutlineParsedChainLast")
{
    // The tail of the parsed children chain must be known to append after it
    constexpr string_view pdf = R"(%PDF-1.4
1 0 obj<</Type/Catalog/Pages 2 0 R/Outlines 4 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</Type/Outlines/First 5 0 R/Last 6 0 R/Count 2>>endobj
5 0 obj<</Title(A)/Parent 4 0 R/Next 6 0 R>>endobj
6 0 obj<</Title(B)/Parent 4 0 R/Prev 5 0 R>>endobj
trailer<</Root 1 0 R/Size 7>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& root = doc.MustGetOutlines();
    auto first = root.First();
    REQUIRE(first != nullptr);
    REQUIRE(first->GetTitle().GetString() == "A");
    REQUIRE(root.Last() == first->Next());
    auto& c = root.CreateChild(PdfString("C"));
    REQUIRE(root.Last() == &c);
    REQUIRE(first->Next()->Next() == &c);
    first->Erase();
    REQUIRE(root.First()->GetTitle().GetString() == "B");
}

TEST_CASE("TestOutlineGettersWithoutValue")
{
    // Items without destination or action report none
    PdfMemDocument doc;
    auto& item = doc.GetOrCreateOutlines().CreateChild(PdfString("A"));
    REQUIRE(item.GetDestination() == nullptr);
    REQUIRE(item.GetAction() == nullptr);
    auto dest = doc.CreateDestination();
    dest->SetDestination(doc.GetPages().CreatePage(PdfPageSize::A4));
    item.SetDestination(*dest);
    REQUIRE(item.GetDestination() != nullptr);
    REQUIRE(item.GetAction() == nullptr);
}

TEST_CASE("TestRichMediaExecuteAction")
{
    // Every action type has a name in the /S table
    PdfMemDocument doc;
    auto action = doc.CreateAction(PdfActionType::RichMediaExecute);
    REQUIRE(action->GetDictionary().MustGetKey("S").GetName().GetString() == "RichMediaExecute");
}

TEST_CASE("TestFileSpecFromObject")
{
    // The /Type check must read a name and reject other values without crashing
    PdfMemDocument doc;
    auto& filespecObj = doc.GetObjects().CreateDictionaryObject();
    filespecObj.GetDictionary().AddKey("Type"_n, PdfName("Filespec"));
    unique_ptr<PdfFileSpec> filespec;
    REQUIRE(PdfFileSpec::TryCreateFromObject(filespecObj, filespec));
    REQUIRE(filespec != nullptr);

    auto& numberTypeObj = doc.GetObjects().CreateDictionaryObject();
    numberTypeObj.GetDictionary().AddKey("Type"_n, PdfObject(static_cast<int64_t>(5)));
    REQUIRE(!PdfFileSpec::TryCreateFromObject(numberTypeObj, filespec));
    REQUIRE(filespec == nullptr);

    auto& otherTypeObj = doc.GetObjects().CreateDictionaryObject();
    otherTypeObj.GetDictionary().AddKey("Type"_n, PdfName("Other"));
    REQUIRE(!PdfFileSpec::TryCreateFromObject(otherTypeObj, filespec));
}

TEST_CASE("TestFormatNumberPrecisionZero")
{
    // With precision 0 there's no decimal point: nothing is trimmed and no byte before the buffer is read
    PdfStringStream stream;
    stream.SetPrecision(0);
    stream << 0.3;
    REQUIRE(stream.GetString() == "0");
    stream.Clear();
    stream << 10.0;
    REQUIRE(stream.GetString() == "10");
    stream.SetPrecision(2);
    stream.Clear();
    stream << 1.5;
    REQUIRE(stream.GetString() == "1.5");
}

TEST_CASE("TestFormatLargeNumbers")
{
    // The output fits the longest number of each type
    string str;
    utls::FormatTo(str, numeric_limits<int>::max());
    REQUIRE(str == "2147483647");
    utls::FormatTo(str, numeric_limits<long long>::min());
    REQUIRE(str == "-9223372036854775808");
    utls::FormatTo(str, 1e8f, 6);
    REQUIRE(str == "100000000");
    utls::FormatTo(str, 1e20, 6);
    REQUIRE(str == "100000000000000000000");
    utls::FormatTo(str, -numeric_limits<double>::max(), 2);
    REQUIRE(str.size() == 310);
    REQUIRE(str.substr(0, 18) == "-17976931348623157");
}

TEST_CASE("TestErrorExternalFilePath")
{
    // Frames from outside the library keep their path, whatever its length
    PdfError shortError(PdfErrorCode::InvalidHandle, "x.cpp", 1, "Some info");
    REQUIRE(string_view(shortError.what()).find("Some info") != string_view::npos);
    REQUIRE(shortError.GetCallStack().front().GetFilePath() == "x.cpp");
    string longPath = "/external/" + string(300, 'a') + ".cpp";
    PdfError longError(PdfErrorCode::InvalidHandle, longPath, 1, "Some info");
    REQUIRE(longError.GetCallStack().front().GetFilePath() == longPath);
}

TEST_CASE("TestOutlineAliasedDestsRetyped")
{
    // The PDF 1.1 /Dests table aliases the item dictionary: a setter writing
    // the key the named destination selects retypes the cached array
    constexpr string_view pdf = R"(%PDF-1.4
1 0 obj<</Type/Catalog/Pages 2 0 R/Outlines 4 0 R/Dests 5 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</Type/Outlines/First 5 0 R/Last 5 0 R/Count 1>>endobj
5 0 obj<</Title(A)/Parent 4 0 R/Dest/F/F[3 0 R/Fit]>>endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto item = doc.MustGetOutlines().First();
    REQUIRE(item != nullptr);
    REQUIRE(item->GetDestination()->GetType() == PdfDestinationType::Fit);
    item->SetTextFormat(PdfOutlineFormat::Bold);
    REQUIRE(item->GetDestination() == nullptr);
}

TEST_CASE("TestOutlineDestinationAfterNameTreeInsertion")
{
    // The named destination is a direct value in the /Names array: an insertion
    // through the typed name tree API relocates it
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/Names 5 0 R/Outlines 6 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</Names[(Foo)[3 0 R/Fit]]>>endobj
5 0 obj<</Dests 4 0 R>>endobj
6 0 obj<</Type/Outlines/First 7 0 R/Last 7 0 R/Count 1>>endobj
7 0 obj<</Title(A)/Parent 6 0 R/Dest(Foo)>>endobj
trailer<</Root 1 0 R/Size 8>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& item = *doc.MustGetOutlines().First();
    REQUIRE(item.GetDestination()->GetType() == PdfDestinationType::Fit);
    shared_ptr<PdfDestination> bar = doc.CreateDestination();
    bar->SetDestination(doc.GetPages().GetPageAt(0));
    doc.GetNames()->MustGetTree<PdfDestinations>().AddValue(PdfString("Bar"), bar);
    REQUIRE(item.GetDestination()->GetType() == PdfDestinationType::Fit);
}

TEST_CASE("TestOutlineDestinationAfterGarbageCollection")
{
    // The cache must not wrap the caller's unreferenced destination object
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    auto& item = doc.GetOrCreateOutlines().CreateChild(PdfString("A"));
    auto dest = doc.CreateDestination();
    dest->SetDestination(page);
    item.SetDestination(*dest);
    REQUIRE(&item.GetDestination()->GetObject() != &dest->GetObject());
    doc.CollectGarbage();
    REQUIRE(item.GetDestination()->GetPage() == &page);
}

TEST_CASE("TestOutlineCreateRootWithoutDestination")
{
    // Only the outlines and the item objects are created, with no destination
    PdfMemDocument doc;
    unsigned count = doc.GetObjects().GetSize();
    auto& root = doc.GetOrCreateOutlines().CreateRoot(PdfString("Root"));
    REQUIRE(root.GetDestination() == nullptr);
    REQUIRE(!root.GetDictionary().HasKey("Dest"));
    REQUIRE(doc.GetObjects().GetSize() == count + 2);
}
