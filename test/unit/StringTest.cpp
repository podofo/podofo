/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

static void TestWriteEscapeSequences(const string_view& str, const string_view& expected);

TEST_CASE("TestEscapeAllCharacters")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestEscapeAllCharacters.pdf"));

    // NOTE: Escaped new line '\n', '\r' characters are ignored
    constexpr string_view VRefString = "\1\2\3\4\5\6\7\10\11\13\14\16\17\20\21\22\23\24\25\26\27\30\31\32\33\34\35\36\37\40\41\42\43\44\45\46\47\50\51\52\53\54\55\56\57\1\2\3\4\5\6\7\70\71\72\73\74\75\76\77\100\101\102\103\104\105\106\107\110\111\112\113\114\115\116\117\120\121\122\123\124\125\126\127\130\131\132\133\134\135\136\137\140\141\b\143\144\145\f\147\150\151\152\153\154\155\n\157\160\161\r\163\t\165\166\167\170\171\172\173\174\175\176\177\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377"sv;

    constexpr string_view TestRefString = "\1\2\3\4\5\6\7\10\11\12\13\14\15\16\17\20\21\22\23\24\25\26\27\30\31\32\33\34\35\36\37\40\41\42\43\44\45\46\47\50\51\52\53\54\55\56\57\1\2\3\4\5\6\7\70\71\72\73\74\75\76\77\100\101\102\103\104\105\106\107\110\111\112\113\114\115\116\117\120\121\122\123\124\125\126\127\130\131\132\133\134\135\136\137\140\141\b\143\144\145\f\147\150\151\152\153\154\155\n\157\160\161\r\163\t\165\166\167\170\171\172\173\174\175\176\177\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377"sv;

    auto& obj = doc.GetObjects().MustGetObject(PdfReference(5, 0));

    REQUIRE(obj.GetDictionary().FindKeyAs<PdfString>("V").GetRawData() == VRefString);
    REQUIRE(obj.GetDictionary().FindKeyAs<PdfString>("Test").GetRawData() == TestRefString);
}

TEST_CASE("TestEncryptedStringsEscaped")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestEncryptedStringsEscaped.pdf"), "userpass");

    REQUIRE(*doc.GetMetadata().GetTitle() == "Test title");

    // This has a escaped `\r` character that shall be ignored. This currently works in Pdf.js but
    // not on Adobe. It's a rare edge case anyway
    REQUIRE(doc.GetMetadata().GetModifyDate()->ToString() == "D:20250403231507+02'00'");

    doc.Load(TestUtils::GetTestInputFilePath("TestEncryptedStringsEscaped2.pdf"), "userpass");

    // Next title has a escaped `\0`
    REQUIRE(*doc.GetMetadata().GetTitle() == "Test title 2");

    // Next producer has a escaped `\n` character that shall be ignored. This
    // works also in Adobe
    REQUIRE(*doc.GetMetadata().GetProducer() == "PoDoFo - https://github.com/podofo/podofo");
}

TEST_CASE("TestStringUtf8")
{
    string_view str = "Hallo PoDoFo!";
    REQUIRE(PdfString(str) == str);

    string_view stringJapUtf8 = "「PoDoFo」は今から日本語も話せます。";
    REQUIRE(PdfString(stringJapUtf8) == stringJapUtf8);
}

TEST_CASE("TestPdfDocEncoding")
{
    const string src = "This string contains PdfDocEncoding Characters: ÄÖÜ";
    string_view ref = "(This string contains PdfDocEncoding Characters: \304\326\334)"sv;
    // Normal ascii string should be converted to UTF8
    PdfString str(src);
    REQUIRE(str == src);
    REQUIRE(str.GetCharset() == PdfStringCharset::PdfDocEncoding);

    // Serialize the string
    string serialized;
    str.ToString(serialized);
    REQUIRE(serialized == ref);

    // Deserialize the string (remove the surrounding parenthesis '(' ')' )
    str = PdfString::FromRaw(serialized.substr(1, serialized.length() - 2));
    REQUIRE(str.GetString() == src);
}

TEST_CASE("TestEscapeBrackets")
{
    // Test balanced brackets ansi
    string_view balanced = "Hello (balanced) World";
    string_view balancedExpect = "(Hello \\(balanced\\) World)";

    PdfString pdfStrAscii(balanced);
    PdfVariant varAscii(pdfStrAscii);
    string strAscii;
    varAscii.ToString(strAscii);

    REQUIRE(pdfStrAscii.GetCharset() == PdfStringCharset::Ascii);
    REQUIRE(strAscii == balancedExpect);

    // Test un-balanced brackets ansi
    string_view unbalanced = "Hello ((unbalanced World";
    string_view unbalancedExpect = "(Hello \\(\\(unbalanced World)";

    PdfString pdfStrAscii2(unbalanced);
    PdfVariant varAscii2(pdfStrAscii2);
    string strAscii2;
    varAscii2.ToString(strAscii2);

    REQUIRE(strAscii2 == unbalancedExpect);

    string_view utf16HexStr =
        "<FEFF00480065006C006C006F0020002800280075006E00620061006C0061006E00630065006400200057006F0072006C00640029>";

    string_view utf16Expected = "Hello ((unbalanced World)";
    // Test reading the unicode string back in
    PdfVariant varRead;
    PdfTokenizer tokenizer;
    SpanStreamDevice input(utf16HexStr);
    (void)tokenizer.ReadNextVariant(input, varRead);
    REQUIRE(varRead.GetDataType() == PdfDataType::String);
    REQUIRE(varRead.GetString() == utf16Expected);
}

TEST_CASE("TestWriteEscapeSequences")
{
    TestWriteEscapeSequences("(1Hello\\nWorld)", "(1Hello\\nWorld)");
    TestWriteEscapeSequences("(Hello\nWorld)", "(Hello\\nWorld)");
    TestWriteEscapeSequences("(Hello\012World)", "(Hello\\nWorld)");
    TestWriteEscapeSequences("(Hello\\012World)", "(Hello\\nWorld)");

    TestWriteEscapeSequences("(2Hello\\rWorld)", "(2Hello\\rWorld)");
    TestWriteEscapeSequences("(Hello\rWorld)", "(Hello\\rWorld)");
    TestWriteEscapeSequences("(Hello\015World)", "(Hello\\rWorld)");
    TestWriteEscapeSequences("(Hello\\015World)", "(Hello\\rWorld)");

    TestWriteEscapeSequences("(3Hello\\tWorld)", "(3Hello\\tWorld)");
    TestWriteEscapeSequences("(Hello\tWorld)", "(Hello\\tWorld)");
    TestWriteEscapeSequences("(Hello\011World)", "(Hello\\tWorld)");
    TestWriteEscapeSequences("(Hello\\011World)", "(Hello\\tWorld)");

    TestWriteEscapeSequences("(4Hello\\fWorld)", "(4Hello\\fWorld)");
    TestWriteEscapeSequences("(Hello\fWorld)", "(Hello\\fWorld)");
    TestWriteEscapeSequences("(Hello\014World)", "(Hello\\fWorld)");
    TestWriteEscapeSequences("(Hello\\014World)", "(Hello\\fWorld)");

    TestWriteEscapeSequences("(5Hello\\(World)", "(5Hello\\(World)");
    TestWriteEscapeSequences("(Hello\\050World)", "(Hello\\(World)");

    TestWriteEscapeSequences("(6Hello\\)World)", "(6Hello\\)World)");
    TestWriteEscapeSequences("(Hello\\051World)", "(Hello\\)World)");

    TestWriteEscapeSequences("(7Hello\\\\World)", "(7Hello\\\\World)");
    TestWriteEscapeSequences("(Hello\\\134World)", "(Hello\\\\World)");

    // Special case, \ at end of line
    TestWriteEscapeSequences("(8Hello\\\nWorld)", "(8HelloWorld)");


    TestWriteEscapeSequences("(9Hello\003World)", "(9Hello\003World)");
}

TEST_CASE("TestEmptyString")
{
    const char* empty = "";
    string strEmpty;
    string strEmpty2(empty);

    PdfString str1;
    PdfString str2(strEmpty);
    PdfString str3(strEmpty2);
    PdfString str4(empty);

    REQUIRE(str1.GetString().length() == 0u);
    REQUIRE(str1.GetString() == strEmpty);
    REQUIRE(str1.GetString() == strEmpty2);

    REQUIRE(str2.GetString().length() == 0u);
    REQUIRE(str2.GetString() == strEmpty);
    REQUIRE(str2.GetString() == strEmpty2);

    REQUIRE(str3.GetString().length() == 0u);
    REQUIRE(str3.GetString() == strEmpty);
    REQUIRE(str3.GetString() == strEmpty2);

    REQUIRE(str4.GetString().length() == 0u);
    REQUIRE(str4.GetString() == strEmpty);
    REQUIRE(str4.GetString() == strEmpty2);
}

TEST_CASE("TestInitFromUtf8")
{
    string_view utf8 = "This string contains non PdfDocEncoding Characters: ЙКЛМ";
    string_view ref = "(\376\377\0\124\0\150\0\151\0\163\0\40\0\163\0\164\0\162\0\151\0\156\0\147\0\40\0\143\0\157\0\156\0\164\0\141\0\151\0\156\0\163\0\40\0\156\0\157\0\156\0\40\0\120\0\144\0\146\0\104\0\157\0\143\0\105\0\156\0\143\0\157\0\144\0\151\0\156\0\147\0\40\0\103\0\150\0\141\0\162\0\141\0\143\0\164\0\145\0\162\0\163\0\72\0\40\4\31\4\32\4\33\4\34)"sv;

    const PdfString str(utf8);

    REQUIRE(str.GetCharset() == PdfStringCharset::Unicode);
    string serialized;
    str.ToString(serialized);
    REQUIRE(serialized == ref);
    REQUIRE(str.GetString().length() == utf8.length());
    REQUIRE(str.GetString() == string(utf8));
}

void TestWriteEscapeSequences(const string_view& str, const string_view& expected)
{
    PdfVariant variant;
    string ret;

    INFO(utls::Format("Testing with value: {}", str));
    PdfPostScriptTokenizer tokenizer;
    SpanStreamDevice device(str);

    tokenizer.TryReadNextVariant(device, variant);
    REQUIRE(variant.GetDataType() == PdfDataType::String);

    variant.ToString(ret);
    INFO(utls::Format("   -> Convert To String: {}", ret));

    REQUIRE(expected == ret);
}
