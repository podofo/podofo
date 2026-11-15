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

TEST_CASE("TestFieldChildrenSharedKidsArray")
{
    // Two fields share the same indirect /Kids array: each one gets its own copy
    // before a modification, so the changes through one don't affect the other
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/AcroForm<</Fields[4 0 R 5 0 R]>>>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</FT/Tx/T(A)/Kids 6 0 R>>endobj
5 0 obj<</FT/Tx/T(B)/Kids 6 0 R>>endobj
6 0 obj[]endobj
trailer<</Root 1 0 R/Size 7>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& form = *doc.GetAcroForm();
    auto& fieldA = form.GetFieldAt(0);
    auto& childrenA = fieldA.GetChildren();
    auto& childrenB = form.GetFieldAt(1).GetChildren();
    auto refA = childrenA.CreateChild().GetObject().GetIndirectReference();
    auto& child = childrenB.CreateChild();
    auto ref = child.GetObject().GetIndirectReference();
    REQUIRE(childrenB.GetCount() == 1);
    REQUIRE(&childrenB.GetField(ref) == &child);
    childrenB.RemoveField(ref);
    REQUIRE(childrenB.GetCount() == 0);
    ASSERT_THROW_WITH_ERROR_CODE(childrenB.GetField(ref), PdfErrorCode::ValueOutOfRange);

    auto& kidsA = fieldA.GetDictionary().MustFindKey("Kids").GetArray();
    REQUIRE(kidsA.GetSize() == 1);
    REQUIRE(kidsA.MustFindAt(0).GetIndirectReference() == refA);
    REQUIRE(doc.GetObjects().MustGetObject(PdfReference(6, 0)).GetArray().GetSize() == 0);
}

TEST_CASE("TestAcroFormSharedFieldsArray")
{
    // The /Fields array is also the /Kids array of its field: the form and the
    // field get their own copy before a modification, without affecting the other
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/AcroForm<</Fields 5 0 R>>>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</FT/Tx/T(A)/Kids 5 0 R>>endobj
5 0 obj[4 0 R]endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& form = *doc.GetAcroForm();
    REQUIRE(form.GetFieldCount() == 1);
    auto& fieldA = form.GetFieldAt(0);
    auto& childrenA = fieldA.GetChildren();
    REQUIRE(childrenA.GetCount() == 1);

    auto& b = form.CreateField<PdfTextBox>("B");
    auto ref = b.GetObject().GetIndirectReference();
    REQUIRE(form.GetFieldCount() == 2);
    REQUIRE(&form.GetField(ref) == &b);
    auto& child = b.GetChildren().CreateChild();
    REQUIRE(child.GetParentSafe() == &b);

    childrenA.CreateChild();
    REQUIRE(childrenA.GetCount() == 2);
    childrenA.RemoveFieldAt(1);
    REQUIRE(fieldA.GetDictionary().MustFindKey("Kids").GetArray().GetSize() == 1);

    form.RemoveField(ref);
    REQUIRE(form.GetFieldCount() == 1);
    ASSERT_THROW_WITH_ERROR_CODE(form.GetField(ref), PdfErrorCode::ValueOutOfRange);
    REQUIRE(doc.GetObjects().MustGetObject(PdfReference(5, 0)).GetArray().GetSize() == 1);
}

TEST_CASE("TestFieldNestedChildren")
{
    // A child field is owned by the children of its parent, not by the form:
    // its own children must be parented to it, and a lookup of its reference
    // in the form must not register it there
    PdfMemDocument doc;
    auto& form = doc.GetOrCreateAcroForm();
    auto& root = form.CreateField<PdfTextBox>("root");
    auto& mid = root.GetChildren().CreateChild();
    auto& leaf = mid.GetChildren().CreateChild();
    REQUIRE(leaf.GetParentSafe() == &mid);
    REQUIRE(leaf.GetDictionary().MustGetKey("Parent").GetReference() == mid.GetObject().GetIndirectReference());

    auto midRef = mid.GetObject().GetIndirectReference();
    ASSERT_THROW_WITH_ERROR_CODE(form.GetField(midRef), PdfErrorCode::ValueOutOfRange);
    form.RemoveField(root.GetObject().GetIndirectReference());
    REQUIRE(form.GetFieldCount() == 0);
    form.RemoveField(midRef);
    REQUIRE(form.GetFieldCount() == 0);
}

TEST_CASE("TestAcroFormDirectDictionaries")
{
    // Direct field dictionaries live in the /Fields array storage, which the
    // addition relocates and the rescan of the next creation reads: they must
    // be moved to indirect objects and the existing wrappers rebound to them
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/AcroForm<</Fields[<</FT/Tx/T(a)>><</FT/Tx/T(b)>><</FT/Tx/T(c)>><</FT/Tx/T(d)>>]>>>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& form = *doc.GetAcroForm();
    REQUIRE(form.GetFieldCount() == 4);
    auto& first = form.GetFieldAt(0);
    auto& last = form.GetFieldAt(3);

    form.CreateField<PdfTextBox>("x");
    form.CreateField<PdfTextBox>("y");
    REQUIRE(form.GetFieldCount() == 6);
    REQUIRE(first.GetObject().IsIndirect());
    REQUIRE(first.GetNameRaw()->GetString() == "a");
    REQUIRE(&form.GetField(first.GetObject().GetIndirectReference()) == &first);
    auto& arr = form.GetDictionary().MustGetKey("Fields").GetArray();
    REQUIRE(arr[0].GetReference() == first.GetObject().GetIndirectReference());

    form.RemoveFieldAt(0);
    REQUIRE(form.GetFieldCount() == 5);
    REQUIRE(last.GetNameRaw()->GetString() == "d");
    REQUIRE(&form.GetFieldAt(2) == &last);
}

TEST_CASE("TestFieldChildrenDirectDictionaries")
{
    // Direct child dictionaries live in the /Kids array storage, which the
    // addition relocates: they must be moved to indirect objects and the
    // existing wrappers rebound to them
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/AcroForm<</Fields[4 0 R]>>>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</FT/Tx/T(p)/Kids[<</T(a)>><</T(b)>><</T(c)>><</T(d)>>]>>endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& children = doc.GetAcroForm()->GetFieldAt(0).GetChildren();
    REQUIRE(children.GetCount() == 4);
    auto& first = children.GetFieldAt(0);
    auto& last = children.GetFieldAt(3);

    children.CreateChild();
    REQUIRE(children.GetCount() == 5);
    REQUIRE(first.GetObject().IsIndirect());
    REQUIRE(first.GetNameRaw()->GetString() == "a");
    REQUIRE(&children.GetField(first.GetObject().GetIndirectReference()) == &first);

    children.RemoveFieldAt(0);
    REQUIRE(children.GetCount() == 4);
    REQUIRE(last.GetNameRaw()->GetString() == "d");
    REQUIRE(&children.GetFieldAt(2) == &last);
}

TEST_CASE("TestFieldFullNameParentCycle")
{
    // The /Parent of the field points to the field itself
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/AcroForm<</Fields[4 0 R]>>>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</FT/Tx/T(a)/Parent 4 0 R>>endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& field = doc.GetAcroForm()->GetFieldAt(0);
    ASSERT_THROW_WITH_ERROR_CODE(field.GetFullName(), PdfErrorCode::MaxRecursionReached);
}

TEST_CASE("TestButtonCaptionsWithoutWidget")
{
    // The caption accessors of a field without widget must raise instead of dereferencing null
    PdfMemDocument doc;
    auto& form = doc.GetOrCreateAcroForm();
    auto& button = form.CreateField<PdfPushButton>("button");
    ASSERT_THROW_WITH_ERROR_CODE(button.GetCaption(), PdfErrorCode::InvalidHandle);
    ASSERT_THROW_WITH_ERROR_CODE(button.SetCaption(PdfString("Push")), PdfErrorCode::InvalidHandle);
    ASSERT_THROW_WITH_ERROR_CODE(button.GetRolloverCaption(), PdfErrorCode::InvalidHandle);
    ASSERT_THROW_WITH_ERROR_CODE(button.SetAlternateCaption(PdfString("Alt")), PdfErrorCode::InvalidHandle);
}

TEST_CASE("TestFieldWidgetAfterPageRemoval")
{
    // A child field bound to a widget of a removed page loses the widget
    PdfMemDocument doc;
    doc.GetPages().CreatePage(PdfPageSize::A4);
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    auto& group = doc.GetOrCreateAcroForm().CreateField<PdfRadioButton>("Group");
    auto& child = group.GetChildren().CreateChild(page, Rect(10, 10, 20, 20));
    REQUIRE(child.GetWidget() != nullptr);
    doc.GetPages().RemovePageAt(1);
    auto& found = doc.GetAcroForm()->GetFieldAt(0).GetChildren().GetFieldAt(0);
    REQUIRE(&found == &child);
    REQUIRE(child.GetWidget() == nullptr);
    ASSERT_THROW_WITH_ERROR_CODE(child.MustGetWidget(), PdfErrorCode::InvalidHandle);
}
