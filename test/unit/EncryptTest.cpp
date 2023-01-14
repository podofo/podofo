/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

static void testAuthenticate(PdfEncrypt& encrypt);
static void testEncrypt(PdfEncrypt& encrypt);
static void createEncryptedPdf(const string_view& filename);

charbuff s_encBuffer;
PdfPermissions s_protection;

#define PDF_USER_PASSWORD "user"
#define PDF_OWNER_PASSWORD "podofo"

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

TEST_CASE("testDefault")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD);
    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("testRC4")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptAlgorithm::RC4V1,
        PdfKeyLength::L40);

    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("testRC4v2_40")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptAlgorithm::RC4V2,
        PdfKeyLength::L40);

    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("testRC4v2_56")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptAlgorithm::RC4V2,
        PdfKeyLength::L56);

    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("testRC4v2_80")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptAlgorithm::RC4V2,
        PdfKeyLength::L80);

    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("testRC4v2_96")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptAlgorithm::RC4V2,
        PdfKeyLength::L96);

    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("testRC4v2_128")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptAlgorithm::RC4V2,
        PdfKeyLength::L128);

    testAuthenticate(*encrypt);
    testEncrypt(*encrypt);
}

TEST_CASE("testAESV2")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptAlgorithm::AESV2,
        PdfKeyLength::L128);

    testAuthenticate(*encrypt);
    // AES decryption is not yet implemented.
    // Therefore we have to disable this test.
    //TestEncrypt(encrypt);
}

#ifdef PODOFO_HAVE_LIBIDN

TEST_CASE("testAESV3")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptAlgorithm::AESV3,
        PdfKeyLength::L256);

    testAuthenticate(*encrypt);
    // AES decryption is not yet implemented.
    // Therefore we have to disable this test.
    //TestEncrypt(encrypt);
}

TEST_CASE("testAESV3R6")
{
    auto encrypt = PdfEncrypt::Create(PDF_USER_PASSWORD, PDF_OWNER_PASSWORD, s_protection,
        PdfEncryptAlgorithm::AESV3R6,
        PdfKeyLength::L256);

    testAuthenticate(*encrypt);
    // AES decryption is not yet implemented.
    // Therefore we have to disable this test.
    //TestEncrypt(encrypt);
}

#endif // PODOFO_HAVE_LIBIDN

TEST_CASE("testEnableAlgorithms")
{
    auto enabledAlgorithms = PdfEncrypt::GetEnabledEncryptionAlgorithms();

    // By default every algorithms should be enabled
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::RC4V1));
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::RC4V2));
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::AESV2));
#ifdef PODOFO_HAVE_LIBIDN
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::AESV3));
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::AESV3R6));
#endif // PODOFO_HAVE_LIBIDN

    PdfEncryptAlgorithm testAlgorithms = PdfEncryptAlgorithm::AESV2;
    testAlgorithms |= PdfEncryptAlgorithm::RC4V1 | PdfEncryptAlgorithm::RC4V2;
#ifdef PODOFO_HAVE_LIBIDN
    testAlgorithms |= PdfEncryptAlgorithm::AESV3 | PdfEncryptAlgorithm::AESV3R6;;
#endif // PODOFO_HAVE_LIBIDN
    REQUIRE(testAlgorithms == PdfEncrypt::GetEnabledEncryptionAlgorithms());

    // Disable AES
    PdfEncrypt::SetEnabledEncryptionAlgorithms(PdfEncryptAlgorithm::RC4V1 |
        PdfEncryptAlgorithm::RC4V2);

    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::RC4V1));
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::RC4V2));
    REQUIRE(!PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::AESV2));

    REQUIRE((PdfEncryptAlgorithm::RC4V1 | PdfEncryptAlgorithm::RC4V2) ==
        PdfEncrypt::GetEnabledEncryptionAlgorithms());

    PdfObject object;
    object.GetDictionary().AddKey("Filter", PdfName("Standard"));
    object.GetDictionary().AddKey("V", static_cast<int64_t>(4L));
    object.GetDictionary().AddKey("R", static_cast<int64_t>(4L));
    object.GetDictionary().AddKey("P", static_cast<int64_t>(1L));
    object.GetDictionary().AddKey("O", PdfString(""));
    object.GetDictionary().AddKey("U", PdfString(""));

    try
    {
        (void)PdfEncrypt::CreateFromObject(object);
        REQUIRE(false);
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::UnsupportedFilter);
    }

    // Restore default
    PdfEncrypt::SetEnabledEncryptionAlgorithms(enabledAlgorithms);
}

TEST_CASE("testLoadEncrypedFilePdfParser")
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

void testAuthenticate(PdfEncrypt& encrypt)
{
    PdfString documentId = PdfString::FromHexData("BF37541A9083A51619AD5924ECF156DF");

    encrypt.GenerateEncryptionKey(documentId);

    INFO("authenticate using user password");
    REQUIRE(encrypt.Authenticate(PDF_USER_PASSWORD, documentId));
    INFO("authenticate using owner password");
    REQUIRE(encrypt.Authenticate(PDF_OWNER_PASSWORD, documentId));
    INFO("authenticate using wrong password");
    REQUIRE(!encrypt.Authenticate("wrongpassword", documentId));
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
        FAIL("Coult not find Arial font");

    painter.GetTextState().SetFont(*font, 16);
    painter.DrawText("Hello World", 100, 100);
    painter.FinishDrawing();

    doc.SetEncrypted(PDF_USER_PASSWORD, "owner");
    doc.Save(filename);

    INFO(utls::Format("Wrote: {} (R={})", filename, doc.GetEncrypt()->GetRevision()));
}
