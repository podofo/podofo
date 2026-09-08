// SPDX-FileCopyrightText: 2026 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

TEST_CASE("TestArrayGrowthKeepsChildrenAttached")
{
    PdfMemDocument doc;
    auto& obj = doc.GetObjects().CreateDictionaryObject();
    obj.GetDictionary().AddKey("Test"_n, PdfArray());
    auto& arr = obj.GetDictionary().MustFindKey("Test").GetArray();

    // Grow well past the initial capacity, so the stored elements are reallocated
    for (unsigned i = 0; i < 1000; i++)
        arr.Add(PdfObject(static_cast<int64_t>(i)));

    ASSERT_EQUAL(arr.GetSize(), 1000u);
    for (unsigned i = 0; i < arr.GetSize(); i++)
        REQUIRE(arr[i].GetDocument() == &doc);
}

TEST_CASE("TestArrayReserveKeepsChildrenAttached")
{
    PdfMemDocument doc;
    auto& obj = doc.GetObjects().CreateDictionaryObject();
    obj.GetDictionary().AddKey("Test"_n, PdfArray());
    auto& arr = obj.GetDictionary().MustFindKey("Test").GetArray();

    arr.Add(PdfObject(static_cast<int64_t>(1)));
    arr.Add(PdfObject(static_cast<int64_t>(2)));
    arr.Reserve(1000);

    REQUIRE(arr[0].GetDocument() == &doc);
    REQUIRE(arr[1].GetDocument() == &doc);
}
