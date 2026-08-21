// SPDX-FileCopyrightText: 2023 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>
#include <podofo/private/OpenSSLInternal.h>
#include <podofo/private/CmsVerifyContext.h>

using namespace std;
using namespace PoDoFo;

constexpr string_view TestSignatureRefHash = "E70819B3655094487BEF1C397FC2E87B"sv;
// A date within the validity period of all the test certificates
static const PdfDate TestSignatureDate = PdfDate::Parse("D:20260713192456+01'00'");

TEST_CASE("TestLoadCertificate")
{
    // Load a PEM certificate now works
    string cert;
    TestUtils::ReadTestInputFileTo(cert, "mycert.pem");

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
    TestUtils::ReadTestInputFileTo(pkey1, "mykey-pkcs1.der");

    // RSA Private key coefficients in der PKCS8 format (binary)
    string pkey8;
    TestUtils::ReadTestInputFileTo(pkey8, "mykey-pkcs8.der");

    auto testSignature = [&](const shared_ptr<StreamDevice>& stream, const bufferview& pkey)
    {
        // X509 Certificate
        string cert;
        TestUtils::ReadTestInputFileTo(cert, "mycert.der");

        PdfMemDocument doc(stream);
        auto& page = doc.GetPages().GetPageAt(0);
        auto& annot = page.GetAnnotations().GetAnnotAt(0);
        auto& field = dynamic_cast<PdfAnnotationWidget&>(annot).GetField();
        auto& signature = dynamic_cast<PdfSignature&>(field);
        signature.SetSignatureDate(TestSignatureDate);

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
    TestUtils::ReadTestInputFileTo(cert, "mycert.der");

    // RSA Private key coefficients in der format (binary)
    string pkey;
    TestUtils::ReadTestInputFileTo(pkey, "mykey-pkcs1.der");

    PdfMemDocument doc(stream);
    auto& page = doc.GetPages().GetPageAt(0);
    auto& annot = page.GetAnnotations().GetAnnotAt(0);
    auto& field = dynamic_cast<PdfAnnotationWidget&>(annot).GetField();
    auto& signature = dynamic_cast<PdfSignature&>(field);
    signature.SetSignatureDate(TestSignatureDate);

    PdfSignerCmsParams params;
    params.SigningService = [&pkey, &params](bufferview hashToSign, bool dryrun, charbuff& signedHash)
    {
        (void)dryrun;
        ssl::SignHash(hashToSign, pkey, params.Hashing, signedHash);
    };
    auto signer = PdfSignerCms(cert, params);
    PoDoFo::SignDocument(doc, *stream, signer, signature, PdfSaveOptions::NoMetadataUpdate);

    utls::ReadTo(buff, outputPath);
    REQUIRE(ssl::ComputeMD5Str(buff) == TestSignatureRefHash);

    // Resign should work
    signature.SetCreatingApplication(PdfName("Sample Application"));
    PoDoFo::SignDocument(doc, *stream, signer, signature, PdfSaveOptions::NoMetadataUpdate);
    utls::ReadTo(buff, outputPath);
    REQUIRE(ssl::ComputeMD5Str(buff) == "764174527FF10CC4CA4FC79EAF2C2F9F");
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
    TestUtils::ReadTestInputFileTo(cert, "mycert.der");

    // RSA Private key coefficients in der format (binary)
    string pkey;
    TestUtils::ReadTestInputFileTo(pkey, "mykey-pkcs8.der");

    PdfMemDocument doc(stream);
    auto& page = doc.GetPages().GetPageAt(0);
    auto& annot = page.GetAnnotations().GetAnnotAt(0);
    auto& field = dynamic_cast<PdfAnnotationWidget&>(annot).GetField();
    auto& signature = dynamic_cast<PdfSignature&>(field);
    signature.SetSignatureDate(TestSignatureDate);

    PdfSignerCmsParams params;
    auto signer = std::make_shared<PdfSignerCms>(cert, params);
    PdfSigningContext ctx;
    auto signerId = ctx.AddSigner(signature, signer);
    PdfSigningResults results;
    ctx.StartSigning(doc, stream, results, PdfSaveOptions::NoMetadataUpdate);
    charbuff signedHash;
    ssl::SignHash(results.Intermediate[signerId], pkey, params.Hashing, signedHash);
    results.Intermediate[signerId] = signedHash;
    ctx.FinishSigning(results);
    
    utls::ReadTo(buff, outputPath);
    REQUIRE(ssl::ComputeMD5Str(buff) == TestSignatureRefHash);
}

// Cross-check the signed hash supplied by an external service
TEST_CASE("TestSignedHashVerification")
{
    auto inputPath = TestUtils::GetTestInputFilePath("blank.pdf");
    auto outputPath = TestUtils::GetTestOutputFilePath("TestSignedHashVerification.pdf");

    auto trySign = [&](const string_view& certFile, const string_view& keyFile,
        PdfHashingAlgorithm hashing, PdfSignerCmsFlags flags, bool corrupt)
    {
        string cert;
        TestUtils::ReadTestInputFileTo(cert, certFile);

        string pkey;
        TestUtils::ReadTestInputFileTo(pkey, keyFile);

        fs::copy_file(fs::u8path(inputPath), fs::u8path(outputPath), fs::copy_options::overwrite_existing);
        auto stream = std::make_shared<FileStreamDevice>(outputPath, FileMode::Open);

        PdfMemDocument doc(stream);
        auto& page = doc.GetPages().GetPageAt(0);
        auto& signature = page.CreateField<PdfSignature>("Signature", Rect());
        signature.SetSignatureDate(TestSignatureDate);

        PdfSignerCmsParams params;
        params.Hashing = hashing;
        params.Flags = flags;
        params.SigningService = [&](bufferview hashToSign, bool dryrun, charbuff& signedHash)
        {
            (void)dryrun;
            // The digest is already wrapped when the service asked for it
            ssl::SignHash(hashToSign, pkey, hashing, signedHash,
                (flags & PdfSignerCmsFlags::ServiceDoWrapDigest) != PdfSignerCmsFlags::None);
            if (corrupt)
                signedHash[0] = (char)(signedHash[0] ^ 0xFF);
        };

        PdfSignerCms signer(cert, params);
        PoDoFo::SignDocument(doc, *stream, signer, signature, PdfSaveOptions::NoMetadataUpdate);
    };

    trySign("mycert.der", "mykey-pkcs8.der", PdfHashingAlgorithm::SHA256, PdfSignerCmsFlags::None, false);
    ASSERT_THROW_WITH_ERROR_CODE(trySign("mycert.der", "mykey-pkcs8.der", PdfHashingAlgorithm::SHA256,
        PdfSignerCmsFlags::None, true), PdfErrorCode::SignatureVerificationError);

    // A wrapped digest is verified against the DigestInfo recovered from the signature
    trySign("mycert.der", "mykey-pkcs8.der", PdfHashingAlgorithm::SHA256, PdfSignerCmsFlags::ServiceDoWrapDigest, false);
    ASSERT_THROW_WITH_ERROR_CODE(trySign("mycert.der", "mykey-pkcs8.der", PdfHashingAlgorithm::SHA256,
        PdfSignerCmsFlags::ServiceDoWrapDigest, true), PdfErrorCode::SignatureVerificationError);

    trySign("sha384ECDSA-cert.pem", "sha384ECDSA-key.pem", PdfHashingAlgorithm::SHA384, PdfSignerCmsFlags::None, false);
    ASSERT_THROW_WITH_ERROR_CODE(trySign("sha384ECDSA-cert.pem", "sha384ECDSA-key.pem", PdfHashingAlgorithm::SHA384,
        PdfSignerCmsFlags::None, true), PdfErrorCode::SignatureVerificationError);

#if OPENSSL_VERSION_MAJOR > 3 || (OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR >= 5)
    // ML-DSA signs the DER encoded signed attributes, so they are verified as they are
    trySign(utls::CombinePaths("PQC", "ML-DSA-44-cert.pem"), utls::CombinePaths("PQC", "ML-DSA-44-key.pem"),
        PdfHashingAlgorithm::SHA256, PdfSignerCmsFlags::None, false);
    ASSERT_THROW_WITH_ERROR_CODE(trySign(utls::CombinePaths("PQC", "ML-DSA-44-cert.pem"), utls::CombinePaths("PQC", "ML-DSA-44-key.pem"),
        PdfHashingAlgorithm::SHA256, PdfSignerCmsFlags::None, true), PdfErrorCode::SignatureVerificationError);
#endif // OPENSSL_VERSION_MAJOR > 3 || (OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR >= 5)

    // A corrupted signed hash is accepted when the verification is skipped
    trySign("mycert.der", "mykey-pkcs8.der", PdfHashingAlgorithm::SHA256, PdfSignerCmsFlags::SkipVerification, true);

    // The hash to sign is cached in the dump, so the cross-check survives a context restore
    auto trySignDeferred = [&](bool corrupt)
    {
        string cert;
        TestUtils::ReadTestInputFileTo(cert, "mycert.der");

        string pkey;
        TestUtils::ReadTestInputFileTo(pkey, "mykey-pkcs8.der");

        charbuff buff;
        utls::ReadTo(buff, inputPath);

        charbuff hashToSign;
        PdfSignerId signerId;
        PdfSignerCmsParams params;

        // NOTE: This block simulates loosing all the original objects
        {
            auto stream = std::make_shared<BufferStreamDevice>(buff);
            PdfMemDocument doc(stream);
            auto& page = doc.GetPages().GetPageAt(0);
            auto& signature = page.CreateField<PdfSignature>("Signature", Rect());
            signature.SetSignatureDate(TestSignatureDate);

            auto signer = std::make_shared<PdfSignerCms>(cert, params);
            PdfSigningContext ctx;
            signerId = ctx.AddSigner(signature, signer);
            PdfSigningResults results;
            ctx.StartSigning(doc, stream, results, PdfSaveOptions::NoMetadataUpdate);
            hashToSign = results.Intermediate[signerId];
            ctx.DumpInPlace();
        }

        auto newStream = std::make_shared<BufferStreamDevice>(buff);
        PdfSigningContext newCtx;
        auto doc = newCtx.Restore(newStream);

        charbuff signedHash;
        ssl::SignHash(hashToSign, pkey, params.Hashing, signedHash);
        if (corrupt)
            signedHash[0] = (char)(signedHash[0] ^ 0xFF);

        PdfSigningResults newResults;
        newResults.Intermediate[signerId] = signedHash;
        newCtx.FinishSigning(newResults);
    };

    trySignDeferred(false);
    ASSERT_THROW_WITH_ERROR_CODE(trySignDeferred(true), PdfErrorCode::SignatureVerificationError);
}

// Validate the signature date against the certificate validity period
TEST_CASE("TestSignatureDateValidation")
{
    auto inputPath = TestUtils::GetTestInputFilePath("blank.pdf");
    auto outputPath = TestUtils::GetTestOutputFilePath("TestSignatureDateValidation.pdf");

    string cert;
    TestUtils::ReadTestInputFileTo(cert, "mycert.der");

    string pkey;
    TestUtils::ReadTestInputFileTo(pkey, "mykey-pkcs8.der");

    auto trySign = [&](nullable<const PdfDate&> date, bool skipValidation)
    {
        fs::copy_file(fs::u8path(inputPath), fs::u8path(outputPath), fs::copy_options::overwrite_existing);
        auto stream = std::make_shared<FileStreamDevice>(outputPath, FileMode::Open);

        PdfMemDocument doc(stream);
        auto& page = doc.GetPages().GetPageAt(0);
        auto& signature = page.CreateField<PdfSignature>("Signature", Rect());
        signature.SetSignatureDate(date);

        PdfSigningContext ctx;
        ctx.SetSkipDateValidation(skipValidation);
        ctx.AddSigner(signature, std::make_shared<PdfSignerCms>(cert, pkey));
        ctx.Sign(doc, *stream, PdfSaveOptions::NoMetadataUpdate);
    };

    trySign(TestSignatureDate, false);

    // A missing date makes the validation fail
    ASSERT_THROW_WITH_ERROR_CODE(trySign(nullptr, false), PdfErrorCode::SignatureVerificationError);

    // mycert.der validity period is 2023-01-08 - 2033-01-05
    ASSERT_THROW_WITH_ERROR_CODE(trySign(PdfDate::Parse("D:20220205192456+06'00'"), false),
        PdfErrorCode::SignatureVerificationError);
    ASSERT_THROW_WITH_ERROR_CODE(trySign(PdfDate::Parse("D:20340205192456+06'00'"), false),
        PdfErrorCode::SignatureVerificationError);

    // The validation can be skipped on the signing context
    trySign(nullptr, true);
}

// Test deferred signing with external service and context dumping/restore
TEST_CASE("TestSignatureDumpRestore")
{
    charbuff buff;
    auto inputPath = TestUtils::GetTestInputFilePath("TestSignature.pdf");
    utls::ReadTo(buff, inputPath);

    // X509 Certificate
    string cert;
    TestUtils::ReadTestInputFileTo(cert, "mycert.der");

    // RSA Private key coefficients in der format (binary)
    string pkey;
    TestUtils::ReadTestInputFileTo(pkey, "mykey-pkcs8.der");

    charbuff hashToSign;
    PdfSignerId signerId;
    PdfSignerCmsParams params;

    // NOTE: This block simulates loosing all the original objects
    // and restore the context in a subsequent phase
    {
        auto stream = std::make_shared<BufferStreamDevice>(buff);
        PdfMemDocument doc(stream);
        auto& page = doc.GetPages().GetPageAt(0);
        auto& annot = page.GetAnnotations().GetAnnotAt(0);
        auto& field = dynamic_cast<PdfAnnotationWidget&>(annot).GetField();
        auto& signature = dynamic_cast<PdfSignature&>(field);
        signature.SetSignatureDate(PdfDate::Parse("D:20250205192456+06'00'"));

        auto signer = std::make_shared<PdfSignerCms>(cert, params);
        PdfSigningContext ctx;
        signerId = ctx.AddSigner(signature, signer);
        PdfSigningResults results;
        ctx.StartSigning(doc, stream, results, PdfSaveOptions::NoMetadataUpdate);

        hashToSign = results.Intermediate[signerId];

        ctx.DumpInPlace();
        utls::WriteTo(TestUtils::GetTestOutputFilePath("TestSignatureDumpRestore.bin"), buff);
    }

    auto newStream = std::make_shared<BufferStreamDevice>(buff);
    PdfSigningContext newCtx;
    auto doc = newCtx.Restore(newStream);

    utls::WriteTo(TestUtils::GetTestOutputFilePath("TestSignatureDumpRestore1.pdf"), buff);

    charbuff signedHash;
    ssl::SignHash(hashToSign, pkey, params.Hashing, signedHash);
    PdfSigningResults newResults;
    newResults.Intermediate[signerId] = signedHash;
    newCtx.FinishSigning(newResults);

    utls::WriteTo(TestUtils::GetTestOutputFilePath("TestSignatureDumpRestore2.pdf"), buff);

    REQUIRE(ssl::ComputeMD5Str(buff) == "4162823DB0FD7A43B7A3FDDFE4FDEC38");
}

TEST_CASE("TestCertificateRSA")
{
    {
        string cert;
        TestUtils::ReadTestInputFileTo(cert, "RSA1024Cert.pem");

        PdfSignerCmsParams params;
        PdfSignerCms signer(cert, params);
        REQUIRE(signer.GetSignedHashSize() == 128);
    }

    {
        string cert;
        TestUtils::ReadTestInputFileTo(cert, "RSA3072Cert.pem");

        PdfSignerCmsParams params;
        PdfSignerCms signer(cert, params);
        REQUIRE(signer.GetSignedHashSize() == 384);
    }

    {
        string cert;
        TestUtils::ReadTestInputFileTo(cert, "RSA4096Cert.pem");

        PdfSignerCmsParams params;
        PdfSignerCms signer(cert, params);
        REQUIRE(signer.GetSignedHashSize() == 512);
    }
}

TEST_CASE("TestSignEncryptedDoc")
{
    auto inputPath = TestUtils::GetTestInputFilePath("AESV3R6-256.pdf");
    auto outputPath = TestUtils::GetTestOutputFilePath("TestSignEncryptedDoc.pdf");

    fs::copy_file(fs::u8path(inputPath), fs::u8path(outputPath), fs::copy_options::overwrite_existing);
    auto stream = std::make_shared<FileStreamDevice>(outputPath, FileMode::Open);

    // X509 Certificate
    string cert;
    TestUtils::ReadTestInputFileTo(cert, "mycert.der");

    // RSA Private key coefficients in der format (binary)
    string pkey;
    TestUtils::ReadTestInputFileTo(pkey, "mykey-pkcs8.der");

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
    TestUtils::ReadTestInputFileTo(x509certbuffer, "mycert.der");

    string pkeybuffer;
    TestUtils::ReadTestInputFileTo(pkeybuffer, "mykey-pkcs8.der");

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
    TestUtils::ReadTestInputFileTo(cert, "mycert.der");

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

TEST_CASE("TestSignatureOffsetStart")
{
    string x509certbuffer;
    TestUtils::ReadTestInputFileTo(x509certbuffer, "mycert.der");

    string pkeybuffer;
    TestUtils::ReadTestInputFileTo(pkeybuffer, "mykey-pkcs8.der");

    charbuff currBuffer;
    utls::ReadTo(currBuffer, TestUtils::GetTestInputFilePath("blank-with-offset-start.pdf"));
    auto inputOutput = std::make_shared<BufferStreamDevice>(currBuffer);

    PdfMemDocument doc;
    doc.Load(inputOutput);
    auto& page = doc.GetPages().GetPageAt(0);
    auto& signature = page.CreateField<PdfSignature>("Signature", Rect());
    signature.SetSignatureDate(TestSignatureDate);

    PdfSignerCms signer(x509certbuffer, pkeybuffer);
    PoDoFo::SignDocument(doc, *inputOutput, signer, signature, PdfSaveOptions::NoMetadataUpdate);

    utls::WriteTo(TestUtils::GetTestOutputFilePath("TestSignatureOffsetStart.pdf"), currBuffer);

    // Try to reload the document
    doc.Load(inputOutput);

    REQUIRE(ssl::ComputeMD5Str(currBuffer) == "6775DFDCB9DB412BA849CCF7196F5BC5");
}

TEST_CASE("TestSignatureCorrupted")
{
    auto currentLogSeverity = PdfCommon::GetMaxLoggingSeverity();
    PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::None);
    string x509certbuffer;
    TestUtils::ReadTestInputFileTo(x509certbuffer, "mycert.der");

    string pkeybuffer;
    TestUtils::ReadTestInputFileTo(pkeybuffer, "mykey-pkcs8.der");

    charbuff currBuffer;

    auto doTest = [&currBuffer, &x509certbuffer, &pkeybuffer](string_view outFilename, string_view expectedMD5)
    {
        auto inputOutput = std::make_shared<BufferStreamDevice>(currBuffer);

        PdfMemDocument doc;
        doc.Load(inputOutput);
        auto& page = doc.GetPages().GetPageAt(0);
        auto& signature = page.CreateField<PdfSignature>("Signature", Rect());
        signature.SetSignatureDate(TestSignatureDate);

        PdfSignerCms signer(x509certbuffer, pkeybuffer);

        try
        {
            PoDoFo::SignDocument(doc, *inputOutput, signer, signature, PdfSaveOptions::NoMetadataUpdate);
            throw runtime_error("Signing should have failed with corrupted document");
        }
        catch (const PdfError& e)
        {
            REQUIRE(e.GetCode() == PdfErrorCode::InvalidXRef);
        }

        PoDoFo::SignDocument(doc, *inputOutput, signer, signature, PdfSaveOptions::NoMetadataUpdate | PdfSaveOptions::IgnoreXRefErrors);
        utls::WriteTo(TestUtils::GetTestOutputFilePath(outFilename), currBuffer);

        // Try to reload the document
        doc.Load(inputOutput);

        REQUIRE(ssl::ComputeMD5Str(currBuffer) == expectedMD5);
    };

    try
    {
        utls::ReadTo(currBuffer, TestUtils::GetTestInputFilePath("TestXRefRecovery1.pdf"));
        doTest("TestSignatureCorrupted1.pdf", "797D700B04F1D67E26A804D45F978FCD");

        // Repeat the test with some garbage at the beginning of the test
        utls::ReadTo(currBuffer, TestUtils::GetTestInputFilePath("TestXRefRecovery1.pdf"));
        currBuffer.insert(0, "% Some garbage before the PDF header\n\n");
        doTest("TestSignatureCorrupted2.pdf", "F08069B4F8FD6B48CB69B1AD7F20B211");
        PdfCommon::SetMaxLoggingSeverity(currentLogSeverity);
    }
    catch (...)
    {
        PdfCommon::SetMaxLoggingSeverity(currentLogSeverity);
        throw;
    }
}

#if OPENSSL_VERSION_MAJOR > 3 || (OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR >= 2)

// Deterministic ECDSA signatures require OpenSSL >= 3.5
TEST_CASE("TestECDSA")
{
    charbuff buff;
    auto inputPath = TestUtils::GetTestInputFilePath("blank.pdf");

    auto outputPath = TestUtils::GetTestOutputFilePath(string("sha384ECDSA").append(".pdf"));
    fs::copy_file(fs::u8path(inputPath), fs::u8path(outputPath), fs::copy_options::overwrite_existing);
    auto stream = std::make_shared<FileStreamDevice>(outputPath, FileMode::Open);

    string pkey;
    TestUtils::ReadTestInputFileTo(pkey, string("sha384ECDSA").append("-key.pem"));

    string cert;
    TestUtils::ReadTestInputFileTo(cert, string("sha384ECDSA").append("-cert.pem"));

    PdfMemDocument doc(stream);
    auto& page = doc.GetPages().GetPageAt(0);
    auto& signature = page.CreateField<PdfSignature>("Signature", Rect());
    signature.SetSignatureDate(TestSignatureDate);

    PdfSignerCmsParams params;
    params.Flags = PdfSignerCmsFlags::Deterministic;
    auto signer = PdfSignerCms(cert, pkey, params);
    PoDoFo::SignDocument(doc, *stream, signer, signature, PdfSaveOptions::NoMetadataUpdate);

    utls::ReadTo(buff, outputPath);
    REQUIRE(ssl::ComputeMD5Str(buff) == "E8F34C90C26982DFBB3CC56F397C2B27");
}

#endif // OPENSSL_VERSION_MAJOR > 3 || (OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR >= 2)

#if OPENSSL_VERSION_MAJOR > 3 || (OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR >= 5)

// PQC signatures require OpenSSL >= 3.5
TEST_CASE("TestPostQuantumCryptography")
{
    charbuff buff;
    auto inputPath = TestUtils::GetTestInputFilePath("blank.pdf");

    auto testSignature = [&](const string_view& algo, const string_view& refHash)
        {
            auto outputPath = TestUtils::GetTestOutputFilePath(string(algo).append(".pdf"));
            fs::copy_file(fs::u8path(inputPath), fs::u8path(outputPath), fs::copy_options::overwrite_existing);
            auto stream = std::make_shared<FileStreamDevice>(outputPath, FileMode::Open);

            string pkey;
            TestUtils::ReadTestInputFileTo(pkey, "PQC", string(algo).append("-key.pem"));

            string cert;
            TestUtils::ReadTestInputFileTo(cert, "PQC", string(algo).append("-cert.pem"));

            PdfMemDocument doc(stream);
            auto& page = doc.GetPages().GetPageAt(0);
            auto& signature = page.CreateField<PdfSignature>("Signature", Rect());
            signature.SetSignatureDate(TestSignatureDate);

            PdfSignerCmsParams params;
            params.Flags = PdfSignerCmsFlags::Deterministic;
            auto signer = PdfSignerCms(cert, pkey, params);
            PoDoFo::SignDocument(doc, *stream, signer, signature, PdfSaveOptions::NoMetadataUpdate);

            utls::ReadTo(buff, outputPath);
            REQUIRE(ssl::ComputeMD5Str(buff) == refHash);
        };

    testSignature("ML-DSA-44", "77F6758B2A3E0184402AA96F9CFE5B7E");
    testSignature("slh-dsa-sha2-128f", "F430FDAAE82AD5B4407CAC5B03AC70FF");
}

#endif // OPENSSL_VERSION_MAJOR > 3 || (OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR >= 5)

// Verify a signature against the bytes delimited by the /ByteRange
TEST_CASE("TestVerifySignature")
{
    string cert;
    TestUtils::ReadTestInputFileTo(cert, "mycert.der");

    string pkey;
    TestUtils::ReadTestInputFileTo(pkey, "mykey-pkcs8.der");

    charbuff document;
    utls::ReadTo(document, TestUtils::GetTestInputFilePath("TestSignature.pdf"));

    auto stream = std::make_shared<BufferStreamDevice>(document);
    PdfMemDocument doc(stream);
    auto& page = doc.GetPages().GetPageAt(0);
    auto& annot = page.GetAnnotations().GetAnnotAt(0);
    auto& field = dynamic_cast<PdfAnnotationWidget&>(annot).GetField();
    auto& signature = dynamic_cast<PdfSignature&>(field);
    signature.SetSignatureDate(TestSignatureDate);

    PdfSignerCms signer(cert, pkey);
    PoDoFo::SignDocument(doc, *stream, signer, signature, PdfSaveOptions::NoMetadataUpdate);

    // NOTE: The signature is always read from the freshly signed document, so the
    // tampered inputs below only alter the bytes that are fed to the verification
    auto verify = [&signature](const bufferview& input, PdfSignatureVerifyStatus& status)
    {
        SpanStreamDevice device(input);
        return signature.TryVerifySignature(device, status);
    };

    PdfSignatureVerifyStatus status;
    REQUIRE(verify(document, status));
    REQUIRE(status == PdfSignatureVerifyStatus::CryptoVerified);

    // A modification inside the signed ranges invalidates the signature
    auto tampered = document;
    tampered[tampered.size() / 2] = (char)(tampered[tampered.size() / 2] ^ 0xFF);
    REQUIRE(!verify(tampered, status));
    REQUIRE(status == PdfSignatureVerifyStatus::Invalid);

    // Content appended after the signed ranges leaves the signature valid but not covering
    auto appended = document;
    appended.append("% some appended content\n");
    REQUIRE(verify(appended, status));
    REQUIRE(status == PdfSignatureVerifyStatus::CryptoVerifiedPartialCoverage);
}

#if OPENSSL_VERSION_MAJOR > 3 || (OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR >= 5)

// Verify pre-signed documents with post quantum cryptography signatures
TEST_CASE("TestVerifyPostQuantumSignature")
{
    auto testVerify = [](const string_view& algo)
    {
        charbuff document;
        utls::ReadTo(document, TestUtils::GetTestInputFilePath("PQC", "Signed", string(algo).append(".pdf")));

        auto stream = std::make_shared<SpanStreamDevice>(document);
        PdfMemDocument doc(stream);
        auto acroForm = doc.GetAcroForm();
        REQUIRE(acroForm != nullptr);
        REQUIRE(acroForm->GetFieldCount() == 1);

        auto& signature = dynamic_cast<PdfSignature&>(acroForm->GetFieldAt(0));
        REQUIRE(signature.GetName()->GetString() == "Signature");

        SpanStreamDevice input(document);
        PdfSignatureVerifyStatus status;
        REQUIRE(signature.TryVerifySignature(input, status));
        REQUIRE(status == PdfSignatureVerifyStatus::CryptoVerified);

        // The last byte is covered by the signature, since the ranges reach the end
        auto tampered = document;
        tampered[tampered.size() - 1] = (char)(tampered[tampered.size() - 1] ^ 0xFF);
        SpanStreamDevice tamperedInput(tampered);
        REQUIRE(!signature.TryVerifySignature(tamperedInput, status));
        REQUIRE(status == PdfSignatureVerifyStatus::Invalid);
    };

    testVerify("ML-DSA-44");
    testVerify("slh-dsa-sha2-128f");
}

#endif // OPENSSL_VERSION_MAJOR > 3 || (OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR >= 5)

// The fixture has a "signingCertificateV2" attribute that doesn't match the
// signer certificate, while the CMS signature itself is valid
TEST_CASE("TestVerifyBadSigningCertificateV2")
{
    charbuff document;
    utls::ReadTo(document, TestUtils::GetTestInputFilePath("TestSignatureBadSigningCertV2.pdf"));

    auto stream = std::make_shared<SpanStreamDevice>(document);
    PdfMemDocument doc(stream);
    auto& page = doc.GetPages().GetPageAt(0);
    auto& annot = page.GetAnnotations().GetAnnotAt(0);
    auto& field = dynamic_cast<PdfAnnotationWidget&>(annot).GetField();
    auto& signature = dynamic_cast<PdfSignature&>(field);

    SpanStreamDevice input(document);
    PdfSignatureVerifyStatus status;
    REQUIRE(!signature.TryVerifySignature(input, status));
    REQUIRE(status == PdfSignatureVerifyStatus::Invalid);

    // Only the certificate binding is broken: verify the rest still passes
    auto valueObj = signature.GetDictionary().FindKey("V");
    REQUIRE(valueObj != nullptr);

    const PdfString* contents;
    const PdfArray* byteRange;
    REQUIRE(valueObj->GetDictionary().TryFindKeyAs("Contents", contents));
    REQUIRE(valueObj->GetDictionary().TryFindKeyAs("ByteRange", byteRange));

    CmsVerifyContext context;
    REQUIRE(context.TryReset(contents->GetRawData()));
    REQUIRE(context.TryLoadSigner(0));

    charbuff buffer;
    for (unsigned i = 0; i < 2; i++)
    {
        buffer.resize((size_t)byteRange->GetAtAs<int64_t>(i * 2 + 1));
        input.Seek((size_t)byteRange->GetAtAs<int64_t>(i * 2));
        input.Read(buffer.data(), buffer.size());
        context.AppendData(buffer);
    }

    REQUIRE(context.VerifySignature());

    bool attrMissing;
    REQUIRE(!context.TryVerifySigningCertificateV2(attrMissing));
    REQUIRE(!attrMissing);
}
