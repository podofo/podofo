/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: MIT-0
 */

#include <PdfTest.h>
#include <podofo/private/OpenSSLInternal.h>

using namespace std;
using namespace PoDoFo;

constexpr string_view TestSignatureRefHash = "1CC60CEA1A7A8D3ECDD18B20FAAAEFE7"sv;

TEST_CASE("TestLoadCertificate")
{
    // Load a PEM certificate now works
    string cert;
    TestUtils::ReadTestInputFile("mycert.pem", cert);

    PdfSignerCms signer(cert);
    // Dummy data append to enforce certificate load
    signer.AppendData("");
}

// Test signing with supplied private key
TEST_CASE("TestSignature1")
{
    charbuff buff;
    auto inputPath = TestUtils::GetTestInputFilePath("TestSignature.pdf");
    auto outputPath = TestUtils::GetTestOutputFilePath("TestSignature1.pdf");

    // RSA Private key coefficients in der PKCS1 format (binary)
    string pkey1;
    TestUtils::ReadTestInputFile("mykey-pkcs1.der", pkey1);

    // RSA Private key coefficients in der PKCS8 format (binary)
    string pkey8;
    TestUtils::ReadTestInputFile("mykey-pkcs8.der", pkey8);

    auto testSignature = [&](const shared_ptr<StreamDevice>& stream, const bufferview& pkey)
    {
        // X509 Certificate
        string cert;
        TestUtils::ReadTestInputFile("mycert.der", cert);

        PdfMemDocument doc(stream);
        auto& page = doc.GetPages().GetPageAt(0);
        auto& annot = page.GetAnnotations().GetAnnotAt(0);
        auto& field = dynamic_cast<PdfAnnotationWidget&>(annot).GetField();
        auto& signature = dynamic_cast<PdfSignature&>(field);

        auto signer = PdfSignerCms(cert, pkey);
        PoDoFo::SignDocument(doc, *stream, signer, signature, PdfSaveOptions::NoMetadataUpdate);
    };

    {
        FileStreamDevice input(inputPath);
        stringstream ss;
        auto stream = std::make_shared<StandardStreamDevice>(ss);
        input.CopyTo(*stream);
        testSignature(stream, pkey1);
        REQUIRE(ssl::ComputeMD5Str(ss.str()) == TestSignatureRefHash);
    }

    {
        utls::ReadTo(buff, inputPath);
        auto stream = std::make_shared<BufferStreamDevice>(buff);
        testSignature(stream, pkey8);
        REQUIRE(ssl::ComputeMD5Str(buff) == TestSignatureRefHash);
    }

    {
        fs::copy_file(fs::u8path(inputPath), fs::u8path(outputPath), fs::copy_options::overwrite_existing);
        auto stream = std::make_shared<FileStreamDevice>(outputPath, FileMode::Open);
        testSignature(stream, pkey8);
        utls::ReadTo(buff, outputPath);
        REQUIRE(ssl::ComputeMD5Str(buff) == TestSignatureRefHash);
    }
}

// Test event driven signing with external service
TEST_CASE("TestSignature2")
{
    charbuff buff;
    auto inputPath = TestUtils::GetTestInputFilePath("TestSignature.pdf");
    auto outputPath = TestUtils::GetTestOutputFilePath("TestSignature2.pdf");

    fs::copy_file(fs::u8path(inputPath), fs::u8path(outputPath), fs::copy_options::overwrite_existing);
    auto stream = std::make_shared<FileStreamDevice>(outputPath, FileMode::Open);

    // X509 Certificate
    string cert;
    TestUtils::ReadTestInputFile("mycert.der", cert);

    // RSA Private key coefficients in der format (binary)
    string pkey;
    TestUtils::ReadTestInputFile("mykey-pkcs1.der", pkey);

    PdfMemDocument doc(stream);
    auto& page = doc.GetPages().GetPageAt(0);
    auto& annot = page.GetAnnotations().GetAnnotAt(0);
    auto& field = dynamic_cast<PdfAnnotationWidget&>(annot).GetField();
    auto& signature = dynamic_cast<PdfSignature&>(field);

    PdfSignerCmsParams params;
    params.SigningService = [&pkey, &params](bufferview hashToSign, bool dryrun, charbuff& signedHash)
    {
        (void)dryrun;
        ssl::DoSign(hashToSign, pkey, params.Hashing, signedHash);
    };
    auto signer = PdfSignerCms(cert, params);
    PoDoFo::SignDocument(doc, *stream, signer, signature, PdfSaveOptions::NoMetadataUpdate);

    utls::ReadTo(buff, outputPath);
    REQUIRE(ssl::ComputeMD5Str(buff) == TestSignatureRefHash);

    // Resign should work
    PoDoFo::SignDocument(doc, *stream, signer, signature, PdfSaveOptions::NoMetadataUpdate);
    utls::ReadTo(buff, outputPath);
    REQUIRE(ssl::ComputeMD5Str(buff) == "F4038250AC2A81F552CF34A317619B86");
}

// Test deferred signing with external service
TEST_CASE("TestSignature3")
{
    charbuff buff;
    auto inputPath = TestUtils::GetTestInputFilePath("TestSignature.pdf");
    auto outputPath = TestUtils::GetTestOutputFilePath("TestSignature3.pdf");

    fs::copy_file(fs::u8path(inputPath), fs::u8path(outputPath), fs::copy_options::overwrite_existing);
    auto stream = std::make_shared<FileStreamDevice>(outputPath, FileMode::Open);

    // X509 Certificate
    string cert;
    TestUtils::ReadTestInputFile("mycert.der", cert);

    // RSA Private key coefficients in der format (binary)
    string pkey;
    TestUtils::ReadTestInputFile("mykey-pkcs8.der", pkey);

    PdfMemDocument doc(stream);
    auto& page = doc.GetPages().GetPageAt(0);
    auto& annot = page.GetAnnotations().GetAnnotAt(0);
    auto& field = dynamic_cast<PdfAnnotationWidget&>(annot).GetField();
    auto& signature = dynamic_cast<PdfSignature&>(field);

    PdfSignerCmsParams params;
    auto signer = std::make_shared<PdfSignerCms>(cert, params);
    PdfSigningContext ctx;
    auto signerId = ctx.AddSigner(signature, signer);
    PdfSigningResults results;
    ctx.StartSigning(doc, stream, results, PdfSaveOptions::NoMetadataUpdate);
    charbuff signedHash;
    ssl::DoSign(results.Intermediate[signerId], pkey, params.Hashing, signedHash);
    results.Intermediate[signerId] = signedHash;
    ctx.FinishSigning(results);
    
    utls::ReadTo(buff, outputPath);
    REQUIRE(ssl::ComputeMD5Str(buff) == TestSignatureRefHash);
}

TEST_CASE("TestSignEncryptedDoc")
{
    auto inputPath = TestUtils::GetTestInputFilePath("AESV3R6-256.pdf");
    auto outputPath = TestUtils::GetTestOutputFilePath("TestSignEncryptedDoc.pdf");

    fs::copy_file(fs::u8path(inputPath), fs::u8path(outputPath), fs::copy_options::overwrite_existing);
    auto stream = std::make_shared<FileStreamDevice>(outputPath, FileMode::Open);

    // X509 Certificate
    string cert;
    TestUtils::ReadTestInputFile("mycert.der", cert);

    // RSA Private key coefficients in der format (binary)
    string pkey;
    TestUtils::ReadTestInputFile("mykey-pkcs8.der", pkey);

    auto date = PdfDate::ParseW3C("2024-07-31T17:03:42+02:00");

    {
        PdfMemDocument doc(stream, "userpass");
        auto& page = doc.GetPages().GetPageAt(0);
        auto& signature = page.CreateField<PdfSignature>("Signature", Rect());
        signature.SetSignatureDate(date);
        auto signer = PdfSignerCms(cert, pkey);
        PoDoFo::SignDocument(doc, *stream, signer, signature, PdfSaveOptions::NoMetadataUpdate);
    }

    {
        // Just reload the signed document with owner password as a simple test
        PdfMemDocument doc(stream, "ownerpass");
        auto& page = doc.GetPages().GetPageAt(0);
        auto& annot = page.GetAnnotations().GetAnnotAt(0);
        auto& field = dynamic_cast<PdfAnnotationWidget&>(annot).GetField();
        auto& signature = dynamic_cast<PdfSignature&>(field);
        REQUIRE(signature.GetSignatureDate() == date);
    }
}

TEST_CASE("TestSaveOnSigning")
{
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    string x509certbuffer;
    TestUtils::ReadTestInputFile("mycert.der", x509certbuffer);

    string pkeybuffer;
    TestUtils::ReadTestInputFile("mykey-pkcs8.der", pkeybuffer);

    auto& signature = page.CreateField<PdfSignature>("Signature", Rect(100, 600, 100, 100));
    signature.SetSignatureDate(PdfDate::LocalNow());
    auto image = doc.CreateImage();
    image->Load(TestUtils::GetTestInputFilePath("ReferenceImage.png"));
    auto xformObj = doc.CreateXObjectForm(Rect(0, 0, image->GetWidth(), image->GetHeight()));

    PdfPainter painter;
    painter.SetCanvas(*xformObj);
    painter.DrawImage(*image, 0, 0, 1, 1);
    painter.FinishDrawing();

    auto signer = PdfSignerCms(x509certbuffer, pkeybuffer);

    signature.MustGetWidget().SetAppearanceStream(*xformObj);

    FileStreamDevice output(TestUtils::GetTestOutputFilePath("TestSaveOnSigning.pdf"), FileMode::Create);
    PoDoFo::SignDocument(doc, output, signer, signature, PdfSaveOptions::SaveOnSigning);
}

TEST_CASE("TestPdfSignerCms")
{
    // X509 Certificate
    string cert;
    TestUtils::ReadTestInputFile("mycert.der", cert);

    charbuff buff;
    {
        PdfSignerCms signer(cert);
        signer.ComputeSignatureDeferred({ }, buff, true);

        try
        {
            signer.ComputeSignature(buff, true);
        }
        catch (PdfError& error)
        {
            // If a deferred signing is started we can't switch to event based
            REQUIRE(error.GetCode() == PdfErrorCode::InternalLogic);
        }
    }

    {
        PdfSignerCms signer(cert);
        try
        {
            signer.ComputeSignature(buff, true);
        }
        catch (PdfError& error)
        {
            // An event based signing requires a private key or a signing service
            REQUIRE(error.GetCode() == PdfErrorCode::InternalLogic);
        }
    }

    {
        PdfSignerCmsParams params;
        params.SigningService = [](bufferview, bool, charbuff&)
        {
            // Do nothing
        };

        PdfSignerCms signer(cert, params);
        signer.ComputeSignature(buff, true);

        try
        {
            signer.ComputeSignatureDeferred({ }, buff, true);
        }
        catch (PdfError& error)
        {
            // If a event based signing is started we can't switch to deferred
            REQUIRE(error.GetCode() == PdfErrorCode::InternalLogic);
        }
    }
}

TEST_CASE("TestGetPreviousRevision")
{
    {
        charbuff currBuffer;

        utls::ReadTo(currBuffer, TestUtils::GetTestInputFilePath("TestBlankSigned.pdf"));
        auto input = std::make_shared<BufferStreamDevice>(currBuffer);

        PdfMemDocument doc;
        doc.Load(input);
        auto& signature = dynamic_cast<PdfSignature&>(
            dynamic_cast<PdfAnnotationWidget&>(
                doc.GetPages().GetPageAt(0).GetAnnotations().GetAnnotAt(0)).GetField());

        charbuff prevBuffer;
        BufferStreamDevice output(prevBuffer);

        REQUIRE(signature.TryGetPreviousRevision(*input, output));

        charbuff refBuffer;
        utls::ReadTo(refBuffer, TestUtils::GetTestInputFilePath("blank.pdf"));
        REQUIRE(prevBuffer == refBuffer);
    }

    {
        charbuff currBuffer;

        utls::ReadTo(currBuffer, TestUtils::GetTestInputFilePath("TestSaveOnSigning.pdf"));
        auto input = std::make_shared<BufferStreamDevice>(currBuffer);

        PdfMemDocument doc;
        doc.Load(input);
        auto& signature = dynamic_cast<PdfSignature&>(
            dynamic_cast<PdfAnnotationWidget&>(
                doc.GetPages().GetPageAt(0).GetAnnotations().GetAnnotAt(0)).GetField());

        charbuff prevBuffer;
        BufferStreamDevice output(prevBuffer);

        // This file is signed but has not incremental updates,
        // so the previous revision is undefined
        REQUIRE(!signature.TryGetPreviousRevision(*input, output));
    }
}
