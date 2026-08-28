// SPDX-FileCopyrightText: 2005 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef PDF_WRITER_H
#define PDF_WRITER_H

#include <podofo/auxiliary/InputDevice.h>
#include <podofo/auxiliary/OutputDevice.h>
#include <podofo/main/PdfIndirectObjectList.h>
#include "PdfEncryptSession.h"

namespace PoDoFo {

class PdfXRef;

/// The PdfWriter class writes a list of PdfObjects as PDF file.
/// The XRef section (which is the required table of contents for any
/// PDF file) is created automatically.
///
/// It does not know about pages but only about PdfObjects.
///
/// Most users will want to use PdfDocument.
class PdfWriter
{
private:
    PdfWriter(PdfIndirectObjectList* objects, const PdfObject& trailer, size_t magicOffset);

public:
    /// Create a new pdf file, from an vector of PdfObjects
    /// and a trailer object.
    /// @param objects the vector of objects
    /// @param trailer a valid trailer object
    PdfWriter(PdfIndirectObjectList& objects, const PdfObject& trailer,
        size_t magicOffset);

    virtual ~PdfWriter();

    /// Internal implementation of the Write() call with the common code
    /// @param device write to this output device
    void Write(OutputStreamDevice& device);

    /// Set the written document to be encrypted using a PdfEncrypt object
    ///
    /// @param encrypt an encryption object which is used to encrypt the written PDF file
    void SetEncrypt(PdfEncryptSession& encrypt);

    /// Add required keys to a trailer object
    /// @param trailer add keys to this object
    /// @param size number of objects in the PDF file
    /// @param onlySizeKey write only the size key
    void FillTrailerObject(PdfObject& trailer, size_t size, bool onlySizeKey) const;

    /// Determine the XRef layout that will be used on writing
    /// @param opts the save options in effect
    /// @param useXRefStreamHint the layout of the parsed document, if any
    static bool ShouldUseXRefStream(PdfSaveOptions opts, bool useXRefStreamHint);

public:
    void SetSaveOptions(PdfSaveOptions saveOptions);

    inline PdfSaveOptions GetSaveOptions() const { return m_SaveOptions; }

    /// Get the write mode used for writing the PDF
    /// @returns the write mode
    inline PdfWriteFlags GetWriteFlags() const { return m_WriteFlags; }

    /// Set the PDF version of the document
    /// @param version version of the pdf document
    /// @remarks It's a hint: writing an XRef stream requires at least PDF 1.5
    inline void SetPdfVersionHint(PdfVersion version) { m_VersionHint = version; }

    /// @returns the PDF version of the document, as previously set
    inline PdfVersion GetPdfVersionHint() const { return m_VersionHint; }

    /// Write a XRef stream
    /// @param useXRefStream if true a XRef stream object will be tentatively created
    /// @remarks It's a hint: PdfSaveOptions::ForceXRefTable or
    /// PdfSaveOptions::ForceXRefStream take precedence over it
    inline void SetUseXRefStreamHint(bool useXRefStream) { m_UseXRefStreamHint = useXRefStream; }

    /// @returns whether an XRef stream is requested, as previously set
    inline bool GetUseXRefStreamHint() const { return m_UseXRefStreamHint; }

    void SetPdfALevel(PdfALevel level);

    inline PdfALevel GetPdfALevel() const { return m_PdfALevel; }

    /// Sets an offset to the previous XRef table. Set it to lower than
    /// or equal to 0, to not write a reference to the previous XRef table.
    /// The default is 0.
    /// @param prevXRefOffset the previous XRef table offset
    inline void SetPrevXRefOffset(size_t prevXRefOffset) { m_PrevXRefOffset = prevXRefOffset; }

    /// @returns offset to the previous XRef table, as previously set
    ///     by SetPrevXRefOffset.
    ///
    /// @see SetPrevXRefOffset
    inline size_t GetPrevXRefOffset() const { return m_PrevXRefOffset; }

    /// Set whether writing an incremental update.
    /// Default is false.
    void SetIncrementalUpdate(bool enabled);

    /// @returns whether writing an incremental update
    inline bool IsIncrementalUpdate() const { return m_IsIncrementalUpdate; }

    /// @returns true if this PdfWriter creates an encrypted PDF file
    inline bool GetEncrypted() const { return m_Encrypt != nullptr; }

    inline PdfIndirectObjectList& GetObjects() { return *m_Objects; }

    inline size_t GetMagicOffset() const { return m_MagicOffset; }

    inline size_t GetCurrXRefOffset() const { return m_CurrXRefOffset; }

protected:
    /// Create a PdfWriter from a PdfIndirectObjectList
    PdfWriter(PdfIndirectObjectList& objects);

    /// Determine the version and the XRef layout to actually write,
    /// from the supplied hints and the save options
    /// @remarks It's called at the beginning of Write(). Writers that
    /// don't go through it must call it before writing anything
    void InitWriteState();

    /// Writes the pdf header to the current file.
    /// @param device write to this output device
    void WritePdfHeader(OutputStreamDevice& device);

    /// Write pdf objects to file
    /// @param device write to this output device
    /// @param objects write all objects in this vector to the file
    /// @param xref add all written objects to this XRefTable
    void WritePdfObjects(OutputStreamDevice& device, const PdfIndirectObjectList& objects, PdfXRef& xref);

    /// Creates a file identifier which is required in several
    /// PDF workflows.
    /// All values from the files document information dictionary are
    /// used to create a unique MD5 key which is added to the trailer dictionary.
    ///
    /// @param identifier write the identifier to this string
    /// @param trailer trailer object
    /// @param originalIdentifier write the original identifier (when using incremental update) to this string
    void CreateFileIdentifier(PdfString& identifier, const PdfObject& trailer, PdfString* originalIdentifier = nullptr);

    const PdfObject& GetTrailer() { return *m_Trailer; }
    PdfEncryptSession* GetEncrypt() { return m_Encrypt; }
    PdfObject* GetEncryptObj() { return m_EncryptObj; }
    const PdfString& GetIdentifier() { return m_identifier; }
    void SetIdentifier(const PdfString& identifier) { m_identifier = identifier; }
    void SetEncryptObj(PdfObject& obj);

    /// @returns the PDF version that is actually written
    /// @remarks Meaningful only after InitWriteState()
    inline PdfVersion GetPdfVersion() const { return m_Version; }

    /// @returns whether an XRef stream is actually written or not
    /// @remarks Meaningful only after InitWriteState()
    inline bool GetUseXRefStream() const { return m_UseXRefStream; }

private:
    void initWriteFlags();
    PdfObject* getExistingEncryptObject();

protected:
    charbuff m_buffer;

private:
    PdfIndirectObjectList* m_Objects;
    const PdfObject* m_Trailer;
    size_t m_MagicOffset;
    PdfVersion m_VersionHint;
    PdfALevel m_PdfALevel;
    bool m_UseXRefStreamHint;

    PdfVersion m_Version;                     // The version actually written
    bool m_UseXRefStream;                     // The XRef layout actually written

    PdfEncryptSession* m_Encrypt;             // If not nullptr encrypt all strings and streams and
                                              // create an encryption dictionary in the trailer
    PdfObject* m_EncryptObj;                  // Used to temporarily store the encryption dictionary

    PdfSaveOptions m_SaveOptions;
    PdfWriteFlags m_WriteFlags;

    PdfString m_identifier;
    PdfString m_originalIdentifier; // used for incremental update
    size_t m_PrevXRefOffset;
    size_t m_CurrXRefOffset;
    bool m_IsIncrementalUpdate;
};

};

#endif // PDF_WRITER_H
