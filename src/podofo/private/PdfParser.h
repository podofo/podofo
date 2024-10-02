/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_PARSER_H
#define PDF_PARSER_H

#include <podofo/main/PdfIndirectObjectList.h>
#include <podofo/main/PdfTokenizer.h>

#include "PdfParserObject.h"
#include "PdfXRefEntry.h"

namespace PoDoFo {

class PdfEncrypt;

/**
 * PdfParser reads a PDF file into memory.
 * The file can be modified in memory and written back using
 * the PdfWriter class.
 * Most PDF features are supported
 */
class PdfParser
{
    friend class PdfParserTest;
    friend class PdfDocument;
    friend class PdfWriter;

public:
    /** Create a new PdfParser object
     *  You have to open a PDF file using ParseFile later.
     *  \param objects vector to write the parsed PdfObjects to
     *
     *  \see ParseFile
     */
    PdfParser(PdfIndirectObjectList& objects);

    /** Open a PDF file and parse it.
     *
     *  \param device the input device to read from
     *  \param loadOnDemand If true all objects will be read from the file at
     *                       the time they are accessed first.
     *                       If false all objects will be read immediately.
     *                       This is faster if you do not need the complete PDF
     *                       file in memory.
     *
     *
     *  This might throw a PdfError( PdfErrorCode::InvalidPassword ) exception
     *  if a password is required to read this PDF.
     *  Call SetPassword() with the correct password in this case.
     *
     *  \see SetPassword
     */
    void Parse(InputStreamDevice& device, bool loadOnDemand);

    const PdfObject& GetTrailer() const;

    std::unique_ptr<PdfObject> TakeTrailer();

    /**
     * Try retrieve the previous revision offset of the document before signing
     * \param currOffset the current offset where to start the search
     * \param eofOffset the previous revision EOF offset
     */
    static bool TryGetPreviousRevisionOffset(InputStreamDevice& input, size_t currOffset, size_t& eofOffset);

public:
    /** If you try to open an encrypted PDF file, which requires
     *  a password to open, PoDoFo will throw a PdfError( PdfErrorCode::InvalidPassword )
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
     *  \param password a user or owner password which can be used to open an encrypted PDF file
     *                   If the password is invalid, a PdfError( PdfErrorCode::InvalidPassword ) exception is thrown!
     */
    inline void SetPassword(const std::string_view& password) { m_Password = password; }
    inline const std::string& GetPassword() { return m_Password; }

    /**
     * Retrieve the number of incremental updates that
     * have been applied to the last parsed PDF file.
     *
     * 0 means no update has been applied.
     *
     * \returns the number of incremental updates to the parsed PDF.
     */
    inline int GetIncrementalUpdatesCount() const { return m_IncrementalUpdateCount; }

    /** Get a reference to the sorted internal objects vector.
     *  \returns the internal objects vector.
     */
    inline const PdfIndirectObjectList* GetObjects() const { return m_Objects; }

    /** Get the file format version of the pdf
     *  \returns the file format version as enum
     */
    inline PdfVersion GetPdfVersion() const { return m_PdfVersion; }

    /** \returns true if this PdfParser loads all objects on demand at
     *                the time they are accessed first.
     *                The default is to load all object immediately.
     *                In this case false is returned.
     */
    inline bool GetLoadOnDemand() const { return m_LoadOnDemand; }

    /** \returns the length of the file
     */
    inline size_t GetFileSize() const { return m_FileSize; }

    /**
     * \returns true if strict parsing mode is enabled
     *
     * \see SetStringParsing
     */
    inline bool IsStrictParsing() const { return m_StrictParsing; }

    /**
     * Enable/disable strict parsing mode.
     * Strict parsing is by default disabled.
     *
     * If you enable strict parsing, PoDoFo will fail
     * on a few more common PDF failures. Please note
     * that PoDoFo's parser is by default very strict
     * already and does not recover from e.g. wrong XREF
     * tables.
     *
     * \param strict new setting for strict parsing mode.
     */
    inline void SetStrictParsing(bool strict) { m_StrictParsing = strict; }

    /**
     * \return if broken objects are ignored while parsing
     */
    inline bool GetIgnoreBrokenObjects() const { return m_IgnoreBrokenObjects; }

    /**
     * Specify if the parser should ignore broken
     * objects, i.e. XRef entries that do not point
     * to valid objects.
     *
     * Default is to ignore broken objects and
     * to not throw an exception if one is found.
     *
     * \param bBroken if true broken objects will be ignored
     */
    inline void SetIgnoreBrokenObjects(bool broken) { m_IgnoreBrokenObjects = broken; }

    inline size_t GetXRefOffset() const { return m_XRefOffset; }

    inline bool HasXRefStream() const { return m_HasXRefStream; }

    const PdfEncryptSession* GetEncrypt() const { return m_Encrypt.get(); }

private:
    /**
     * Reads the xref sections and the trailers of the file
     * in the correct order in the memory
     * \param eofSearchOffset the offset where to start the search for EOF
     * \param skipFollowPrevious don't follow previous incremental update
     */
    void ReadDocumentStructure(InputStreamDevice& device, ssize_t eofSearchOffset = -1, bool skipFollowPrevious = false);

    /**
     * Reads the xref table from a pdf file.
     * If there is no xref table, ReadXRefStreamContents() is called.
     * \param offset read the table from this offset
     * \param skipFollowPrevious don't follow previous incremental update
     */
    void ReadXRefContents(InputStreamDevice& device, size_t offset, bool skipFollowPrevious);

    /** Read a xref subsection
     *
     *  Throws PdfErrorCode::NoXref if the number of objects read was not
     *  the number specified by the subsection header (as passed in
     *  'objectCount').
     *
     *  \param firstObject object number of the first object
     *  \param objectCount  how many objects should be read from this section
     */
    void ReadXRefSubsection(InputStreamDevice& device, int64_t& firstObject, int64_t& objectCount);

    /** Reads an XRef stream contents object
     *  \param offset read the stream from this offset
     *  \param readOnlyTrailer only the trailer is skipped over, the contents
     *         of the xref stream are not parsed
     */
    void ReadXRefStreamContents(InputStreamDevice& device, size_t offset, bool skipFollowPrevious);

    /** Reads all objects from the pdf into memory
     *  from the previously read entries
     *
     *  If required an encryption object is setup first.
     *
     *  The actual reading happens in ReadObjectsInternal()
     *  either if no encryption is required or a correct
     *  encryption object was initialized from SetPassword.
     */
    void ReadObjects(InputStreamDevice& device);

    /** Checks the magic number at the start of the pdf file
     *  and sets the m_PdfVersion member to the correct version
     *  of the pdf file.
     *
     *  \returns true if this is a pdf file, otherwise false
     */
    bool IsPdfFile(InputStreamDevice& device);

private:
    /** Searches backwards from the specified position of the file
     *  and tries to find a token.
     *  The current file is positioned right after the token.
     *
     *  \param token a token to find
     *  \param range range in bytes in which to search
     *                beginning at the specified position of the file
     *  \param searchEnd specifies position
     */
    void findTokenBackward(InputStreamDevice& device, const char* token, size_t range, size_t searchEnd);

    /** Merge the information of this trailer object
     *  in the parsers main trailer object.
     *  \param trailer take the keys to merge from this dictionary.
     */
    void mergeTrailer(const PdfObject& trailer);

    /** Looks for a startxref entry at the current file position
     *  and saves its byteoffset to pXRefOffset.
     *  \param xRefOffset store the byte offset of the xref section into this variable.
     */
    void findXRef(InputStreamDevice& device, size_t& xRefOffset);

    /** Reads all objects from the pdf into memory
     *  from the previously read entries
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
    void readObjectsInternal(InputStreamDevice& device);

    /** Read the object with index from the object stream nObjNo
     *  and push it on the objects vector
     *
     *  All objects are read from this stream and the stream object
     *  is free'd from memory. Further calls who try to read from the
     *  same stream simply do nothing.
     *
     *  \param objNo object number of the stream object
     *  \param index index of the object which should be parsed
     *
     */
    void readCompressedObjectFromStream(uint32_t objNo, const cspan<int64_t>& objectList);

    void readNextTrailer(InputStreamDevice& device, bool skipFollowPrevious);


    /** Checks for the existence of the %%EOF marker at the end of the file.
     *  When strict mode is off it will also attempt to setup the parser to ignore
     *  any garbage after the last %%EOF marker.
     *  Simply raises an error if there is a problem with the marker.
     *
     */
    void checkEOFMarker(InputStreamDevice& device);

    /** Initializes all private members
     *  with their initial values.
     */
    void reset();

    /** Small helper method to retrieve the document id from the trailer
     *
     *  \returns the document id of this PDF document
     */
    const PdfString& getDocumentId();

    /** Determines the correct version of the PDF
     *  from the document catalog (if available),
     *  as PDF > 1.4 allows updating the version.
     *
     *  If no catalog dictionary is present or no /Version
     *  key is available, the version from the file header will
     *  be used.
     */
    void updateDocumentVersion();

private:
    std::shared_ptr<charbuff> m_buffer;
    PdfTokenizer m_tokenizer;

    PdfVersion m_PdfVersion;
    bool m_LoadOnDemand;

    size_t m_magicOffset;
    bool m_HasXRefStream;
    size_t m_XRefOffset;
    size_t m_FileSize;
    size_t m_lastEOFOffset;

    PdfXRefEntries m_entries;
    PdfIndirectObjectList* m_Objects;

    std::unique_ptr<PdfParserObject> m_Trailer;
    std::shared_ptr<PdfEncryptSession> m_Encrypt;

    std::string m_Password;

    bool m_StrictParsing;
    bool m_IgnoreBrokenObjects;

    unsigned m_IncrementalUpdateCount;

    std::set<size_t> m_visitedXRefOffsets;
};

};

#endif // PDF_PARSER_H
