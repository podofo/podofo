/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_WRITER_H
#define PDF_WRITER_H

#include <podofo/auxiliary/InputDevice.h>
#include <podofo/auxiliary/OutputDevice.h>
#include <podofo/main/PdfIndirectObjectList.h>
#include <podofo/main/PdfEncryptSession.h>

namespace PoDoFo {

class PdfXRef;

/** The PdfWriter class writes a list of PdfObjects as PDF file.
 *  The XRef section (which is the required table of contents for any
 *  PDF file) is created automatically.
 *
 *  It does not know about pages but only about PdfObjects.
 *
 *  Most users will want to use PdfDocument.
 */
class PdfWriter
{
private:
    PdfWriter(PdfIndirectObjectList* objects, const PdfObject& trailer);

public:
    /** Create a new pdf file, from an vector of PdfObjects
     *  and a trailer object.
     *  \param objects the vector of objects
     *  \param trailer a valid trailer object
     */
    PdfWriter(PdfIndirectObjectList& objects, const PdfObject& trailer);

    virtual ~PdfWriter();

    /** Internal implementation of the Write() call with the common code
     *  \param device write to this output device
     *  \param bRewriteXRefTable whether will rewrite whole XRef table (used only if GetIncrementalUpdate() returns true)
     */
    void Write(OutputStreamDevice& device);

    /** Create a XRef stream which is in some case
     *  more compact but requires at least PDF 1.5
     *  Default is false.
     *  \param useXRefStream if true a XRef stream object will be created
     */
    void SetUseXRefStream(bool useXRefStream);

    /** Set the written document to be encrypted using a PdfEncrypt object
     *
     *  \param encrypt an encryption object which is used to encrypt the written PDF file
     */
    void SetEncrypt(PdfEncryptSession& encrypt);

    /** Add required keys to a trailer object
     *  \param trailer add keys to this object
     *  \param size number of objects in the PDF file
     *  \param onlySizeKey write only the size key
     */
    void FillTrailerObject(PdfObject& trailer, size_t size, bool onlySizeKey) const;

public:
    void SetSaveOptions(PdfSaveOptions saveOptions);

    inline PdfSaveOptions GetSaveOptions() const { return m_SaveOptions; }

    /** Get the write mode used for writing the PDF
     *  \returns the write mode
     */
    inline PdfWriteFlags GetWriteFlags() const { return m_WriteFlags; }

    /** Set the PDF Version of the document. Has to be called before Write() to
     *  have an effect.
     *  \param version  version of the pdf document
     */
    inline void SetPdfVersion(PdfVersion version) { m_Version = version; }

    /** Get the PDF version of the document
     *  \returns PdfVersion version of the pdf document
     */
    inline PdfVersion GetPdfVersion() const { return m_Version; }

    void SetPdfALevel(PdfALevel level);

    inline PdfALevel GetPdfALevel() const { return m_PdfALevel; }

    /**
     *  \returns whether an XRef stream is used or not
     */
    inline bool GetUseXRefStream() const { return m_UseXRefStream; }

    /** Sets an offset to the previous XRef table. Set it to lower than
     *  or equal to 0, to not write a reference to the previous XRef table.
     *  The default is 0.
     *  \param lPrevXRefOffset the previous XRef table offset
     */
    inline void SetPrevXRefOffset(int64_t prevXRefOffset) { m_PrevXRefOffset = prevXRefOffset; }

    /**
     *  \returns offset to the previous XRef table, as previously set
     *     by SetPrevXRefOffset.
     *
     * \see SetPrevXRefOffset
     */
    inline int64_t GetPrevXRefOffset() const { return m_PrevXRefOffset; }

    /** Set whether writing an incremental update.
     *  Default is false.
     *  \param incrementalUpdate if true an incremental update will be written
     */
    void SetIncrementalUpdate(bool rewriteXRefTable);

    /**
     *  \returns whether writing an incremental update
     */
    inline bool GetIncrementalUpdate() const { return m_IncrementalUpdate; }

    /**
     * \returns true if this PdfWriter creates an encrypted PDF file
     */
    inline bool GetEncrypted() const { return m_Encrypt != nullptr; }

    inline PdfIndirectObjectList& GetObjects() { return *m_Objects; }

protected:
    /**
     * Create a PdfWriter from a PdfIndirectObjectList
     */
    PdfWriter(PdfIndirectObjectList& objects);

    /** Writes the pdf header to the current file.
     *  \param device write to this output device
     */
    void WritePdfHeader(OutputStreamDevice& device);

    /** Write pdf objects to file
     *  \param device write to this output device
     *  \param objects write all objects in this vector to the file
     *  \param pXref add all written objects to this XRefTable
     *  \param bRewriteXRefTable whether will rewrite whole XRef table (used only if GetIncrementalUpdate() returns true)
     */
    void WritePdfObjects(OutputStreamDevice& device, const PdfIndirectObjectList& objects, PdfXRef& xref);

    /** Creates a file identifier which is required in several
     *  PDF workflows.
     *  All values from the files document information dictionary are
     *  used to create a unique MD5 key which is added to the trailer dictionary.
     *
     *  \param identifier write the identifier to this string
     *  \param trailer trailer object
     *  \param pOriginalIdentifier write the original identifier (when using incremental update) to this string
     */
    void CreateFileIdentifier(PdfString& identifier, const PdfObject& trailer, PdfString* originalIdentifier = nullptr);

    const PdfObject& GetTrailer() { return *m_Trailer; }
    PdfEncryptSession* GetEncrypt() { return m_Encrypt; }
    PdfObject* GetEncryptObj() { return m_EncryptObj; }
    const PdfString& GetIdentifier() { return m_identifier; }
    void SetIdentifier(const PdfString& identifier) { m_identifier = identifier; }
    void SetEncryptObj(PdfObject& obj);

private:
    void initWriteFlags();

protected:
    charbuff m_buffer;

private:
    PdfIndirectObjectList* m_Objects;
    const PdfObject* m_Trailer;
    PdfVersion m_Version;
    PdfALevel m_PdfALevel;

    bool m_UseXRefStream;

    PdfEncryptSession* m_Encrypt;             // If not nullptr encrypt all strings and streams and
                                              // create an encryption dictionary in the trailer
    PdfObject* m_EncryptObj;                  // Used to temporarily store the encryption dictionary

    PdfSaveOptions m_SaveOptions;
    PdfWriteFlags m_WriteFlags;

    PdfString m_identifier;
    PdfString m_originalIdentifier; // used for incremental update
    int64_t m_PrevXRefOffset;
    bool m_IncrementalUpdate;
    bool m_rewriteXRefTable; // Only used if incremental update
};

};

#endif // PDF_WRITER_H
