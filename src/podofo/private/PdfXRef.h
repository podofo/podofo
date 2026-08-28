// SPDX-FileCopyrightText: 2007 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef PDF_XREF_H
#define PDF_XREF_H

#include <podofo/main/PdfReference.h>
#include "PdfXRefEntry.h"

namespace PoDoFo {

class PdfWriter;
class OutputStreamDevice;

/// Creates an XRef table.
///
/// This is an internal class of PoDoFo used by PdfWriter.
class PdfXRef
{
    friend class PdfXRefStream;

public:
    PdfXRef(PdfWriter& writer);

public:
    virtual ~PdfXRef();

public:

    /// Add an used object to the XRef table.
    /// The object should have been written to an output device already.
    ///
    /// @param ref reference of this object
    /// @param offset the offset where on the device the object was written
    ///                if std::nullopt, the object will be accounted for
    ///                 trailer's /Size but not written in the entries list
    void AddInUseObject(const PdfReference& ref, uint64_t offset);

    /// Add a free object to the XRef table.
    ///
    /// @param ref reference of this object
    void AddFreeObject(const PdfReference& ref);

    void AddUnavailableObject(uint32_t objNum);

    /// Add an object that is stored in an object stream to the XRef table
    /// @param objNum object number of this object
    /// @param objStmNum object number of the object stream storing it
    /// @param index index of the object within the object stream
    /// @remarks The generation of a compressed object is implicitly zero.
    /// Only an XRef stream can address compressed objects
    void AddCompressedObject(uint32_t objNum, uint32_t objStmNum, uint32_t index);

    /// Write the XRef table to an output device.
    ///
    /// @param device an output device (usually a PDF file)
    ///
    void Write(OutputStreamDevice& device, charbuff& buffer);

    /// Get the size of the XRef table.
    /// I.e. the highest object number + 1.
    ///
    /// @returns the size of the xref table
    uint32_t GetSize() const;

    /// Should skip writing for this object
    /// @param ref reference of the object
    virtual bool ShouldSkipWrite(const PdfReference& ref);

public:
    inline PdfWriter& GetWriter() const { return *m_writer; }

    /// @returns the offset in the file at which the XRef table
    ///          starts after it was written
    virtual size_t GetOffset() const;

protected:
    /// Called at the start of writing the XRef table.
    /// This method can be overwritten in subclasses
    /// to write a general header for the XRef table.
    ///
    /// @param device the output device to which the XRef table
    ///                 should be written.
    virtual void BeginWrite(OutputStreamDevice& device, charbuff& buffer);

    /// Begin an XRef subsection.
    /// All following calls of WriteXRefEntry belong to this XRef subsection.
    ///
    /// @param device the output device to which the XRef table
    ///                 should be written.
    /// @param first the object number of the first object in this subsection
    /// @param count the number of entries in this subsection
    virtual void WriteSubSection(OutputStreamDevice& device, uint32_t first, uint32_t count, charbuff& buffer);

    /// Write a single entry to the XRef table
    ///
    /// @param device the output device to which the XRef table
    ///                 should be written.
    /// @param ref the reference of object of the entry
    /// @param entry the XRefEntry of this object
    virtual void WriteXRefEntry(OutputStreamDevice& device, const PdfReference& ref, const PdfXRefEntry& entry, charbuff& buffer);

    /// Sub classes can overload this method to finish a XRef table.
    ///
    /// @param device the output device to which the XRef table
    ///                 should be written.
    virtual void EndWriteImpl(OutputStreamDevice& device, charbuff& buffer);

private:
    /// The reference is decomposed, so the entry type fits
    /// the padding and the structure stays 16 bytes wide
    struct XRefObject final
    {
        static XRefObject CreateInUse(const PdfReference& ref, uint64_t offset);

        static XRefObject CreateFree(const PdfReference& ref);

        static XRefObject CreateCompressed(uint32_t objNum, uint32_t objStmNum, uint32_t index);

        union
        {
            int64_t Offset;             ///< Offset of the object, in InUse entries
            struct
            {
                uint32_t Number;        ///< Object number of the object stream
                uint32_t Index;         ///< Index of the object within the object stream
            } ObjectStream;             ///< Set in Compressed entries
        };
        uint32_t ObjectNumber;
        uint16_t GenerationNumber;
        PdfXRefEntryType Type;

        bool IsFree() const;

        bool IsInUse() const;

        bool IsCompressed() const;

        bool IsUnavailable() const;

        PdfReference GetReference() const;
    };

    static_assert(sizeof(XRefObject) == 16, "XRefObject is expected to not grow");

    struct XRefObjectInequality
    {
        using is_transparent = std::true_type;

        bool operator()(const XRefObject& lhs, const XRefObject& rhs) const
        {
            return lhs.ObjectNumber < rhs.ObjectNumber;
        }
        bool operator()(const XRefObject& lhs, const PdfReference& rhs) const
        {
            return lhs.ObjectNumber < rhs.ObjectNumber();
        }
        bool operator()(const PdfReference& lhs, const XRefObject& rhs) const
        {
            return lhs.ObjectNumber() < rhs.ObjectNumber;
        }
    };

    using XRefObjectList = std::vector<XRefObject>;
    using XRefObjectSet = std::set<XRefObject, XRefObjectInequality>;

    class XRefSubSectionList;

    class XRefSubSection
    {
        friend class XRefSubSectionList;
    public:
        class iterator
        {
            friend class XRefSubSection;

            iterator(uint32_t objNum, XRefObjectList::const_iterator&& objIt);

            uint32_t ObjectNum;
            XRefObjectList::const_iterator ObjectIt;
        };

    private:
        XRefSubSection();

        XRefSubSection(const XRefSubSection&) = default;

    public:
        /// Try add the object to this subsection, only
        /// but only if the object number is the next after
        /// last object in the section
        bool TryAddObject(const XRefObject& obj);

        /// Try to the get the XRef object for the object referenced
        /// by the iterator and increment it if successful
        bool TryGetXRefEntryIncrement(iterator& it, PdfReference& ref, PdfXRefEntry& entry) const;

    public:
        XRefSubSection& operator=(const XRefSubSection&) = delete;

    public:
        inline uint32_t GetFirst() const { return m_First; }

        inline uint32_t GetLast() const { return m_Last; }

        inline uint32_t GetCount() const;

        /// This gives raw access to the objects
        /// For entry iteration use TryGetNextEntry()
        inline const XRefObjectList& GetObjects() const { return m_Objects; }

        iterator begin() const;

    private:
        XRefSubSectionList* m_parent;
        size_t m_Index;
        uint32_t m_First;
        uint32_t m_Last;
        XRefObjectList m_Objects;
    };

    class XRefSubSectionList
    {
        friend class XRefSubSection;
    public:
        XRefSubSectionList();

        XRefSubSectionList(const XRefSubSectionList&) = delete;
    public:
        /// Push a sub section with just a single unavailable,
        /// generation 65535, object 0
        XRefSubSection& PushSubSection();

        /// Push a sub section with single object starting at
        /// the given reference object number
        XRefSubSection& PushSubSection(const XRefObject& obj);


        /// Push a sub section with all objects from the input,
        /// and forcibly setting first and last object numbers
        XRefSubSection& PushSubSection(const XRefObjectSet& objects, uint32_t firstObjectNum, uint32_t lastObjectNum);

    public:
        const XRefSubSection& operator[](unsigned index) const
        {
            return *m_Sections[index];
        }

        XRefSubSectionList& operator=(const XRefSubSectionList&) = delete;

    public:
        unsigned GetSize() const { return (unsigned)m_Sections.size(); }

    private:
        uint32_t GetNextFreeXRefObjectNumber(size_t sectionIdx, uint32_t objectNum, XRefObjectList::const_iterator itObject) const;
    private:
        std::vector<std::unique_ptr<XRefSubSection>> m_Sections;
    };

    void buildSubSections(XRefSubSectionList& sections);

    void addObject(const XRefObject& obj);

    /// Called at the end of writing the XRef table.
    /// Sub classes can overload this method to finish a XRef table.
    ///
    /// @param device the output device to which the XRef table
    ///                 should be written.
    void endWrite(OutputStreamDevice& device, charbuff& buffer);

private:
    PdfWriter* m_writer;
    XRefObjectSet m_xrefObjects;
    size_t m_offset;
};

};

#endif // PDF_XREF_H
