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

#ifndef _GRAYSCALE_CONVERTER_H_
#define _GRAYSCALE_CONVERTER_H_

#include "iconverter.h"

/**
 * A converter which converts every color to its grayscale value.
 */
class GrayscaleConverter : public IConverter {
public:
    GrayscaleConverter();
    virtual ~GrayscaleConverter();

    /**
     * A helper method that is called to inform the converter
     * when a new page is analyzed.
     * 
     * @param pPage page object
     * @param nPageIndex index of the page in the document
     */
    virtual void StartPage( PoDoFo::PdfPage* pPage, int nPageIndex );

    /**
     * A helper method that is called to inform the converter
     * when a new page has been analyzed completely.
     * 
     * @param pPage page object
     * @param nPageIndex index of the page in the document
     */
    virtual void EndPage( PoDoFo::PdfPage* pPage, int nPageIndex );

    /**
     * A helper method that is called to inform the converter
     * when a new xobjext is analyzed.
     * 
     * @param pObj the xobject
     */
    virtual void StartXObject( PoDoFo::PdfXObject* pObj );

    /**
     * A helper method that is called to inform the converter
     * when a xobjext has been analyzed.
     * 
     * @param pObj the xobject
     */
    virtual void EndXObject( PoDoFo::PdfXObject* pObj );

    /**
     * This method is called whenever a gray stroking color is set
     * using the 'G' PDF command.
     *
     * @param a grayscale color object
     * @returns a new color which should be set instead (any colorspace)
     */
    virtual PoDoFo::PdfColor SetStrokingColorGray( const PoDoFo::PdfColor & rColor );

    virtual PoDoFo::PdfColor SetStrokingColorRGB( const PoDoFo::PdfColor & rColor );
    virtual PoDoFo::PdfColor SetStrokingColorCMYK( const PoDoFo::PdfColor & rColor );
  
    virtual PoDoFo::PdfColor SetNonStrokingColorGray( const PoDoFo::PdfColor & rColor );
    virtual PoDoFo::PdfColor SetNonStrokingColorRGB( const PoDoFo::PdfColor & rColor );
    virtual PoDoFo::PdfColor SetNonStrokingColorCMYK( const PoDoFo::PdfColor & rColor );
  
};

#endif // _GRAYSCALE_CONVERTER_H_
