/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#ifndef _PDF_WRITER_H_
#define _PDF_WRITER_H_

#include "PdfDefines.h"
#include "PdfOutputDevice.h"
#include "PdfVecObjects.h"

namespace PoDoFo {

class PdfDictionary;
class PdfDocument;
class PdfName;
class PdfObject;
class PdfParser;
class PdfVecObjects;

struct TXRefTable{    
    unsigned int nFirst;
    unsigned int nCount;

    TVecOffsets  vecOffsets;    
};

typedef std::vector<TXRefTable>       TVecXRefTable;
typedef TVecXRefTable::iterator       TIVecXRefTable;
typedef TVecXRefTable::const_iterator TCIVecXRefTable;


/** The PdfWriter class writes a list of PdfObjects as PDF file.
 *  The XRef section (which is the required table of contents for any
 *  PDF file) is created automatically.
 *
 *  It does not know about pages but only about PdfObjects.
 *
 *  Most users will want to use PdfSimpleWriter.
 */
class PdfWriter {
 public:
    /** Create a PdfWriter object
     *  \see Init()
     */
    PdfWriter();
    virtual ~PdfWriter();

    /** Create a new pdf file, based on an existing pdf file.
     *  \param pParser     a pdf parser object
     */
    void Init( PdfParser* pParser );

    /** Create a new pdf file, based on an existing pdf file.
     *  \param pDocument a PdfDocument
     */
    void Init( PdfDocument* pDocument );

    /** Create a new pdf file, from an vector of PdfObjects
     *  and a trailer object.
     *  \param pVecObjects the vector of objects
     *  \param pTrailer a valid trailer object
     */
    void Init( PdfVecObjects* pVecObjects, const PdfObject* pTrailer );

    /** Create a new pdf file from scratch.
     */
    void Init();

    /** Writes the complete document to a PDF file.
     *
     *  \param pszFilename filename of a PDF file.
     */
    void Write( const char* pszFilename );

    /** Writes the complete document to a PdfOutputDevice
     *
     *  \param pDevice write to the specified device 
     */
    void Write( PdfOutputDevice* pDevice );

    /** Set the PDF Version of the document. Has to be called before Write() to
     *  have an effect.
     *  \param eVersion  version of the pdf document
     */
    void SetPdfVersion( EPdfVersion eVersion ) { m_eVersion = eVersion;}

    /** Get the PDF version of the document
     *  \returns EPdfVersion version of the pdf document
     */
    EPdfVersion GetPdfVersion() const { return m_eVersion; }

    /** Get the file format version of the pdf
     *  \returns the file format version as string
     */
    const char* GetPdfVersionString() const { return s_szPdfVersionNums[(int)m_eVersion]; }

    /** Set wether all streams in the pdf document should
     *  be compress using the FlateDecode algorithm.
     *  Only streams that are already JPEG compressed are not affected
     *  by this flag.
     *  By default all streams are compressed using FlateDecode.
     *  You can set this value to false if you want to disable
     *  compression, for example for debugging purposes.
     *
     *  \param bCompress enable/disable compression
     */
    void SetPdfCompression( bool bCompress ) { m_bCompress = bCompress; }

    /** Get wether PDF compression is enabled.
     *  \see SetPdfCompression
     *  \returns true if all streams will be compressed using
     *           the FlateDecode algorithm.
     */
    bool GetPdfCompression() const { return m_bCompress; }

    /** Calculate the byte offset of the object pObject in the PDF file
     *  if the file was written to disk at the moment of calling this function.
     *
     *  This function is very calculation intensive!
     *
     *  \param pObject object to calculate the byte offset (has to be a 
     *                 child of this PdfWriter)
     *  \param pulOffset pointer to an unsigned long to save the offset
     */
    void GetByteOffset( PdfObject* pObject, unsigned long* pulOffset );

    /** Make sure the all objects are flate decoded if the user enabled flate decoding
     *  This might be necessary to call before calculating the length of objects.
     *  It is called automatically before writing.
     *
     *  \param vecObjects compress all objects in this vector
     */
    void CompressObjects( const TVecObjects& vecObjects );

    /** Write the whole document to a buffer in memory.
     *  
     *  \param ppBuffer will be malloc'ed and the document 
     *         will be written to this buffer.
     *  \param pulLen the length of the buffer will be returned in this parameter
     *  \returns ErrOk on success
     *  
     *  \see Write
     */
    void WriteToBuffer( char** ppBuffer, unsigned long* pulLen );

 private:
    /** Delete all internal structures and free allocated memory.
     *  This function is called from the destructor and from
     *  Init(), so that calling Init() twice does not 
     *  cause a memory leak.
     */
    void Clear();

    /** Writes the pdf header to the current file.
     *  \param pDevice write to this output device
     */       
    void WritePdfHeader( PdfOutputDevice* pDevice );

    /** Write pdf objects to file
     *  \param pDevice write to this output device
     *  \param vecObjects write all objects in this vector to the file
     *  \param bFillXRefOnly only fille the m_vecXRef vector and do not write the objects
     */ 
    void WritePdfObjects( PdfOutputDevice* pDevice, const TVecObjects& vecObjects, bool bFillXRefOnly = false );

    /** Writes a list of xref entries to the current file
     *  \param pDevice write to this output device
     *  \param vecOffsets list of objects which will be written
     */
    void WriteXRefEntries( PdfOutputDevice* pDevice, const TVecOffsets & vecOffsets );

    /** Writes the xref table.
     *  \param pDevice write to this output device
     */
    void WritePdfTableOfContents( PdfOutputDevice* pDevice );

    /** Copies a key and value from a trailer PdfObject into a trailer dictionary that is immediately written
     *  to a PdfOutputDevice. The key is only copied if it exists in pTrailer
     * 
     *  \param pDevice write the key and value to this device
     *  \param pTrailer the source dictionary
     *  \param key the key to copy
     */
    void WriteTrailerKey( PdfOutputDevice* pDevice, const PdfObject* pTrailer, const PdfName & key );

 protected:
    PdfVecObjects*  m_vecObjects;

 private:
    EPdfVersion     m_eVersion;

    TVecXRefTable   m_vecXRef;
    PdfObject*      m_pTrailer;

    bool            m_bCompress;
};

};

#endif // _PDF_WRITER_H_
