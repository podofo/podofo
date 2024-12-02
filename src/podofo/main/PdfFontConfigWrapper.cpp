/**
 * SPDX-FileCopyrightText: (C) 2011 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontConfigWrapper.h"

#include <fontconfig/fontconfig.h>

using namespace std;
using namespace PoDoFo;

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
// Windows, Android and Apple architectures don't primarily
// use fontconfig. We can supply a fallback configuration,
// if a system configuration is not found
#define HAS_FALLBACK_CONFIGURATION
#endif

PdfFontConfigWrapper::PdfFontConfigWrapper(const string_view& configStr)
    : m_FcConfig(FcConfigCreate())
{
    m_FcConfig = FcConfigCreate();
    if (m_FcConfig == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Could not allocate font config");

    // No system config found, supply a fallback configuration
    if (!FcConfigParseAndLoadFromMemory(m_FcConfig, (const FcChar8*)configStr.data(), true))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Could not parse font config");

    if (!FcConfigBuildFonts(m_FcConfig))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Could not parse font config");
}

PdfFontConfigWrapper::PdfFontConfigWrapper(FcConfig* fcConfig)
    : m_FcConfig(fcConfig)
{
    if (fcConfig == nullptr)
        createDefaultConfig();
}

PdfFontConfigWrapper::~PdfFontConfigWrapper()
{
    FcConfigDestroy(m_FcConfig);
}

string PdfFontConfigWrapper::SearchFontPath(const string_view fontPattern, unsigned& faceIndex)
{
    return SearchFontPath(fontPattern, { }, faceIndex);
}

string PdfFontConfigWrapper::SearchFontPath(const string_view fontPattern,
    const PdfFontConfigSearchParams& params, unsigned& faceIndex)
{
    FcPattern* matched = nullptr;
    FcResult result = FcResultNoMatch;
    FcValue value;
    string path;
    faceIndex = 0;

    auto cleanup = [&]()
    {
        FcPatternDestroy(matched);
    };

    try
    {
        if ((params.Flags & PdfFontConfigSearchFlags::SkipMatchPostScriptName) == PdfFontConfigSearchFlags::None)
        {
            // Try to match postscript name only first

            unique_ptr<FcPattern, decltype(&FcPatternDestroy)> pattern(FcPatternCreate(), FcPatternDestroy);
            if (pattern == nullptr)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "FcPatternCreate returned NULL");

            if (!FcPatternAddString(pattern.get(), FC_POSTSCRIPT_NAME, (const FcChar8*)fontPattern.data()))
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "FcPatternAddString");

            if (params.Style.has_value())
            {
                if (*params.Style == PdfFontStyle::Regular)
                {
                    // Ensure the font will be at least not italic/oblique
                    if (!FcPatternAddInteger(pattern.get(), FC_SLANT, FC_SLANT_ROMAN))
                        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "FcPatternAddInteger");
                }
                else
                {
                    bool isItalic = (*params.Style & PdfFontStyle::Italic) == PdfFontStyle::Italic;
                    bool isBold = (*params.Style & PdfFontStyle::Bold) == PdfFontStyle::Bold;

                    if (isBold && !FcPatternAddInteger(pattern.get(), FC_WEIGHT, FC_WEIGHT_BOLD))
                        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "FcPatternAddInteger");

                    if (isItalic && !FcPatternAddInteger(pattern.get(), FC_SLANT, FC_SLANT_ITALIC))
                        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "FcPatternAddInteger");
                }


            }

            // We will enlist all fonts with the requested style. We produce font
            // collections that has a limited set of properties, so subsequent match
            // will be faster
            unique_ptr<FcObjectSet, decltype(&FcObjectSetDestroy)> objectSet(FcObjectSetBuild(FC_POSTSCRIPT_NAME, FC_FILE, FC_INDEX, nullptr), FcObjectSetDestroy);

            unique_ptr<FcFontSet, decltype(&FcFontSetDestroy)> fontSet(FcFontList(m_FcConfig, pattern.get(), objectSet.get()), FcFontSetDestroy);
            if (fontSet->nfont > 0)
            {
                matched = fontSet->fonts[0];
                FcPatternReference(matched);
                result = FcResultMatch;
            }
        }

        if (result == FcResultNoMatch)
        {
            // Match on family name, using also styles if set
            unique_ptr<FcPattern, decltype(&FcPatternDestroy)> pattern(FcPatternCreate(), FcPatternDestroy);
            if (pattern == nullptr)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "FcPatternCreate returned NULL");

            if (params.FontFamilyPattern.length() == 0)
            {
                if (!FcPatternAddString(pattern.get(), FC_FAMILY, (const FcChar8*)fontPattern.data()))
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "FcPatternAddString");
            }
            else
            {
                if (!FcPatternAddString(pattern.get(), FC_FAMILY, (const FcChar8*)params.FontFamilyPattern.data()))
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "FcPatternAddString");
            }

            if (params.Style.has_value())
            {
                // NOTE: No need to set FC_SLANT_ROMAN, FC_WEIGHT_MEDIUM for PdfFontStyle::Regular.
                // It's done already by FcDefaultSubstitute

                bool isItalic = (*params.Style & PdfFontStyle::Italic) == PdfFontStyle::Italic;
                bool isBold = (*params.Style & PdfFontStyle::Bold) == PdfFontStyle::Bold;

                if (isBold && !FcPatternAddInteger(pattern.get(), FC_WEIGHT, FC_WEIGHT_BOLD))
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "FcPatternAddInteger");

                if (isItalic && !FcPatternAddInteger(pattern.get(), FC_SLANT, FC_SLANT_ITALIC))
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "FcPatternAddInteger");
            }

            // Perform recommended normalization, as documented in
            // https://www.freedesktop.org/software/fontconfig/fontconfig-devel/fcfontmatch.html
            FcDefaultSubstitute(pattern.get());

            matched = FcFontMatch(m_FcConfig, pattern.get(), &result);
        }

        if (result != FcResultNoMatch)
        {
            (void)FcPatternGet(matched, FC_FILE, 0, &value);
            path = reinterpret_cast<const char*>(value.u.s);
            (void)FcPatternGet(matched, FC_INDEX, 0, &value);
            faceIndex = (unsigned)value.u.i;

#if _WIN32
            // Font config in Windows returns unix conventional path
            // separator. Fix it
            std::replace(path.begin(), path.end(), '/', '\\');
#endif
        }
    }
    catch (const exception& ex)
    {
        PoDoFo::LogMessage(PdfLogSeverity::Error, ex.what());
    }
    catch (...)
    {
        PoDoFo::LogMessage(PdfLogSeverity::Error, "Unknown error during FontConfig search");
    }

    cleanup();
    return path;
}

void PdfFontConfigWrapper::AddFontDirectory(const string_view& path)
{
    if (!FcConfigAppFontAddDir(m_FcConfig, (const FcChar8*)path.data()))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Unable to add font directory");
}

FcConfig* PdfFontConfigWrapper::GetFcConfig()
{
    return m_FcConfig;
}

void PdfFontConfigWrapper::createDefaultConfig()
{
#ifdef _WIN32
    const char* fontconf =
        R"(<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
    <dir>WINDOWSFONTDIR</dir>
    <dir>WINDOWSUSERFONTDIR</dir>
    <dir prefix="xdg">fonts</dir>
    <cachedir>LOCAL_APPDATA_FONTCONFIG_CACHE</cachedir>
    <cachedir prefix="xdg">fontconfig</cachedir>
</fontconfig>
)";
#elif __ANDROID__
    // On android fonts are located in /system/fonts
    const char* fontconf =
        R"(<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
    <dir>/system/fonts</dir>
    <dir prefix="xdg">fonts</dir>
    <cachedir prefix="xdg">fontconfig</cachedir>
</fontconfig>
)";
#elif __APPLE__
    // Fonts location https://stackoverflow.com/a/2557291/213871
    const char* fontconf =
        R"(<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
    <dir>/System/Library/Fonts</dir>
    <dir prefix="xdg">fonts</dir>
    <cachedir prefix="xdg">fontconfig</cachedir>
</fontconfig>
)";
#endif

#ifdef HAS_FALLBACK_CONFIGURATION
    // Implement the fallback as discussed in fontconfig mailing list
    // https://lists.freedesktop.org/archives/fontconfig/2022-February/006883.html

    auto config = FcConfigCreate();
    if (config == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Could not allocate font config");

    // Manually try to load the config to determine
    // if a system configuration exists. Tell FontConfig
    // to not complain if it doesn't
    (void)FcConfigParseAndLoad(config, nullptr, FcFalse);

    auto configFiles = FcConfigGetConfigFiles(config);
    if (FcStrListNext(configFiles) == nullptr)
    {
        // No system config found, supply a fallback configuration
        if (!FcConfigParseAndLoadFromMemory(config, (const FcChar8*)fontconf, true))
        {
            FcConfigDestroy(config);
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Could not parse font config");
        }

        // Load fonts for the config
        if (!FcConfigBuildFonts(config))
        {
            FcConfigDestroy(config);
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Could not parse font config");
        }

        m_FcConfig = config;
    }
    else
    {
        // Destroy the temporary config
        FcStrListDone(configFiles);
        FcConfigDestroy(config);
#endif
        // Default initialize a local FontConfig configuration
        // http://mces.blogspot.com/2015/05/how-to-use-custom-application-fonts.html
        m_FcConfig = FcInitLoadConfigAndFonts();
        PODOFO_ASSERT(m_FcConfig != nullptr);
#ifdef HAS_FALLBACK_CONFIGURATION
    }
#endif
}
