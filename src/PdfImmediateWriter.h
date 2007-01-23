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

#ifndef _PDF_IMMEDIATE_WRITER_H_
#define _PDF_IMMEDIATE_WRITER_H_

#include "PdfDefines.h"
#include "PdfWriter.h"

namespace PoDoFo {

class PdfOutputDevice;

class PODOFO_API PdfImmediateWriter : private PdfWriter {
 public:
    PdfImmediateWriter( PdfOutputDevice* pDevice, PdfVecObjects* pVecObjects, EPdfVersion eVersion = ePdfVersion_1_5 );
    ~PdfImmediateWriter();

 private:
    void WriteOut(); 
    void WriteObject( const PdfObject* pObject );

 private:
    PdfVecObjects*   m_pParent;
    PdfOutputDevice* m_pDevice;
};

};

#endif /* _PDF_IMMEDIATE_WRITER_H_ */

