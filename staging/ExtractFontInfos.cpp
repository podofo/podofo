#include <iostream>
#include <cassert>
#include <filesystem>

#include <podofo/podofo.h>
#include <podofo/private/PdfDeclarationsPrivate.h>
#include <podofo/private/FreetypePrivate.h>

#include <freetype/t1tables.h>

using namespace std;
using namespace PoDoFo;
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    auto teXGyreFontsPath = fs::u8path(utls::GetEnvironmentVariable("LIBERATION_SLIM_FONTS_PATH"));
    if (teXGyreFontsPath.empty())
        throw runtime_error("LIBERATION_SLIM_FONTS_PATH environment variable is missing");

    vector<string> fonts = {
        "LiberaLeanSerif-Regular.otf",
        "LiberaLeanSerif-Italic.otf",
        "LiberaLeanSerif-Bold.otf",
        "LiberaLeanSerif-BoldItalic.otf",
        "LiberaLeanSans-Regular.otf",
        "LiberaLeanSans-Italic.otf",
        "LiberaLeanSans-Bold.otf",
        "LiberaLeanSans-BoldItalic.otf",
        "LiberaLeanMono-Regular.otf",
        "LiberaLeanMono-Italic.otf",
        "LiberaLeanMono-Bold.otf",
        "LiberaLeanMono-BoldItalic.otf",
    };

    charbuff buffer;
    //for (auto& filepath : fonts)
    auto fontName = fonts[0];
    {
        auto metrics = PdfFontMetrics::Create((teXGyreFontsPath / fontName).u8string());
        auto matrix = metrics->GetMatrix();

        unsigned count = metrics->GetGlyphCount();
        for (unsigned i = 0; i < count; i++)
        {
            cout << "    " << (int16_t)(std::round(metrics->GetGlyphWidth(i) * 1000)) << "," << endl;
        }

        auto face = const_cast<PdfFontMetricsFreetype&>(static_cast<const PdfFontMetricsFreetype&>(*metrics)).GetFaceHandle();

        FT_Error rc = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
        assert(rc == 0);

        PS_PrivateRec rec;
        rc = FT_Get_PS_Font_Private(face, &rec);

        FT_ULong  charcode;
        FT_UInt   gindex;

        charcode = FT_Get_First_Char(face, &gindex);
        std::map<unsigned short, unsigned short> map;
        while (gindex != 0)
        {
            assert(charcode < 65536);
            map.insert({ (unsigned short)charcode, (unsigned short)gindex });
            charcode = FT_Get_Next_Char(face, charcode, &gindex);
        }

        char buff[2];
        unsigned char ch;
        for (auto& pair : map)
        {
            cout << "        { 0x";
            ch = (unsigned char)(pair.first >> 8 & 0xFFu);
            utls::WriteCharHexTo(buff, ch);
            cout << string(buff, 2);
            ch = (unsigned char)(pair.first & 0xFFu);
            utls::WriteCharHexTo(buff, ch);
            cout << string(buff, 2) <<", " << pair.second << " }," << endl;
        }

        cout << "            " << (int16_t)(std::round(metrics->GetDefaultWidth() * 1000)) << "," << endl;
        cout << "            " << "PdfFontStretch::Normal," << endl;
        cout << "            " << (int16_t)(std::round(metrics->GetAscent() * 1000)) << "," << endl;
        cout << "            " << (int16_t)(std::round(metrics->GetDescent() * 1000)) << "," << endl;
        cout << "            " << (int16_t)(std::round(metrics->GetXHeight() * 1000)) << "," << endl;
        cout << "            " << (int16_t)(std::round(metrics->GetCapHeight() * 1000)) << "," << endl;
        cout << "            " << (int16_t)(std::round(metrics->GetItalicAngle() * 1000)) << "," << endl;
        cout << "            " << metrics->GetWeight() << "," << endl;
        cout << "            " << (int16_t)(std::round(metrics->GetStemV() * 1000)) << "," << endl;
        cout << "            " << (int16_t)(std::round(metrics->GetStemH() * 1000)) << "," << endl;
        cout << "            " << (int16_t)(std::round(metrics->GetStrikeThroughPosition() * 1000)) << "," << endl;
        cout << "            " << (int16_t)(std::round(metrics->GetUnderlinePosition() * 1000)) << "," << endl;
        auto box = metrics->GetBoundingBox();
        cout << "            " << "Corners("
            << (int16_t)(std::round(box.GetCorner1().X * 1000)) << ", "
            << (int16_t)(std::round(box.GetCorner1().Y * 1000)) << ", "
            << (int16_t)(std::round(box.GetCorner2().X * 1000)) << ", "
            << (int16_t)(std::round(box.GetCorner2().Y * 1000))
            << ")" << endl;
    }

    return 0;
}
