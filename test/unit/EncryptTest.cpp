/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>
#include <podofo/private/PdfParser.h>
#include <podofo/private/OpenSSLInternal.h>

using namespace std;
using namespace PoDoFo;

static void testAuthenticate(PdfEncrypt& encrypt);
static void testEncrypt(PdfEncrypt& encrypt);
static void createEncryptedPdf(const string_view& filename);

charbuff s_encBuffer;
PdfPermissions s_protection;

constexpr string_view ReferenceHash("298ACCFDC32BB2BC32BFD580883219AB");

#define PDF_USER_PASSWORD "user"
#define PDF_OWNER_PASSWORD "podofo"

namespace PoDoFo
{
    class PdfEncryptTest
    {
    public:
        static void TestLoadEncrypedFilePdfParser();
    };
}

namespace
{
    struct Paths
    {
        Paths()
        {
            const char* buffer1 = "Somekind of drawing \001 buffer that possibly \003 could contain PDF drawing commands";
            const char* buffer2 = " possibly could contain PDF drawing\003  commands";

            size_t len = strlen(buffer1) + 2 * strlen(buffer2);
            s_encBuffer.resize(len);

            memcpy(s_encBuffer.data(), buffer1, strlen(buffer1) * sizeof(char));
            memcpy(s_encBuffer.data() + strlen(buffer1), buffer2, strlen(buffer2));
            memcpy(s_encBuffer.data() + strlen(buffer1) + strlen(buffer2), buffer2, strlen(buffer2));

            s_protection = PdfPermissions::Print |
                PdfPermissions::Edit |
                PdfPermissions::Copy |
                PdfPermissions::EditNotes |
                PdfPermissions::FillAndSign |
                PdfPermissions::Accessible |
                PdfPermissions::DocAssembly |
                PdfPermissions::HighPrint;
        }
    } s_init;
}

METHOD_AS_TEST_CASE(PdfEncryptTest::TestLoadEncrypedFilePdfParser)

TEST_CASE("TestEncryptedPDFs")
{
    charbuff buffer;
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TemplateClearText.pdf"));
    doc.GetObjects().MustGetObject(PdfReference(11, 0)).MustGetStream().CopyTo(buffer);
    REQUIRE(ssl::ComputeMD5Str(buffer) == ReferenceHash);

    vector<string> testPaths = {
        TestUtils::GetTestInputFilePath("RC4V2-40.pdf"),
        TestUtils::GetTestInputFilePath("RC4V2-56.pdf"),
        TestUtils::GetTestInputFilePath("RC4V2-80.pdf"),
        TestUtils::GetTestInputFilePath("RC4V2-96.pdf"),
        TestUtils::GetTestInputFilePath("RC4V2-128.pdf"),
        TestUtils::GetTestInputFilePath("AESV2-128.pdf"),
        TestUtils::GetTestInputFilePath("AESV3R6-256.pdf"),
        TestUtils::GetTestInputFilePath("RC4V2-40_KeyLength41Violation.pdf"),
        TestUtils::GetTestInputFilePath("RC4V2-56_KeyLength57Violation.pdf"),
        TestUtils::GetTestInputFilePath("RC4V2-80_KeyLength81Violation.pdf"),
        TestUtils::GetTestInputFilePath("RC4V2-96_KeyLength97Violation.pdf"),
        TestUtils::GetTestInputFilePath("RC4V2-128_KeyLength129Violation.pdf"),
        TestUtils::GetTestInputFilePath("AESV2-128_KeyLength129Violation.pdf"),
        TestUtils::GetTestInputFilePath("AESV3R6-256_KeyLength257Violation.pdf"),
    };

    for (auto& path : testPaths)
    {
        doc.Load(path, "userpass");
        doc.GetObjects().MustGetObject(PdfReference(11, 0)).MustGetStream().CopyTo(buffer);
        REQUIRE(ssl::ComputeMD5Str(buffer) == ReferenceHash);

        doc.Load(path, "ownerpass");
        doc.GetObjects().MustGetObject(PdfReference(11, 0)).MustGetStream().CopyTo(buffer);
        REQUIRE(ssl::ComputeMD5Str(buffer) == ReferenceHash);
    }
}

TEST_CASE("TestEncryptDecryptPDFs")
{
    PdfEncryptionAlgorithm algorithms[] = {
        PdfEncryptionAlgorithm::RC4V2,
        PdfEncryptionAlgorithm::RC4V2,
        PdfEncryptionAlgorithm::RC4V2,
        PdfEncryptionAlgorithm::RC4V2,
        PdfEncryptionAlgorithm::RC4V2,
        PdfEncryptionAlgorithm::AESV2,
        PdfEncryptionAlgorithm::AESV3R6,
    };

    PdfKeyLength keyLengths[] = {
        PdfKeyLength::L40,
        PdfKeyLength::L56,
        PdfKeyLength::L80,
        PdfKeyLength::L96,
        PdfKeyLength::L128,
        PdfKeyLength::L128,
        PdfKeyLength::L256,
    };

    charbuff pdfBuffer;
    charbuff objBuffer;
    PdfMemDocument doc;
    for (unsigned i = 0; i < std::size(algorithms); i++)
    {
        doc.Load(TestUtils::GetTestInputFilePath("TemplateClearText.pdf"));
        doc.SetEncrypted("userpass", "ownerpass", PdfPermissions::Default,
            algorithms[i], keyLengths[i]);

        pdfBuffer.clear();
        BufferStreamDevice device(pdfBuffer);
        doc.Save(device);

        doc.LoadFromBuffer(pdfBuffer, "userpass");
        doc.GetObjects().MustGetObject(PdfReference(11, 0)).MustGetStream().CopyTo(objBuffer);
        REQUIRE(ssl::ComputeMD5Str(objBuffer) == ReferenceHash);

        doc.LoadFromBuffer(pdfBuffer, "ownerpass");
        doc.GetObjects().MustGetObject(PdfReference(11, 0)).MustGetStream().CopyTo(objBuffer);
        REQUIRE(ssl::ComputeMD5Str(objBuffer) == ReferenceHash);
    }
}

TEST_CASE("TestDefaultEncryption")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD);
    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("TestRC4")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::RC4V1,
        PdfKeyLength::L40);

    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("TestRC4v2_40")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::RC4V2,
        PdfKeyLength::L40);

    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("TestRC4v2_56")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::RC4V2,
        PdfKeyLength::L56);

    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("TestRC4v2_80")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::RC4V2,
        PdfKeyLength::L80);

    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("TestRC4v2_96")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::RC4V2,
        PdfKeyLength::L96);

    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("TestRC4v2_128")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::RC4V2,
        PdfKeyLength::L128);

    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("TestAESV2")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::AESV2,
        PdfKeyLength::L128);

    testAuthenticate(*encrypt);
    // AES decryption is not yet implemented.
    // Therefore we have to disable this test.
    //TestEncrypt(encrypt);
}

#ifdef PODOFO_HAVE_LIBIDN

TEST_CASE("TestAESV3R5")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::AESV3R5,
        PdfKeyLength::L256);

    testAuthenticate(*encrypt);
    // AES decryption is not yet implemented.
    // Therefore we have to disable this test.
    //TestEncrypt(encrypt);
}

TEST_CASE("testAESV3R6")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::AESV3R6,
        PdfKeyLength::L256);

    testAuthenticate(*encrypt);
    // AES decryption is not yet implemented.
    // Therefore we have to disable this test.
    //TestEncrypt(encrypt);
}

#endif // PODOFO_HAVE_LIBIDN

TEST_CASE("testEnableAlgorithms")
{
    // By default every algorithms should be enabled
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::RC4V1));
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::RC4V2));
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::AESV2));
#ifdef PODOFO_HAVE_LIBIDN
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::AESV3R5));
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::AESV3R6));
#endif // PODOFO_HAVE_LIBIDN

    PdfEncryptionAlgorithm testAlgorithms = PdfEncryptionAlgorithm::AESV2;
    testAlgorithms |= PdfEncryptionAlgorithm::RC4V1 | PdfEncryptionAlgorithm::RC4V2;
#ifdef PODOFO_HAVE_LIBIDN
    testAlgorithms |= PdfEncryptionAlgorithm::AESV3R5 | PdfEncryptionAlgorithm::AESV3R6;;
#endif // PODOFO_HAVE_LIBIDN
    REQUIRE(testAlgorithms == PdfEncrypt::GetEnabledEncryptionAlgorithms());
}

void PdfEncryptTest::TestLoadEncrypedFilePdfParser()
{
    string tempFile = TestUtils::GetTestOutputFilePath("testLoadEncrypedFilePdfParser.pdf");
    createEncryptedPdf(tempFile);

    auto device = std::make_shared<FileStreamDevice>(tempFile);
    // Try loading with PdfParser
    PdfIndirectObjectList objects;
    PdfParser parser(objects);

    try
    {
        parser.Parse(*device, true);

        // Must throw an exception
        FAIL("Encrypted file not recognized!");
    }
    catch (PdfError& e)
    {
        if (e.GetCode() != PdfErrorCode::InvalidPassword)
            FAIL("Invalid encryption exception thrown!");
    }

    parser.SetPassword(PDF_USER_PASSWORD);
}

TEST_CASE("testLoadEncrypedFilePdfMemDocument")
{
    string tempFile = TestUtils::GetTestOutputFilePath("testLoadEncrypedFilePdfMemDocument.pdf");
    createEncryptedPdf(tempFile);

    // Try loading with PdfParser
    PdfMemDocument document;
    try
    {
        document.Load(tempFile);

        // Must throw an exception
        FAIL("Encrypted file not recognized!");
    }
    catch (...)
    {

    }

    document.Load(tempFile, PDF_USER_PASSWORD);
}

// Test a big encrypted content writing and reading
TEST_CASE("TestEncryptBigBuffer")
{
    string tempFile = TestUtils::GetTestOutputFilePath("TestBigBuffer.pdf");
    PdfReference bufferRef;

    constexpr unsigned BufferSize = 100000;

    {
        // Create a document with a big enough buffer and ensure it won't
        // be compressed, so the encryption will operate on a big buffer 
        PdfMemDocument doc;
        (void)doc.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
        auto& obj = doc.GetObjects().CreateDictionaryObject();
        {
            vector<char> testBuff(BufferSize);
            auto stream = obj.GetOrCreateStream().GetOutputStream(PdfFilterList());
            stream.Write(testBuff.data(), testBuff.size());
        }
        bufferRef = obj.GetIndirectReference();
        doc.GetCatalog().GetDictionary().AddKeyIndirect("TestBigBuffer", obj);

        doc.SetEncrypted(PDF_USER_PASSWORD, "owner");
        doc.Save(tempFile, PdfSaveOptions::NoFlateCompress);
    }

    {
        PdfMemDocument doc;
        doc.Load(tempFile, PDF_USER_PASSWORD);
        auto& obj = doc.GetObjects().MustGetObject(bufferRef);
        charbuff buff;
        obj.MustGetStream().CopyTo(buff);
        REQUIRE(buff.size() == BufferSize);
    }
}

void testAuthenticate(PdfEncrypt& encrypt)
{
    PdfString documentId = PdfString::FromHexData("BF37541A9083A51619AD5924ECF156DF");

    encrypt.GenerateEncryptionKey(documentId);

    INFO("authenticate using user password");
    REQUIRE(encrypt.Authenticate(PDF_USER_PASSWORD, documentId) == PdfAuthResult::User);
    INFO("authenticate using owner password");
    REQUIRE(encrypt.Authenticate(PDF_OWNER_PASSWORD, documentId) == PdfAuthResult::Owner);
    INFO("authenticate using wrong password");
    REQUIRE(encrypt.Authenticate("wrongpassword", documentId) == PdfAuthResult::Failed);
}

void testEncrypt(PdfEncrypt& encrypt)
{
    charbuff encrypted;
    // Encrypt buffer
    try
    {
        encrypt.EncryptTo(encrypted, s_encBuffer, PdfReference(7, 0));
    }
    catch (PdfError& e)
    {
        FAIL(e.ErrorMessage(e.GetCode()));
    }

    charbuff decrypted;
    // Decrypt buffer
    try
    {
        encrypt.DecryptTo(decrypted, encrypted, PdfReference(7, 0));
    }
    catch (PdfError& e)
    {
        FAIL(e.ErrorMessage(e.GetCode()));
    }

    INFO("compare encrypted and decrypted buffers");
    REQUIRE(memcmp(s_encBuffer.data(), decrypted.data(), s_encBuffer.size()) == 0);
}

void createEncryptedPdf(const string_view& filename)
{
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    PdfPainter painter;
    painter.SetCanvas(page);

    auto font = doc.GetFonts().SearchFont("LiberationSans");
    if (font == nullptr)
        FAIL("Could not find Arial font");

    painter.TextState.SetFont(*font, 16);
    painter.DrawText("Hello World", 100, 100);
    painter.FinishDrawing();

    doc.SetEncrypted(PDF_USER_PASSWORD, "owner");
    doc.Save(filename);

    INFO(utls::Format("Wrote: {} (R={})", filename, doc.GetEncrypt()->GetRevision()));
}
