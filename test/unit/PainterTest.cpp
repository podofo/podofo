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

namespace
{
    class FakeCanvas : public PdfCanvas
    {
    public:
        FakeCanvas() { }

    public:
        PdfObjectStream& GetStreamForAppending(PdfStreamAppendFlags flags)
        {
            return m_resourceObj.GetOrCreateStream();
        }

        PdfResources& GetOrCreateResources() override
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
        }

        /** Get the current canvas size in PDF Units
         *  \returns a PdfRect containing the page size available for drawing
         */
        PdfRect GetRect() const override
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
        }

        bool HasRotation(double& teta) const override
        {
            teta = 0;
            return false;
        }

        charbuff GetCopy() const
        {
            return m_resourceObj.MustGetStream().GetCopy();
        }

    protected:
        PdfObject* getContentsObject() override
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
        }
        PdfResources* getResources() override
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
        }
        PdfElement& getElement() override
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
        }

    private:
        PdfObject m_resourceObj;
    };
}

static string getContents(const PdfPage& page);
static void compareStreamContent(PdfObjectStream& stream, const string_view& expected);
static void drawSample(PdfPainter& painter);

auto s_expected = R"(q
120 500 m
120 511.045695 111.045695 520 100 520 c
88.954305 520 80 511.045695 80 500 c
80 488.954305 88.954305 480 100 480 c
111.045695 480 120 488.954305 120 500 c
h
f
Q
)"sv;

TEST_CASE("TestPainter1")
{
    FakeCanvas canvas;
    PdfPainter painter;
    painter.SetCanvas(canvas);
    drawSample(painter);
    painter.FinishDrawing();
    auto copy = canvas.GetCopy();
    REQUIRE(copy == s_expected);
}

TEST_CASE("TestPainter2")
{
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    PdfPainter painter;
    painter.SetCanvas(page);
    drawSample(painter);
    painter.FinishDrawing();
    doc.Save(TestUtils::GetTestOutputFilePath("TestPainter2.pdf"));

    auto out = getContents(page);
    REQUIRE(out == s_expected);
}

TEST_CASE("TestPainter3")
{
    auto expected = R"(q
BT
/Ft5 15 Tf
100 500 Td q
0.75 w
100 498.5 m
172.075 498.5 l
h
S
0.75 w
100 503.93 m
172.075 503.93 l
h
S
Q
<0001020203040503060207> Tj
ET
Q
)"sv;

    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    PdfPainter painter;
    painter.SetCanvas(page);
    painter.GetTextState().SetFont(doc.GetFonts().GetStandard14Font(PdfStandard14FontType::TimesRoman), 15);
    painter.DrawText("Hello world", 100, 500, PdfDrawTextStyle::StrikeOut | PdfDrawTextStyle::Underline);
    painter.FinishDrawing();
    doc.Save(TestUtils::GetTestOutputFilePath("TestPainter3.pdf"));

    auto out = getContents(page);
    REQUIRE(out == expected);
}

TEST_CASE("TestAppend")
{
    string_view example = "BT (Hello) Tj ET";

    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));

    auto& contents = page.GetOrCreateContents();
    auto& stream = contents.GetStreamForAppending();
    stream.SetData(example);

    compareStreamContent(stream, example);

    PdfPainter painter;
    painter.SetCanvas(page);
    painter.GetGraphicsState().SetFillColor(PdfColor(1.0, 1.0, 1.0));
    painter.FinishDrawing();

    auto out = getContents(page);
    REQUIRE(out == "q\nBT (Hello) Tj ET\nQ\nq\n1 1 1 rg\nQ\n");
}

static void drawSample(PdfPainter& painter)
{
    painter.DrawCircle(100, 500, 20, PdfDrawMode::Fill);
}

void compareStreamContent(PdfObjectStream& stream, const string_view& expected)
{
    charbuff buffer;
    stream.CopyTo(buffer);
    REQUIRE(buffer == expected);
}

string getContents(const PdfPage& page)
{
    PdfCanvasInputDevice input(page);
    string ret;
    StringStreamDevice output(ret);
    input.CopyTo(output);
    return ret;
}
