// SPDX-FileCopyrightText: 2008 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>
#include <podofo/private/PdfParserObject.h>

using namespace std;
using namespace PoDoFo;

static void TestObjectsDirty(
    const PdfObject& objBool,
    const PdfObject& objNum,
    const PdfObject& objReal,
    const PdfObject& objStr,
    const PdfObject& objRef,
    const PdfObject& objArray,
    const PdfObject& objDict,
    const PdfObject& objStream,
    const PdfObject& objVariant,
    bool testValue);

TEST_CASE("TestEmptyObject")
{
    auto device = std::make_shared<SpanStreamDevice>("10 0 obj\nendobj\n"sv);
    PdfParserObject parserObj(*device);
    REQUIRE(parserObj.IsNull());
}

TEST_CASE("TestEmptyStream")
{
    auto device = std::make_shared<SpanStreamDevice>("10 0 obj<</Length 0>>stream\nendstream\nendobj\n"sv);
    PdfParserObject parserObj(*device);
    REQUIRE(parserObj.IsDictionary());
    REQUIRE(parserObj.HasStream());
    REQUIRE(parserObj.GetStream()->GetLength() == 0);
}

TEST_CASE("TestNameObject")
{
    auto device = std::make_shared<SpanStreamDevice>("10 0 obj / endobj\n"sv);
    PdfParserObject parserObj(*device);
    REQUIRE(parserObj.IsName());
    REQUIRE(parserObj.GetName().GetString() == "");
}

TEST_CASE("TestReferenceIsIndirect")
{
    REQUIRE(PdfReference(1, 0).IsIndirect());
    REQUIRE(PdfReference(1, 5).IsIndirect());

    // The object number 0 is always unavailable
    REQUIRE(!PdfReference().IsIndirect());
    REQUIRE(!PdfReference(0, 5).IsIndirect());
}

TEST_CASE("TestIsDirtyTrue")
{
    PdfMemDocument doc;

    auto& objBool = doc.GetObjects().CreateObject(true);
    auto& objNum = doc.GetObjects().CreateObject(static_cast<int64_t>(1));
    auto& objReal = doc.GetObjects().CreateObject(1.0);
    auto& objStr = doc.GetObjects().CreateObject(PdfString("Any"));
    auto& objName = doc.GetObjects().CreateObject(PdfName("Name"));
    auto& objRef = doc.GetObjects().CreateObject(PdfReference(0, 0));
    auto& objArray = doc.GetObjects().CreateArrayObject();
    auto& objDict = doc.GetObjects().CreateDictionaryObject();
    auto& objStream = doc.GetObjects().CreateDictionaryObject();
    objStream.GetOrCreateStream().SetData("Test"sv);
    auto& objVariant = doc.GetObjects().CreateObject(PdfVariant(false));

    // IsDirty should be true after construction
    TestObjectsDirty(objBool, objNum, objReal, objStr, objRef, objArray, objDict, objStream, objVariant, true);

    (void)objBool.GetBool();
    (void)objNum.GetNumber();
    (void)objReal.GetReal();
    (void)objStr.GetString();
    (void)objName.GetName();
    (void)objRef.GetReference();
    (void)objArray.GetArray();
    (void)objDict.GetDictionary();
    (void)objVariant.GetBool();

    string temp;
    StringStreamDevice out(temp);
    // NOTE: Save the document but prevent indirect objects from being collected
    doc.Save(out, PdfSaveOptions::NoCollectGarbage);

    // IsDirty should be false after saving
    TestObjectsDirty(objBool, objNum, objReal, objStr, objRef, objArray, objDict, objStream, objVariant, false);

    (void)objBool.GetBool();
    (void)objNum.GetNumber();
    (void)objReal.GetReal();
    (void)objStr.GetString();
    (void)objName.GetName();
    (void)objRef.GetReference();
    (void)objArray.GetArray();
    (void)objDict.GetDictionary();
    (void)objVariant.GetBool();
    (void)objStream.MustGetStream().GetCopy();

    // IsDirty should be false after calling getter
    TestObjectsDirty(objBool, objNum, objReal, objStr, objRef, objArray, objDict, objStream, objVariant, false);

    objBool.SetBool(false);
    objNum.SetNumber(static_cast<int64_t>(2));
    objReal.SetReal(2.0);
    objStr.SetString("Other");
    objName.SetName("Name2");
    objRef.SetReference(PdfReference(2, 0));
    objArray.GetArray().Add(objBool);
    objDict.GetDictionary().AddKey(objName.GetName(), objStr);
    objStream.MustGetStream().SetData("Test2"sv);
    objVariant = objNum;

    // IsDirty should be true after calling setter
    TestObjectsDirty(objBool, objNum, objReal, objStr, objRef, objArray, objDict, objStream, objVariant, true);

    temp.clear();
    // NOTE: Save the document but prevent indirect objects from being collected
    doc.Save(out, PdfSaveOptions::NoCollectGarbage);

    // IsDirty should be false after saving
    TestObjectsDirty(objBool, objNum, objReal, objStr, objRef, objArray, objDict, objStream, objVariant, false);
}

TEST_CASE("TestIsDirtyFalse")
{
    PdfObject objBool(true);
    PdfObject objNum(static_cast<int64_t>(1));
    PdfObject objReal(1.0);
    PdfObject objStr(PdfString("Any"));
    PdfObject objName(PdfName("Name"));
    PdfObject objRef(PdfReference(0, 0));
    PdfObject objArray = PdfObject(PdfArray());
    PdfObject objDict = PdfObject(PdfDictionary());
    PdfObject objStream = PdfObject(PdfDictionary());
    objStream.GetOrCreateStream().SetData("Test"sv);
    PdfObject objVariant(objBool);
    PdfObject objEmpty;
    PdfObject objData(PdfData("/Name"));

    // IsDirty should be false after construction
    TestObjectsDirty(objBool, objNum, objReal, objStr, objRef, objArray, objDict, objStream, objVariant, false);
    INFO("EMPTY     IsDirty() == false"); REQUIRE(!objEmpty.IsDirty());
    INFO("DATA      IsDirty() == false"); REQUIRE(!objData.IsDirty());

    (void)objBool.GetBool();
    (void)objNum.GetNumber();
    (void)objReal.GetReal();
    (void)objStr.GetString();
    (void)objName.GetName();
    (void)objRef.GetReference();
    (void)objArray.GetArray();
    (void)objDict.GetDictionary();
    (void)objVariant.GetBool();

    // IsDirty should be false after calling getter
    TestObjectsDirty(objBool, objNum, objReal, objStr, objRef, objArray, objDict, objStream, objVariant, false);

    objBool.SetBool(false);
    objNum.SetNumber(static_cast<int64_t>(2));
    objReal.SetReal(2.0);
    objStr.SetString("Other");
    objName.SetName("Name2");
    objRef.SetReference(PdfReference(2, 0));
    objArray.GetArray().Add(objBool);
    objDict.GetDictionary().AddKey(objName.GetName(), objStr);
    objStream.MustGetStream().SetData("Test1"sv);
    objVariant = objNum;

    // IsDirty should be false after calling setter
    TestObjectsDirty(objBool, objNum, objReal, objStr, objRef, objArray, objDict, objStream, objVariant, false);
}

void TestObjectsDirty(
    const PdfObject& objBool,
    const PdfObject& objNum,
    const PdfObject& objReal,
    const PdfObject& objStr,
    const PdfObject& objRef,
    const PdfObject& objArray,
    const PdfObject& objDict,
    const PdfObject& objStream,
    const PdfObject& objVariant,
    bool testValue)
{
    INFO(utls::Format("BOOL      IsDirty() == {}", testValue)); REQUIRE(objBool.IsDirty() == testValue);
    INFO(utls::Format("LONG      IsDirty() == {}", testValue)); REQUIRE(objNum.IsDirty() == testValue);
    INFO(utls::Format("DOUBLE    IsDirty() == {}", testValue)); REQUIRE(objReal.IsDirty() == testValue);
    INFO(utls::Format("STRING    IsDirty() == {}", testValue)); REQUIRE(objStr.IsDirty() == testValue);
    INFO(utls::Format("REFERENCE IsDirty() == {}", testValue)); REQUIRE(objRef.IsDirty() == testValue);
    INFO(utls::Format("ARRAY     IsDirty() == {}", testValue)); REQUIRE(objArray.IsDirty() == testValue);
    INFO(utls::Format("DICT      IsDirty() == {}", testValue)); REQUIRE(objDict.IsDirty() == testValue);
    INFO(utls::Format("STREAM    IsDirty() == {}", testValue)); REQUIRE(objStream.IsDirty() == testValue);
    INFO(utls::Format("VARIANT   IsDirty() == {}", testValue)); REQUIRE(objVariant.IsDirty() == testValue);
}

TEST_CASE("TestAliasedAssignment")
{
    // Assigning a variant from itself or from a value nested in its own container
    PdfDictionary inner;
    inner.AddKey("X"_n, PdfObject(PdfString("abc")));
    PdfDictionary outer;
    outer.AddKey("Kids"_n, PdfObject(std::move(inner)));

    PdfVariant var(outer);
    PdfVariant& varRef = var;
    var = varRef;
    REQUIRE(var.GetDictionary().MustGetKey("Kids").GetDictionary().MustGetKey("X").GetString().GetString() == "abc");
    var = var.GetDictionary().MustGetKey("Kids").GetVariant();
    REQUIRE(var.GetDictionary().MustGetKey("X").GetString().GetString() == "abc");

    PdfObject obj(outer);
    obj = *obj.GetDictionary().GetKey("Kids");
    REQUIRE(obj.GetDictionary().MustGetKey("X").GetString().GetString() == "abc");

    obj = PdfObject(outer);
    obj = std::move(*obj.GetDictionary().GetKey("Kids"));
    REQUIRE(obj.GetDictionary().MustGetKey("X").GetString().GetString() == "abc");

    PdfArray arr;
    arr.Add(PdfObject(outer));
    arr.SetAt(0, arr[0]);
    REQUIRE(arr[0].GetDictionary().MustGetKey("Kids").GetDictionary().MustGetKey("X").GetString().GetString() == "abc");
}

TEST_CASE("TestDictionaryAliasedAssignment")
{
    // Assigning a dictionary from one nested in its own entries
    PdfDictionary inner;
    inner.AddKey("X"_n, PdfObject(PdfString("abc")));
    PdfDictionary dict;
    dict.AddKey("Kids"_n, PdfObject(std::move(inner)));
    dict = dict.MustGetKey("Kids").GetDictionary();
    REQUIRE(dict.MustGetKey("X").GetString().GetString() == "abc");

    inner.AddKey("Y"_n, PdfObject(static_cast<int64_t>(1)));
    dict.AddKey("Kids"_n, PdfObject(std::move(inner)));
    dict = std::move(dict.MustGetKey("Kids").GetDictionary());
    REQUIRE(dict.MustGetKey("Y").GetNumber() == 1);
    REQUIRE(!dict.HasKey("X"));
}

TEST_CASE("TestAssignDropsStream")
{
    // Assigning an object without stream drops the stream of the target
    PdfMemDocument doc;
    auto& obj = doc.GetObjects().CreateDictionaryObject();
    obj.GetOrCreateStream().SetData(string_view("abc"));
    PdfObject num(static_cast<int64_t>(5));
    obj = num;
    REQUIRE(obj.GetNumber() == 5);
    REQUIRE(!obj.HasStream());
    doc.Save(TestUtils::GetTestOutputFilePath("TestAssignDropsStream.pdf"));
}

TEST_CASE("TestAssignDropsPendingStream")
{
    // Assigning over a parsed object whose stream is still to be parsed drops that stream
    auto device = std::make_shared<SpanStreamDevice>("10 0 obj<</Length 3/BBox[0 0 1 1]>>stream\nabc\nendstream\nendobj\n"sv);
    PdfParserObject parserObj(*device);
    PdfObject& obj = parserObj;
    REQUIRE(obj.IsDictionary());
    REQUIRE(obj.HasStream());
    obj = PdfObject::Null;
    REQUIRE(obj.IsNull());
    REQUIRE(!obj.HasStream());
    unique_ptr<PdfXObject> xobj;
    REQUIRE(!PdfXObject::TryCreateFromObject(obj, xobj));

    auto device2 = std::make_shared<SpanStreamDevice>("10 0 obj<</Length 3/BBox[0 0 1 1]>>stream\nabc\nendstream\nendobj\n"sv);
    PdfParserObject parserObj2(*device2);
    PdfObject& obj2 = parserObj2;
    REQUIRE(obj2.HasStream());
    obj2 = PdfObject(static_cast<int64_t>(5));
    REQUIRE(!obj2.HasStream());
    REQUIRE(!PdfXObject::TryCreateFromObject(obj2, xobj));
}

TEST_CASE("TestUnloadFailedReload")
{
    // An object is unloaded, then reloading it fails because the device
    // content changed: the stream of the null variant must not be parsed
    constexpr string_view ValidObject = "10 0 obj<</Length 3>>stream\nabc\nendstream\nendobj\n";
    constexpr string_view CorruptedObject = "xx x xxx<</Length 3>>stream\nabc\nendstream\nendobj\n";
    string buffer(ValidObject);
    auto device = std::make_shared<SpanStreamDevice>(buffer);
    PdfParserObject obj(*device);
    REQUIRE(obj.MustGetStream().GetLength() == 3);
    REQUIRE(obj.TryUnload());

    // Overwrite the bytes the device reads, in place
    std::copy(CorruptedObject.begin(), CorruptedObject.end(), buffer.begin());
    REQUIRE_THROWS_AS(obj.GetDictionary(), PdfError);
    REQUIRE(obj.IsNull());
    REQUIRE(obj.GetStream() == nullptr);
}

TEST_CASE("TestObjectStreamCopyWhileAppending")
{
    // Copying the content while an output stream is open is rejected
    PdfMemDocument doc;
    auto& obj = doc.GetObjects().CreateDictionaryObject();
    auto& objStream = obj.GetOrCreateStream();
    objStream.SetData(string_view("Hello"));
    auto output = objStream.GetOutputStreamRaw(true);
    charbuff buffer;
    BufferStreamDevice device(buffer);
    ASSERT_THROW_WITH_ERROR_CODE(objStream.CopyTo(device), PdfErrorCode::InternalLogic);
    ASSERT_THROW_WITH_ERROR_CODE(objStream.CopyToSafe(device), PdfErrorCode::InternalLogic);
}

TEST_CASE("TestObjectStreamWrappersMove")
{
    // Moved wrappers keep the wrapped streams, and assigning over
    // a wrapper releases the object stream it was wrapping
    PdfMemDocument doc;
    auto& stream = doc.GetObjects().CreateDictionaryObject().GetOrCreateStream();
    stream.SetData(string_view("Hello"));

    auto input = stream.GetInputStream();
    PdfObjectInputStream movedInput(std::move(input));
    PdfObjectInputStream assignedInput;
    assignedInput = std::move(movedInput);
    char data[5];
    assignedInput.Read(data, sizeof(data));
    REQUIRE(string_view(data, sizeof(data)) == "Hello");
    assignedInput = PdfObjectInputStream();

    auto output = stream.GetOutputStreamRaw(true);
    PdfObjectOutputStream movedOutput(std::move(output));
    PdfObjectOutputStream assignedOutput;
    assignedOutput = std::move(movedOutput);
    assignedOutput.Write(string_view(" World"));
    assignedOutput = PdfObjectOutputStream();

    REQUIRE(stream.GetCopy() == "Hello World");
}

TEST_CASE("TestFindKeyParentOwnerless")
{
    // A dictionary without owner with a direct /Parent dictionary
    PdfDictionary parent;
    parent.AddKey("Foo"_n, PdfObject(static_cast<int64_t>(1)));
    PdfDictionary dict;
    dict.AddKey("Parent"_n, PdfObject(parent));
    auto found = dict.FindKeyParent("Foo");
    REQUIRE(found != nullptr);
    REQUIRE(found->GetNumber() == 1);
    REQUIRE(dict.FindKeyParent("Bar") == nullptr);
}
