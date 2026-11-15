// SPDX-FileCopyrightText: 2009 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

#include <atomic>
#include <mutex>
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

TEST_CASE("TestCIDFontWidthsDescendingRange")
{
    // The /W range starting at CID 2 ends at -1: the end must not wrap,
    // skipping the resize and writing the width past the widths vector
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Resources<</Font<</F1 4 0 R>>>>>>endobj
4 0 obj<</Type/Font/Subtype/Type0/BaseFont/Test/Encoding/Identity-H/DescendantFonts[5 0 R]>>endobj
5 0 obj<</Type/Font/Subtype/CIDFontType2/BaseFont/Test/CIDSystemInfo<</Registry(Adobe)/Ordering(Identity)/Supplement 0>>/FontDescriptor 6 0 R/DW 1000/W[0[500 600]2 -1 700 2 3 800]>>endobj
6 0 obj<</Type/FontDescriptor/FontName/Test/Flags 4/FontBBox[0 0 1000 1000]/ItalicAngle 0/Ascent 800/Descent -200/CapHeight 700/StemV 80>>endobj
trailer<</Root 1 0 R/Size 7>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& resources = doc.GetPages().GetPageAt(0).GetResources();
    unique_ptr<PdfFont> font;
    REQUIRE(PdfFont::TryCreateFromObject(*resources.GetResource(PdfResourceType::Font, "F1"), font));
    auto& metrics = font->GetMetrics();
    REQUIRE(metrics.GetGlyphCount(PdfGlyphAccess::ReadMetrics) == 4);
    ASSERT_EQUAL(metrics.GetGlyphWidth(0, PdfGlyphAccess::ReadMetrics), 0.5);
    ASSERT_EQUAL(metrics.GetGlyphWidth(1, PdfGlyphAccess::ReadMetrics), 0.6);
    ASSERT_EQUAL(metrics.GetGlyphWidth(2, PdfGlyphAccess::ReadMetrics), 0.8);
    ASSERT_EQUAL(metrics.GetGlyphWidth(3, PdfGlyphAccess::ReadMetrics), 0.8);
}

TEST_CASE("TestCIDFontWidthsExceedingMaxCID")
{
    // The /W range ends at 4294967295 and the sub-array starting at
    // CID 1114111 (0x10FFFF) has 2 entries: both must be clamped to the
    // maximum CID instead of sizing the widths vector from the file
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Resources<</Font<</F1 4 0 R>>>>>>endobj
4 0 obj<</Type/Font/Subtype/Type0/BaseFont/Test/Encoding/Identity-H/DescendantFonts[5 0 R]>>endobj
5 0 obj<</Type/Font/Subtype/CIDFontType2/BaseFont/Test/CIDSystemInfo<</Registry(Adobe)/Ordering(Identity)/Supplement 0>>/FontDescriptor 6 0 R/DW 1000/W[0 4294967295 700 1114111[500 600]]>>endobj
6 0 obj<</Type/FontDescriptor/FontName/Test/Flags 4/FontBBox[0 0 1000 1000]/ItalicAngle 0/Ascent 800/Descent -200/CapHeight 700/StemV 80>>endobj
trailer<</Root 1 0 R/Size 7>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& resources = doc.GetPages().GetPageAt(0).GetResources();
    unique_ptr<PdfFont> font;
    REQUIRE(PdfFont::TryCreateFromObject(*resources.GetResource(PdfResourceType::Font, "F1"), font));
    auto& metrics = font->GetMetrics();
    REQUIRE(metrics.GetGlyphCount(PdfGlyphAccess::ReadMetrics) == 1114112);
    ASSERT_EQUAL(metrics.GetGlyphWidth(0, PdfGlyphAccess::ReadMetrics), 0.7);
    ASSERT_EQUAL(metrics.GetGlyphWidth(1114110, PdfGlyphAccess::ReadMetrics), 0.7);
    ASSERT_EQUAL(metrics.GetGlyphWidth(1114111, PdfGlyphAccess::ReadMetrics), 0.5);
}

TEST_CASE("TestFontStretchString")
{
    // /FontStretch is a string instead of a name
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Resources<</Font<</F1 4 0 R>>>>>>endobj
4 0 obj<</Type/Font/Subtype/TrueType/BaseFont/Test/Encoding/WinAnsiEncoding/FirstChar 32/LastChar 32/Widths[500]/FontDescriptor 5 0 R>>endobj
5 0 obj<</Type/FontDescriptor/FontName/Test/Flags 32/FontStretch(Condensed)>>endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& resources = doc.GetPages().GetPageAt(0).GetResources();
    unique_ptr<PdfFont> font;
    REQUIRE(PdfFont::TryCreateFromObject(*resources.GetResource(PdfResourceType::Font, "F1"), font));
    REQUIRE(font->GetMetrics().GetFontStretch() == PdfFontStretch::Condensed);
}

TEST_CASE("TestTrueTypeHugeCmapRange")
{
    // The embedded symbolic TrueType font's only cmap subtable is format 13
    // mapping the whole 32-bit range to a single glyph. Loading the font must
    // not enumerate billions of codes while building the builtin CID-to-GID map
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("Fonts", "HugeCmapRange.pdf"));
    auto& resources = doc.GetPages().GetPageAt(0).GetResources();
    unique_ptr<PdfFont> font;
    REQUIRE(PdfFont::TryCreateFromObject(*resources.GetResource(PdfResourceType::Font, "F1"), font));
    REQUIRE(font != nullptr);
}

TEST_CASE("TestTrueTypePostTableGlyphName")
{
    // The non-symbolic TrueType font has an empty cmap and names glyph 1
    // "glyphone" in its post table, which /Differences maps code 65 to.
    // The post table lookup must resolve the name to the glyph
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("Fonts", "PostTableGlyphNames.pdf"));
    auto& resources = doc.GetPages().GetPageAt(0).GetResources();
    unique_ptr<PdfFont> font;
    REQUIRE(PdfFont::TryCreateFromObject(*resources.GetResource(PdfResourceType::Font, "F1"), font));
    unsigned gid;
    REQUIRE(font->TryGetGID(U'A', PdfGlyphAccess::FontProgram, gid));
    REQUIRE(gid == 1);
}

TEST_CASE("TestType3ProxyFontMissingCharProc")
{
    // The /Differences name of the Type3 font has no entry in /CharProcs:
    // embedding the proxy font must create a dummy glyph stream for it
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Resources<</Font<</F1 4 0 R>>>>>>endobj
4 0 obj<</Type/Font/Subtype/Type3/FontBBox[0 0 1 1]/FontMatrix[1 0 0 1 0 0]/CharProcs 5 0 R/Encoding<</Type/Encoding/Differences[0/A]>>/FirstChar 0/LastChar 0/Widths[1]/Resources<<>>>>endobj
5 0 obj<<>>endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& resources = doc.GetPages().GetPageAt(0).GetResources();
    unique_ptr<PdfFont> font;
    REQUIRE(PdfFont::TryCreateFromObject(*resources.GetResource(PdfResourceType::Font, "F1"), font));
    PdfFont* proxy;
    REQUIRE(font->TryCreateProxyFont(proxy));
    static_cast<PdfResourceOperations&>(resources).AddResource(PdfResourceType::Font, "F1", proxy->GetObject());
    charbuff buffer;
    BufferStreamDevice device(buffer);
    doc.Save(device);
    REQUIRE(proxy->GetDictionary().MustFindKey("CharProcs").GetDictionary().MustFindKey("A").HasStream());
}

TEST_CASE("TestGetOrCreateFontFromBufferInvalid")
{
    // A buffer FreeType can't open as a font must raise instead of
    // dereferencing null metrics, like the file path overload does
    PdfMemDocument doc;
    string_view junk = "this is not a font file";
    ASSERT_THROW_WITH_ERROR_CODE(doc.GetFonts().GetOrCreateFontFromBuffer(junk), PdfErrorCode::InvalidFontData);
    ASSERT_THROW_WITH_ERROR_CODE(doc.GetFonts().GetOrCreateFontFromBuffer(bufferview()), PdfErrorCode::InvalidFontData);
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

TEST_CASE("TestFontConfigReplaceDuringSearch")
{
    // Searches keep using their wrapper while another thread replaces it
    atomic<unsigned> running(4);
    vector<thread> threads;
    for (unsigned i = 0; i < 4; i++)
    {
        threads.emplace_back([&running]
        {
            PdfFontSearchParams params;
            for (unsigned j = 0; j < 10; j++)
                (void)PdfFontManager::SearchFontMetrics("Liberation Sans", params);

            running--;
        });
    }

    // Wrappers sharing the current config are quick to create
    auto config = PdfFontManager::GetFontConfigWrapper().GetFcConfig();
    while (running != 0)
        PdfFontManager::SetFontConfigWrapper(std::make_shared<PdfFontConfigWrapper>(FcConfigReference(config)));

    for (auto& thread : threads)
        thread.join();
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

TEST_CASE("TestInlineFontDirectRoot")
{
    // /Root and /Pages are direct dictionaries in the trailer, so the inline
    // font lookup walks up to the trailer without finding an indirect ancestor
    // NOTE: Of the major viewers only pdf.js renders it. Adobe, Foxit and pdfium fail
    string_view pdf = R"(%PDF-1.7
1 0 obj
<< /Type /Page /MediaBox [0 0 595 842] /Contents 2 0 R >>
endobj
2 0 obj
<< /Length 42 >>
stream
BT /F1 24 Tf 72 770 Td (Hello World) Tj ET
endstream
endobj
xref
0 3
0000000000 65535 f 
0000000009 00000 n 
0000000082 00000 n 
trailer
<< /Size 3 /Root << /Type /Catalog /Pages << /Type /Pages /Kids [1 0 R] /Count 1 /Resources << /Font << /F1 << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >> >> >> >> >> >>
startxref
174
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf, PdfLoadOptions::SkipXRefRecovery);
    auto& page = doc.GetPages().GetPageAt(0);
    auto font = page.GetResources().GetFont("F1");
    REQUIRE(font != nullptr);
    REQUIRE(font->GetMetrics().GetFontName() == "Helvetica");

    vector<PdfTextEntry> entries;
    page.ExtractTextTo(entries);
    REQUIRE(entries.size() == 1);
    REQUIRE(entries[0].Text == "Hello World");
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

TEST_CASE("TestSubsetCFFCIDNoGlyphs")
{
    // A CID keyed CFF with an empty CharStrings INDEX: header, Name, Top DICT
    // (ROS Adobe-Identity-0, CharStrings, FDArray, FDSelect), String and Global
    // Subr INDEXes, CharStrings INDEX, FDSelect format 0, FDArray, Private DICT.
    // Subsetting always looks up CID 0 first, which must fail without a glyph
    const unsigned char cffData[] = {
        0x01, 0x00, 0x04, 0x01,
        0x00, 0x01, 0x01, 0x01, 0x02, 0x41,
        0x00, 0x01, 0x01, 0x01, 0x1C,
        0xF8, 0x1B, 0xF8, 0x1C, 0x8B, 0x0C, 0x1E,
        0x1D, 0x00, 0x00, 0x00, 0x3F, 0x11,
        0x1D, 0x00, 0x00, 0x00, 0x42, 0x0C, 0x24,
        0x1D, 0x00, 0x00, 0x00, 0x41, 0x0C, 0x25,
        0x00, 0x02, 0x01, 0x01, 0x06, 0x0E, 'A', 'd', 'o', 'b', 'e', 'I', 'd', 'e', 'n', 't', 'i', 't', 'y',
        0x00, 0x00,
        0x00, 0x00,
        0x00,
        0x00, 0x01, 0x01, 0x01, 0x08, 0x8D, 0x1D, 0x00, 0x00, 0x00, 0x4E, 0x12,
        0x8B, 0x14,
    };
    auto metrics = PdfFontMetrics::CreateFromBuffer(bufferview((const char*)cffData, sizeof(cffData)));
    REQUIRE(metrics->GetFontFileType() == PdfFontFileType::CIDKeyedCFF);

    vector<PdfCharGIDInfo> subsetInfos;
    subsetInfos.push_back({ 1, 1, PdfGID(0, 0) });

    PdfCIDSystemInfo cidInfo;
    cidInfo.Registry = PdfString("Adobe");
    cidInfo.Ordering = PdfString("Identity");
    cidInfo.Supplement = 0;

    charbuff cff;
    ASSERT_THROW_WITH_ERROR_CODE(afdko::SubsetFontCFF(*metrics, subsetInfos, cidInfo, cff), PdfErrorCode::InvalidFontData);
}

#endif // PODOFO_ENABLE_AFDKO

TEST_CASE("TestInlineFontCacheStale")
{
    // Inline fonts are cached by the resources ancestor and the name: loading
    // another document in the same instance, or replacing the resource, must
    // not return the wrapper of the previous object
    string_view pdf1 = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Resources<</Font<</F1<</Type/Font/Subtype/Type1/BaseFont/Helvetica>>>>>>>>endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";
    string_view pdf2 = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Resources<</Font<</F1<</Type/Font/Subtype/Type1/BaseFont/Courier>>>>>>>>endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf1);
    auto font = doc.GetPages().GetPageAt(0).GetResources().GetFont("F1");
    REQUIRE(font != nullptr);
    REQUIRE(font->GetMetrics().GetFontName() == "Helvetica");

    doc.LoadFromBuffer(pdf2);
    auto& resources = doc.GetPages().GetPageAt(0).GetResources();
    font = resources.GetFont("F1");
    REQUIRE(font != nullptr);
    REQUIRE(&font->GetObject() == resources.GetResource(PdfResourceType::Font, "F1"));
    REQUIRE(font->GetMetrics().GetFontName() == "Courier");

    // NOTE: The replacement may reuse the address of the removed object
    PdfObject fontDict;
    fontDict.GetDictionary().AddKey("Type"_n, "Font"_n);
    fontDict.GetDictionary().AddKey("Subtype"_n, "Type1"_n);
    fontDict.GetDictionary().AddKey("BaseFont"_n, "Times-Roman"_n);
    static_cast<PdfResourceOperations&>(resources).AddResource(PdfResourceType::Font, "F1"_n, fontDict);
    font = resources.GetFont("F1");
    REQUIRE(font != nullptr);
    REQUIRE(&font->GetObject() == resources.GetResource(PdfResourceType::Font, "F1"));
    REQUIRE(font->GetMetrics().GetFontName() == "Times-Roman");
}

TEST_CASE("TestInlineFontFillFromPageNoResources")
{
    // Filling a form from a page without /Resources keeps the
    // form ones, so their cached inline fonts must stay valid
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R 4 0 R]/Count 2>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
4 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Resources<</XObject<</X1 5 0 R>>>>>>endobj
5 0 obj<</Type/XObject/Subtype/Form/BBox[0 0 9 9]/Resources<</Font<</F1<</Type/Font/Subtype/Type1/BaseFont/Helvetica>>>>>>/Length 0>>stream

endstream
endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    unique_ptr<PdfXObjectForm> form;
    REQUIRE(PdfXObject::TryCreateFromObject(*doc.GetPages().GetPageAt(1).GetResources().GetResource(PdfResourceType::XObject, "X1"), form));
    auto font = form->GetResources()->GetFont("F1");
    REQUIRE(font != nullptr);

    form->FillFromPage(doc.GetPages().GetPageAt(0));
    REQUIRE(font->GetMetrics().GetFontName() == "Helvetica");
    REQUIRE(form->GetResources()->GetFont("F1") == font);
}

TEST_CASE("TestFreeFacesWhileCreatingOnAnotherThread")
{
    // Faces created on another thread are freed here while
    // that thread keeps creating faces on its FreeType library
    auto fontPath = TestUtils::GetTestInputFilePath("Fonts", "LiberationSans-Regular.ttf");
    mutex createdMutex;
    vector<unique_ptr<const PdfFontMetrics>> created;
    atomic<bool> done(false);
    thread creator([&]
    {
        for (unsigned i = 0; i < 40; i++)
        {
            auto metrics = PdfFontMetrics::Create(fontPath);
            lock_guard<mutex> lock(createdMutex);
            created.push_back(std::move(metrics));
        }

        done = true;
    });

    while (!done)
    {
        lock_guard<mutex> lock(createdMutex);
        created.clear();
    }

    creator.join();
}

TEST_CASE("TestGetOrCreateFontNullMetrics")
{
    PdfMemDocument doc;
    ASSERT_THROW_WITH_ERROR_CODE(doc.GetFonts().GetOrCreateFont(PdfFontMetricsConstPtr()), PdfErrorCode::InvalidHandle);
}

TEST_CASE("TestAddSubsetCIDsWithoutSubsetting")
{
    // Adding CIDs to a proxy font created without subsetting is a no-op
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestEmbedFont.pdf"));
    unique_ptr<PdfFont> font;
    REQUIRE(PdfFont::TryCreateFromObject(doc.GetObjects().MustGetObject(PdfReference(6, 0)), font));
    PdfFont* proxy;
    REQUIRE(font->TryCreateProxyFont(PdfFontCreateFlags::DontSubset, proxy));
    REQUIRE(!proxy->IsSubsettingEnabled());
    proxy->AddSubsetCIDs(PdfString::FromRaw("TEST"));
}

TEST_CASE("TestFontManagerAfterGarbageCollection")
{
    // Imported fonts not yet used are collected: the manager drops them
    // instead of embedding through the freed object at the next save
    PdfMemDocument doc;
    doc.GetPages().CreatePage(PdfPageSize::A4);
    auto& font = doc.GetFonts().GetStandard14Font(PdfStandard14FontType::Helvetica);
    auto ref = font.GetObject().GetIndirectReference();
    doc.CollectGarbage();
    REQUIRE(doc.GetFonts().GetCachedFont(ref) == nullptr);
    doc.GetFonts().EmbedFonts();
    auto& font2 = doc.GetFonts().GetStandard14Font(PdfStandard14FontType::Helvetica);
    REQUIRE(font2.GetObject().GetIndirectReference() != ref);
    doc.Save(TestUtils::GetTestOutputFilePath("TestFontManagerAfterGarbageCollection.pdf"));
}
