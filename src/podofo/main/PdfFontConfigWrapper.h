/**
 * SPDX-FileCopyrightText: (C) 2011 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_CONFIG_WRAPPER_H
#define PDF_FONT_CONFIG_WRAPPER_H

#include "PdfDeclarations.h"

FORWARD_DECLARE_FCONFIG();

namespace PoDoFo {

enum class PdfFontConfigSearchFlags
{
    None = 0,
    MatchPostScriptName = 1,        ///< Match postscript font name. The default is match family name. This search may be more specific
};

struct PdfFontConfigSearchParams
{
    PdfFontStyle Style = PdfFontStyle::Regular;
    PdfFontConfigSearchFlags Flags = PdfFontConfigSearchFlags::None;
};

/**
 * This class initializes and destroys the FontConfig library.
 * 
 * As initializing fontconfig can take a long time, you 
 * can create a wrapper by yourself to cache initialization of
 * fontconfig.
 *
 * This class is reference counted. The last user of the fontconfig library
 * will destroy the fontconfig handle.
 *
 * The fontconfig library is initialized on first used (lazy loading!)
 */
class PODOFO_API PdfFontConfigWrapper final
{
public:
    /**
     * Create a new FontConfigWrapper and initialize the fontconfig library.
     */
    PdfFontConfigWrapper(FcConfig* fcConfig = nullptr);

    ~PdfFontConfigWrapper();

    /** Get the path of a font file on a Unix system using fontconfig
     *
     *  This method is only available if PoDoFo was compiled with
     *  fontconfig support. Make sure to lock any FontConfig mutexes before
     *  calling this method by yourself!
     *
     *  \param fontPattern search pattern of the requested font
     *  \param style font style
     *  \param faceIndex index of the face
     *  \returns the path to the fontfile or an empty string
     */
    std::string SearchFontPath(const std::string_view fontPattern, unsigned& faceIndex);
    std::string SearchFontPath(const std::string_view fontPattern, const PdfFontConfigSearchParams& params,
        unsigned& faceIndex);

    void AddFontDirectory(const std::string_view& path);

    FcConfig* GetFcConfig();

private:
    PdfFontConfigWrapper(const PdfFontConfigWrapper& rhs) = delete;
    const PdfFontConfigWrapper& operator=(const PdfFontConfigWrapper& rhs) = delete;

    void createDefaultConfig();

private:
    FcConfig* m_FcConfig;
};

};

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfFontConfigSearchFlags);

#endif // PDF_FONT_CONFIG_WRAPPER_H
