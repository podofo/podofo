/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _LUA_CONVERTER_H_
#define _LUA_CONVERTER_H_

#include "iconverter.h"

struct lua_State;

class LuaMachina
{
    lua_State* L;

public:
    LuaMachina();
    ~LuaMachina();

    inline lua_State* State()
    {
        return L;
    }
};

/**
 * Converter that calls a lua script
 * to do the color conversions.
 */
class LuaConverter : public IConverter
{
public:
    LuaConverter(const std::string_view& luaScript);
    virtual ~LuaConverter();

    /**
     * A helper method that is called to inform the converter
     * when a new page is analyzed.
     *
     * @param pPage page object
     * @param nPageIndex index of the page in the document
     */
    virtual void StartPage(PoDoFo::PdfPage& page, int pageIndex);

    /**
     * A helper method that is called to inform the converter
     * when a new page has been analyzed completely.
     *
     * @param pPage page object
     * @param nPageIndex index of the page in the document
     */
    virtual void EndPage(PoDoFo::PdfPage& page, int pageIndex);

    /**
     * A helper method that is called to inform the converter
     * when a new xobjext is analyzed.
     *
     * @param pObj the xobject
     */
    virtual void StartXObject(PoDoFo::PdfXObject& obj);

    /**
     * A helper method that is called to inform the converter
     * when a xobjext has been analyzed.
     *
     * @param pObj the xobject
     */
    virtual void EndXObject(PoDoFo::PdfXObject& obj);

    /**
     * This method is called whenever a gray stroking color is set
     * using the 'G' PDF command.
     *
     * @param a grayscale color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetStrokingColorGray(const PoDoFo::PdfColor& color);

    /**
     * This method is called whenever a RGB stroking color is set
     * using the 'RG' PDF command.
     *
     * @param a RGB color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetStrokingColorRGB(const PoDoFo::PdfColor& color);

    /**
     * This method is called whenever a CMYK stroking color is set
     * using the 'K' PDF command.
     *
     * @param a CMYK color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetStrokingColorCMYK(const PoDoFo::PdfColor& color);

    /**
     * This method is called whenever a gray non-stroking color is set
     * using the 'g' PDF command.
     *
     * @param a grayscale color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetNonStrokingColorGray(const PoDoFo::PdfColor& color);

    /**
     * This method is called whenever a RGB non-stroking color is set
     * using the 'rg' PDF command.
     *
     * @param a RGB color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetNonStrokingColorRGB(const PoDoFo::PdfColor& color);

    /**
     * This method is called whenever a CMYK non-stroking color is set
     * using the 'k' PDF command.
     *
     * @param a CMYK color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetNonStrokingColorCMYK(const PoDoFo::PdfColor& color);

private:
    /**
     * Create a PdfColor from an array returned on the stack
     * by a Lua function.
     * @param pszFunctionName name of Lua function for error reporting
     * @returns a PdfColor or throws a PdfError if color cannot be created
     */
    PoDoFo::PdfColor GetColorFromReturnValue(const char* functionName);

private:
    LuaMachina m_machina;
};


#endif // _LUA_CONVERTER_H_
