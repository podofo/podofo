/**
 * Copyright (C) 2011 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

static void CompareStreamContent(PdfObjectStream& stream, const string_view& expected);

TEST_CASE("testAppend")
{
    string_view example = "BT (Hello) Tj ET";

    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));

    auto& contents = page.GetOrCreateContents();
    auto& stream = contents.GetStreamForAppending();
    stream.SetData(example);

    CompareStreamContent(stream, example);

    PdfPainter painter;
    painter.SetCanvas(page);
    painter.GetGraphicsState().SetFillColor(PdfColor(1.0, 1.0, 1.0));
    painter.FinishDrawing();

    PdfCanvasInputDevice input(doc.GetPages().GetPageAt(0));
    string out;
    StringStreamDevice output(out);
    input.CopyTo(output);

    REQUIRE(out == "q\nBT (Hello) Tj ET\nQ\nq\n1 1 1 rg\nQ\n");
}

void CompareStreamContent(PdfObjectStream& stream, const string_view& expected)
{
    charbuff buffer;
    stream.CopyTo(buffer);
    REQUIRE(buffer == expected);
}
