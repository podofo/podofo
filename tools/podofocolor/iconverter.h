/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _ICONVERTER_H_
#define _ICONVERTER_H_

#include <podofo/podofo.h>

 /**
  * Interface for a converter that can be used
  * together with a ColorChanger object
  * to convert colors in a PDF file.
  */
class IConverter {

public:
    IConverter();
    virtual ~IConverter();

    /**
     * A helper method that is called to inform the converter
     * when a new page is analyzed.
     *
     * @param page page object
     * @param nPageIndex index of the page in the document
     */
    virtual void StartPage(PoDoFo::PdfPage& page, int pageIndex) = 0;

    /**
     * A helper method that is called to inform the converter
     * when a new page has been analyzed completely.
     *
     * @param page page object
     * @param nPageIndex index of the page in the document
     */
    virtual void EndPage(PoDoFo::PdfPage& page, int pageIndex) = 0;

    /**
     * A helper method that is called to inform the converter
     * when a new xobjext is analyzed.
     *
     * @param obj the xobject
     */
    virtual void StartXObject(PoDoFo::PdfXObject& obj) = 0;

    /**
     * A helper method that is called to inform the converter
     * when a xobjext has been analyzed.
     *
     * @param obj the xobject
     */
    virtual void EndXObject(PoDoFo::PdfXObject& obj) = 0;

    /**
     * This method is called whenever a gray stroking color is set
     * using the 'G' PDF command.
     *
     * @param a grayscale color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetStrokingColorGray(const PoDoFo::PdfColor& color) = 0;

    /**
     * This method is called whenever a RGB stroking color is set
     * using the 'RG' PDF command.
     *
     * @param a RGB color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetStrokingColorRGB(const PoDoFo::PdfColor& color) = 0;

    /**
     * This method is called whenever a CMYK stroking color is set
     * using the 'K' PDF command.
     *
     * @param a CMYK color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetStrokingColorCMYK(const PoDoFo::PdfColor& color) = 0;

    /**
     * This method is called whenever a gray non-stroking color is set
     * using the 'g' PDF command.
     *
     * @param a grayscale color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetNonStrokingColorGray(const PoDoFo::PdfColor& color) = 0;

    /**
     * This method is called whenever a RGB non-stroking color is set
     * using the 'rg' PDF command.
     *
     * @param a RGB color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetNonStrokingColorRGB(const PoDoFo::PdfColor& color) = 0;

    /**
     * This method is called whenever a CMYK non-stroking color is set
     * using the 'k' PDF command.
     *
     * @param a CMYK color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetNonStrokingColorCMYK(const PoDoFo::PdfColor& color) = 0;

};

#endif // _ICONVERTER_H_
