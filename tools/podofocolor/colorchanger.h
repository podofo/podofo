/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _COLORCHANGER_H_
#define _COLORCHANGER_H_

#include <string>
#include <vector>
#include <podofo/podofo.h>

class IConverter;
class GraphicsStack;

/**
 * This class provides a tool to change all colors
 * in a PDF file.
 */
class ColorChanger
{
public:
    enum EKeywordType
    {
        eKeyword_GraphicsStack_Push,
        eKeyword_GraphicsStack_Pop,

        eKeyword_SelectGray_Stroking,
        eKeyword_SelectRGB_Stroking,
        eKeyword_SelectCMYK_Stroking,

        eKeyword_SelectGray_NonStroking,
        eKeyword_SelectRGB_NonStroking,
        eKeyword_SelectCMYK_NonStroking,

        eKeyword_SelectColorSpace_Stroking,
        eKeyword_SelectColorSpace_NonStroking,

        eKeyword_SelectColor_Stroking,
        eKeyword_SelectColor_NonStroking,

        eKeyword_SelectColor_Stroking2,
        eKeyword_SelectColor_NonStroking2,

        eKeyword_Undefined = 0xffff
    };


    /**
     * KWInfo describes a single PDF keyword's characteristics. See kwInfo[] .
     */
    struct KWInfo
    {
        ColorChanger::EKeywordType eKeywordType;
        /// null-terminated keyword text
        const char pszText[6];
        /// Number of arguments
        int nNumArguments;
        /// Short description text (optional, set to NULL if undesired).
        const char* pszDescription;
    };

    /**
     * Construct a new colorchanger object
     * @param pConverter a converter which is applied to all color definitions
     * @param sInput the input PDF file
     * @param sOutput write output to this filename
     */
    ColorChanger(IConverter* convert, const std::string_view& input, const std::string_view& output);

    /**
     * Start processing the input file.
     */
    void start();

private:
    /**
     * Replace all colors in the given page.
     * @param pPage may not be NULL
     */
    void ReplaceColorsInPage(PoDoFo::PdfCanvas& page);

    /**
     * Convert a keyword name to a keyword typee
     * @param pszKeyword name of a keyword
     * @return the keyword type or eKeywordType_Undefined if unknown
     */
    const KWInfo* FindKeyWordByName(const std::string_view& keyword);

    PoDoFo::PdfColor GetColorFromStack(int argCount, std::vector<PoDoFo::PdfVariant>& args);
    void PutColorOnStack(const PoDoFo::PdfColor& color, std::vector<PoDoFo::PdfVariant>
        & args);

    const char* ProcessColor(EKeywordType keywordType, int numArgs, std::vector<PoDoFo::PdfVariant>& args, GraphicsStack& graphicsStack);

    const char* GetKeywordForColor(const PoDoFo::PdfColor& color, bool isStroking);

    /** Write a list of arguments and optionally a keyword
     *  to an output device
     *
     *  @param rArgs list of arguments which will be written to rDevice (will be cleared afterwards).
     *  @param pszKeyword a keyword or NULL to be written after the arguments
     *  @param rDevice output device
     */
    void WriteArgumentsAndKeyword(std::vector<PoDoFo::PdfVariant>& args, const std::string_view& keyword, PoDoFo::OutputStreamDevice& stream);

    /**
     * unused
     */
    PoDoFo::PdfColorSpaceType GetColorSpaceForName(const PoDoFo::PdfName& name, PoDoFo::PdfCanvas& page);

    /**
     * unused
     */
    PoDoFo::PdfColorSpaceType GetColorSpaceForArray(const PoDoFo::PdfArray& arr, PoDoFo::PdfCanvas& page);
private:
    IConverter* m_converter;
    std::string m_input;
    std::string m_output;
};

#endif // _COLORCHANGER_H_
