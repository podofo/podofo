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
    charbuff outputBuffer;
    BufferStreamDevice output(outputBuffer);

    {
        PdfMemDocument doc;
        doc.GetMetadata().SetCreationDate(PdfDate());
        auto& page = doc.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
        (void)page.CreateField<PdfSignature>("Signature1", Rect());
        doc.Save(output, PdfSaveOptions::NoMetadataUpdate);
    }

    auto input = std::make_shared<SpanStreamDevice>(outputBuffer);

    // X509 Certificate
    string cert;
    TestUtils::ReadTestInputFile("mycert.der", cert);

    // RSA Private key coefficients in der format (binary)
    string pkey;
    TestUtils::ReadTestInputFile("mykey.der", pkey);

    {
        PdfMemDocument doc(input);
        auto& page = doc.GetPages().GetPageAt(0);
        auto& annot = page.GetAnnotations().GetAnnotAt(0);
        auto& field = dynamic_cast<PdfAnnotationWidget&>(annot).GetField();
        auto& signature = dynamic_cast<PdfSignature&>(field);

        auto signerCm = PdfSignerCms(cert, pkey);
        PoDoFo::SignDocument(doc, output, signerCm, signature, PdfSaveOptions::NoMetadataUpdate);
        utls::WriteTo(TestUtils::GetTestOutputFilePath("TestSignature1.pdf"), outputBuffer);
        REQUIRE(ssl::ComputeMD5Str(outputBuffer) == "312837C62DA72DBC13D588A2AD42BFC1");
    }
}
