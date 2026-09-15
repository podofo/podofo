// SPDX-FileCopyrightText: 2026 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

static bool isXRefStream(const charbuff& buff);
static unsigned countObjectStreams(const PdfMemDocument& doc);
static vector<PdfReference> getLastSectionFreeEntries(const charbuff& buff);

// A document whose XRef table already carries a free list: objects 5, 6 and 7
// are free at generation 1, chained as 0 -> 5 -> 6 -> 7 -> 0. Object 8 is the
// information dictionary, so that saving doesn't allocate one from the list
constexpr string_view FreeListDocument = R"PDF(%PDF-1.7
1 0 obj
<</Type/Catalog/Pages 2 0 R/Names 4 0 R>>
endobj
2 0 obj
<</Type/Pages/Kids[3 0 R]/Count 1>>
endobj
3 0 obj
<</Type/Page/Parent 2 0 R/MediaBox[0 0 595 842]/Resources<<>>>>
endobj
4 0 obj
<</Names[]>>
endobj
8 0 obj
<</Producer(PoDoFo test)>>
endobj
xref
0 9
0000000005 65535 f 
0000000009 00000 n 
0000000066 00000 n 
0000000117 00000 n 
0000000196 00000 n 
0000000006 00001 f 
0000000007 00001 f 
0000000000 00001 f 
0000000224 00000 n 
trailer
<</Size 9/Root 1 0 R/Info 8 0 R>>
startxref
266
%%EOF
)PDF"sv;

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

// An incremental update writes only the free entries it actually modified:
// re-writing the whole free list would make validators report every free
// object of the document as freed in this revision
TEST_CASE("TestIncrementalUpdateFreeObjectsDelta")
{
    charbuff buff(FreeListDocument);
    {
        PdfMemDocument doc;
        doc.LoadFromBuffer(FreeListDocument);

        // Object 4 is referenced only by the catalog, dropping the key makes it garbage
        doc.GetCatalog().GetDictionary().RemoveKey("Names");
        doc.CollectGarbage();

        // NOTE: The device is positioned at the end of the buffer by default
        BufferStreamDevice device(buff);
        doc.SaveUpdate(device, PdfSaveOptions::ForceXRefTable);
    }

    // Objects 5, 6 and 7 were already free in the previous revision
    auto freeEntries = getLastSectionFreeEntries(buff);
    REQUIRE(freeEntries.size() == 1);
    REQUIRE(freeEntries[0] == PdfReference(4, 1));
}

// An object that is freed and reused before saving needs no free entry,
// as it's written in use in the same revision
TEST_CASE("TestIncrementalUpdateFreeObjectReused")
{
    charbuff buff(FreeListDocument);
    {
        PdfMemDocument doc;
        doc.LoadFromBuffer(FreeListDocument);
        doc.GetCatalog().GetDictionary().RemoveKey("Names");
        doc.CollectGarbage();

        // Object 4 was just freed and is the lowest in the free list, so it's reused here
        auto& obj = doc.GetObjects().CreateDictionaryObject("Test"_n);
        REQUIRE(obj.GetIndirectReference() == PdfReference(4, 1));

        // Reference it, or the garbage collection done on save would free it again
        doc.GetCatalog().GetDictionary().AddKeyIndirect("Names"_n, obj);

        // NOTE: The device is positioned at the end of the buffer by default
        BufferStreamDevice device(buff);
        doc.SaveUpdate(device, PdfSaveOptions::ForceXRefTable);
    }

    REQUIRE(getLastSectionFreeEntries(buff).size() == 0);
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

// Collect the free entries of the last XRef section, excluding the
// entry for object 0, which is the always rewritten free list head
vector<PdfReference> getLastSectionFreeEntries(const charbuff& buff)
{
    constexpr string_view startxref = "startxref"sv;
    string_view view(buff.data(), buff.size());
    auto found = view.rfind(startxref);
    REQUIRE(found != string_view::npos);

    size_t offset = (size_t)stoul(string(view.substr(found + startxref.length())));
    REQUIRE(view.substr(offset, 5) == "xref\n");

    vector<PdfReference> ret;
    size_t pos = offset + 5;
    while (view.compare(pos, 7, "trailer") != 0)
    {
        auto eol = view.find('\n', pos);
        REQUIRE(eol != string_view::npos);

        // Read the "first count" subsection header
        auto header = view.substr(pos, eol - pos);
        auto space = header.find(' ');
        REQUIRE(space != string_view::npos);
        unsigned first = (unsigned)stoul(string(header.substr(0, space)));
        unsigned count = (unsigned)stoul(string(header.substr(space + 1)));
        pos = eol + 1;

        // ISO 32000-2:2020 7.5.4 "Cross-reference table": the entries
        // have a fixed layout and are exactly 20 bytes wide
        for (unsigned i = 0; i < count; i++, pos += 20)
        {
            REQUIRE(pos + 20 <= view.length());
            auto entry = view.substr(pos, 20);
            if (entry[17] != 'f' || first + i == 0)
                continue;

            ret.push_back(PdfReference(first + i, (uint16_t)stoul(string(entry.substr(11, 5)))));
        }
    }

    return ret;
}
