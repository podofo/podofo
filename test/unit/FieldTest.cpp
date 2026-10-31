// SPDX-FileCopyrightText: 2026 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

TEST_CASE("TestFieldsDuplicateReferences")
{
    // The same field is referenced twice in /Fields and in /Kids: the duplicate
    // is not wrapped and the second removal must not erase a reference already
    // gone from the map
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/AcroForm<</Fields[4 0 R 4 0 R]>>>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</FT/Tx/T(Parent)/Kids[5 0 R 5 0 R]>>endobj
5 0 obj<</Parent 4 0 R/T(Kid)>>endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& form = *doc.GetAcroForm();
    REQUIRE(form.GetFieldCount() == 2);
    ASSERT_THROW_WITH_ERROR_CODE(form.GetFieldAt(1), PdfErrorCode::InvalidHandle);

    auto& children = form.GetFieldAt(0).GetChildren();
    REQUIRE(children.GetCount() == 2);
    ASSERT_THROW_WITH_ERROR_CODE(children.GetFieldAt(1), PdfErrorCode::InvalidHandle);
    children.RemoveFieldAt(0);
    children.RemoveFieldAt(0);
    REQUIRE(children.GetCount() == 0);

    form.RemoveFieldAt(0);
    form.RemoveFieldAt(0);
    REQUIRE(form.GetFieldCount() == 0);
}

TEST_CASE("TestFieldsNonDictionaryEntries")
{
    // A number and a dangling reference in /Fields are placeholders, a non numeric
    // /Ff is ignored: none of them must leave the form half initialized
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/AcroForm<</Fields[42 4 0 R 999 0 R 5 0 R]>>>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</FT/Tx/T(Foo)>>endobj
5 0 obj<</FT/Btn/Ff(x)/T(Bar)>>endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& form = *doc.GetAcroForm();
    REQUIRE(form.GetFieldCount() == 4);
    ASSERT_THROW_WITH_ERROR_CODE(form.GetFieldAt(0), PdfErrorCode::InvalidHandle);
    REQUIRE(form.GetFieldAt(1).GetType() == PdfFieldType::TextBox);
    ASSERT_THROW_WITH_ERROR_CODE(form.GetFieldAt(2), PdfErrorCode::InvalidHandle);
    REQUIRE(form.GetFieldAt(3).GetType() == PdfFieldType::CheckBox);

    auto& created = form.CreateField<PdfTextBox>("Baz");
    REQUIRE(form.GetFieldCount() == 5);
    REQUIRE(&form.GetField(created.GetObject().GetIndirectReference()) == &created);
}

TEST_CASE("TestFieldsParentCycle")
{
    // The field has no /FT and its /Parent chain loops: the type detection
    // throws and the following calls must not use a half initialized form
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/AcroForm<</Fields[4 0 R]>>>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</T(Foo)/Parent 5 0 R>>endobj
5 0 obj<</T(Bar)/Parent 4 0 R>>endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& form = *doc.GetAcroForm();
    ASSERT_THROW_WITH_ERROR_CODE(form.GetFieldCount(), PdfErrorCode::MaxRecursionReached);
    ASSERT_THROW_WITH_ERROR_CODE(form.CreateField<PdfTextBox>("Baz"), PdfErrorCode::MaxRecursionReached);
}
