// SPDX-FileCopyrightText: 2016 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

// The FileStreamDevice buffer capacity. The tests below deliberately
// straddle it
constexpr size_t DeviceBufferSize = 4096;

static string createTestFile(const string_view& filename, size_t size);

TEST_CASE("TestDevices")
{
    string_view testString = "Hello World Buffer!";
    charbuff buffer1;

    // large appends
    StringStreamDevice streamLarge(buffer1);
    for (unsigned i = 0; i < 100; i++)
        streamLarge.Write(testString);

    if (buffer1.size() != testString.size() * 100)
        FAIL(utls::Format("Buffer1 size is wrong after 100 attaches: {}", buffer1.size()));
}

TEST_CASE("TestSaveIncremental")
{
    PdfMemDocument doc;
    auto testPath = TestUtils::GetTestOutputFilePath("TestSaveIncremental.pdf");
    doc.GetPages().CreatePage(PdfPageSize::A4);
    doc.Save(testPath);
    doc.Load(testPath);
    doc.SaveUpdate(testPath);
    doc.Load(testPath);
}

TEST_CASE("TestStreamedDocument")
{
    auto testPath = TestUtils::GetTestOutputFilePath("TestStreamedDocument.pdf");
    PdfStreamedDocument document(testPath);
    auto& page = document.GetPages().CreatePage(PdfPageSize::A4);
    // NOTE: use a TTC version of the LiberationSans format to test TTC extraction
    auto& font = document.GetFonts().GetOrCreateFont(TestUtils::GetTestInputFilePath("FontsTTC", "LiberationSans.ttc"), 2);
    PdfPainter painter;
    painter.SetCanvas(page);
    painter.TextState.SetFont(font, 18);
    painter.DrawText("Hello World!", 56.69, page.GetRect().Height - 56.69);
    painter.FinishDrawing();
}

TEST_CASE("TestFileDeviceReadWriteInterleaving")
{
    // The signing shape: write, seek back, read, then write where the read left off
    auto testPath = createTestFile("TestFileDeviceInterleaving.bin", DeviceBufferSize * 2 + 100);
    FileStreamDevice device(testPath, FileMode::Open, DeviceAccess::ReadWrite);

    device.Seek(10);
    device.Write("ABCD", 4);
    REQUIRE(device.GetPosition() == 14);

    device.Seek(10);
    charbuff read(4);
    device.Read(read.data(), 4);
    REQUIRE(string_view(read) == "ABCD");
    REQUIRE(device.GetPosition() == 14);

    device.Write("EFGH", 4);
    REQUIRE(device.GetPosition() == 18);
    device.Close();

    FileStreamDevice verify(testPath);
    REQUIRE(verify.GetLength() == DeviceBufferSize * 2 + 100);
    verify.Seek(10);
    charbuff verifyBuff(8);
    verify.Read(verifyBuff.data(), 8);
    REQUIRE(string_view(verifyBuff) == "ABCDEFGH");
}

TEST_CASE("TestFileDeviceFlushPosition")
{
    // The m_BufferOffset re-anchor: a lost one rewinds GetPosition() by the
    // flushed amount and corrupts every subsequent xref offset
    auto testPath = TestUtils::GetTestOutputFilePath("TestFileDeviceFlushPosition.bin");
    FileStreamDevice device(testPath, FileMode::Create);

    string chunk(100, 'x');
    size_t written = 0;
    for (unsigned i = 0; i < 100; i++)
    {
        device.Write(chunk);
        written += chunk.size();
        REQUIRE(device.GetPosition() == written);
    }

    size_t beforeFlush = device.GetPosition();
    device.Flush();
    REQUIRE(device.GetPosition() == beforeFlush);
    REQUIRE(device.GetLength() == written);
    device.Close();

    REQUIRE(fs::file_size(fs::u8path(testPath)) == written);
}

TEST_CASE("TestFileDeviceEofClearedOnWrite")
{
    // The m_Eof clear on the read -> write transition, which is the one
    // path into the Write direction that skips resetBuffers()
    auto testPath = createTestFile("TestFileDeviceEofOnWrite.bin", 100);
    FileStreamDevice device(testPath, FileMode::Open, DeviceAccess::ReadWrite);

    charbuff buffer(100);
    device.Read(buffer.data(), 100);
    char ch;
    REQUIRE(!device.Read(ch));
    REQUIRE(device.Eof());

    device.Write("tail", 4);
    REQUIRE(!device.Eof());
    REQUIRE(device.GetPosition() == 104);
    device.Close();

    REQUIRE(fs::file_size(fs::u8path(testPath)) == 104);
}

TEST_CASE("TestFileDeviceEofClearedOnSeek")
{
    // The m_Eof reset in resetBuffers(), the readForSignature() shape
    auto testPath = createTestFile("TestFileDeviceEofOnSeek.bin", DeviceBufferSize + 10);
    FileStreamDevice device(testPath);

    charbuff buffer(DeviceBufferSize + 10);
    device.Read(buffer.data(), DeviceBufferSize + 10);
    char ch;
    REQUIRE(!device.Read(ch));
    REQUIRE(device.Eof());

    device.Seek(0);
    REQUIRE(!device.Eof());
    REQUIRE(device.Read(ch));
    REQUIRE(ch == buffer[0]);
}

TEST_CASE("TestFileDeviceSeekDirections")
{
    auto testPath = createTestFile("TestFileDeviceSeek.bin", 1000);
    FileStreamDevice device(testPath);

    device.Seek(100, SeekDirection::Begin);
    REQUIRE(device.GetPosition() == 100);

    // Read first, so read ahead leaves the OS file offset past the logical position
    char ch;
    REQUIRE(device.Read(ch));
    device.Seek(50, SeekDirection::Current);
    REQUIRE(device.GetPosition() == 151);
    REQUIRE(device.Read(ch));

    device.Seek(-100, SeekDirection::Current);
    REQUIRE(device.GetPosition() == 52);

    device.Seek(-10, SeekDirection::End);
    REQUIRE(device.GetPosition() == 990);

    device.Seek(0, SeekDirection::End);
    REQUIRE(device.GetPosition() == 1000);
    REQUIRE(!device.Read(ch));
}

TEST_CASE("TestFileDeviceBufferBoundaries")
{
    for (size_t size : { (size_t)0, DeviceBufferSize - 1, DeviceBufferSize,
        DeviceBufferSize + 1, DeviceBufferSize * 2 })
    {
        auto testPath = createTestFile("TestFileDeviceBoundary.bin", size);
        FileStreamDevice device(testPath);
        REQUIRE(device.GetLength() == size);

        charbuff read(size);
        char ch;
        for (size_t i = 0; i < size; i++)
        {
            REQUIRE(device.Read(ch));
            read[i] = ch;
        }

        REQUIRE(!device.Read(ch));
        REQUIRE(device.Eof());
        REQUIRE(device.GetPosition() == size);
        REQUIRE(TestUtils::IsBufferEqual(read, testPath));
    }
}

TEST_CASE("TestFileDevicePeek")
{
    auto testPath = createTestFile("TestFileDevicePeek.bin", DeviceBufferSize + 10);
    FileStreamDevice device(testPath);

    // Peek across the refill boundary
    char peeked;
    char read;
    device.Seek(DeviceBufferSize - 1);
    REQUIRE(device.Peek(peeked));
    REQUIRE(device.Read(read));
    REQUIRE(peeked == read);
    REQUIRE(device.Peek(peeked));
    REQUIRE(device.Read(read));
    REQUIRE(peeked == read);
    REQUIRE(device.GetPosition() == DeviceBufferSize + 1);

    // Peek at EOF
    device.Seek(0, SeekDirection::End);
    REQUIRE(!device.Peek(peeked));
    REQUIRE(peeked == '\0');
}

TEST_CASE("TestFileDeviceAppend")
{
    auto testPath = createTestFile("TestFileDeviceAppend.bin", 500);
    {
        FileStreamDevice device(testPath, FileMode::Append);
        REQUIRE(device.GetPosition() == 500);
        device.Write("appended", 8);
        REQUIRE(device.GetPosition() == 508);
        device.Flush();
        REQUIRE(device.GetPosition() == 508);
        REQUIRE(device.GetLength() == 508);
    }

    FileStreamDevice verify(testPath);
    verify.Seek(500);
    charbuff read(8);
    verify.Read(read.data(), 8);
    REQUIRE(string_view(read) == "appended");
}

TEST_CASE("TestFileDeviceLargeTransfers")
{
    // Both the read and the write bypass, which skip the buffer entirely
    constexpr size_t largeSize = DeviceBufferSize * 3 + 7;
    auto testPath = TestUtils::GetTestOutputFilePath("TestFileDeviceLarge.bin");
    charbuff source(largeSize);
    for (size_t i = 0; i < largeSize; i++)
        source[i] = (char)(i % 251);

    {
        FileStreamDevice device(testPath, FileMode::Create);
        device.Write("head", 4);
        device.Write(source.data(), largeSize);
        REQUIRE(device.GetPosition() == largeSize + 4);
    }

    FileStreamDevice device(testPath);
    REQUIRE(device.GetLength() == largeSize + 4);
    char ch;
    REQUIRE(device.Read(ch));
    REQUIRE(ch == 'h');

    charbuff read(largeSize);
    device.Read(read.data(), largeSize);
    REQUIRE(device.GetPosition() == largeSize + 1);
    REQUIRE(string_view(read).substr(3) == string_view(source).substr(0, largeSize - 3));
}

TEST_CASE("TestFileDeviceSeekRetainsBuffer")
{
    // A seek landing inside the already filled buffer re-arms the window
    // instead of refilling. The bytes served must be the right ones
    constexpr size_t fileSize = DeviceBufferSize * 3;
    auto testPath = createTestFile("TestFileDeviceRetain.bin", fileSize);
    charbuff expected;
    utls::ReadTo(expected, testPath);

    FileStreamDevice device(testPath);
    char ch;

    // Fill the buffer, then walk back and forth inside it
    REQUIRE(device.Read(ch));
    for (size_t pos : { (size_t)0, (size_t)4095, (size_t)1, (size_t)2000, (size_t)4095 })
    {
        device.Seek(pos);
        REQUIRE(device.GetPosition() == pos);
        REQUIRE(device.Read(ch));
        REQUIRE(ch == expected[pos]);
        REQUIRE(device.GetPosition() == pos + 1);
    }

    // An End seek leaves the OS file offset away from the end of the retained
    // content, so a seek back into it must restore the offset before the next
    // fill. Reading across the end of the retained range is what exposes it
    device.Seek(-10, SeekDirection::End);
    device.Seek(DeviceBufferSize - 2);
    REQUIRE(device.GetPosition() == DeviceBufferSize - 2);
    charbuff crossing(8);
    device.Read(crossing.data(), 8);
    REQUIRE(string_view(crossing) == string_view(expected).substr(DeviceBufferSize - 2, 8));
    REQUIRE(device.GetPosition() == DeviceBufferSize + 6);

    // Out of the buffer, then back into the one filled afterwards
    for (size_t pos : { DeviceBufferSize * 2 + 5, DeviceBufferSize * 2 + 1, (size_t)10,
        DeviceBufferSize, fileSize - 1 })
    {
        device.Seek(pos);
        REQUIRE(device.GetPosition() == pos);
        REQUIRE(device.Read(ch));
        REQUIRE(ch == expected[pos]);
    }

    // A retained seek must not resurrect a stale EOF
    device.Seek(0, SeekDirection::End);
    REQUIRE(!device.Read(ch));
    REQUIRE(device.Eof());
    device.Seek(fileSize - 1);
    REQUIRE(!device.Eof());
    REQUIRE(device.Read(ch));
    REQUIRE(ch == expected[fileSize - 1]);
}

TEST_CASE("TestStringStreamWriteOwnContent")
{
    // Writing the content of the stream into itself, which grows the buffer
    PdfStringStream stream;
    stream << "0123456789";
    string content(stream.GetString());
    for (unsigned i = 0; i < 4; i++)
    {
        stream << stream.GetString();
        content += content;
    }

    stream << stream.GetString().substr(0, 10);
    content += content.substr(0, 10);
    REQUIRE(stream.GetString() == content);
}

TEST_CASE("TestWriteOnlyDeviceCopyTo")
{
    // Reading from a write only device is rejected also when copying
    ostringstream out;
    StandardStreamDevice writeOnly(out);
    charbuff buffer;
    BufferStreamDevice destination(buffer);
    ASSERT_THROW_WITH_ERROR_CODE(writeOnly.CopyTo(destination), PdfErrorCode::InternalLogic);
    ASSERT_THROW_WITH_ERROR_CODE(writeOnly.CopyTo(destination, 10), PdfErrorCode::InternalLogic);
}

TEST_CASE("TestReadOnlyDeviceFlush")
{
    // Flushing a read only device is rejected, also through an empty copy
    istringstream in("data");
    StandardStreamDevice readOnly(in);
    ASSERT_THROW_WITH_ERROR_CODE(readOnly.Flush(), PdfErrorCode::InternalLogic);
    SpanStreamDevice empty(""sv);
    ASSERT_THROW_WITH_ERROR_CODE(empty.CopyTo(readOnly), PdfErrorCode::InternalLogic);
}

TEST_CASE("TestStreamedDocumentUnwrittenStream")
{
    // A stream never written can't be read back when finishing the document:
    // the failure is reported instead of escaping the destructor
    charbuff buffer;
    {
        auto device = std::make_shared<BufferStreamDevice>(buffer);
        PdfStreamedDocument doc(device);
        doc.GetPages().CreatePage(PdfPageSize::A4);
        auto& obj = doc.GetObjects().CreateDictionaryObject();
        obj.GetOrCreateStream();
    }

    REQUIRE(string_view(buffer).substr(0, 5) == "%PDF-");
}

TEST_CASE("TestStreamedDocumentRewriteStream")
{
    // An object stream can be written to the streamed device only once
    charbuff buffer;
    auto device = std::make_shared<BufferStreamDevice>(buffer);
    PdfStreamedDocument doc(device);
    auto form = doc.CreateXObjectForm(Rect(0, 0, 10, 10));
    auto& objStream = form->GetObject().GetOrCreateStream();
    objStream.SetData(string_view("0 0 m 10 10 l S"));
    ASSERT_THROW_WITH_ERROR_CODE(objStream.SetData(string_view("1 1 m 9 9 l S")), PdfErrorCode::ChangeOnImmutable);
}

TEST_CASE("TestStreamedDocumentResetRejected")
{
    // Reset is rejected before anything is cleared, as the writer keeps
    // pointers to the trailer and to the objects already streamed
    charbuff buffer;
    {
        auto device = std::make_shared<BufferStreamDevice>(buffer);
        PdfStreamedDocument doc(device);
        doc.GetPages().CreatePage(PdfPageSize::A4);
        auto form = doc.CreateXObjectForm(Rect(0, 0, 10, 10));
        form->GetObject().GetOrCreateStream().SetData(string_view("0 0 m 10 10 l S"));
        ASSERT_THROW_WITH_ERROR_CODE(doc.Reset(), PdfErrorCode::NotImplemented);
        REQUIRE(doc.GetPages().GetCount() == 1);
    }

    PdfMemDocument loaded;
    loaded.LoadFromBuffer(buffer);
    REQUIRE(loaded.GetPages().GetCount() == 1);
}

TEST_CASE("TestStreamedDocumentDeviceFailure")
{
    // A device failing while the streams and the document are finished:
    // the failures are reported instead of escaping the destructors
    class FailingStringBuf final : public stringbuf
    {
    public:
        bool Fail = false;
    protected:
        streamsize xsputn(const char* s, streamsize n) override
        {
            return Fail ? 0 : stringbuf::xsputn(s, n);
        }
        int_type overflow(int_type ch) override
        {
            return Fail ? traits_type::eof() : stringbuf::overflow(ch);
        }
        int sync() override
        {
            return Fail ? -1 : stringbuf::sync();
        }
    };

    FailingStringBuf buf;
    ostream stream(&buf);
    {
        PdfStreamedDocument doc(std::make_shared<StandardStreamDevice>(stream));
        auto& obj = doc.GetObjects().CreateDictionaryObject();
        auto output = obj.GetOrCreateStream().GetOutputStreamRaw();
        output.Write("data"sv);
        buf.Fail = true;
    }

    REQUIRE(buf.str().substr(0, 5) == "%PDF-");
}

TEST_CASE("TestStreamedDocumentCollectGarbageRejected")
{
    // Garbage collection would free objects the writer still references
    charbuff buffer;
    auto device = std::make_shared<BufferStreamDevice>(buffer);
    PdfStreamedDocument doc(device);
    ASSERT_THROW_WITH_ERROR_CODE(doc.CollectGarbage(), PdfErrorCode::NotImplemented);
    ASSERT_THROW_WITH_ERROR_CODE(doc.GetObjects().CollectGarbage(), PdfErrorCode::NotImplemented);
}

string createTestFile(const string_view& filename, size_t size)
{
    charbuff buffer(size);
    for (size_t i = 0; i < size; i++)
        buffer[i] = (char)('a' + (i % 26));

    TestUtils::WriteTestOutputFileTo(filename, buffer);
    return TestUtils::GetTestOutputFilePath(filename);
}
