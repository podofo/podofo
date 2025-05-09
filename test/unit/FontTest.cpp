/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

#include <podofo/private/FreetypePrivate.h>
#include <podofo/private/FontUtils.h>

using namespace std;
using namespace PoDoFo;

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

TEST_CASE("TestConversionPBF2CFF")
{
    {
        charbuff font1;
        utls::ReadTo(font1, TestUtils::GetTestInputFilePath("FontsType1", "Lato-Regular.pfb"));

        charbuff cff;
        PoDoFo::ConvertFontType1ToCFF(font1, cff);

        TestUtils::IsBufferEqual(cff, TestUtils::GetTestInputFilePath("FontsType1", "ConvCFF", "Lato-Regular.cff"));
    }

    {
        charbuff font1;
        utls::ReadTo(font1, TestUtils::GetTestInputFilePath("FontsType1", "lmb10.pfb"));

        charbuff cff;
        PoDoFo::ConvertFontType1ToCFF(font1, cff);

        TestUtils::IsBufferEqual(cff, TestUtils::GetTestInputFilePath("FontsType1", "ConvCFF", "lmb10.cff"));
    }
}

TEST_CASE("TestSubsetCFFDegenerate")
{
    charbuff font1;
    utls::ReadTo(font1, TestUtils::GetTestInputFilePath("FontsType1", "Degenerate1Glyph.cff"));
    auto metrics = PdfFontMetrics::CreateFromBuffer(font1);

    vector<PdfCharGIDInfo> subsetInfos;
    subsetInfos.push_back({ 1, 1, PdfGID(0, 0)});

    PdfCIDSystemInfo cidInfo;
    cidInfo.Registry = PdfString("Adobe");
    cidInfo.Ordering = PdfString("Test");
    cidInfo.Supplement = 0;

    charbuff cff;
    PoDoFo::SubsetFontCFF(*metrics, subsetInfos, cidInfo, cff);

    TestUtils::IsBufferEqual(cff, TestUtils::GetTestInputFilePath("FontsType1", "SubsetDegenerate1Glyph.cff"));
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
