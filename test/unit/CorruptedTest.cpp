/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: MIT-0
 */

#include <PdfTest.h>
#include <podofo/private/OpenSSLInternal.h>

using namespace std;
using namespace PoDoFo;

TEST_CASE("TestFixInvalidCrossReferenceTable")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestFixInvalidCrossReferenceTable.pdf"));
    doc.Save(TestUtils::GetTestOutputFilePath("TestFixInvalidCrossReferenceTable.pdf"), PdfSaveOptions::NoMetadataUpdate);
    charbuff buff;
    utls::ReadTo(buff, TestUtils::GetTestOutputFilePath("TestFixInvalidCrossReferenceTable.pdf"));
    REQUIRE(ssl::ComputeMD5Str(buff) == "CBD04FBCAADE32271C45BFA0EEFF8D2D");
}
