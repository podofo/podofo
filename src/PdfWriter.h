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

class PdfParser;

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
     *  \param pszFilename file name of the new pdf file
     *  \param pParser     a pdf parser object
     *  \returns ErrOk on success.
     */
    PdfError Init( const char* pszFilename, PdfParser* pParser );

    /** Create a new pdf file from scratch.
     *  \param pszFilename file name of the new pdf file
     *  \returns ErrOk on success.
     */
    PdfError Init( const char* pszFilename );

    /** Writes the complete document to the pdf file specified 
     *  at Init()
     *
     *  \param pDevice write to the specified device instead of the file specified
     *                 to Init if pDevice is not NULL.
     *
     *  \returns ErrOk on success.
     */
    PdfError Write( PdfOutputDevice* pDevice = NULL );

    /** Set the PDF Version of the document. Has to be called before Write() to
     *  have an effect.
     *  \param eVersion  version of the pdf document
     */
    inline void SetPdfVersion( EPdfVersion eVersion );

    /** Get the PDF version of the document
     *  \returns EPdfVersion version of the pdf document
     */
    inline EPdfVersion GetPdfVersion() const;

    /** Creates a new object and inserts it into the internal
     *  object tree. The object is owned by the PdfWriter and 
     *  will be deleted if necessary.
     *  \param pszType optionall value of the /Type key of the object
     *  \param bInternal set to true to create an internal object which 
     *                  is written at the end. This is used only internally
     *                  in the PdfWriter.
     *  \returns PdfObject pointer to the new PdfObject
     */
    PdfObject* CreateObject( const char* pszType = NULL, bool bInternal = false );

    /** Create a PdfObject of type T which must be a subclasss of PdfObject
     *  and it does not need a parameter for pszType.
     *  \returns a new PdfObject subclasss
     */
    template <class T> T* CreateObject();

    /** Get access to the internal Catalog dictionary
     *  or root object.
     *  
     *  \returns PdfObject the documents catalog or NULL 
     *                     if no catalog is available
     */
    inline PdfObject* GetCatalog() const;
    
    /** Get access to the internal Info dictionary
     *  \returns PdfObject the info dictionary
     */
    inline PdfObject* GetInfo() const;

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
    inline void SetPdfCompression( bool bCompress );

    /** Get wether PDF compression is enabled.
     *  \see SetPdfCompression
     *  \returns true if all streams will be compressed using
     *           the FlateDecode algorithm.
     */
    inline bool GetPdfCompression() const;

    /** Get a reference to the sorted internal objects vector.
     *  \returns the internal objects vector.
     */
    inline const TVecObjects & GetObjects() const;

    /** Calculate the byte offset of the object pObject in the PDF file
     *  if the file was written to disk at the moment of calling this function.
     *
     *  This function is very calculation intensive!
     *
     *  \param pObject object to calculate the byte offset (has to be a 
     *                 child of this PdfWriter)
     *  \param pulOffset pointer to an unsigned long to save the offset
     *  \returns ErrOk on success
     */
    PdfError GetByteOffset( PdfObject* pObject, unsigned long* pulOffset );

    /** Make sure the all objects are flate decoded if the user enabled flate decoding
     *  This might be necessary to call before calculating the length of objects.
     *  It is called automatically before writing.
     *
     *  \param vecObjects compress all objects in this vector
     *  \returns ErrOk on success
     */
    PdfError CompressObjects( const TVecObjects& vecObjects );

    /** Write the whole document to a buffer in memory.
     *  
     *  \param ppBuffer will be malloc'ed and the document 
     *         will be written to this buffer.
     *  \param pulLen the length of the buffer will be returned in this parameter
     *  \returns ErrOk on success
     *  
     *  \see Write
     */
    PdfError WriteToBuffer( char** ppBuffer, unsigned long* pulLen );

 private:
    /** Writes the pdf header to the current file.
     *  \param pDevice write to this output device
     *
     *  \returns ErrOk on success
     */       
    PdfError WritePdfHeader( PdfOutputDevice* pDevice );

    /** Write pdf objects to file
     *  \param pDevice write to this output device
     *  \param vecObjects write all objects in this vector to the file
     *  \param bFillXRefOnly only fille the m_vecXRef vector and do not write the objects
     *
     *  \returns ErrOk on success
     */ 
    PdfError WritePdfObjects( PdfOutputDevice* pDevice, const TVecObjects& vecObjects, bool bFillXRefOnly = false );

    /** Writes a list of xref entries to the current file
     *  \param pDevice write to this output device
     *  \param vecOffsets list of objects which will be written
     *
     *  \returns ErrOk on success
     */
    PdfError WriteXRefEntries( PdfOutputDevice* pDevice, const TVecOffsets & vecOffsets );

    /** Writes the xref table.
     *  \param pDevice write to this output device
     *
     *  \returns ErrOk on success
     */
    PdfError WritePdfTableOfContents( PdfOutputDevice* pDevice );

 protected:
    unsigned int    m_nObjectCount;
    TVecObjects     m_vecObjects;

 private:
    PdfOutputDevice m_cDevice;
    EPdfVersion     m_eVersion;
    PdfParser*      m_pParser;

    TVecXRefTable   m_vecXRef;
    TVecObjects     m_vecInternalObjects;

    PdfObject*      m_pCatalog;
    PdfObject*      m_pInfo;

    bool            m_bCompress;
};

void PdfWriter::SetPdfVersion( EPdfVersion eVersion )
{
    m_eVersion = eVersion;
}

EPdfVersion PdfWriter::GetPdfVersion() const
{
    return m_eVersion;
}

PdfObject* PdfWriter::GetCatalog() const
{
    return m_pCatalog;
}
    
PdfObject* PdfWriter::GetInfo() const
{
    return m_pInfo;
}

void PdfWriter::SetPdfCompression( bool bCompress )
{
    m_bCompress = bCompress;
}

bool PdfWriter::GetPdfCompression() const
{
    return m_bCompress;
}

const TVecObjects & PdfWriter::GetObjects() const
{
    return m_vecObjects;
}

template <class T>
T* PdfWriter::CreateObject()
{
    T*         pTemplate = new T( m_nObjectCount++, 0 );
    PdfObject* pObject   = dynamic_cast<PdfObject*>(pTemplate);

    if( !pObject )
    {
        delete pTemplate;
        return NULL;
    }

    m_vecObjects.push_back( pObject );
    return pTemplate;
}


};

#endif // _PDF_WRITER_H_
