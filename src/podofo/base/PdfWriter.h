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

#ifndef _PDF_WRITER_H_
#define _PDF_WRITER_H_

#include "PdfDefines.h"
#include "PdfInputDevice.h"
#include "PdfOutputDevice.h"
#include "PdfVecObjects.h"

#include "PdfEncrypt.h"

namespace PoDoFo {

class PdfDictionary;
class PdfName;
class PdfObject;
class PdfPage;
class PdfPagesTree;
class PdfParser;
class PdfVecObjects;
class PdfXRef;

namespace NonPublic { class PdfHintStream; }

/** The PdfWriter class writes a list of PdfObjects as PDF file.
 *  The XRef section (which is the required table of contents for any
 *  PDF file) is created automatically.
 *
 *  It does not know about pages but only about PdfObjects.
 *
 *  Most users will want to use PdfDocument.
 */
class PODOFO_API PdfWriter {

 public:
    /** Create a PdfWriter object from a PdfParser object
     *  \param pParser a pdf parser object
     */
    PdfWriter( PdfParser* pParser );

    /** Create a new pdf file, from an vector of PdfObjects
     *  and a trailer object.
     *  \param pVecObjects the vector of objects
     *  \param pTrailer a valid trailer object
     */
    PdfWriter( PdfVecObjects* pVecObjects, const PdfObject* pTrailer );

    virtual ~PdfWriter();

    /** Writes the complete document to a PDF file.
     *
     *  \param pszFilename filename of a PDF file.
     */
    void Write( const char* pszFilename );

#ifdef _WIN32
    /** Writes the complete document to a PDF file.
     *
     *  \param pszFilename filename of a PDF file.
     *
     *  This is an overloaded member function to allow working
     *  with unicode characters. On Unix systems you can also path
     *  UTF-8 to the const char* overload.
     */
    void Write( const wchar_t* pszFilename );
#endif // _WIN32

    /** Writes the complete document to a PdfOutputDevice
     *
     *  \param pDevice write to the specified device
     *
     *  \see WriteUpdate
     */
    void Write( PdfOutputDevice* pDevice );

    /** Writes the document changes to an output device
     *
     *  \param pDevice write to the specified device
     *  \param pSourceInputDevice where to copy original content from; can be NULL
     *  \param bRewriteXRefTable whether to rewrite whole XRef table
     *
     *  Writes an incremental update to the pDevice, using the pSourceInputDevice
     *  content as the original file content.
     *  When the pSourceInputDevice is NULL, the caller is responsible to have the pDevice
     *  filled with the previous content and to have it positioned at the end of the stream.
     *
     *  Calling this also switches the writer to the incremental update mode.
     *
     *  \see SetIncrementalUpdate
     */
    void WriteUpdate( PdfOutputDevice* pDevice, PdfInputDevice* pSourceInputDevice, bool bRewriteXRefTable );

    /** Set the write mode to use when writing the PDF.
     *  \param eWriteMode write mode
     */
    void SetWriteMode( EPdfWriteMode eWriteMode ) { m_eWriteMode = eWriteMode; }

    /** Get the write mode used for wirting the PDF
     *  \returns the write mode
     */
    EPdfWriteMode GetWriteMode() const { return m_eWriteMode; }

    /** Set the PDF Version of the document. Has to be called before Write() to
     *  have an effect.
     *  \param eVersion  version of the pdf document
     */
    void SetPdfVersion( EPdfVersion eVersion ) { m_eVersion = eVersion; }

    /** Get the PDF version of the document
     *  \returns EPdfVersion version of the pdf document
     */
    EPdfVersion GetPdfVersion() const { return m_eVersion; }

    /** Enabled linearization for this document.
     *  I.e. optimize it for web usage. Default is false.
     *  \param bLinearize if true create a web optimized PDF file
     */
    inline void SetLinearized( bool bLinearize );

    /**
     *  \returns true if this PDF file is web optimized.
     */
    inline bool GetLinearized() const;

    /** Create a XRef stream which is in some case
     *  more compact but requires at least PDF 1.5
     *  Default is false.
     *  \param bStream if true a XRef stream object will be created
     */
    inline void SetUseXRefStream( bool bStream );

    /** 
     *  \returns wether a XRef stream is used or not
     */
    inline bool GetUseXRefStream() const;

    /** Sets an offset to the previous XRef table. Set it to lower than
     *  or equal to 0, to not write a reference to the previous XRef table.
     *  The default is 0.
     *  \param lPrevXRefOffset the previous XRef table offset
     */
    inline void SetPrevXRefOffset( pdf_int64 lPrevXRefOffset );

    /** 
     *  \returns offset to the previous XRef table, as previously set
     *     by SetPrevXRefOffset.
     *
     * \see SetPrevXRefOffset
     */
    inline pdf_int64 GetPrevXRefOffset() const;

    /** Set whether writing an incremental update.
     *  Default is false.
     *  \param bIncrementalUpdate if true an incremental update will be written
     */
    inline void SetIncrementalUpdate( bool bIncrementalUpdate );

    /** 
     *  \returns whether writing an incremental update
     */
    inline bool GetIncrementalUpdate( void ) const;

    /** Get the file format version of the pdf
     *  \returns the file format version as string
     */
    const char* GetPdfVersionString() const { return s_szPdfVersionNums[static_cast<int>(m_eVersion)]; }
    
    /** Set the written document to be encrypted using a PdfEncrypt object
     *
     *  \param rEncrypt an encryption object which is used to encrypt the written PDF file
     */
    void SetEncrypted( const PdfEncrypt & rEncrypt );

    /** 
     * \returns true if this PdfWriter creates an encrypted PDF file
     */
    bool GetEncrypted() const { return (m_pEncrypt != NULL); }

    /** Calculate the byte offset of the object pObject in the PDF file
     *  if the file was written to disk at the moment of calling this function.
     *
     *  This function is very calculation intensive!
     *
     *  \param pObject object to calculate the byte offset (has to be a 
     *                 child of this PdfWriter)
     *  \param pulOffset pointer to an unsigned long to save the offset
     */
    void GetByteOffset( PdfObject* pObject, pdf_long* pulOffset );

    /** Write the whole document to a buffer in memory.
     *  
     *  Better use a PdfOutputDevice that writes to a PdfRefCountedBuffer.
     *
     *  \param ppBuffer where this points the address of the buffer will
     *         be written after that being malloc()'d and the document
     *         will be written to that buffer.
     *  \param pulLen the buffer length will be returned in this parameter
     *
     *  \see Write
     */
    void WriteToBuffer( char** ppBuffer, pdf_long* pulLen );

    /** Add required keys to a trailer object
     *  \param pTrailer add keys to this object
     *  \param lSize number of objects in the PDF file
     *  \param bOnlySizeKey write only the size key
     */
    void FillTrailerObject( PdfObject* pTrailer, pdf_long lSize, bool bOnlySizeKey ) const;

 protected:
    /**
     * Create a PdfWriter from a PdfVecObjects
     */
    PdfWriter( PdfVecObjects* pVecObjects );

    /** Writes the pdf header to the current file.
     *  \param pDevice write to this output device
     */       
    void PODOFO_LOCAL WritePdfHeader( PdfOutputDevice* pDevice );

    /** Write pdf objects to file
     *  \param pDevice write to this output device
     *  \param vecObjects write all objects in this vector to the file
     *  \param pXref add all written objects to this XRefTable
     *  \param bRewriteXRefTable whether will rewrite whole XRef table (used only if GetIncrementalUpdate() returns true)
     */ 
    void WritePdfObjects( PdfOutputDevice* pDevice, const PdfVecObjects& vecObjects, PdfXRef* pXref, bool bRewriteXRefTable = false ) PODOFO_LOCAL;

    /** Creates a file identifier which is required in several
     *  PDF workflows. 
     *  All values from the files document information dictionary are
     *  used to create a unique MD5 key which is added to the trailer dictionary.
     *
     *  \param identifier write the identifier to this string
     *  \param pTrailer trailer object
     *  \param pOriginalIdentifier write the original identifier (when using incremental update) to this string
     */
    void CreateFileIdentifier( PdfString& identifier, const PdfObject* pTrailer, PdfString* pOriginalIdentifier = NULL ) const PODOFO_LOCAL;

    /** Internal implementation of the Write() call with the common code
     *  \param pDevice write to this output device
     *  \param bRewriteXRefTable whether will rewrite whole XRef table (used only if GetIncrementalUpdate() returns true)
     */ 
    void Write( PdfOutputDevice* pDevice, bool bRewriteXRefTable );

 protected:
    /** Writes a linearized PDF file
     *  \param pDevice write to this output device
     */       
    void PODOFO_LOCAL WriteLinearized( PdfOutputDevice* pDevice );

    /** Create a linearization dictionary for the current
     *  document and return a pointer to it after inserting
     *  it into the vector of PdfObjects
     *
     *  \returns a pointer to the linearization dictionary
     */
    PdfObject* CreateLinearizationDictionary() PODOFO_LOCAL;

    /** Reorder and renumber all object as required for linearized PDF files.
     *  This function is very slow.
     *
     *  \param pLinearize linearization dictionary
     *  \param pHint primary hint stream dictionary
     *  \param pPage first page to display in the document
     *  \param ppLast the pointer will be initialized to the last object
     *         belonging to the first page
     */
    //void ReorderObjectsLinearized( PdfObject* pLinearize, NonPublic::PdfHintStream* pHint, PdfPage* pPage, PdfObject** ppLast ) PODOFO_LOCAL;

    /** Initialize m_pPagesTree with its correct value
     *  Always call this function befre accessing the pages tree.
     */
    //void FetchPagesTree();

    /** Find dependencies required for creating a linearized PDF of the catalog dictionary.
     */
    //void FindCatalogDependencies( PdfObject* pCatalog, const PdfName & rName, TPdfReferenceList* pList, bool bWithDependencies ) PODOFO_LOCAL;

    /** Fill all keys in the linearization dictionary with their values
     *  \param pLinearize a linearization dictionary
     *  \param pHint the hint stream
     *  \param pPage the first page in the linerarized PDF file
     *  \param pLast pointer of the last object belonging to the first page
     *  \param pVecXRefOffset xref table entries for previous entry
     */
    // void FillLinearizationDictionary( PdfObject* pLinearize, PdfOutputDevice* pDevice, PdfPage* pPage, PdfObject* pLast, NonPublic::PdfHintStream* pHint, TVecXRefOffset* pVecXRefOffset ) PODOFO_LOCAL;

 protected:
    PdfVecObjects*  m_vecObjects;
    PdfObject*      m_pTrailer;

    bool            m_bXRefStream;

    PdfEncrypt*     m_pEncrypt;    ///< If not NULL encrypt all strings and streams and create an encryption dictionary in the trailer
    PdfObject*      m_pEncryptObj; ///< Used to temporarly store the encryption dictionary

    PdfString       m_identifier;
    PdfString       m_originalIdentifier; // used for incremental update

 private:
    EPdfWriteMode   m_eWriteMode;
    EPdfVersion     m_eVersion;
    pdf_int64       m_lPrevXRefOffset;
    bool            m_bIncrementalUpdate;

    bool            m_bLinearized;
 
    /**
     * This value is required when writing
     * a linearized PDF file.
     * It represents the offset of the whitespace
     * character before the first line in the XRef
     * section.
     */
    size_t            m_lFirstInXRef;
    size_t            m_lLinearizedOffset;
    size_t            m_lLinearizedLastOffset;
    size_t            m_lTrailerOffset;
    PdfVecObjects   m_vecLinearized;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfWriter::SetLinearized( bool bLinearize )
{
    m_bLinearized = bLinearize;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfWriter::GetLinearized() const
{
    return m_bLinearized;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfWriter::SetUseXRefStream( bool bStream )
{
    if( bStream && this->GetPdfVersion() < ePdfVersion_1_5 )
        this->SetPdfVersion( ePdfVersion_1_5 );
    m_bXRefStream = bStream;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfWriter::GetUseXRefStream() const
{
    return m_bXRefStream;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfWriter::SetPrevXRefOffset( pdf_int64 lPrevXRefOffset )
{
    m_lPrevXRefOffset = lPrevXRefOffset;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
pdf_int64 PdfWriter::GetPrevXRefOffset() const
{
    return m_lPrevXRefOffset;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfWriter::SetIncrementalUpdate( bool bIncrementalUpdate )
{
    m_bIncrementalUpdate = bIncrementalUpdate;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfWriter::GetIncrementalUpdate( void ) const
{
    return m_bIncrementalUpdate;
}

};

#endif // _PDF_WRITER_H_
