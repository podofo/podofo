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

#ifndef _PDF_PARSER_H_
#define _PDF_PARSER_H_

#include "PdfDefines.h"
#include "PdfTokenizer.h"
#include "PdfVecObjects.h"

#define W_ARRAY_SIZE 3
#define W_MAX_BYTES  4

namespace PoDoFo {

typedef std::map<int,PdfObject*>    TMapObjects;
typedef TMapObjects::iterator       TIMapObjects;
typedef TMapObjects::const_iterator TCIMapObjects;

class PdfEncrypt;
class PdfString;

/**
 * PdfParser reads a PDF file into memory. 
 * The file can be modified in memory and written back using
 * the PdfWriter class.
 * Most PDF features are supported
 */
class PODOFO_API PdfParser : public PdfTokenizer {
    friend class PdfDocument;
    friend class PdfWriter;

 public:
    struct TXRefEntry {
        inline TXRefEntry() : lOffset(0), lGeneration(0), cUsed('\x00'), bParsed(false) { }
        pdf_long lOffset;
        long lGeneration;
        char cUsed;
        bool bParsed;
    };

    typedef std::vector<TXRefEntry>      TVecOffsets;
    typedef TVecOffsets::iterator        TIVecOffsets;
    typedef TVecOffsets::const_iterator  TCIVecOffsets;

    /** Create a new PdfParser object
     *  You have to open a PDF file using ParseFile later.
     *  \param pVecObjects vector to write the parsed PdfObjects to
     *
     *  \see ParseFile  
     */
    PdfParser( PdfVecObjects* pVecObjects );

    /** Create a new PdfParser object and open a PDF file and parse
     *  it into memory.
     *
     *  \param pVecObjects vector to write the parsed PdfObjects to
     *  \param pszFilename filename of the file which is going to be parsed
     *  \param bLoadOnDemand If true all objects will be read from the file at
     *                       the time they are accesed first.
     *                       If false all objects will be read immediately.
     *                       This is faster if you do not need the complete PDF 
     *                       file in memory.
     *
     *  This might throw a PdfError( ePdfError_InvalidPassword ) exception
     *  if a password is required to read this PDF.
     *  Call SetPassword with the correct password in this case.
     *  
     *  \see SetPassword
     */
    PdfParser( PdfVecObjects* pVecObjects, const char* pszFilename, bool bLoadOnDemand = true );

#ifdef _WIN32
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200    // not for MS Visual Studio 6
#else
    /** Create a new PdfParser object and open a PDF file and parse
     *  it into memory.
     *
     *  \param pVecObjects vector to write the parsed PdfObjects to
     *  \param pszFilename filename of the file which is going to be parsed
     *  \param bLoadOnDemand If true all objects will be read from the file at
     *                       the time they are accesed first.
     *                       If false all objects will be read immediately.
     *                       This is faster if you do not need the complete PDF 
     *                       file in memory.
     *
     *  This might throw a PdfError( ePdfError_InvalidPassword ) exception
     *  if a password is required to read this PDF.
     *  Call SetPassword with the correct password in this case.
     *  
     *  This is an overloaded member function to allow working
     *  with unicode characters. On Unix systes you can also path
     *  UTF-8 to the const char* overload.
     *
     *  \see SetPassword
     */
    PdfParser( PdfVecObjects* pVecObjects, const wchar_t* pszFilename, bool bLoadOnDemand = true );
#endif
#endif // _WIN32

    /** Create a new PdfParser object and open a PDF file and parse
     *  it into memory.
     *
     *  \param pVecObjects vector to write the parsed PdfObjects to
     *  \param pBuffer buffer containing a PDF file in memory
     *  \param lLen length of the buffer containing the PDF file
     *  \param bLoadOnDemand If true all objects will be read from the file at
     *                       the time they are accesed first.
     *                       If false all objects will be read immediately.
     *                       This is faster if you do not need the complete PDF 
     *                       file in memory.
     *
     *  This might throw a PdfError( ePdfError_InvalidPassword ) exception
     *  if a password is required to read this PDF.
     *  Call SetPassword with the correct password in this case.
     *  
     *  \see SetPassword
     */
    PdfParser( PdfVecObjects* pVecObjects, const char* pBuffer, long lLen, bool bLoadOnDemand = true );

    /** Create a new PdfParser object and open a PDF file and parse
     *  it into memory.
     *
     *  \param pVecObjects vector to write the parsed PdfObjects to
     *  \param rDevice read from this PdfRefCountedInputDevice
     *  \param bLoadOnDemand If true all objects will be read from the file at
     *                       the time they are accesed first.
     *                       If false all objects will be read immediately.
     *                       This is faster if you do not need the complete PDF 
     *                       file in memory.
     *
     *  This might throw a PdfError( ePdfError_InvalidPassword ) exception
     *  if a password is required to read this PDF.
     *  Call SetPassword with the correct password in this case.
     *  
     *  \see SetPassword
     */
    PdfParser( PdfVecObjects* pVecObjects, const PdfRefCountedInputDevice & rDevice, 
               bool bLoadOnDemand = true );

    /** Delete the PdfParser and all PdfObjects
     */
    virtual ~PdfParser();

    /** Open a PDF file and parse it.
     *
     *  \param pszFilename filename of the file which is going to be parsed
     *  \param bLoadOnDemand If true all objects will be read from the file at
     *                       the time they are accesed first.
     *                       If false all objects will be read immediately.
     *                       This is faster if you do not need the complete PDF 
     *                       file in memory.
     *
     *
     *  This might throw a PdfError( ePdfError_InvalidPassword ) exception
     *  if a password is required to read this PDF.
     *  Call SetPassword with the correct password in this case.
     *  
     *  \see SetPassword
     */
    void ParseFile( const char* pszFilename, bool bLoadOnDemand = true );

#ifdef _WIN32
    /** Open a PDF file and parse it.
     *
     *  \param pszFilename filename of the file which is going to be parsed
     *  \param bLoadOnDemand If true all objects will be read from the file at
     *                       the time they are accesed first.
     *                       If false all objects will be read immediately.
     *                       This is faster if you do not need the complete PDF 
     *                       file in memory.
     *
     *
     *  This might throw a PdfError( ePdfError_InvalidPassword ) exception
     *  if a password is required to read this PDF.
     *  Call SetPassword with the correct password in this case.
     *  
     *  This is an overloaded member function to allow working
     *  with unicode characters. On Unix systes you can also path
     *  UTF-8 to the const char* overload.
     *
     *  \see SetPassword
     */
    void ParseFile( const wchar_t* pszFilename, bool bLoadOnDemand = true );
#endif // _WIN32

    /** Open a PDF file and parse it.
     *
     *  \param pBuffer buffer containing a PDF file in memory
     *  \param lLen length of the buffer containing the PDF file
     *  \param bLoadOnDemand If true all objects will be read from the file at
     *                       the time they are accesed first.
     *                       If false all objects will be read immediately.
     *                       This is faster if you do not need the complete PDF 
     *                       file in memory.
     *
     *
     *  This might throw a PdfError( ePdfError_InvalidPassword ) exception
     *  if a password is required to read this PDF.
     *  Call SetPassword with the correct password in this case.
     *  
     *  \see SetPassword
     */
    void ParseFile( const char* pBuffer, long lLen, bool bLoadOnDemand = true );

    /** Open a PDF file and parse it.
     *
     *  \param rDevice the input device to read from
     *  \param bLoadOnDemand If true all objects will be read from the file at
     *                       the time they are accesed first.
     *                       If false all objects will be read immediately.
     *                       This is faster if you do not need the complete PDF 
     *                       file in memory.
     *
     *
     *  This might throw a PdfError( ePdfError_InvalidPassword ) exception
     *  if a password is required to read this PDF.
     *  Call SetPassword with the correct password in this case.
     *  
     *  \see SetPassword
     */
    void ParseFile( const PdfRefCountedInputDevice & rDevice, bool bLoadOnDemand = true );

    /** Quick method to detect secured PDF files, i.e.
     *  a PDF with an /Encrypt key in the trailer directory.
     *
     *  \returns true if document is secured, false otherwise
     */
    bool QuickEncryptedCheck( const char* pszFilename );

    /**
     * Retrieve the number of incremental updates that 
     * have been applied to the last parsed PDF file.
     *
     * 0 means no update has been applied.
     *
     * \returns the number of incremental updates to the parsed PDF.
     */
    inline int GetNumberOfIncrementalUpdates() const;

    /** Get a reference to the sorted internal objects vector.
     *  \returns the internal objects vector.
     */
    inline const PdfVecObjects* GetObjects() const;

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
    inline bool GetLoadOnDemand() const;

    /** \returns whether the parsed document contains linearization tables
     */
    bool IsLinearized() const { return m_pLinearization != NULL; }

    /** \returns the length of the file
     */
    size_t GetFileSize() const { return m_nFileSize; }

    /** 
     * \returns true if this PdfWriter creates an encrypted PDF file
     */
    bool GetEncrypted() const { return (m_pEncrypt != NULL); }

    /** 
     * \returns the parsers encryption object or NULL if the read PDF file was not encrypted
     */
    const PdfEncrypt* GetEncrypt() const { return m_pEncrypt; }

    /** 
     * Takes the encryption object fro mthe parser. The internal handle will be set
     * to NULL and the ownership of the object is given to the caller.
     *
     * Only call this if you need access to the encryption object
     * before deleting the parser.
     *
     * \returns the parsers encryption object or NULL if the read PDF file was not encrypted
     */
    inline PdfEncrypt* TakeEncrypt();
    

    /** If you try to open an encrypted PDF file, which requires
     *  a password to open, PoDoFo will throw a PdfError( ePdfError_InvalidPassword ) 
     *  exception. 
     *  
     *  If you got such an exception, you have to set a password
     *  which should be used for opening the PDF.
     *
     *  The usual way will be to ask the user for the password
     *  and set the password using this method.
     *
     *  PdfParser will immediately continue to read the PDF file.
     *
     *  \param sPassword a user or owner password which can be used to open an encrypted PDF file
     *                   If the password is invalid, a PdfError( ePdfError_InvalidPassword ) exception is thrown!
     */
    void SetPassword( const std::string & sPassword );

    /**
     * \returns true if strict parsing mode is enabled
     *
     * \see SetStringParsing
     */
    inline bool IsStrictParsing() const;

    /**
     * Enable/disable strict parsing mode.
     * Strict parsing is by default disabled.
     *
     * If you enable strict parsing, PoDoFo will fail
     * on a few more common PDF failures. Please not 
     * that PoDoFo's parser is by default very strict
     * already and does not recover from e.g. wrong XREF
     * tables.
     *
     * \param bStrict new setting for strict parsing mode.
     */
    inline void SetStrictParsing( bool bStrict );

    /**
     * \return if broken objects are ignored while parsing
     */
    inline bool GetIgnoreBrokenObjects();

    /**
     * Specify if the parser should ignore broken
     * objects, i.e. XRef entries that do not point
     * to valid objects.
     *
     * Default is to not ignore broken objects and
     * throw an exception if one is found.
     *
     * \param bBroken if true broken objects will be ignored
     */
    inline void SetIgnoreBrokenObjects( bool bBroken );

    /**
     * \return maximum object count to read (default is LONG_MAX
     * which means no limit)
     */
    inline static long GetMaxObjectCount();
    
    /**
     * Specify the maximum number of objects the parser should
     * read. An exception is thrown if document contains more
     * objects than this. Use to avoid problems with very large
     * documents with millions of objects, which use 500MB of
     * working set and spend 15 mins in Load() before throwing
     * an out of memory exception.
     *
     * \param nMaxObjects set max number of objects
     */
    inline static void SetMaxObjectCount( long nMaxObjects );

    inline pdf_long GetXRefOffset(void);
    
    bool HasXRefStream();


 protected:
    /** Searches backwards from the end of the file
     *  and tries to find a token.
     *  The current file is positioned right after the token.
     * 
     *  \param pszToken a token to find
     *  \param lRange range in bytes in which to search
     *                begining at the end of the file
     */
    void FindToken( const char* pszToken, const long lRange );

    // Peter Petrov 23 December 2008
    /** Searches backwards from the specified position of the file
     *  and tries to find a token.
     *  The current file is positioned right after the token.
     * 
     *  \param pszToken a token to find
     *  \param lRange range in bytes in which to search
     *                begining at the specified position of the file
     *  \param searchEnd specifies position 
     */
    void FindToken2( const char* pszToken, const long lRange, size_t searchEnd );

    /** Reads the xref sections and the trailers of the file
     *  in the correct order in the memory
     *  and takes care for linearized pdf files.
     */
    void ReadDocumentStructure();

    /** Checks wether this pdf is linearized or not.
     *  Initializes the linearization directory on sucess.
     */
    void HasLinearizationDict();

    /** Merge the information of this trailer object
     *  in the parsers main trailer object.
     *  \param pTrailer take the keys to merge from this dictionary.
     */
    void MergeTrailer( const PdfObject* pTrailer );

    /** Read the trailer directory at the end of the file.
     */
    void ReadTrailer();

    /** Looks for a startxref entry at the current file position
     *  and saves its byteoffset to pXRefOffset.
     *  \param pXRefOffset store the byte offset of the xref section into this variable.
     */
    void ReadXRef( pdf_long* pXRefOffset );

    /** Reads the xref table from a pdf file.
     *  If there is no xref table, ReadXRefStreamContents() is called.
     *  \param lOffset read the table from this offset
     *  \param bPositionAtEnd if true the xref table is not read, but the 
     *                        file stream is positioned directly 
     *                        after the table, which allows reading
     *                        a following trailer dictionary.
     */
    void ReadXRefContents( pdf_long lOffset, bool bPositionAtEnd = false );

    /** Read a xref subsection
     *  
     *  Throws ePdfError_NoXref if the number of objects read was not
     *  the number specified by the subsection header (as passed in
     *  `nNumObjects').
     *
     *  \param nFirstObject object number of the first object
     *  \param nNumObjects  how many objects should be read from this section
     */
    void ReadXRefSubsection( pdf_int64 & nFirstObject, pdf_int64 & nNumObjects );

    /** Reads a xref stream contens object
     *  \param lOffset read the stream from this offset
     *  \param bReadOnlyTrailer only the trailer is skipped over, the contents
     *         of the xref stream are not parsed
     */
    void ReadXRefStreamContents( pdf_long lOffset, bool bReadOnlyTrailer );

    /** Reads all objects from the pdf into memory
     *  from the offsets listed in m_vecOffsets.
     *
     *  If required an encryption object is setup first.
     *
     *  The actual reading happens in ReadObjectsInternal()
     *  either if no encryption is required or a correct
     *  encryption object was initialized from SetPassword.
     */
    void ReadObjects();

    /** Reads all objects from the pdf into memory
     *  from the offsets listed in m_vecOffsets.
     *
     *  Requires a correctly setup PdfEncrypt object
     *  with correct password.
     *
     *  This method is called from ReadObjects
     *  or SetPassword.
     *
     *  \see ReadObjects
     *  \see SetPassword
     */
    void ReadObjectsInternal();

    /** Read the object with index nIndex from the object stream nObjNo
     *  and push it on the objects vector m_vecOffsets.
     *
     *  All objects are read from this stream and the stream object
     *  is free'd from memory. Further calls who try to read from the
     *  same stream simply do nothing.
     *
     *  \param nObjNo object number of the stream object
     *  \param nIndex index of the object which should be parsed
     *
     */
    void ReadObjectFromStream( int nObjNo, int nIndex );

    /** Checks the magic number at the start of the pdf file
     *  and sets the m_ePdfVersion member to the correct version
     *  of the pdf file.
     *
     *  \returns true if this is a pdf file, otherwise false
     */
    bool    IsPdfFile();

    void ReadNextTrailer();


    /** Checks for the existence of the %%EOF marker at the end of the file
     *  When strict mode is off it will also attempt to setup the parser to ignore
     *  any garbage after the last %%EOF marker
     *  Simply raises an error if there is a problem with the marker
     *
     */
    void CheckEOFMarker();

 private:
    /** Free all internal data structures
     */
    void         Clear();

    /** Initializes all private members
     *  with their initial values.
     */
    void         Init();

    /** Small helper method to retrieve the document id from the trailer
     *
     *  \returns the document id of this PDF document
     */
    const PdfString & GetDocumentId();

    /** Determines the correct version of the PDF
     *  from the document catalog (if available).
     *  as, PDF > 1.4 allows updating the version.
     *
     *  If no catalog dictionary is present or no /Version
     *  key is available, the version from the file header will
     *  be used.
     */
    void         UpdateDocumentVersion();

 private:
    EPdfVersion   m_ePdfVersion;

    bool          m_bLoadOnDemand;

    pdf_long      m_nXRefOffset;
    long          m_nFirstObject;
    long          m_nNumObjects;
    pdf_long      m_nXRefLinearizedOffset;
    size_t        m_nFileSize;
    pdf_long      m_lLastEOFOffset;

    TVecOffsets   m_offsets;
    PdfVecObjects* m_vecObjects;

    PdfObject*    m_pTrailer;
    PdfObject*    m_pLinearization;
    PdfEncrypt*   m_pEncrypt;

    bool          m_xrefSizeUnknown;

    std::set<int> m_setObjectStreams;

    bool          m_bStrictParsing;
    bool          m_bIgnoreBrokenObjects;

    int           m_nIncrementalUpdates;
    int           m_nReadNextTrailerLevel;

    static long   s_nMaxObjects;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfParser::GetLoadOnDemand() const
{
    return m_bLoadOnDemand;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfVersion PdfParser::GetPdfVersion() const
{
    return m_ePdfVersion;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
int PdfParser::GetNumberOfIncrementalUpdates() const
{
    return m_nIncrementalUpdates;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfVecObjects* PdfParser::GetObjects() const
{
    return m_vecObjects;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfObject* PdfParser::GetTrailer() const
{
    return m_pTrailer;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfEncrypt* PdfParser::TakeEncrypt() 
{ 
    PdfEncrypt* pEncrypt = m_pEncrypt;
    m_pEncrypt = NULL; 
    return pEncrypt; 
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfParser::IsStrictParsing() const
{
    return m_bStrictParsing;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfParser::SetStrictParsing( bool bStrict )
{
    m_bStrictParsing = bStrict;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfParser::GetIgnoreBrokenObjects()
{
    return m_bIgnoreBrokenObjects;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfParser::SetIgnoreBrokenObjects( bool bBroken )
{
    m_bIgnoreBrokenObjects = bBroken;
}

// -----------------------------------------------------
//
// -----------------------------------------------------
long PdfParser::GetMaxObjectCount()
{
    return PdfParser::s_nMaxObjects;
}

// -----------------------------------------------------
//
// -----------------------------------------------------
void PdfParser::SetMaxObjectCount( long nMaxObjects )
{
    PdfParser::s_nMaxObjects = nMaxObjects;
}

// -----------------------------------------------------
//
// -----------------------------------------------------
pdf_long PdfParser::GetXRefOffset()
{
    return m_nXRefOffset;
}

};

#endif // _PDF_PARSER_H_

