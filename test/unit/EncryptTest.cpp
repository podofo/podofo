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

static void testAuthenticate(PdfEncrypt& encrypt, PdfEncryptContext& context);
static void testEncrypt(PdfEncrypt& encrypt, PdfEncryptContext& context);
static void createEncryptedPdf(const string_view& filename);

charbuff s_encBuffer;
PdfPermissions s_protection;

constexpr string_view ReferenceHash_R_11_0("298ACCFDC32BB2BC32BFD580883219AB");

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

METHOD_AS_TEST_CASE(PdfEncryptTest::TestLoadEncrypedFilePdfParser, "TestLoadEncrypedFilePdfParser")

TEST_CASE("TestEncryptedPDFs")
{
    charbuff buffer;
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TemplateClearText.pdf"));
    doc.GetObjects().MustGetObject(PdfReference(11, 0)).MustGetStream().CopyTo(buffer);
    REQUIRE(ssl::ComputeMD5Str(buffer) == ReferenceHash_R_11_0);

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
        REQUIRE(ssl::ComputeMD5Str(buffer) == ReferenceHash_R_11_0);

        doc.Load(path, "ownerpass");
        doc.GetObjects().MustGetObject(PdfReference(11, 0)).MustGetStream().CopyTo(buffer);
        REQUIRE(ssl::ComputeMD5Str(buffer) == ReferenceHash_R_11_0);
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
        REQUIRE(ssl::ComputeMD5Str(objBuffer) == ReferenceHash_R_11_0);

        doc.LoadFromBuffer(pdfBuffer, "ownerpass");
        doc.GetObjects().MustGetObject(PdfReference(11, 0)).MustGetStream().CopyTo(objBuffer);
        REQUIRE(ssl::ComputeMD5Str(objBuffer) == ReferenceHash_R_11_0);
    }
}

TEST_CASE("TestDefaultEncryption")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD);

    PdfEncryptContext context;
    testAuthenticate(*encrypt, context);
    testEncrypt(*encrypt, context);
}

TEST_CASE("TestRC4")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::RC4V1,
        PdfKeyLength::L40);

    PdfEncryptContext context;
    testAuthenticate(*encrypt, context);
    testEncrypt(*encrypt, context);
}

TEST_CASE("TestRC4v2_40")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::RC4V2,
        PdfKeyLength::L40);

    PdfEncryptContext context;
    testAuthenticate(*encrypt, context);
    testEncrypt(*encrypt, context);
}

TEST_CASE("TestRC4v2_56")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::RC4V2,
        PdfKeyLength::L56);

    PdfEncryptContext context;
    testAuthenticate(*encrypt, context);
    testEncrypt(*encrypt, context);
}

TEST_CASE("TestRC4v2_80")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::RC4V2,
        PdfKeyLength::L80);

    PdfEncryptContext context;
    testAuthenticate(*encrypt, context);
    testEncrypt(*encrypt, context);
}

TEST_CASE("TestRC4v2_96")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::RC4V2,
        PdfKeyLength::L96);

    PdfEncryptContext context;
    testAuthenticate(*encrypt, context);
    testEncrypt(*encrypt, context);
}

TEST_CASE("TestRC4v2_128")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::RC4V2,
        PdfKeyLength::L128);

    PdfEncryptContext context;
    testAuthenticate(*encrypt, context);
    testEncrypt(*encrypt, context);
}

TEST_CASE("TestAESV2")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::AESV2,
        PdfKeyLength::L128);

    PdfEncryptContext context;
    testAuthenticate(*encrypt, context);
    // AES decryption is not yet implemented.
    // Therefore we have to disable this test.
    //TestEncrypt(encrypt, context);
}

TEST_CASE("TestAESV3R5")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::AESV3R5,
        PdfKeyLength::L256);

    PdfEncryptContext context;
    testAuthenticate(*encrypt, context);
    // AES decryption is not yet implemented.
    // Therefore we have to disable this test.
    //TestEncrypt(encrypt, context);
}

TEST_CASE("TestAESV3R6")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptionAlgorithm::AESV3R6,
        PdfKeyLength::L256);

    PdfEncryptContext context;
    testAuthenticate(*encrypt, context);
    // AES decryption is not yet implemented.
    // Therefore we have to disable this test.
    //TestEncrypt(encrypt, context);
}

TEST_CASE("TestEnableAlgorithms")
{
    // By default every algorithms should be enabled
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::RC4V1));
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::RC4V2));
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::AESV2));
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::AESV3R5));
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::AESV3R6));

    PdfEncryptionAlgorithm testAlgorithms = PdfEncryptionAlgorithm::AESV2;
    testAlgorithms |= PdfEncryptionAlgorithm::RC4V1 | PdfEncryptionAlgorithm::RC4V2;
    testAlgorithms |= PdfEncryptionAlgorithm::AESV3R5 | PdfEncryptionAlgorithm::AESV3R6;;
    REQUIRE(testAlgorithms == PdfEncrypt::GetEnabledEncryptionAlgorithms());
}

void PdfEncryptTest::TestLoadEncrypedFilePdfParser()
{
    string tempFile = TestUtils::GetTestOutputFilePath("TestLoadEncrypedFilePdfParser.pdf");
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

TEST_CASE("TestLoadEncrypedFilePdfMemDocument")
{
    string tempFile = TestUtils::GetTestOutputFilePath("TestLoadEncrypedFilePdfMemDocument.pdf");
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
        (void)doc.GetPages().CreatePage(PdfPageSize::A4);
        auto& obj = doc.GetObjects().CreateDictionaryObject();
        {
            vector<char> testBuff(BufferSize);
            auto stream = obj.GetOrCreateStream().GetOutputStream(PdfFilterList());
            stream.Write(testBuff.data(), testBuff.size());
        }
        bufferRef = obj.GetIndirectReference();
        doc.GetCatalog().GetDictionary().AddKeyIndirect("TestBigBuffer"_n, obj);

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

TEST_CASE("TestEncryptMetadataFalse")
{
    PdfMemDocument doc;
    // This one has /EncryptMetadata false and /Filter[/Crypt] in /Metadata
    doc.Load(TestUtils::GetTestInputFilePath("EncryptMetadataFalseCrypt.pdf"), "userpass");
    REQUIRE(doc.GetMetadata().GetProducer()->GetString() == "PoDoFo - http://podofo.sf.net");

    // This one has /EncryptMetadata false and no /Filter in /Metadata. Should still work
    doc.Load(TestUtils::GetTestInputFilePath("EncryptMetadataFalseNoCrypt.pdf"), "userpass");
    REQUIRE(doc.GetMetadata().GetProducer()->GetString() == "PoDoFo - http://podofo.sf.net");
}

TEST_CASE("TestRemoveEncryption")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("AESV2-128.pdf"), "userpass");
    doc.SetEncrypt(nullptr);
    doc.Save(TestUtils::GetTestOutputFilePath("Decrypted.pdf"));
    doc.Load(TestUtils::GetTestOutputFilePath("Decrypted.pdf"));
    charbuff objBuffer;
    doc.GetObjects().MustGetObject(PdfReference(11, 0)).MustGetStream().CopyTo(objBuffer);
    REQUIRE(ssl::ComputeMD5Str(objBuffer) == ReferenceHash_R_11_0);
}

void testAuthenticate(PdfEncrypt& encrypt, PdfEncryptContext& context)
{
    PdfString documentId = PdfString::FromHexData("BF37541A9083A51619AD5924ECF156DF");

    encrypt.EnsureEncryptionInitialized(documentId, context);

    PdfEncryptContext authenticationTestContext;
    INFO("authenticate using user password");
    encrypt.Authenticate(PDF_USER_PASSWORD, documentId, authenticationTestContext);
    REQUIRE(authenticationTestContext.GetAuthResult() == PdfAuthResult::User);
    INFO("authenticate using owner password");
    encrypt.Authenticate(PDF_OWNER_PASSWORD, documentId, authenticationTestContext);
    REQUIRE(authenticationTestContext.GetAuthResult() == PdfAuthResult::Owner);
    INFO("authenticate using wrong password");
    encrypt.Authenticate("wrongpassword", documentId, authenticationTestContext);
    REQUIRE(authenticationTestContext.GetAuthResult() == PdfAuthResult::Failed);
}

TEST_CASE("TestPreserveEncrypt")
{
    vector<string> testPaths = {
        TestUtils::GetTestInputFilePath("AESV3R6-256.pdf"),
        TestUtils::GetTestInputFilePath("AESV2-128.pdf"),
        TestUtils::GetTestInputFilePath("RC4V2-128.pdf")
    };

    // Saving the PDF should preserve both user/owner authorizations

    PdfMemDocument doc;
    charbuff pdfBuffer;
    charbuff objBuffer;
    auto testSave = [&](bool incremental, bool isOwner)
    {
        for (auto& path : testPaths)
        {
            doc.Load(path, isOwner ? "ownerpass" : "userpass");
            pdfBuffer.clear();
            BufferStreamDevice device(pdfBuffer);
            if (incremental)
            {
                utls::ReadTo(pdfBuffer, path);
                doc.SaveUpdate(device);
            }
            else
            {
                doc.Save(device);
            }

            doc.LoadFromBuffer(pdfBuffer, isOwner ? "userpass" : "ownerpass");
            doc.GetObjects().MustGetObject(PdfReference(11, 0)).MustGetStream().CopyTo(objBuffer);
            REQUIRE(ssl::ComputeMD5Str(objBuffer) == ReferenceHash_R_11_0);
        }
    };

    // Try all combinations regular/incremental save and user/owner access
    testSave(false, false);
    testSave(false, true);
    testSave(true, false);
    testSave(true, true);
}

void testEncrypt(PdfEncrypt& encrypt, PdfEncryptContext& context)
{
    charbuff encrypted;
    // Encrypt buffer
    try
    {
        encrypt.EncryptTo(encrypted, s_encBuffer, context, PdfReference(7, 0));
    }
    catch (PdfError& e)
    {
        FAIL(e.ErrorMessage(e.GetCode()));
    }

    charbuff decrypted;
    // Decrypt buffer
    try
    {
        encrypt.DecryptTo(decrypted, encrypted, context, PdfReference(7, 0));
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
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
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
