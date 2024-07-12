/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_XREF_H
#define PDF_XREF_H

#include <podofo/main/PdfReference.h>
#include "PdfXRefEntry.h"

namespace PoDoFo {

class PdfWriter;
class OutputStreamDevice;

/**
 * Creates an XRef table.
 *
 * This is an internal class of PoDoFo used by PdfWriter.
 */
class PdfXRef
{
    friend class PdfWriter;
    friend class PdfImmediateWriter;
    friend class PdfXRefStream;

private:
    PdfXRef(PdfWriter& writer);

public:
    virtual ~PdfXRef();

protected:
    struct XRefItem
    {
        XRefItem(const PdfReference& ref, uint64_t off)
            : Reference(ref), Offset(off) { }

        PdfReference Reference;
        uint64_t Offset;

        bool operator<(const XRefItem& rhs) const
        {
            return this->Reference < rhs.Reference;
        }
    };

    using XRefItemList = std::vector<XRefItem>;
    using ReferenceList = std::vector<PdfReference>;

    struct PdfXRefBlock
    {
        PdfXRefBlock()
            : First(0), Count(0) { }

        PdfXRefBlock(const PdfXRefBlock& rhs) = default;

        bool InsertItem(const PdfReference& ref, nullable<uint64_t> offset, bool bUsed);

        bool operator<(const PdfXRefBlock& rhs) const
        {
            return First < rhs.First;
        }

        PdfXRefBlock& operator=(const PdfXRefBlock& rhs) = default;

        uint32_t First;
        uint32_t Count;
        XRefItemList Items;
        ReferenceList FreeItems;
    };

    using XRefBlockList = std::vector<PdfXRefBlock>;

public:

    /** Add an used object to the XRef table.
     *  The object should have been written to an output device already.
     *
     *  \param ref reference of this object
     *  \param offset the offset where on the device the object was written
     *                if std::nullopt, the object will be accounted for
     *                 trailer's /Size but not written in the entries list
     */
    void AddInUseObject(const PdfReference& ref, nullable<uint64_t> offset);

    /** Add a free object to the XRef table.
     *
     *  \param ref reference of this object
     *  \param offset the offset where on the device the object was written
     *  \param bUsed specifies whether this is an used or free object.
     *               Set this value to true for all normal objects and to false
     *               for free object references.
     */
    void AddFreeObject(const PdfReference& ref);

    /** Write the XRef table to an output device.
     *
     *  \param device an output device (usually a PDF file)
     *
     */
    void Write(OutputStreamDevice& device, charbuff& buffer);

    /** Get the size of the XRef table.
     *  I.e. the highest object number + 1.
     *
     *  \returns the size of the xref table
     */
    uint32_t GetSize() const;

    /**
     * Mark as empty block.
     */
    void SetFirstEmptyBlock();

    /** Should skip writing for this object
     *  \param ref reference of the object
     */
    virtual bool ShouldSkipWrite(const PdfReference& ref);

public:
    inline PdfWriter& GetWriter() const { return *m_writer; }

    /**
     * \returns the offset in the file at which the XRef table
     *          starts after it was written
     */
    inline virtual uint64_t GetOffset() const { return m_offset; }

protected:
    /** Called at the start of writing the XRef table.
     *  This method can be overwritten in subclasses
     *  to write a general header for the XRef table.
     *
     *  \param device the output device to which the XRef table
     *                 should be written.
     */
    virtual void BeginWrite(OutputStreamDevice& device, charbuff& buffer);

    /** Begin an XRef subsection.
     *  All following calls of WriteXRefEntry belong to this XRef subsection.
     *
     *  \param device the output device to which the XRef table
     *                 should be written.
     *  \param first the object number of the first object in this subsection
     *  \param count the number of entries in this subsection
     */
    virtual void WriteSubSection(OutputStreamDevice& device, uint32_t first, uint32_t count, charbuff& buffer);

    /** Write a single entry to the XRef table
     *
     *  \param device the output device to which the XRef table
     *                 should be written.
     *  \param ref the reference of object of the entry
     *  \param entry the XRefEntry of this object
     */
    virtual void WriteXRefEntry(OutputStreamDevice& device, const PdfReference& ref, const PdfXRefEntry& entry, charbuff& buffer);

    /**  Sub classes can overload this method to finish a XRef table.
     *
     *  \param device the output device to which the XRef table
     *                 should be written.
     */
    virtual void EndWriteImpl(OutputStreamDevice& device, charbuff& buffer);

private:
    void addObject(const PdfReference& ref, nullable<uint64_t> offset, bool inUse);

    /** Called at the end of writing the XRef table.
     *  Sub classes can overload this method to finish a XRef table.
     *
     *  \param device the output device to which the XRef table
     *                 should be written.
     */
    void endWrite(OutputStreamDevice& device, charbuff& buffer);

    const PdfReference* getFirstFreeObject(XRefBlockList::const_iterator itBlock, ReferenceList::const_iterator itFree) const;
    const PdfReference* getNextFreeObject(XRefBlockList::const_iterator itBlock, ReferenceList::const_iterator itFree) const;

    /** Merge all xref blocks that follow immediately after each other
     *  into a single block.
     *
     *  This results in slightly smaller PDF files which are easier to parse
     *  for other applications.
     */
    void mergeBlocks();

private:
    uint32_t m_maxObjCount;
    XRefBlockList m_blocks;
    PdfWriter* m_writer;
    uint64_t m_offset;
};

};

#endif // PDF_XREF_H
