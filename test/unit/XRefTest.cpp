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

    // A legacy XRef table can't address compressed objects, so the content of
    // the object streams is rewritten as top level objects and the containers
    // are collected as garbage
    charbuff savedBuff;
    {
        BufferStreamDevice device(savedBuff);
        doc.Save(device, PdfSaveOptions::ForceXRefTable);
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

TEST_CASE("TestPreserveObjectStreamsOnSave")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("PDFUA-Reference", "PDFUA-Ref-2-02_Invoice.pdf"));
    unsigned objStmCount = countObjectStreams(doc);
    REQUIRE(objStmCount != 0);

    // An unmodified object stream is written as it is and its objects
    // are addressed with compressed entries, instead of being expanded
    charbuff preservedBuff;
    {
        BufferStreamDevice device(preservedBuff);
        doc.Save(device);
    }

    PdfMemDocument preservedDoc;
    preservedDoc.LoadFromBuffer(preservedBuff);
    REQUIRE(isXRefStream(preservedBuff));
    REQUIRE(countObjectStreams(preservedDoc) == objStmCount);
    REQUIRE(preservedDoc.GetPages().GetCount() == doc.GetPages().GetCount());

    // Expanding the same document is measurably bigger
    charbuff expandedBuff;
    {
        BufferStreamDevice device(expandedBuff);
        doc.Save(device, PdfSaveOptions::ForceXRefTable);
    }

    REQUIRE(preservedBuff.size() < expandedBuff.size());
}

TEST_CASE("TestModifiedCompressedObjectExpandedOnSave")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("PDFUA-Reference", "PDFUA-Ref-2-02_Invoice.pdf"));

    // Modifying one compressed object doesn't expand the whole object stream:
    // the modified one is written as a top level object and the stale copy
    // left in the object stream is not addressed anymore
    auto& catalog = doc.GetCatalog().GetObject();
    REQUIRE(!catalog.IsDirty());
    catalog.GetDictionary().AddKey("PoDoFoTest"_n, PdfString("Modified"));
    REQUIRE(catalog.IsDirty());

    charbuff savedBuff;
    {
        BufferStreamDevice device(savedBuff);
        doc.Save(device);
    }

    PdfMemDocument savedDoc;
    savedDoc.LoadFromBuffer(savedBuff);
    REQUIRE(countObjectStreams(savedDoc) != 0);
    REQUIRE(savedDoc.GetPages().GetCount() == doc.GetPages().GetCount());
    REQUIRE(savedDoc.GetCatalog().GetDictionary().MustFindKey("PoDoFoTest")
        .GetString().GetString() == "Modified");
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

// Prepending data to a document makes all its offsets relative to the header:
// the /XRefStm of a hybrid-reference file must be fixed with it as well
TEST_CASE("TestMagicOffsetHybridXRef")
{
    charbuff hybridBuff("% Data before the header\n"sv);
    {
        FileStreamDevice input(TestUtils::GetTestInputFilePath("TechDocs", "Acrobat_SignatureCreationQuickKeyAll.pdf"));
        // NOTE: The device is positioned at the end of the buffer by default
        BufferStreamDevice output(hybridBuff);
        input.CopyTo(output);
    }

    // NOTE: Skip the XRef recovery, or a broken section would be
    // silently rebuilt and the test would pass regardless
    PdfMemDocument doc;
    doc.LoadFromBuffer(hybridBuff, PdfLoadOptions::SkipXRefRecovery);
    REQUIRE(countObjectStreams(doc) != 0);
    REQUIRE(doc.GetPages().GetCount() != 0);
}

// The entries of an XRef stream are relative to the header as well: a
// document with data before it is read correctly whatever wrote it
TEST_CASE("TestMagicOffsetXRefStreamEntries")
{
    charbuff buff("% Data before the header\n"sv);
    {
        FileStreamDevice input(TestUtils::GetTestInputFilePath("TestXRefCheckboxUnicode.pdf"));
        // NOTE: The device is positioned at the end of the buffer by default
        BufferStreamDevice output(buff);
        input.CopyTo(output);
    }

    // NOTE: Skip the XRef recovery, or a broken section would be
    // silently rebuilt and the test would pass regardless
    PdfMemDocument doc;
    doc.LoadFromBuffer(buff, PdfLoadOptions::SkipXRefRecovery);
    REQUIRE(doc.GetPages().GetCount() == 1);
}

// The offsets stored in the XRef sections of a document that has data before
// the "%PDF" header are relative to it, for both the layouts
TEST_CASE("TestMagicOffsetXRefRoundTrip")
{
    auto saveUpdateAndReload = [](PdfSaveOptions opts) {
        string path = TestUtils::GetTestOutputFilePath("TestMagicOffsetXRefRoundTrip.pdf");
        {
            FileStreamDevice input(TestUtils::GetTestInputFilePath("blank-with-offset-start.pdf"));
            FileStreamDevice output(path, FileMode::Create);
            input.CopyTo(output);
        }

        {
            PdfMemDocument doc;
            doc.Load(path);
            doc.SaveUpdate(path, opts);
        }

        // NOTE: Skip the XRef recovery, or a broken section would be
        // silently rebuilt and the test would pass regardless
        PdfMemDocument doc;
        doc.Load(path, PdfLoadOptions::SkipXRefRecovery);
        REQUIRE(doc.GetPages().GetCount() == 1);

        // The information dictionary is the object rewritten by the update, so it's
        // the one addressed by the new section. Objects are loaded lazily, so its
        // contents must be actually read, and the modification date tells the
        // rewritten object apart from the one of the previous revision
        auto modDate = doc.GetTrailer().GetDictionary().MustFindKey("Info")
            .GetDictionary().MustFindKey("ModDate").GetString().GetString();
        REQUIRE(modDate != "D:20190315122551+01'");
    };

    saveUpdateAndReload(PdfSaveOptions::ForceXRefTable);
    saveUpdateAndReload(PdfSaveOptions::ForceXRefStream);
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
