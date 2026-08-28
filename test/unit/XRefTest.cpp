// SPDX-FileCopyrightText: 2026 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

static bool isXRefStream(const charbuff& buff);
static unsigned countObjectStreams(const PdfMemDocument& doc);

TEST_CASE("TestXRefLayoutPreserved")
{
    // A document created from scratch is written with a legacy XRef table
    charbuff tableBuff;
    {
        PdfMemDocument doc;
        doc.GetPages().CreatePage(PdfPageSize::A4);
        BufferStreamDevice device(tableBuff);
        doc.Save(device);
    }

    REQUIRE(!isXRefStream(tableBuff));

    // An XRef stream can be forced on a document parsed with a legacy XRef table
    charbuff streamBuff;
    {
        PdfMemDocument doc;
        doc.LoadFromBuffer(tableBuff);
        BufferStreamDevice device(streamBuff);
        doc.Save(device, PdfSaveOptions::ForceXRefStream);
    }

    REQUIRE(isXRefStream(streamBuff));

    // The XRef stream layout is preserved on a regular save
    charbuff preservedBuff;
    {
        PdfMemDocument doc;
        doc.LoadFromBuffer(streamBuff);
        BufferStreamDevice device(preservedBuff);
        doc.Save(device);
    }

    REQUIRE(isXRefStream(preservedBuff));

    // ... and a legacy XRef table can be forced back
    charbuff forcedTableBuff;
    {
        PdfMemDocument doc;
        doc.LoadFromBuffer(preservedBuff);
        BufferStreamDevice device(forcedTableBuff);
        doc.Save(device, PdfSaveOptions::ForceXRefTable);
    }

    REQUIRE(!isXRefStream(forcedTableBuff));

    PdfMemDocument doc;
    doc.LoadFromBuffer(forcedTableBuff);
    REQUIRE(doc.GetPages().GetCount() == 1);
}

TEST_CASE("TestXRefLayoutForceMutuallyExclusive")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage(PdfPageSize::A4);
    charbuff buff;
    BufferStreamDevice device(buff);
    ASSERT_THROW_WITH_ERROR_CODE(doc.Save(device,
        PdfSaveOptions::ForceXRefTable | PdfSaveOptions::ForceXRefStream), PdfErrorCode::InvalidInput);
}

TEST_CASE("TestCollectObjectStreamsOnSave")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("PDFUA-Reference", "PDFUA-Ref-2-02_Invoice.pdf"));
    REQUIRE(countObjectStreams(doc) != 0);

    // On a full save the content of the object streams is rewritten
    // as top level objects, so the containers are collected as garbage
    charbuff savedBuff;
    {
        BufferStreamDevice device(savedBuff);
        doc.Save(device);
    }

    PdfMemDocument savedDoc;
    savedDoc.LoadFromBuffer(savedBuff);
    REQUIRE(savedDoc.GetPages().GetCount() == doc.GetPages().GetCount());
    REQUIRE(countObjectStreams(savedDoc) == 0);

    // On an incremental update they are preserved instead, as the
    // previous revision still references them for its compressed objects
    string outpath = TestUtils::GetTestOutputFilePath("TestCollectObjectStreamsOnSave.pdf");
    {
        FileStreamDevice input(TestUtils::GetTestInputFilePath("PDFUA-Reference", "PDFUA-Ref-2-02_Invoice.pdf"));
        FileStreamDevice output(outpath, FileMode::Create);
        input.CopyTo(output);
    }

    {
        PdfMemDocument updatedDoc;
        updatedDoc.Load(outpath);
        updatedDoc.SaveUpdate(outpath);
    }

    PdfMemDocument updatedDoc;
    updatedDoc.Load(outpath);
    REQUIRE(countObjectStreams(updatedDoc) != 0);
}

TEST_CASE("TestHybridXRefSavedAsStream")
{
    // A hybrid-reference file has a legacy XRef table with a /XRefStm entry
    // pointing to the cross reference stream that addresses its compressed
    // objects. Writing a hybrid layout is not supported, so an XRef stream
    // is written, which is the only layout that can address them
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TechDocs", "Acrobat_SignatureCreationQuickKeyAll.pdf"));
    REQUIRE(countObjectStreams(doc) != 0);

    charbuff savedBuff;
    {
        BufferStreamDevice device(savedBuff);
        doc.Save(device);
    }

    REQUIRE(isXRefStream(savedBuff));

    PdfMemDocument savedDoc;
    savedDoc.LoadFromBuffer(savedBuff);
    REQUIRE(savedDoc.GetPages().GetCount() == doc.GetPages().GetCount());
}

TEST_CASE("TestStreamedXRefLayout")
{
    charbuff buff;
    auto device = make_shared<BufferStreamDevice>(buff);

    // A streamed document can't write an XRef stream
    ASSERT_THROW_WITH_ERROR_CODE(PdfStreamedDocument(device, PdfVersionDefault,
        nullptr, PdfSaveOptions::ForceXRefStream), PdfErrorCode::UnsupportedOperation);
}

bool isXRefStream(const charbuff& buff)
{
    constexpr string_view startxref = "startxref"sv;
    string_view view(buff.data(), buff.size());
    auto found = view.rfind(startxref);
    REQUIRE(found != string_view::npos);

    // A legacy XRef table starts with the "xref" keyword,
    // an XRef stream with a regular object header instead
    size_t offset = (size_t)stoul(string(view.substr(found + startxref.length())));
    return view.substr(offset, 4) != "xref";
}

unsigned countObjectStreams(const PdfMemDocument& doc)
{
    unsigned ret = 0;
    for (auto obj : doc.GetObjects())
    {
        const PdfDictionary* dict;
        if (!obj->TryGetDictionary(dict))
            continue;

        if (dict->FindKeyAsSafe<PdfName>("Type") == "ObjStm")
            ret++;
    }

    return ret;
}
