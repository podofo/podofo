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

#ifndef _PDF_PARSER_H_
#define _PDF_PARSER_H_

#include "PdfDefines.h"
#include "PdfParserBase.h"
#include "PdfVecObjects.h"

#define W_ARRAY_SIZE 3
#define W_MAX_BYTES  4

namespace PoDoFo {

typedef std::map<int,PdfObject*>    TMapObjects;
typedef TMapObjects::iterator       TIMapObjects;
typedef TMapObjects::const_iterator TCIMapObjects;

typedef std::map<int,TMapObjects>              TMapObjectStreamCache;
typedef TMapObjectStreamCache::iterator        TIMapObjectStreamCache;
typedef TMapObjectStreamCache::const_iterator  TCIMapObjectStreamCache;

/**
 * PdfParser reads a PDF file into memory. 
 * The file can be modified in memory and written back using
 * the PdfWriter class.
 * Most PDF features are supported
 */
class PdfParser : public PdfParserBase {
    friend class PdfWriter;

 public:
    /** Create a new PdfParser object
     */
    PdfParser();
    virtual ~PdfParser();

    /** Initialize the PdfParser object before you can use it.
     *  This function has to be called before you can access any other
     *  function.
     *  \param pszFilename filename of the file which is going to be parsed
     *  \param bLoadOnDemand If true all objects will be read from the file at
     *                       the time they are accesed first.
     *                       If false all objects will be read immediately.
     *                       This is faster if you do not need the complete PDF 
     *                       file in memory.
     *  \returns ErrOk on success
     */
    PdfError Init( const char* pszFilename, bool bLoadOnDemand = true );

    /** Get a reference to the sorted internal objects vector.
     *  \returns the internal objects vector.
     */
    inline const TVecObjects & GetObjects() const;

    /** Get the file format version of the pdf
     *  \returns the file format version as enum
     */
    inline EPdfVersion GetPdfVersion() const;

    /** Get the file format version of the pdf
     *  \returns the file format version as string
     */
    const char* GetPdfVersionString() const;

    /** Get the trailer dictionary
     *  which can be written unmodified to a pdf file.
     */
    inline const PdfObject* GetTrailer() const;

    /** \returns true if this PdfParser loads all objects on demand at
     *                the time they are accessed for the first time.
     *                The default is to load all object immediately.
     *                In this case false is returned.
     */
    inline bool LoadOnDemand() const;

    /** \returns whether the parsed document contains linearization tables
     */
    bool IsLinearized() const { return m_pLinearization != NULL; }

    /** \returns the length of the file
     */
    size_t FileSize() const { return m_nFileSize; }

 protected:
    /** Reads the xref sections and the trailers of the file
     *  in the correct order in the memory
     *  and takes care for linearized pdf files.
     */
    PdfError ReadDocumentStructure();

    /** Checks wether this pdf is linearized or not.
     *  Initializes the linearization directory on sucess.
     *  \returns ErrOk on success
     */
    PdfError HasLinearizationDict();

    /** Merge the information of this trailer object
     *  in the parsers main trailer object.
     *  \param pTrailer take the keys to merge from this dictionary.
     *  \returns ErrOk on success
     */
    PdfError MergeTrailer( const PdfObject* pTrailer );

    /** Read the trailer directory at the end of the file.
     */
    PdfError ReadTrailer();

    /** Looks for a startxref entry at the current file position
     *  and saves its byteoffset to pXRefOffset.
     *  \param pXRefOffset store the byte offset of the xref section into this variable.
     *  \returns ErrOk on sucess.
     */
    PdfError ReadXRef( long* pXRefOffset );

    /** Reads the xref table from a pdf file.
     *  If there is no xref table, ReadXRefStreamContents() is called.
     *  \param lOffset read the table from this offset
     *  \param bPositionAtEnd if true the xref table is not read, but the 
     *                        file stream is positioned directly 
     *                        after the table, which allows reading
     *                        a following trailer dictionary.
     *  \returns ErrOk on success
     */
    PdfError ReadXRefContents( long lOffset, bool bPositionAtEnd = false );

    /** Read a xref subsection
     *  \param nFirstObject object number of the first object
     *  \param nNumObjects  how many objects should be read from this section
     *  \returns ErrOk on success
     */
    PdfError ReadXRefSubsection( long & nFirstObject, long & nNumObjects );

    /** Reads a xref stream contens object
     *  \param lOffset read the stream from this offset
     *  \param bReadOnlyTrailer only the trailer is skipped over, the contents
     *         of the xref stream are not parsed
     *  \returns ErrOk on success
     */
    PdfError ReadXRefStreamContents( long lOffset, bool bReadOnlyTrailer );

    PdfError ReadXRefStreamEntry( char* pBuffer, long lLen, long lW[W_ARRAY_SIZE], int nObjNo );

    /** Reads all objects from the pdf into memory
     *  from the offsets listed in m_vecOffsets.
     *
     *  \returns ErrOk on success
     */
    PdfError ReadObjects();

    /** Read the object with index nIndex from the object stream nObjNo
     *  and push it on the objects vector m_vecOffsets.
     *
     *  \param nObjNo object number of the stream object
     *  \param nIndex index of the object which should be parsed
     *
     *  \returns ErrOk on success
     */
    PdfError ReadObjectFromStream( int nObjNo, int nIndex );

    /** Checks the magic number at the start of the pdf file
     *  and sets the m_ePdfVersion member to the correct version
     *  of the pdf file.
     *
     *  \returns true if this is a pdf file, otherwise false
     */
    bool    IsPdfFile();

    PdfError ReadNextTrailer();

 private:
    /** Free all internal data structures
     */
    void         Clear();

 private:
    EPdfVersion  m_ePdfVersion;

    bool         m_bLoadOnDemand;

    long         m_nXRefOffset;
    long         m_nFirstObject;
    long         m_nNumObjects;
    long         m_nXRefLinearizedOffset;
	size_t       m_nFileSize;

    TXRefEntry** m_ppOffsets;
    TVecObjects  m_vecObjects;

    PdfObject*   m_pTrailer;
    PdfObject*   m_pLinearization;

    TMapObjectStreamCache m_mapStreamCache;
};

bool PdfParser::LoadOnDemand() const
{
    return m_bLoadOnDemand;
}

EPdfVersion PdfParser::GetPdfVersion() const
{
    return m_ePdfVersion;
}

const TVecObjects & PdfParser::GetObjects() const
{
    return m_vecObjects;
}

const PdfObject* PdfParser::GetTrailer() const
{
    return m_pTrailer;
}

};

#endif // _PDF_PARSER_H_

