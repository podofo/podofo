/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

using namespace PoDoFo;
using namespace std;

void TestEscapedName(const string_view& nameStr, const string_view& expectedEncoded);
void TestEncodedName(const string_view& pszString, const string_view& pszExpected);
void TestNameEquality(const string_view& name1, const string_view& name2);
void TestNameWrite(const string_view& view, const string_view& expected);
void TestFromEscape(const string_view& name1, const string_view& name2);

TEST_CASE("TestParseAndWrite")
{
    const char* data = "/#E5#8A#A8#E6#80#81#E8#BF#9E#E6#8E#A5#E7#BA#BF";
    auto device = std::make_shared<SpanStreamDevice>(data);
    PdfTokenizer tokenizer;

    string_view token;
    PdfTokenType type;
    bool gotToken = tokenizer.TryReadNextToken(*device, token, type);

    REQUIRE(gotToken);
    REQUIRE(type == PdfTokenType::Slash);

    gotToken = tokenizer.TryReadNextToken(*device, token, type);

    REQUIRE(gotToken);
    REQUIRE(type == PdfTokenType::Literal);

    // Test with const char* constructor
    PdfName name = PdfName::FromEscaped(token);
    PdfVariant var(name);
    string str;
    var.ToString(str);

    REQUIRE(str == data);
    // str.c_str() + 1 <- ignore leading slash 
    REQUIRE(name.GetEscapedName() == (string_view)str.substr(1));

    // Test with string constructor
    PdfName name2 = PdfName::FromEscaped(token);
    PdfVariant var2(name);
    string str2;
    var.ToString(str2);

    REQUIRE(str2 == data);
    // str.c_str() + 1 <- ignore leading slash 
    REQUIRE(name2.GetEscapedName() == (string_view)str2.substr(1));
}

TEST_CASE("TestNameEncoding")
{
    // Test some names. The first argument is the unencoded representation, the second
    // is the expected encoded result. The result must not only be /a/ correct encoded
    // name for the unencoded form, but must be the exact one PoDoFo should produce.
    TestEscapedName("Length With Spaces", "Length#20With#20Spaces");
    TestEscapedName("Length\001\002\003Spaces", "Length#01#02#03Spaces");
    TestEscapedName("Length#01#02#03Spaces#7F", "Length#2301#2302#2303Spaces#237F");
    TestEscapedName("Tab\tTest", "Tab#09Test");
}

TEST_CASE("TestEncodedNames")
{
    // Test some pre-encoded names. The first argument is the encoded name that'll be
    // read from the PDF; the second is the expected representation.
    TestEncodedName("PANTONE#205757#20CV", "PANTONE 5757 CV");
    TestEncodedName("paired#28#29parentheses", "paired()parentheses");
    TestEncodedName("The_Key_of_F#23_Minor", "The_Key_of_F#_Minor");
    TestEncodedName("A#42", "AB");
    TestEncodedName("ANPA#20723-0#20AdPro", "ANPA 723-0 AdPro");
}

TEST_CASE("TestEquality")
{
    // Make sure differently encoded names compare equal if their decoded values
    // are equal.
    TestNameEquality("With Spaces", "With#20Spaces");
    TestNameEquality("#57#69#74#68#20#53#70#61#63#65#73", "With#20Spaces");
}

TEST_CASE("TestWrite")
{
    // Make sure all names are written correctly to an output device!
    TestNameWrite("Length With Spaces", "/Length#20With#20Spaces");
    TestNameWrite("Length\001\002\003Spaces", "/Length#01#02#03Spaces");
    TestNameWrite("Tab\tTest", "/Tab#09Test");
    TestNameWrite("ANPA 723-0 AdPro", "/ANPA#20723-0#20AdPro");
}

TEST_CASE("TestFromEscaped")
{
    TestFromEscape("ANPA#20723-0#20AdPro", "ANPA 723-0 AdPro");
    TestFromEscape("Length#20With#20Spaces", "Length With Spaces");
}

//
// Test encoding of names.
// pszString : internal representation, ie unencoded name
// pszExpectedEncoded: the encoded string PoDoFo should produce
//
void TestEscapedName(const string_view& nameStr, const string_view& expectedEncoded)
{
    INFO(utls::Format("Testing name: {}", nameStr));

    PdfName name(nameStr);
    INFO(utls::Format("   -> Expected   Value: {}", expectedEncoded));
    INFO(utls::Format("   -> Got        Value: {}", name.GetEscapedName()));
    INFO(utls::Format("   -> Unescaped  Value: {}", name.GetString()));

    REQUIRE(name.GetEscapedName() == expectedEncoded);

    // Ensure the encoded string compares equal to its unencoded
    // variant
    REQUIRE(PdfName::FromEscaped(expectedEncoded) == name);
}

void TestEncodedName(const string_view& escaped, const string_view& expected)
{
    PdfName name(PdfName::FromEscaped(escaped));
    INFO(utls::Format("Testing encoded name: {}", escaped));
    INFO(utls::Format("   -> Expected   Value: {}", expected));
    INFO(utls::Format("   -> Got        Value: {}", name.GetString()));
    INFO(utls::Format("   -> Escaped    Value: {}", name.GetEscapedName()));

    REQUIRE(name == expected);

    // Ensure the name compares equal with one constructed from the
    // expected unescaped form
    REQUIRE(PdfName(expected) == name);
}

void TestNameEquality(const string_view& name1Str, const string_view& name2Str)
{
    PdfName name1(PdfName::FromEscaped(name1Str));
    PdfName name2(PdfName::FromEscaped(name2Str));

    INFO(utls::Format("Testing equality of encoded names '{}' and '{}'", name1Str, name2Str));
    INFO(utls::Format("   -> Name1    Decoded Value: {}", name1.GetString()));
    INFO(utls::Format("   -> Name2    Decoded Value: {}", name2.GetString()));

    REQUIRE(name1 == name2); // use operator==
    REQUIRE(!(name1 != name2)); // use operator!=
}

void TestNameWrite(const string_view& view, const string_view& expected)
{
    PdfName name(view);
    REQUIRE(name.ToString() == expected);
}

void TestFromEscape(const string_view& name1, const string_view& name2)
{
    PdfName name = PdfName::FromEscaped(name1);
    REQUIRE(name == name2);
}
