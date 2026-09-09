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

TEST_CASE("TestArrayGrowthKeepsNestedChildrenAttached")
{
    PdfMemDocument doc;
    auto& obj = doc.GetObjects().CreateDictionaryObject();
    obj.GetDictionary().AddKey("Test"_n, PdfArray());
    auto& arr = obj.GetDictionary().MustFindKey("Test").GetArray();

    // Nested arrays are relocated together with the outer ones
    for (unsigned i = 0; i < 200; i++)
    {
        PdfArray inner;
        inner.Add(PdfObject(static_cast<int64_t>(i)));
        inner.Add(PdfObject(PdfString("test")));
        arr.Add(PdfObject(inner));
    }

    for (unsigned i = 0; i < arr.GetSize(); i++)
    {
        auto& inner = arr[i].GetArray();
        REQUIRE(inner.GetOwner() == &arr[i]);
        ASSERT_EQUAL(inner.GetSize(), 2u);
        for (unsigned j = 0; j < inner.GetSize(); j++)
            REQUIRE(inner[j].GetDocument() == &doc);
    }
}

// Shifting relocates the stored objects, so the nested containers must keep
// pointing at the element that owns them
TEST_CASE("TestArrayShiftKeepsNestedChildrenAttached")
{
    PdfMemDocument doc;
    auto& obj = doc.GetObjects().CreateDictionaryObject();
    obj.GetDictionary().AddKey("Test"_n, PdfArray());
    auto& arr = obj.GetDictionary().MustFindKey("Test").GetArray();

    for (unsigned i = 0; i < 50; i++)
    {
        PdfArray inner;
        inner.Add(PdfObject(static_cast<int64_t>(i)));
        arr.Add(PdfObject(inner));
    }

    arr.RemoveAt(10);
    ASSERT_EQUAL(arr.GetSize(), 49u);
    arr.insert(arr.begin() + 20, PdfObject(PdfArray()));
    ASSERT_EQUAL(arr.GetSize(), 50u);
    arr.erase(arr.begin() + 5, arr.begin() + 8);
    ASSERT_EQUAL(arr.GetSize(), 47u);

    for (unsigned i = 0; i < arr.GetSize(); i++)
    {
        auto& inner = arr[i].GetArray();
        REQUIRE(inner.GetOwner() == &arr[i]);
        if (inner.GetSize() != 0)
            REQUIRE(inner[0].GetDocument() == &doc);
    }

    // The surviving values are the original ones minus what was removed
    ASSERT_EQUAL(arr[0].GetArray()[0].GetNumber(), static_cast<int64_t>(0));
    ASSERT_EQUAL(arr[5].GetArray()[0].GetNumber(), static_cast<int64_t>(8));
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
