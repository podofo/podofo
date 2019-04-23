/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_ENCODING_OBJECT_FACTORY_H_
#define _PDF_ENCODING_OBJECT_FACTORY_H_

#include "podofo/base/PdfDefines.h"

namespace PoDoFo {

class PdfEncoding;
class PdfObject;

/** This factory creates a PdfEncoding
 *  from an existing object in the PDF.
 */
class PODOFO_DOC_API PdfEncodingObjectFactory {
 public:
    /** Create a new PdfEncoding from either an
     *  encoding name or an encoding dictionary.
     *
     *  \param pObject must be a name or an encoding dictionary
     *  \param pToUnicode the optional ToUnicode dictionary
     *  \param bExplicitNames if true, glyph names are meaningless explicit keys on the font (used for Type3 fonts)
     *
     *  \returns a PdfEncoding or NULL
     */
    static const PdfEncoding* CreateEncoding( PdfObject* pObject, PdfObject *pToUnicode = NULL, bool bExplicitNames = false );

private:
    /**
     * Hidden default constructor
     */
    PdfEncodingObjectFactory();
};

}; /* namespace PoDoFo */

#endif // _PDF_ENCODING_OBJECT_FACTORY_H_
