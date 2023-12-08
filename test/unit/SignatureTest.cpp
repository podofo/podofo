/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: MIT-0
 */

#include <PdfTest.h>
#include <podofo/private/OpenSSLInternal.h>

using namespace std;
using namespace PoDoFo;

TEST_CASE("TestSignature1")
{
    charbuff buff;
    auto inputPath = TestUtils::GetTestInputFilePath("TestSignature.pdf");
    auto outputPath = TestUtils::GetTestOutputFilePath("TestSignature1.pdf");

    auto testSignature = [&](const shared_ptr<StreamDevice>& stream)
    {
        // X509 Certificate
        string cert;
        TestUtils::ReadTestInputFile("mycert.der", cert);

        // RSA Private key coefficients in der format (binary)
        string pkey;
        TestUtils::ReadTestInputFile("mykey.der", pkey);

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
        testSignature(stream);
        REQUIRE(ssl::ComputeMD5Str(ss.str()) == "312837C62DA72DBC13D588A2AD42BFC1");
    }

    {
        fs::copy_file(fs::u8path(inputPath), fs::u8path(outputPath), fs::copy_options::overwrite_existing);
        utls::ReadTo(buff, outputPath);
        auto stream = std::make_shared<BufferStreamDevice>(buff);
        testSignature(stream);
        REQUIRE(ssl::ComputeMD5Str(buff) == "312837C62DA72DBC13D588A2AD42BFC1");
    }

    {
        fs::copy_file(fs::u8path(inputPath), fs::u8path(outputPath), fs::copy_options::overwrite_existing);
        auto stream = std::make_shared<FileStreamDevice>(outputPath, FileMode::Open);
        testSignature(stream);
        utls::ReadTo(buff, outputPath);
        REQUIRE(ssl::ComputeMD5Str(buff) == "312837C62DA72DBC13D588A2AD42BFC1");
    }
}

// Test sequential signing
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
    TestUtils::ReadTestInputFile("mykey.der", pkey);

    PdfMemDocument doc(stream);
    auto& page = doc.GetPages().GetPageAt(0);
    auto& annot = page.GetAnnotations().GetAnnotAt(0);
    auto& field = dynamic_cast<PdfAnnotationWidget&>(annot).GetField();
    auto& signature = dynamic_cast<PdfSignature&>(field);

    PdfSignerCmsParams params;
    auto signer = std::make_shared<PdfSignerCms>(cert, params);
    PdfSigningContext ctx;
    ctx.SetSaveOptions(PdfSaveOptions::NoMetadataUpdate);
    auto signerId = ctx.AddSigner(signature, signer);
    auto intermediateResults = ctx.StartSigning(doc, stream);
    charbuff signedHash;
    ssl::DoSign(intermediateResults[signerId], pkey, params.Hashing, signedHash);
    intermediateResults[signerId] = signedHash;
    ctx.FinishSigning(intermediateResults);
    
    utls::ReadTo(buff, outputPath);
    REQUIRE(ssl::ComputeMD5Str(buff) == "312837C62DA72DBC13D588A2AD42BFC1");
}
