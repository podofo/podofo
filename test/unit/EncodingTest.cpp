/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

#include <ostream>
#include <iostream>

using namespace std;
using namespace PoDoFo;

static void outofRangeHelper(PdfEncoding& encoding);

namespace PoDoFo
{
    class PdfEncodingTest
    {
    public:
        static void TestToUnicodeParse();
        static void TestDifferencesObject();
    };
}

METHOD_AS_TEST_CASE(PdfEncodingTest::TestToUnicodeParse, "TestToUnicodeParse")
METHOD_AS_TEST_CASE(PdfEncodingTest::TestDifferencesObject, "TestDifferencesObject")

TEST_CASE("TestDifferences")
{
    PdfDifferenceMap difference;

    // Newly created encoding should be empty
    REQUIRE(difference.GetCount() == 0);

    // Adding 0 should work
    difference.AddDifference(0, u'A');
    REQUIRE(difference.GetCount() == 1);

    // Adding 255 should work
    difference.AddDifference(255, u'B');
    REQUIRE(difference.GetCount() == 2);

    // Convert to array
    PdfArray data;
    PdfArray expected;
    expected.Add(static_cast<int64_t>(0));
    expected.Add(PdfName("A"));
    expected.Add(static_cast<int64_t>(255));
    expected.Add(PdfName("B"));

    difference.ToArray(data);

    REQUIRE(expected.GetSize() == data.GetSize());
    for (unsigned int i = 0; i < data.GetSize(); i++)
        REQUIRE(expected[i] == data[i]);


    // Test replace
    expected.Clear();
    expected.Add(static_cast<int64_t>(0));
    expected.Add(PdfName("A"));
    expected.Add(static_cast<int64_t>(255));
    expected.Add(PdfName("X"));

    difference.AddDifference(255, u'X');

    difference.ToArray(data);

    REQUIRE(expected.GetSize() == data.GetSize());
    for (unsigned int i = 0; i < data.GetSize(); i++)
        REQUIRE(expected[i] == data[i]);


    // Test more complicated array
    expected.Clear();
    expected.Add(static_cast<int64_t>(0));
    expected.Add(PdfName("A"));
    expected.Add(PdfName("B"));
    expected.Add(PdfName("C"));
    expected.Add(static_cast<int64_t>(4));
    expected.Add(PdfName("D"));
    expected.Add(PdfName("E"));
    expected.Add(static_cast<int64_t>(9));
    expected.Add(PdfName("F"));
    expected.Add(static_cast<int64_t>(255));
    expected.Add(PdfName("X"));

    difference.AddDifference(1, u'B');
    difference.AddDifference(2, u'C');
    difference.AddDifference(4, u'D');
    difference.AddDifference(5, u'E');
    difference.AddDifference(9, u'F');

    difference.ToArray(data);

    REQUIRE(expected.GetSize() == data.GetSize());
    for (unsigned int i = 0; i < data.GetSize(); i++)
        REQUIRE(expected[i] == data[i]);

    // Test if contains works correctly
    const PdfName* name;
    CodePointSpan codepoints;
    REQUIRE(difference.TryGetMappedName(0, name, codepoints));
    REQUIRE(*name == "A");
    REQUIRE(static_cast<int>(*codepoints) == 0x41);

    REQUIRE(difference.TryGetMappedName(9, name, codepoints));
    REQUIRE(*name == "F");
    REQUIRE(static_cast<int>(*codepoints) == 0x46);

    REQUIRE(difference.TryGetMappedName(255, name, codepoints));
    REQUIRE(*name  == "X");
    REQUIRE(static_cast<int>(*codepoints) == 0x58);

    REQUIRE(!difference.TryGetMappedName(100, name, codepoints));
}

void PdfEncodingTest::TestDifferencesObject()
{
    PdfDifferenceMap differences;
    differences.AddDifference(1, 'B');
    differences.AddDifference(2, 'C');
    differences.AddDifference(4, 'D');
    differences.AddDifference(5, 'E');
    differences.AddDifference(9, 'F');

    PdfDifferenceEncoding encoding(PdfEncodingMapFactory::GetMacRomanEncodingInstancePtr(), std::move(differences));

    // Check for encoding key
    PdfMemDocument doc;
    PdfName name;
    PdfObject* encodingObj;
    REQUIRE(encoding.TryGetExportObject(doc.GetObjects(), name, encodingObj));
    REQUIRE(encodingObj != nullptr);

    // Test BaseEncoding
    PdfObject* baseObj = encodingObj->GetDictionary().GetKey("BaseEncoding");
    REQUIRE(baseObj->GetName() == "MacRomanEncoding");

    // Test differences
    PdfObject* diff = encodingObj->GetDictionary().GetKey("Differences");
    PdfArray expected;

    expected.Add(static_cast<int64_t>(1));
    expected.Add(PdfName("B"));
    expected.Add(PdfName("C"));
    expected.Add(static_cast<int64_t>(4));
    expected.Add(PdfName("D"));
    expected.Add(PdfName("E"));
    expected.Add(static_cast<int64_t>(9));
    expected.Add(PdfName("F"));

    const PdfArray& data = diff->GetArray();
    REQUIRE(expected.GetSize() == data.GetSize());
    for (unsigned int i = 0; i < data.GetSize(); i++)
        REQUIRE(expected[i] == data[i]);
}

TEST_CASE("TestDifferencesEncoding")
{
    // Create a differences encoding where A and B are exchanged
    PdfDifferenceMap differences;
    differences.AddDifference((unsigned char)'A', 'B');
    differences.AddDifference((unsigned char)'B', 'A');
    differences.AddDifference((unsigned char)'C', 'D');

    PdfMemDocument doc;

    PdfFontCreateParams params;
    params.Encoding = PdfEncoding(std::make_shared<PdfDifferenceEncoding>(PdfEncodingMapFactory::GetWinAnsiEncodingInstancePtr(), std::move(differences)));
    auto& font = doc.GetFonts().GetStandard14Font(PdfStandard14FontType::Helvetica, params);

    charbuff encoded;
    INFO("'C' in \"BAABC\" is already reserved for mapping in 'D'");
    REQUIRE(!font.GetEncoding().TryConvertToEncoded("BAABC", encoded));
    encoded = font.GetEncoding().ConvertToEncoded("BAABI");
    REQUIRE(encoded == "ABBAI");
    auto unicode = params.Encoding.ConvertToUtf8(PdfString::FromRaw(encoded));
    REQUIRE(unicode == "BAABI");
}

TEST_CASE("TestGetCharCode")
{
    auto winAnsiEncoding = PdfEncodingFactory::CreateWinAnsiEncoding();
    INFO("WinAnsiEncoding");
    outofRangeHelper(winAnsiEncoding);

    auto macRomanEncoding = PdfEncodingFactory::CreateMacRomanEncoding();
    INFO("MacRomanEncoding");
    outofRangeHelper(macRomanEncoding);

    PdfDifferenceMap differences;
    differences.AddDifference((unsigned char)'A', 'B');
    differences.AddDifference((unsigned char)'B', 'A');
    PdfEncoding differenceEncoding(std::make_shared<PdfDifferenceEncoding>(PdfEncodingMapFactory::GetWinAnsiEncodingInstancePtr(), std::move(differences)));
    outofRangeHelper(differenceEncoding);
}

TEST_CASE("CMapIdentityTest")
{
    constexpr string_view OneByteIdentity = R"(
/CIDInit /ProcSet findresource begin
12 dict begin
begincmap
/CIDSystemInfo 3 dict dup begin
/Registry (Adobe) def
/Ordering (Identity) def
/Supplement 0 def
end def
/CMapName /OneByteIdentityH def
/CMapVersion 1.000 def
/CMapType 1 def
/UIDOffset 0 def
/XUID [1 10 25404 9999] def
/WMode 0 def
1 begincodespacerange
<00> <FF>
endcodespacerange
1 begincidrange
<00> <FF> 0
endcidrange
endcmap
CMapName currentdict /CMap defineresource pop
end
end
)";

    SpanStreamDevice device(OneByteIdentity);
    auto map = PdfCMapEncoding::Parse(device);
    REQUIRE(map.GetCharMap().IsTrivialIdentity());
}

void PdfEncodingTest::TestToUnicodeParse()
{
    string_view toUnicode =
        "3 beginbfrange\n"
        "<0001> <0004> <1001>\n"
        "<0005> <000A> [<000A> <0009> <0008> <0007> <0006> <0005>]\n"
        "<000B> <000F> <100B>\n"
        "endbfrange\n";
    charbuff encodedStr("\x00\x01\x00\x02\x00\x03\x00\x04\x00\x05\x00\x06\x00\x07\x00\x08\x00\x09\x00\x0A\x00\x0B\x00\x0C\x00\x0D\x00\x0E\x00\x0F"sv);
    string expected("\xE1\x80\x81\xE1\x80\x82\xE1\x80\x83\xE1\x80\x84\x0A\x09\x08\x07\x06\x05\xE1\x80\x8B\xE1\x80\x8C\xE1\x80\x8D\xE1\x80\x8E\xE1\x80\x8F"sv);

    PdfMemDocument doc;
    auto& toUnicodeObj = doc.GetObjects().CreateDictionaryObject();
    toUnicodeObj.GetOrCreateStream().SetData(toUnicode);

    PdfEncoding encoding(PdfEncodingMapConstPtr(new PdfIdentityEncoding(PdfEncodingMapType::Indeterminate, 2)), PdfEncodingMapFactory::ParseCMapEncoding(toUnicodeObj));

    auto utf8str = encoding.ConvertToUtf8(PdfString::FromRaw(encodedStr));
    REQUIRE(utf8str == expected);

    const char* toUnicodeInvalidTests[] =
    {
        // missing object numbers
        "beginbfrange\n",
        "beginbfchar\n",

        // invalid hex digits
        "2 beginbfrange <WXYZ> endbfrange\n",
        "2 beginbfrange <-123> endbfrange\n",
        "2 beginbfrange <<00>> endbfrange\n",

        // missing hex digits
        "2 beginbfrange <> endbfrange\n",

        // empty array
        "2 beginbfrange [] endbfrange\n",

        nullptr
    };

    for (size_t i = 0; toUnicodeInvalidTests[i] != nullptr; i++)
    {
        try
        {
            PdfIndirectObjectList invalidList;
            auto& invalidObject = invalidList.CreateDictionaryObject();
            invalidObject.GetOrCreateStream().SetData(bufferview(toUnicodeInvalidTests[i], char_traits<char>::length(toUnicodeInvalidTests[i])));

            PdfEncoding encodingTestInvalid(PdfEncodingMapConstPtr(new PdfIdentityEncoding(PdfEncodingMapType::Indeterminate, 2)), PdfEncodingMapFactory::ParseCMapEncoding(invalidObject));

            auto unicodeStringTestInvalid = encodingTestInvalid.ConvertToUtf8(PdfString::FromRaw(encodedStr));

            // exception not thrown - should never get here
            // TODO not all invalid input throws an exception (e.g. no hex digits in <WXYZ>)
            FAIL();
        }
        catch (PdfError&)
        {
            // parsing every invalid test string should throw an exception
        }
        catch (exception&)
        {
            FAIL("Unexpected exception type");
        }
    }
}

void outofRangeHelper(PdfEncoding& encoding)
{
    (void)encoding.GetCodePoint(encoding.GetFirstChar());
    REQUIRE(encoding.GetCodePoint(encoding.GetFirstChar().Code - 1) == U'\0');
    (void)encoding.GetCodePoint(encoding.GetLastChar());
    REQUIRE(encoding.GetCodePoint(encoding.GetLastChar().Code + 1) == U'\0');
}
