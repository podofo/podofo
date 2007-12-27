/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _PDF_FONT_FACTORY_H_
#define _PDF_FONT_FACTORY_H_

#include "PdfDefines.h"
#include "PdfFont.h"

namespace PoDoFo {

class PdfFontMetrics;
class PdfVecObjects;

/** This is a factory class which knows
 *  which implementation of PdfFont is required
 *  for a certain font type with certain features (like encoding).
 */
class PODOFO_API PdfFontFactory {
 public:

    /** Create a new PdfFont object.
     *
     *  \returns a new PdfFont object or NULL in case of an error.
     */
    static PdfFont* CreateFont( PdfFontMetrics* pMetrics, bool bEmbedd, bool bBold, bool bItalic, PdfVecObjects* pParent );

};

};

#endif /* _PDF_FONT_FACTORY_H_ */


