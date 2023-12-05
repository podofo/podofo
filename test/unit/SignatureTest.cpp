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
    auto inputPath = TestUtils::GetTestInputFilePath("TestSignature.pdf");
    auto outputPath = TestUtils::GetTestOutputFilePath("TestSignature1.pdf");

    fs::copy_file(fs::u8path(inputPath), fs::u8path(outputPath), fs::copy_options::overwrite_existing);
    auto stream = std::make_shared<FileStreamDevice>(outputPath, FileMode::Open);

    // X509 Certificate
    string cert;
    TestUtils::ReadTestInputFile("mycert.der", cert);

    // RSA Private key coefficients in der format (binary)
    string pkey;
    TestUtils::ReadTestInputFile("mykey.der", pkey);

    {
        PdfMemDocument doc(stream);
        auto& page = doc.GetPages().GetPageAt(0);
        auto& annot = page.GetAnnotations().GetAnnotAt(0);
        auto& field = dynamic_cast<PdfAnnotationWidget&>(annot).GetField();
        auto& signature = dynamic_cast<PdfSignature&>(field);

        auto signerCm = PdfSignerCms(cert, pkey);
        PoDoFo::SignDocument(doc, *stream, signerCm, signature, PdfSaveOptions::NoMetadataUpdate);
        charbuff buff;
        utls::ReadTo(buff, outputPath);
        REQUIRE(ssl::ComputeMD5Str(buff) == "312837C62DA72DBC13D588A2AD42BFC1");
    }
}
