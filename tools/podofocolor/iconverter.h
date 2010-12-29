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

#ifndef _ICONVERTER_H_
#define _ICONVERTER_H_

#include <podofo.h>

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
     * @param pPage page object
     * @param nPageIndex index of the page in the document
     */
    virtual void StartPage( PoDoFo::PdfPage* pPage, int nPageIndex ) = 0;

    /**
     * A helper method that is called to inform the converter
     * when a new page has been analyzed completely.
     * 
     * @param pPage page object
     * @param nPageIndex index of the page in the document
     */
    virtual void EndPage( PoDoFo::PdfPage* pPage, int nPageIndex ) = 0;

    /**
     * A helper method that is called to inform the converter
     * when a new xobjext is analyzed.
     * 
     * @param pObj the xobject
     */
    virtual void StartXObject( PoDoFo::PdfXObject* pObj ) = 0;

    /**
     * A helper method that is called to inform the converter
     * when a xobjext has been analyzed.
     * 
     * @param pObj the xobject
     */
    virtual void EndXObject( PoDoFo::PdfXObject* pObj ) = 0;

    /**
     * This method is called whenever a gray stroking color is set
     * using the 'G' PDF command.
     *
     * @param a grayscale color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetStrokingColorGray( const PoDoFo::PdfColor & rColor ) = 0;

    /**
     * This method is called whenever a RGB stroking color is set
     * using the 'RG' PDF command.
     *
     * @param a RGB color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetStrokingColorRGB( const PoDoFo::PdfColor & rColor ) = 0;

    /**
     * This method is called whenever a CMYK stroking color is set
     * using the 'K' PDF command.
     *
     * @param a CMYK color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetStrokingColorCMYK( const PoDoFo::PdfColor & rColor ) = 0;

    /**
     * This method is called whenever a gray non-stroking color is set
     * using the 'g' PDF command.
     *
     * @param a grayscale color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetNonStrokingColorGray( const PoDoFo::PdfColor & rColor ) = 0;

    /**
     * This method is called whenever a RGB non-stroking color is set
     * using the 'rg' PDF command.
     *
     * @param a RGB color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetNonStrokingColorRGB( const PoDoFo::PdfColor & rColor ) = 0;

    /**
     * This method is called whenever a CMYK non-stroking color is set
     * using the 'k' PDF command.
     *
     * @param a CMYK color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetNonStrokingColorCMYK( const PoDoFo::PdfColor & rColor ) = 0;

};

#endif // _ICONVERTER_H_
