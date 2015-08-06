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

#ifndef _PDF_IMMEDIATE_WRITER_H_
#define _PDF_IMMEDIATE_WRITER_H_

#include "PdfDefines.h"
#include "PdfVecObjects.h"
#include "PdfWriter.h"

namespace PoDoFo {

class PdfEncrypt;
class PdfOutputDevice;
class PdfXRef;

/** A kind of PdfWriter that writes objects with streams immediately to
 *  a PdfOutputDevice
 */
class PODOFO_API PdfImmediateWriter : private PdfWriter, 
    private PdfVecObjects::Observer, 
    private PdfVecObjects::StreamFactory {

 public:
    /** Create a new PdfWriter that writes objects with streams immediately to a PdfOutputDevice
     *
     *  This has the advantage that large documents can be created without
     *  having to keep the whole document in memory.
     *
     *  @param pDevice all stream streams are immediately written to this output device
     *                 while the document is created.
     *  @param pVecObjects a vector of objects containing the objects which are written to disk
     *  @param pTrailer the trailer object
     *  @param eVersion the PDF version of the document to write.
     *                      The PDF version can only be set in the constructor
     *                      as it is the first item written to the document on disk.
     *  @param pEncrypt pointer to an encryption object or NULL. If not NULL
     *                  the PdfEncrypt object will be copied and used to encrypt the
     *                  created document.
     *  @param eWriteMode additional options for writing the pdf
     */
    PdfImmediateWriter( PdfOutputDevice* pDevice, PdfVecObjects* pVecObjects, const PdfObject* pTrailer, 
                        EPdfVersion eVersion = ePdfVersion_1_5, PdfEncrypt* pEncrypt = NULL,
                        EPdfWriteMode eWriteMode = ePdfWriteMode_Default );

    ~PdfImmediateWriter();

    /** Get the write mode used for wirting the PDF
     *  \returns the write mode
     */
    inline EPdfWriteMode GetWriteMode() const;

    /** Get the PDF version of the document
     *  The PDF version can only be set in the constructor
     *  as it is the first item written to the document on disk
     *
     *  \returns EPdfVersion version of the pdf document
     */
    inline EPdfVersion GetPdfVersion() const;

 private:
    void WriteObject( const PdfObject* pObject );

    /** Called when the PdfVecObjects we observer is deleted.
     */
    void ParentDestructed();

    /** Finish the PDF file.
     *  I.e. write the XRef and close the output device.
     */
    void Finish();

    /** Called whenever appending to a stream is started.
     *  \param pStream the stream object the user currently writes to.
     */
    void BeginAppendStream( const PdfStream* pStream );
    
    /** Called whenever appending to a stream has ended.
     *  \param pStream the stream object the user currently writes to.
     */
    void EndAppendStream( const PdfStream* pStream );

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

    bool             m_bOpenStream;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline EPdfWriteMode PdfImmediateWriter::GetWriteMode() const
{
    return PdfWriter::GetWriteMode();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline EPdfVersion PdfImmediateWriter::GetPdfVersion() const
{
    return PdfWriter::GetPdfVersion();
}

};

#endif /* _PDF_IMMEDIATE_WRITER_H_ */

