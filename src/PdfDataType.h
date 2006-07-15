/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#ifndef _PDF_DATATYPE_H_
#define _PDF_DATATYPE_H_

#include "PdfDefines.h"

namespace PoDoFo {

class PdfOutputDevice;

/** An interface for all PDF datatype classes.
 *
 *  
 *  \see PdfName \see PdfArray \see PdfReference 
 *  \see PdfVariant \see PdfDictionary \see PdfString
 */
class PdfDataType {

 protected:
    /** Create a new PdfDataType.
     *  Can only be called by subclasses
     */
    PdfDataType();

 public:
    virtual ~PdfDataType();

    /** Write the complete datatype to a file.
     *  \param pDevice write the object to this device
     */
    virtual void Write( PdfOutputDevice* pDevice ) const = 0;
};

};


#endif /* _PDF_DATATYPE_H_ */

