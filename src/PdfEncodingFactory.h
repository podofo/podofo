/***************************************************************************
 *   Copyright (C) 2008 by Dominik Seichter                                *
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

#ifndef _PDF_ENCODING_FACTORY_H_
#define _PDF_ENCODING_FACTORY_H_

#include "PdfDefines.h"

namespace PoDoFo {

class PdfEncoding;
class PdfObject;

/** This factory creates a PdfEncoding
 *  from an existing object in the PDF.
 */
class PODOFO_API PdfEncodingFactory {
 public:
    /** Create a new PdfEncoding from either an
     *  encoding name or an encoding dictionary.
     *
     *  \param pObject must be a name or an encoding dictionary
     *
     *  \returns a PdfEncoding or NULL
     */
    static const PdfEncoding* const CreateEncoding( PdfObject* pObject );
};

}; /* namespace PoDoFo */


#endif // _PDF_ENCODING_FACTORY_H__

