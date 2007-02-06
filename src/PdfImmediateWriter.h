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
#include "PdfVecObjects.h"
#include "PdfWriter.h"

namespace PoDoFo {

class PdfOutputDevice;
class PdfXRef;

class PODOFO_API PdfImmediateWriter : private PdfWriter, 
    private PdfVecObjects::Observer, 
    private PdfVecObjects::StreamFactory {

 public:
    PdfImmediateWriter( PdfOutputDevice* pDevice, PdfVecObjects* pVecObjects, const PdfObject* pTrailer, EPdfVersion eVersion = ePdfVersion_1_5 );
    ~PdfImmediateWriter();

 private:
    void WriteObject( const PdfObject* pObject );

    /** Called when the PdfVecObjects we observer is deleted.
     */
    void ParentDestructed();

    /** Finish the PDF file.
     *  I.e. write the XRef and close the output device.
     */
    void Finish();

    /** Creates a stream object
     *
     *  \param pParent parent object
     *
     *  \returns a new stream object 
     */
    PdfStream* CreateStream( PdfObject* pParent );

    /** Assume the stream for the last object has
     *  been written complete.
     *  Therefore close the stream of the object
     *  now so that the next object can be written
     *  to disk
     */
    void FinishLastObject();

 private:
    PdfVecObjects*   m_pParent;
    PdfOutputDevice* m_pDevice;

    PdfXRef*         m_pXRef;
    PdfObject*       m_pLast;

};

};

#endif /* _PDF_IMMEDIATE_WRITER_H_ */

