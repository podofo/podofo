/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

TEST_CASE("testEmptyContentsStream")
{
    PdfMemDocument doc;
    auto& page1 = doc.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    auto& annot1 = page1.GetAnnotations().CreateAnnot<PdfAnnotationPopup>(PdfRect(300.0, 20.0, 250.0, 50.0));
    PdfString title("Author: Dominik Seichter");
    annot1.SetContents(title);
    annot1.SetOpen(true);

    string filename = TestUtils::GetTestOutputFilePath("testEmptyContentsStream.pdf");
    doc.Save(filename);

    // Read annotation again
    PdfMemDocument doc2;
    doc2.Load(filename);
    REQUIRE(doc2.GetPages().GetCount() == 1);
    auto& page2 = doc2.GetPages().GetPageAt(0);
    REQUIRE(page2.GetAnnotations().GetCount() == 1);
    auto& annot2 = page2.GetAnnotations().GetAnnotAt(0);
    REQUIRE(annot2.GetContents() == title);

    auto& pageObj = page2.GetObject();
    REQUIRE(!pageObj.GetDictionary().HasKey("Contents"));
}
