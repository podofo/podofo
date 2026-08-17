// SPDX-FileCopyrightText: 2009 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

#include <thread>

#include <podofo/private/FreetypePrivate.h>
#ifdef PODOFO_ENABLE_AFDKO
#include <podofo/private/FontUtilsAFDKO.h>
#endif

using namespace std;
using namespace PoDoFo;

TEST_CASE("TestEmbedFont")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestEmbedFont.pdf"));

    unique_ptr<PdfFont> font;
    (void)PdfFont::TryCreateFromObject(doc.GetObjects().MustGetObject(PdfReference(6, 0)), font);

    // The font is not embedded in this document
    REQUIRE(font->GetMetrics().GetOrLoadFontFileData().size() == 0);

    // Create a substitute font from a font without a "/FontFile2" entry
    PdfFont* substituteFont;
    REQUIRE(font->TryCreateProxyFont(substituteFont));
    // Add all used  GIDs for this font. The following is hardcoded:
    // this should require scanning of the entire document page contents
    substituteFont->AddSubsetCIDs(PdfString::FromRaw("TEST"));

    {
        // Substitute existing font in the resources of the oage
        auto& page = doc.GetPages().GetPageAt(0);
        static_cast<PdfResourceOperations&>(page.GetResources()).AddResource(PdfResourceType::Font, "Ft0", substituteFont->GetObject());
    }

    doc.Save(TestUtils::GetTestOutputFilePath("TestEmbedFont.pdf"));

    // Reload the file and verify the font has now font file data
    doc.Load(TestUtils::GetTestOutputFilePath("TestEmbedFont.pdf"));
    {
        auto& page = doc.GetPages().GetPageAt(0);
        auto fontObj = page.GetResources().GetResource(PdfResourceType::Font, "Ft0");
        (void)PdfFont::TryCreateFromObject(*fontObj, font);
        REQUIRE(font->GetMetrics().GetOrLoadFontFileData().size() != 0);
    }
}

// Faces are created on a thread local FreeType library instance: freeing
// them after the creation thread was disposed must not access any storage
// that went away with that thread
TEST_CASE("TestFreeFontAfterCreationThreadDisposal")
{
    unique_ptr<PdfMemDocument> doc;
    thread creator([&doc]
    {
        doc.reset(new PdfMemDocument());
        (void)doc->GetFonts().GetOrCreateFont(TestUtils::GetTestInputFilePath("Fonts", "LiberationSans-Regular.ttf"));
    });
    creator.join();

    doc.reset();
}

TEST_CASE("TestCreateFontExtract")
{
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);

    // Play a bit with font path caching
    auto fontPath1 = TestUtils::GetTestInputFilePath("Fonts", "LiberationSans-Regular.ttf");
    auto fontPath2 = TestUtils::GetTestInputFilePath("Fonts", "..", "Fonts", "LiberationSans-Regular.ttf");
    auto fontRef = &doc.GetFonts().GetOrCreateFont(fontPath1);
    auto& font = doc.GetFonts().GetOrCreateFont(fontPath2);

    PdfFont* fontFromBuffer;

    {
        charbuff fontbuffer;
        utls::ReadTo(fontbuffer, TestUtils::GetTestInputFilePath("Fonts", "LiberationSans-Regular.ttf"));
        fontFromBuffer = &doc.GetFonts().GetOrCreateFontFromBuffer(fontbuffer);
    }

    // The matched fonts should be the same one
    REQUIRE(&font == fontRef);

    {
        PdfPainter painter;
        painter.SetCanvas(page);

        painter.TextState.SetFont(font, 30.0);
        painter.DrawText("ěščř", 100, 600);

        painter.TextState.SetFont(*fontFromBuffer, 30.0);
        painter.DrawText("ěščř buffer", 100, 500);
        painter.FinishDrawing();
    }

    auto outputpath = TestUtils::GetTestOutputFilePath("TestCreateFontExtract.pdf");

    try
    {
        FileStreamDevice stream(outputpath, FileMode::Create);
        doc.Save(stream);
    }
    catch (const PdfError& error)
    {
        // Don't continue further the test in this case
        if (error.GetCode() == PdfErrorCode::UnsupportedFontFormat)
            return;

        throw;
    }

    // FIXME: The test crash if we tried to extract
    // text directly on the original "doc" page

    PdfMemDocument doc2;
    doc2.Load(outputpath);

    vector<PdfTextEntry> entries;
    doc2.GetPages().GetPageAt(0).ExtractTextTo(entries);

    REQUIRE(entries[0].Text == "ěščř");
    REQUIRE(entries[0].X == 100);
    REQUIRE(entries[0].Y == 600);

    REQUIRE(entries[1].Text == "ěščř buffer");
    REQUIRE(entries[1].X == 100);
    REQUIRE(entries[1].Y == 500);
}

TEST_CASE("TestGetOrCreateFontAfterEmbedAndReset")
{
    auto fontPath = TestUtils::GetTestInputFilePath("Fonts", "LiberationSans-Regular.ttf");

    PdfMemDocument doc;
    auto& font1 = doc.GetFonts().GetOrCreateFont(fontPath);
    REQUIRE(font1.GetMetrics().GetFontName() == "LiberationSans");

    doc.GetFonts().EmbedFonts();
    doc.Reset();

    auto& font2 = doc.GetFonts().GetOrCreateFont(fontPath);
    REQUIRE(font2.GetMetrics().GetFontName() == "LiberationSans");
}

TEST_CASE("TestSimpleFontMetricsReferences")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestFontWidthsRef.pdf"));
    auto& page = doc.GetPages().GetPageAt(0);
    auto& resources = page.GetResources();

    // /MediaBox[2] and /MediaBox[3] of the page are references
    auto mediaBox = page.GetMediaBoxRaw();
    ASSERT_EQUAL(mediaBox.X2, 595.0);
    ASSERT_EQUAL(mediaBox.Y2, 842.0);

    // "/Ft0" is a /TrueType font where /Widths[32], /Widths[33]
    // and /FontBBox[0], /FontBBox[2] are references
    unique_ptr<PdfFont> font;
    REQUIRE(PdfFont::TryCreateFromObject(*resources.GetResource(PdfResourceType::Font, "Ft0"), font));
    ASSERT_EQUAL(font->GetMetrics().GetGlyphWidth(32, PdfGlyphAccess::ReadMetrics), 0.278);
    ASSERT_EQUAL(font->GetMetrics().GetGlyphWidth(33, PdfGlyphAccess::ReadMetrics), 0.333);
    ASSERT_EQUAL(font->GetMetrics().GetGlyphWidth(34, PdfGlyphAccess::ReadMetrics), 0.474);

    auto bbox = font->GetMetrics().GetBoundingBox();
    ASSERT_EQUAL(bbox.X1, -0.628);
    ASSERT_EQUAL(bbox.Y1, -0.376);
    ASSERT_EQUAL(bbox.X2, 2.0);
    ASSERT_EQUAL(bbox.Y2, 1.056);

    // "/Ft1" is a Standard14 font, which parses /Widths on its own
    unique_ptr<PdfFont> std14Font;
    REQUIRE(PdfFont::TryCreateFromObject(*resources.GetResource(PdfResourceType::Font, "Ft1"), std14Font));
    ASSERT_EQUAL(std14Font->GetMetrics().GetGlyphWidth(0, PdfGlyphAccess::ReadMetrics), 0.25);
    ASSERT_EQUAL(std14Font->GetMetrics().GetGlyphWidth(1, PdfGlyphAccess::ReadMetrics), 0.555);
    ASSERT_EQUAL(std14Font->GetMetrics().GetGlyphWidth(2, PdfGlyphAccess::ReadMetrics), 0.408);

    // "/Ft2" is a Type3 font with a custom /FontMatrix, where
    // /FontMatrix[0], /FontBBox[2] and /Widths[0] are references
    unique_ptr<PdfFont> type3Font;
    REQUIRE(PdfFont::TryCreateFromObject(*resources.GetResource(PdfResourceType::Font, "Ft2"), type3Font));
    auto& type3Metrics = type3Font->GetMetrics();
    ASSERT_EQUAL(type3Metrics.GetMatrix()[0], 0.01);

    // The /FontMatrix scales both the widths and the bounding box
    ASSERT_EQUAL(type3Metrics.GetGlyphWidth(0, PdfGlyphAccess::ReadMetrics), 0.5);

    // The /Differences of the Type3 font encoding are "97 /spade 98 /bullet",
    // where both names and the first code are references
    auto& encoding = type3Font->GetEncoding();
    REQUIRE(encoding.GetCodePoint(97) == U'♠');
}

TEST_CASE("TestCIDFontMetricsReferences")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestFontWidthsRefCID.pdf"));
    auto& resources = doc.GetPages().GetPageAt(0).GetResources();

    unique_ptr<PdfFont> font;
    REQUIRE(PdfFont::TryCreateFromObject(*resources.GetResource(PdfResourceType::Font, "C0_0"), font));
    auto& metrics = font->GetMetrics();

    // CID 0 is not covered by /W and falls back to /DW
    ASSERT_EQUAL(metrics.GetGlyphWidth(0, PdfGlyphAccess::ReadMetrics), 1.0);

    // The sub-array of the range starting at CID 1 is a reference
    ASSERT_EQUAL(metrics.GetGlyphWidth(1, PdfGlyphAccess::ReadMetrics), 0.207);
    ASSERT_EQUAL(metrics.GetGlyphWidth(16, PdfGlyphAccess::ReadMetrics), 0.334);

    // The first CID of the 17-26 range is a reference
    ASSERT_EQUAL(metrics.GetGlyphWidth(17, PdfGlyphAccess::ReadMetrics), 0.462);
    ASSERT_EQUAL(metrics.GetGlyphWidth(26, PdfGlyphAccess::ReadMetrics), 0.462);

    // Both the last CID and the width of the 27-28 range are references
    ASSERT_EQUAL(metrics.GetGlyphWidth(27, PdfGlyphAccess::ReadMetrics), 0.238);
    ASSERT_EQUAL(metrics.GetGlyphWidth(28, PdfGlyphAccess::ReadMetrics), 0.238);

    ASSERT_EQUAL(metrics.GetGlyphWidth(29, PdfGlyphAccess::ReadMetrics), 0.605);

    // The first element of the sub-array starting at CID 32 is a reference
    ASSERT_EQUAL(metrics.GetGlyphWidth(32, PdfGlyphAccess::ReadMetrics), 0.344);
    ASSERT_EQUAL(metrics.GetGlyphWidth(33, PdfGlyphAccess::ReadMetrics), 0.748);

    // /FontBBox[0] and /FontBBox[3] of the descriptor are references
    auto bbox = metrics.GetBoundingBox();
    ASSERT_EQUAL(bbox.X1, -0.025);
    ASSERT_EQUAL(bbox.Y1, -0.254);
    ASSERT_EQUAL(bbox.X2, 1.0);
    ASSERT_EQUAL(bbox.Y2, 0.88);

    // /Matrix[0], /Matrix[5] and /BBox[2], /BBox[3] of the form XObject are references
    unique_ptr<PdfXObjectForm> form;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfXObjectForm>(
        *resources.GetResource(PdfResourceType::XObject, "Fm0"), form));
    ASSERT_EQUAL(form->GetMatrix()[0], 2.0);
    ASSERT_EQUAL(form->GetMatrix()[5], 15.0);
    ASSERT_EQUAL(form->GetRect().Width, 200.0);
    ASSERT_EQUAL(form->GetRect().Height, 100.0);
}

#ifdef PODOFO_HAVE_FONTCONFIG

#include <fontconfig/fontconfig.h>

static bool getFontInfo(FcPattern* font, string& fontFamily, string& fontPath,
    PdfFontStyle& style);
static void testSingleFont(FcPattern* font);

TEST_CASE("TestFontConfigMatch")
{
    // Create a simple platform invariant FC config
    string fontconf =
        R"(<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
    <dir>FONT_DIR</dir>
    <dir prefix="xdg">fonts</dir>
    <cachedir>FONT_CACHE_DIR</cachedir>
    <cachedir prefix="xdg">fontconfig</cachedir>
</fontconfig>
)";

    utls::Replace(fontconf, "FONT_DIR", TestUtils::GetTestInputFilePath("Fonts"));
    utls::Replace(fontconf, "FONT_CACHE_DIR", TestUtils::GetTestOutputFilePath("TestFontConfig"));

    PdfFontManager::SetFontConfigWrapper(std::make_shared<PdfFontConfigWrapper>(fontconf));

    {
        PdfFontSearchParams parmas;

        auto metrics = PdfFontManager::SearchFontMetrics("NotoSans-Regular", parmas);
        REQUIRE(metrics->GetFontName() == "NotoSans-Regular");

        metrics = PdfFontManager::SearchFontMetrics("LiberationSans", parmas);
        REQUIRE(metrics->GetFontName() == "LiberationSans");

        metrics = PdfFontManager::SearchFontMetrics("Liberation Sans", parmas);
        REQUIRE(metrics->GetFontName() == "LiberationSans");

        metrics = PdfFontManager::SearchFontMetrics("LiberationMono", parmas);
        REQUIRE(metrics->GetFontName() == "LiberationMono");

        parmas.Style = PdfFontStyle::Italic;
        metrics = PdfFontManager::SearchFontMetrics("LiberationSans", parmas);
        REQUIRE(metrics->GetFontName() == "LiberationSans-Italic");

        parmas.Style = PdfFontStyle::Bold;
        metrics = PdfFontManager::SearchFontMetrics("Noto Sans", parmas);
        REQUIRE(metrics->GetFontName() == "NotoSans-Bold");

        parmas.MatchBehavior |= PdfFontMatchBehaviorFlags::SkipMatchPostScriptName;
        metrics = PdfFontManager::SearchFontMetrics("LiberationSans", parmas);
        REQUIRE(metrics->GetFontName() == "LiberationSans-Bold");
    }
}

// Disable load all fonts for now
TEST_CASE("TestFonts", "[.]")
{
    // Get all installed fonts
    auto pattern = FcPatternCreate();
    auto objectSet = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_SLANT, FC_WEIGHT, nullptr);
    auto fontSet = FcFontList(nullptr, pattern, objectSet);

    FcObjectSetDestroy(objectSet);
    FcPatternDestroy(pattern);

    if (fontSet == nullptr)
    {
        INFO("Unable to search for fonts");
        return;
    }

    INFO(utls::Format("Testing {} fonts", fontSet->nfont));
    for (int i = 0; i < fontSet->nfont; i++)
        testSingleFont(fontSet->fonts[i]);

    FcFontSetDestroy(fontSet);
}

void testSingleFont(FcPattern* font)
{
    PdfMemDocument doc;
    string fontFamily;
    string fontPath;
    PdfFontStyle style;
    PdfFontConfigSearchParams fcParams;
    auto& fcWrapper = PdfFontManager::GetFontConfigWrapper();

    if (getFontInfo(font, fontFamily, fontPath, style))
    {
        unsigned faceIndex;
        fcParams.Style = style;
        fontPath = fcWrapper.SearchFontPath(fontFamily, fcParams, faceIndex);
        if (fontPath.length() != 0)
        {
            PdfFontSearchParams params;
            params.Style = style;
            INFO(utls::Format("Font failed: {}", fontPath));
            (void)doc.GetFonts().SearchFont(fontFamily, params);
        }
    }
}

bool getFontInfo(FcPattern* font, string& fontFamily, string& fontPath,
    PdfFontStyle& style)
{
    FcChar8* family = nullptr;
    FcChar8* path = nullptr;
    int slant;
    int weight;
    style = PdfFontStyle::Regular;

    if (FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch)
    {
        fontFamily = reinterpret_cast<char*>(family);
        if (FcPatternGetString(font, FC_FILE, 0, &path) == FcResultMatch)
        {
            fontPath = reinterpret_cast<char*>(path);

            if (FcPatternGetInteger(font, FC_SLANT, 0, &slant) == FcResultMatch)
            {
                if (slant == FC_SLANT_ITALIC || slant == FC_SLANT_OBLIQUE)
                    style |= PdfFontStyle::Italic;

                if (FcPatternGetInteger(font, FC_WEIGHT, 0, &weight) == FcResultMatch)
                {
                    if (weight >= FC_WEIGHT_BOLD)
                        style |= PdfFontStyle::Bold;

                    return true;
                }
            }
            //free( file );
        }
        //free( family );
    }

    return false;
}

#endif // PODOFO_HAVE_FONTCONFIG

#ifdef PODOFO_ENABLE_AFDKO

TEST_CASE("TestConversionPBF2CFF")
{
    {
        charbuff font1;
        utls::ReadTo(font1, TestUtils::GetTestInputFilePath("FontsType1", "Lato-Regular.pfb"));

        charbuff cff;
        afdko::ConvertFontType1ToCFF(font1, cff);

        TestUtils::IsBufferEqual(cff, TestUtils::GetTestInputFilePath("FontsType1", "ConvCFF", "Lato-Regular.cff"));
    }

    {
        charbuff font1;
        utls::ReadTo(font1, TestUtils::GetTestInputFilePath("FontsType1", "lmb10.pfb"));

        charbuff cff;
        afdko::ConvertFontType1ToCFF(font1, cff);

        TestUtils::IsBufferEqual(cff, TestUtils::GetTestInputFilePath("FontsType1", "ConvCFF", "lmb10.cff"));
    }
}

TEST_CASE("TestSubsetCFFDegenerate")
{
    charbuff font1;
    utls::ReadTo(font1, TestUtils::GetTestInputFilePath("FontsType1", "Degenerate1Glyph.cff"));
    auto metrics = PdfFontMetrics::CreateFromBuffer(font1);

    vector<PdfCharGIDInfo> subsetInfos;
    subsetInfos.push_back({ 1, 1, PdfGID(0, 0) });

    PdfCIDSystemInfo cidInfo;
    cidInfo.Registry = PdfString("Adobe");
    cidInfo.Ordering = PdfString("Test");
    cidInfo.Supplement = 0;

    charbuff cff;
    afdko::SubsetFontCFF(*metrics, subsetInfos, cidInfo, cff);

    TestUtils::IsBufferEqual(cff, TestUtils::GetTestInputFilePath("FontsType1", "SubsetDegenerate1Glyph.cff"));
}

#endif // PODOFO_ENABLE_AFDKO
