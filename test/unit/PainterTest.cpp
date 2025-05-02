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
        FakeCanvas()
            : m_Resources(m_doc.GetPages().CreatePage().GetResources())
        {
        }

    public:
        PdfObjectStream& GetOrCreateContentsStream(PdfStreamAppendFlags flags) override
        {
            (void)flags;
            return m_resourceObj.GetOrCreateStream();
        }

        PdfObjectStream& ResetContentsStream() override
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
        }

        PdfResources& GetOrCreateResources() override
        {
            // NOTE: Return a dummy resource
            return m_Resources;
        }

        Corners GetRectRaw() const override
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
        }

        void CopyContentsTo(OutputStream& stream) const override
        {
            auto objStream = m_resourceObj.GetStream();
            if (objStream == nullptr)
                return;

            objStream->CopyTo(stream);
        }

        bool TryGetRotationRadians(double& teta) const override
        {
            teta = 0;
            return false;
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
        PdfDictionaryElement& getElement() override
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
        }

    private:
        PdfObject m_resourceObj;
        PdfMemDocument m_doc;
        PdfResources& m_Resources;
    };
}

static string getContents(const PdfPage& page);
static void compareStreamContent(PdfObjectStream& stream, const string_view& expected);
static void drawSample(PdfPainter& painter);
static void drawSquareWithCross(PdfPainter& painter, double x, double y);

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
    auto copy = canvas.GetContentsCopy();
    REQUIRE(copy == s_expected);
}

TEST_CASE("TestPainter2")
{
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
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
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    PdfPainter painter;
    painter.SetCanvas(page);
    auto& font = doc.GetFonts().GetStandard14Font(PdfStandard14FontType::TimesRoman);
    painter.TextState.SetFont(font, 15);
    painter.DrawText("Hello world", 100, 500, PdfDrawTextStyle::StrikeThrough | PdfDrawTextStyle::Underline);
    painter.FinishDrawing();
    doc.Save(TestUtils::GetTestOutputFilePath("TestPainter3.pdf"));

    auto expectedContent = R"(q
q
BT
/Ft0 15 Tf
0.75 w
100 500 Td
<0203040405010605070408> Tj
ET
100 498.5 m
172.075 498.5 l
S
100 503.93 m
172.075 503.93 l
S
Q
Q
)"sv;

    auto out = getContents(page);
    REQUIRE(out == expectedContent);

    auto expectedToUnicode = R"(/CIDInit /ProcSet findresource begin
12 dict begin
begincmap
/CIDSystemInfo <<
   /Registry (Adobe)
   /Ordering (UCS)
   /Supplement 0
>> def
/CMapName /Adobe-Identity-UCS def
/CMapType 2 def
1 begincodespacerange
<00><7F>
endcodespacerange
8 beginbfchar
<01> <0020>
<02> <0048>
<03> <0065>
<04> <006C>
<05> <006F>
<06> <0077>
<07> <0072>
<08> <0064>
endbfchar
endcmap
CMapName currentdict /CMap defineresource pop
end
end)";

    auto& toUnicodeObj = font.GetDictionary().MustFindKey("ToUnicode");
    REQUIRE(toUnicodeObj.MustGetStream().GetCopy() == expectedToUnicode);

    auto expectedEncoding = R"(/CIDInit /ProcSet findresource begin
12 dict begin
begincmap
/CIDSystemInfo <<
   /Registry (PoDoFo)
   /Ordering (BAAAAA+Times-Roman-subset)
   /Supplement 0
>> def
/CMapName /CMap-BAAAAA+Times-Roman-subset def
/CMapType 1 def
1 begincodespacerange
<00><7F>
endcodespacerange
8 begincidchar
<01> 1
<02> 2
<03> 3
<04> 4
<05> 5
<06> 6
<07> 7
<08> 8
endcidchar
endcmap
CMapName currentdict /CMap defineresource pop
end
end)";

    auto& encodingObj = font.GetDictionary().MustFindKey("Encoding");
    REQUIRE(encodingObj.MustGetStream().GetCopy() == expectedEncoding);
}

TEST_CASE("TestPainter4")
{
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);

    PdfFontCreateParams params;
    params.Encoding = PdfEncoding(PdfEncodingMapFactory::GetWinAnsiEncodingInstancePtr());
    auto& font = doc.GetFonts().GetStandard14Font(PdfStandard14FontType::Helvetica, params);

    PdfPainter painter;
    auto& operators = static_cast<PdfContentStreamOperators&>(painter);
    painter.SetCanvas(page);
    painter.TextState.SetFont(font, 15);
    painter.TextObject.Begin();
    painter.TextObject.MoveTo(100, 500);
    painter.TextObject.AddText("Test1");
    // Some low level operations
    operators.TJ_Operator_Begin();
    operators.TJ_Operator_Glyphs("_W", false);
    operators.TJ_Operator_Delta(-500);
    operators.TJ_Operator_Glyphs("orld", false);
    operators.TJ_Operator_End();
    painter.TextObject.End();
    painter.DrawText("Test2", 100, 600, PdfDrawTextStyle::StrikeThrough);

    PdfPainterPath path;
    path.MoveTo(20, 20);
    path.AddArcTo(150, 20, 150, 70, 50);
    path.AddLineTo(150, 120);
    path.AddArc(200, 120, 50, numbers::pi, numbers::pi/8, true);

    auto currPoint1 = path.GetCurrentPoint();

    PdfPainterPath path2;
    path2.MoveTo(250, 120);
    path2.AddLineTo(250, 80);
    path.AddPath(path2, true);

    auto currPoint2 = path.GetCurrentPoint();
    painter.DrawPath(path, PdfPathDrawMode::Stroke);
    path.Reset();
    path.MoveTo(40, 40);
    path.AddLineTo(100, 40);
    path.AddLineTo(70, 80);
    path.AddLineTo(40, 40);
    path.AddCircle(200, 300, 60);
    painter.DrawPath(path, PdfPathDrawMode::Fill);
    
    drawSquareWithCross(painter, 100, 20);
    drawSquareWithCross(painter, 100, 70);
    drawSquareWithCross(painter, 150, 70);
    drawSquareWithCross(painter, currPoint1.X, currPoint1.Y);
    drawSquareWithCross(painter, currPoint2.X, currPoint2.Y);

    painter.FinishDrawing();
    doc.Save(TestUtils::GetTestOutputFilePath("TestPainter4.pdf"));

    auto expected = R"(q
BT
/Ft0 15 Tf
100 500 Td
(Test1) Tj
[ (_W) -500 (orld) ] TJ

ET
q
BT
0.75 w
100 600 Td
(Test2) Tj
ET
100 604.35 m
137.515 604.35 l
S
Q
20 20 m
100 20 l
127.614237 20 150 42.385763 150 70 c
150 120 l
150 120 l
150 143.853715 166.850112 164.385635 190.245484 169.039264 c
213.640856 173.692893 237.065555 161.17213 246.193977 139.134172 c
250 120 l
250 120 m
250 80 l
S
40 40 m
100 40 l
70 80 l
40 40 l
260 300 m
260 333.137085 233.137085 360 200 360 c
166.862915 360 140 333.137085 140 300 c
140 266.862915 166.862915 240 200 240 c
233.137085 240 260 266.862915 260 300 c
h
f
q
0.6 w
97 17 6 6 re
S
0 w
100 17 m
100 23 l
S
97 20 m
103 20 l
S
Q
q
0.6 w
97 67 6 6 re
S
0 w
100 67 m
100 73 l
S
97 70 m
103 70 l
S
Q
q
0.6 w
147 67 6 6 re
S
0 w
150 67 m
150 73 l
S
147 70 m
153 70 l
S
Q
q
0.6 w
243.193977 136.134172 6 6 re
S
0 w
246.193977 136.134172 m
246.193977 142.134172 l
S
243.193977 139.134172 m
249.193977 139.134172 l
S
Q
q
0.6 w
247 77 6 6 re
S
0 w
250 77 m
250 83 l
S
247 80 m
253 80 l
S
Q
Q
)";
    auto out = getContents(page);
    REQUIRE(out == expected);
}

TEST_CASE("TestPainter5")
{
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);

    PdfFontCreateParams params;
    params.Encoding = PdfEncoding(PdfEncodingMapFactory::GetWinAnsiEncodingInstancePtr());
    auto& font = doc.GetFonts().GetStandard14Font(PdfStandard14FontType::Helvetica, params);

    PdfPainter painter;
    painter.SetCanvas(page);
    painter.TextState.SetFont(font, 15);
    painter.DrawTextMultiLine("Hello\nWorld", 100, 600, 100, 40);

    painter.FinishDrawing();
    doc.Save(TestUtils::GetTestOutputFilePath("TestPainter5.pdf"));

    auto expected = R"(q
q
100 600 100 40 re
W
n
BT
/Ft0 15 Tf
100 628.75 Td
(Hello) Tj
0 -15 Td
(World) Tj
ET
Q
Q
)";

    auto out = getContents(page);
    REQUIRE(out == expected);
}

TEST_CASE("TestPainter6")
{
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);

    PdfFontCreateParams params;
    params.Encoding = PdfEncoding(PdfEncodingMapFactory::GetWinAnsiEncodingInstancePtr());

    PdfPainter painter;
    painter.SetCanvas(page);
    REQUIRE(painter.GetStateStack().Current->CurrentPoint == nullptr);
    PdfPainterPath path;
    path.AddRectangle(Rect(10,10, 100, 50));
    painter.Save();
    painter.DrawPath(path);
    REQUIRE(path.GetCurrentPoint() == Vector2(10, 10));
    REQUIRE(painter.GetStateStack().Current->CurrentPoint == nullptr);
    painter.Save();
    auto& operators = static_cast<PdfContentStreamOperators&>(painter);
    operators.n_Operator();
    REQUIRE(painter.GetStateStack().Current->CurrentPoint == nullptr);
    painter.FinishDrawing();
    doc.Save(TestUtils::GetTestOutputFilePath("TestPainter6.pdf"));

    auto expected = R"(q
q
10 10 100 50 re
S
q
n
Q
)";

    auto out = getContents(page);
    REQUIRE(out == expected);
}

TEST_CASE("TestAppend")
{
    string_view example = "BT (Hello) Tj ET";

    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);

    auto& contents = page.GetOrCreateContents();
    auto& stream = contents.CreateStreamForAppending();
    stream.SetData(example);

    compareStreamContent(stream, example);

    PdfPainter painter;
    painter.SetCanvas(page);
    painter.GraphicsState.SetNonStrokingColor(PdfColor(1.0, 1.0, 1.0));
    painter.FinishDrawing();

    auto out = getContents(page);
    REQUIRE(out == "q\nBT (Hello) Tj ET\nQ\nq\n1 1 1 rg\nQ\n");
}

TEST_CASE("TestRotate")
{
    unordered_map<int, Matrix> matrices = {
        { 90, Matrix(6.1232339957367660e-17, 1, -1, 6.1232339957367660e-17, 9.9999999999999982, 0) },
        { 270, Matrix(-1.8369701987210297e-16, -1, 1, -1.8369701987210297e-16, 0, 20.000000000000004) },
    };

    auto test = [&](int angle)
    {
        PdfMemDocument doc;
        doc.Load(TestUtils::GetTestInputFilePath(utls::Format("blank-rotated-{}.pdf", angle)));
        auto& page = doc.GetPages().GetPageAt(0);
        page.SetRect(Rect(0, 0, 5, 7));

        auto& signature = page.CreateField<PdfSignature>("Test", Rect(2, 1, 2, 1));
        auto xobj = doc.CreateXObjectForm(Rect(0, 0, 20, 10));
        PdfPainter painter;
        painter.SetCanvas(*xobj);
        PdfPainterPath path;
        path.MoveTo(1, 1);
        path.AddLineTo(19, 1);
        path.AddLineTo(10, 9);
        path.Close();
        painter.DrawPath(path, PdfPathDrawMode::Fill);
        painter.FinishDrawing();
        signature.MustGetWidget().SetAppearanceStream(*xobj);
        auto apObj = signature.MustGetWidget().GetAppearanceStream();
        unique_ptr<PdfXObjectForm> form;
        (void)PdfXObject::TryCreateFromObject(*apObj, form);
        REQUIRE(form->GetMatrix() == matrices[angle]);
        doc.Save(TestUtils::GetTestOutputFilePath(utls::Format("Rotated-{}.pdf", angle)));
    };

    test(90);
    test(270);
}

// Test coming from https://github.com/podofo/podofo/issues/137
TEST_CASE("BigDynamicCMAPTest")
{
    string_view textOver255 = "12345糟姨集鞋南槍痕痰林托入笑為潮立碰慘紡命窯舒喬檔脊吸渣誘餓躁強瓣倚扣拼襯裙凈錄釀薯憂擇十肅亭宰都愉冬乃考摟償老居題釣盯侵臣騾購標搬輛映納銷蜂宋頭號鄭藝駛斥鏟遵饑絨挨草保示她房礙宜扶涼困供探濫裁鴨膏橫坦傍愧蜓山儀辜略機評疑寸浩韻挪墻含帆由化里肌目淹誤匹枕浸有協斯名哥其香響逼裂油館慰七狹置露河樓弊增熱懂劇難盞拘罵撇芽胡慧關準補必舌遼晴奏愛江掏疲番走芬秩撤搭饅槐伸填灣蝦載簾哄寫急病攤田惕次泡捏糧附刷李鉆解阿違嫁天塌句善訊夠衰唇險學欠堆弟貪爆徐太孤鎮膛婆褲傷謹憶鵝踢贈擔仗膀挽兄扔基窩幕裹血暴米政覆柴力豎悼劫肥書翁屑";

    auto outputFile = TestUtils::GetTestOutputFilePath("BigDynamicCMAPTest.pdf");
    {
        PdfMemDocument document;

        auto& fontManager = document.GetFonts();
        auto metrics = PdfFontMetrics::Create(TestUtils::GetTestInputFilePath("Fonts", "NotoSansTC-Regular.ttf"));
        auto& font = fontManager.GetOrCreateFont(std::move(metrics));

        // Draw a XObjectForm
        unique_ptr<PdfXObjectForm> xObjectPtr = document.CreateXObjectForm(Rect(0, 0, 720, 1280));
        PdfXObjectForm* xObject = xObjectPtr.get();

        {
            PdfPainter painter;
            painter.SetCanvas(*xObject);

            painter.GraphicsState.SetNonStrokingColor(PdfColor(0, 0, 0));
            painter.GraphicsState.SetStrokingColor(PdfColor(0, 0, 0));

            painter.TextState.SetFont(font, 12);

            painter.DrawTextMultiLine(textOver255, 0, 0, 720, 1280);

            painter.FinishDrawing();
        }

        // Draw XObjectForm to Page
        auto& pages = document.GetPages();
        auto& page = pages.CreatePage(Rect(0, 0, 720, 1280));

        {
            PdfPainter painter;
            painter.SetCanvas(page);
            painter.DrawXObject(*xObject, 0, 0, 1.0, 1.0);

            painter.FinishDrawing();
        }

        document.Save(outputFile);
    }

    {
        PdfMemDocument document;
        document.Load(outputFile);
        auto& page = document.GetPages().GetPageAt(0);
        vector<PdfTextEntry> entries;
        page.ExtractTextTo(entries);
        REQUIRE(entries.size() == 5);
        REQUIRE(entries[0].Text == "12345糟姨集鞋南槍痕痰林托入笑為潮立碰慘紡命窯舒喬檔脊吸渣誘餓躁強瓣倚扣拼襯裙凈錄釀薯憂擇十肅亭宰都愉冬乃考摟償老居題釣");
        REQUIRE(entries[1].Text == "盯侵臣騾購標搬輛映納銷蜂宋頭號鄭藝駛斥鏟遵饑絨挨草保示她房礙宜扶涼困供探濫裁鴨膏橫坦傍愧蜓山儀辜略機評疑寸浩韻挪墻含帆由");
        REQUIRE(entries[2].Text == "化里肌目淹誤匹枕浸有協斯名哥其香響逼裂油館慰七狹置露河樓弊增熱懂劇難盞拘罵撇芽胡慧關準補必舌遼晴奏愛江掏疲番走芬秩撤搭饅");
        REQUIRE(entries[3].Text == "槐伸填灣蝦載簾哄寫急病攤田惕次泡捏糧附刷李鉆解阿違嫁天塌句善訊夠衰唇險學欠堆弟貪爆徐太孤鎮膛婆褲傷謹憶鵝踢贈擔仗膀挽兄扔");
        REQUIRE(entries[4].Text == "基窩幕裹血暴米政覆柴力豎悼劫肥書翁屑");
    }
}

static void drawSample(PdfPainter& painter)
{
    painter.DrawCircle(100, 500, 20, PdfPathDrawMode::Fill);
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

void drawSquareWithCross(PdfPainter& painter, double x, double y)
{
    painter.Save();
    const double SquareSize = 6;
    painter.GraphicsState.SetLineWidth(0.6);
    painter.DrawRectangle(x - SquareSize / 2, y - SquareSize / 2, SquareSize, SquareSize);

    painter.GraphicsState.SetLineWidth(0);
    painter.DrawLine(x, y - SquareSize / 2, x, y + SquareSize / 2);
    painter.DrawLine(x - SquareSize / 2, y, x + SquareSize / 2, y);
    painter.Restore();
}
