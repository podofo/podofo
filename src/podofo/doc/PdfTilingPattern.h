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

#ifndef _PDF_TILING_PATTERN_H_
#define _PDF_TILING_PATTERN_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfName.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfImage;
class PdfObject;
class PdfPage;
class PdfWriter;

/** 
 * This class defined a tiling pattern which can be used
 * to fill abitrary shapes with a pattern using PdfPainter.
 */
class PODOFO_DOC_API PdfTilingPattern : public PdfElement {
 public:
    virtual ~PdfTilingPattern();

    /** Returns the identifier of this TilingPattern how it is known
     *  in the pages resource dictionary.
     *  \returns PdfName containing the identifier (e.g. /PtrnXXXXX)
     */
    inline const PdfName & GetIdentifier() const;

    /** Create a new PdfTilingPattern object, which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param eTilingType the type of this tiling pattern
     *  \param strokeR strok color red component
     *  \param strokeG strok color green component
     *  \param strokeB strok color blue component
     *  \param doFill whether tile fills content first, with fill color
     *  \param fillR fill color red component
     *  \param fillG fill color green component
     *  \param fillB fill color blue component
     *  \param offsetX tile offset on X axis
     *  \param offsetY tile offset on Y axis
     *  \param pImage image to use - can be set only if eTilingType is ePdfTilingPatternType_Image
     *  \param pParent parent vector of objects
     *  
     *  \note stroke and fill colors are ignored if eTilingType is ePdfTilingPatternType_Image
     *
     *  \note fill color is ignored if doFill is false
     *
     *  \note pImage is ignored for all but ePdfTilingPatternType_Image eTilingType types, where it cannot be NULL
     *  
     */
    PdfTilingPattern( EPdfTilingPatternType eTilingType,
		 double strokeR, double strokeG, double strokeB,
		 bool doFill, double fillR, double fillG, double fillB,
		 double offsetX, double offsetY,
		 PdfImage *pImage,
		 PdfVecObjects* pParent);

    /** Create a new PdfTilingPattern object, which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param eTilingType the type of this tiling pattern
     *  \param strokeR strok color red component
     *  \param strokeG strok color green component
     *  \param strokeB strok color blue component
     *  \param doFill whether tile fills content first, with fill color
     *  \param fillR fill color red component
     *  \param fillG fill color green component
     *  \param fillB fill color blue component
     *  \param offsetX tile offset on X axis
     *  \param offsetY tile offset on Y axis
     *  \param pImage image to use - can be set only if eTilingType is ePdfTilingPatternType_Image
     *  \param pParent parent document
     *
     *  \note stroke and fill colors are ignored if eTilingType is ePdfTilingPatternType_Image
     *
     *  \note fill color is ignored if doFill is false
     *
     *  \note pImage is ignored for all but ePdfTilingPatternType_Image eTilingType types, where it cannot be NULL
     *  
     */
    PdfTilingPattern( EPdfTilingPatternType eTilingType,
		 double strokeR, double strokeG, double strokeB,
		 bool doFill, double fillR, double fillG, double fillB,
		 double offsetX, double offsetY,
		 PdfImage *pImage,
		 PdfDocument* pParent);

 private:
    /** Initialize the object
     */
    void Init( EPdfTilingPatternType eTilingType,
		 double strokeR, double strokeG, double strokeB,
		 bool doFill, double fillR, double fillG, double fillB,
		 double offsetX, double offsetY,
		 PdfImage *pImage);

	 void AddToResources(const PdfName &rIdentifier, const PdfReference &rRef, const PdfName &rName);
 private: 
    PdfName m_Identifier;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfName & PdfTilingPattern::GetIdentifier() const
{
    return m_Identifier;
}

} // end namespace

#endif // _PDF_TILING_PATTERN_H_
