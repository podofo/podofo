/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_PARSER_OBJECT_H
#define PDF_PARSER_OBJECT_H

#include "PdfDeclarations.h"
#include "PdfObject.h"
#include "PdfTokenizer.h"

namespace PoDoFo {

class PdfEncrypt;

/**
 * A PdfParserObject constructs a PdfObject from a PDF file.
 * Parsing starts always at the current file position.
 */
class PODOFO_API PdfParserObject : public PdfObject
{
    friend class PdfParser;

private:
    /** Parse the object data from the given file handle starting at
     *  the current position.
     *  \param doc document where to resolve object references
     *  \param device an open reference counted input device which is positioned in
     *                 front of the object which is going to be parsed.
     *  \param buffer buffer to use for parsing to avoid reallocations
     *  \param offset the position in the device from which the object shall be read
     *                 if lOffset = -1, the object will be read from the current
     *                 position in the file.
     */
    PdfParserObject(PdfDocument& doc, const PdfReference& indirectReference,
        InputStreamDevice& device, ssize_t offset = -1);
    PdfParserObject(PdfDocument& doc, InputStreamDevice& device, ssize_t offset = -1);

public:
    /**
     *  \warning This constructor is for testing usage only
     */
    PdfParserObject(InputStreamDevice& device, ssize_t offset = -1);

protected:
    PdfParserObject(PdfDocument* doc, const PdfReference& indirectReference,
        InputStreamDevice& device, ssize_t offset);

public:
    /** Tries to free all memory allocated by this
     *  PdfObject (variables and streams) and reads
     *  it from disk again if it is requested another time.
     *
     *  This will only work if load on demand is used.
     *  If the object is dirty if will not be free'd.
     *
     *  \param force if true the object will be free'd
     *                even if IsDirty() returns true.
     *                So you will loose any changes made
     *                to this object.
     *
     *  \see IsLoadOnDemand
     *  \see IsDirty
     */
    void FreeObjectMemory(bool force = false);

    void Parse();

    void ParseStream();

    /** Returns if this object has a stream object appended.
     *  which has to be parsed.
     *  \returns true if there is a stream
     */
    inline bool HasStreamToParse() const { return m_HasStream; }

    /** Gets an offset in which the object beginning is stored in the file.
     *  Note the offset points just after the object identificator ("0 0 obj").
     *
     * \returns an offset in which the object is stored in the source device,
     *     or -1, if the object was created on demand.
     */
    inline ssize_t GetOffset() const { return m_Offset; }

    inline void SetEncrypt(PdfEncrypt* encrypt) { m_Encrypt = encrypt; }

    inline void SetIsTrailer(bool isTrailer) { m_IsTrailer = isTrailer; }

protected:
    void DelayedLoadImpl() override;
    void DelayedLoadStreamImpl() override;
    PdfReference ReadReference(PdfTokenizer& tokenizer);
    void Parse(PdfTokenizer& tokenizer);

private:
    PdfParserObject(const PdfParserObject&) = delete;
    PdfParserObject& operator=(const PdfParserObject&) = delete;

    /** Starts reading at the file position m_StreamOffset and interprets all bytes
     *  as contents of the objects stream.
     *  It is assumed that the dictionary has a valid /Length key already.
     *
     *  Called from DelayedLoadStream(). Do not call directly.
     */
    void parseStream();

    PdfReference readReference(PdfTokenizer& tokenizer);

    void checkReference(PdfTokenizer& tokenizer);

private:
    InputStreamDevice*m_device;
    PdfEncrypt* m_Encrypt;
    bool m_IsTrailer;
    size_t m_Offset;
    bool m_HasStream;
    size_t m_StreamOffset;
};

};

#endif // PDF_PARSER_OBJECT_H
