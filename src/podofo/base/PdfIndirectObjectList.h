/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_INDIRECT_OBJECT_LIST_H
#define PDF_INDIRECT_OBJECT_LIST_H

#include <list>
#include <unordered_set>

#include "PdfObject.h"

namespace PoDoFo {

class PdfObjectStreamProvider;
using ReferenceList = std::deque<PdfReference>;

/** A list of PdfObjects that constitutes the indirect object list
 *  of the document
 *  The PdfParser will read the PdfFile into memory and create
 *  a PdfIndirectObjectList of all dictionaries found in the PDF file.
 *
 *  The PdfWriter class contrary creates a PdfIndirectObjectList internally
 *  and writes it to a PDF file later with an appropriate table of
 *  contents.
 *
 *  This class contains also advanced functions for searching of PdfObject's
 *  in a PdfIndirectObjectList.
 */
class PODOFO_API PdfIndirectObjectList final
{
    friend class PdfWriter;
    friend class PdfDocument;
    friend class PdfParser;
    friend class PdfObjectStreamParser;
    friend class PdfImmediateWriter;

private:
    static bool CompareObject(const PdfObject* p1, const PdfObject* p2);
    static bool CompareReference(const PdfObject* obj, const PdfReference& ref);

private:
    using ObjectList = std::set<PdfObject*, decltype(CompareObject)*>;

public:
    // An incomplete set of container typedefs, just enough to handle
    // the begin() and end() methods we wrap from the internal vector.
    // TODO: proper wrapper iterator class.
    using iterator = ObjectList::const_iterator;
    using reverse_iterator = ObjectList::const_reverse_iterator;

    /** Every observer of PdfIndirectObjectList has to implement this interface.
     */
    class PODOFO_API Observer
    {
        friend class PdfIndirectObjectList;
    public:
        virtual ~Observer() { }

        virtual void WriteObject(const PdfObject& obj) = 0;

        /** Called whenever appending to a stream is started.
         *  \param stream the stream object the user currently writes to.
         */
        virtual void BeginAppendStream(PdfObjectStream& stream) = 0;

        /** Called whenever appending to a stream has ended.
         *  \param stream the stream object the user currently writes to.
         */
        virtual void EndAppendStream(PdfObjectStream& stream) = 0;

        virtual void Finish() = 0;
    };

    /** This class is used to implement stream factories in PoDoFo.
     */
    class PODOFO_API StreamFactory
    {
    public:
        virtual ~StreamFactory() { }

        /** Creates a stream object
         *
         *  \param parent parent object
         *
         *  \returns a new stream object
         */
        virtual std::unique_ptr<PdfObjectStreamProvider> CreateStream() = 0;
    };

public:
    PdfIndirectObjectList();

    ~PdfIndirectObjectList();

    /** Enable/disable object numbers re-use.
     *  By default object numbers re-use is enabled.
     *
     *  \param canReuseObjectNumbers if true, free object numbers can be re-used when creating new objects.
     *
     *  If set to false, the list of free object numbers is automatically cleared.
     */
    void SetCanReuseObjectNumbers(bool canReuseObjectNumbers);

    /** Removes all objects from the vector
     *  and resets it to the default state.
     *
     *  If SetAutoDelete is true all objects are deleted.
     *  All observers are removed from the vector.
     *
     *  \see SetAutoDelete
     *  \see IsAutoDelete
     */
    void Clear();

    /**
     *  \returns the size of the internal vector
     */
    unsigned GetSize() const;

    /**
     *  \returns the highest object number in the vector
     */
    unsigned GetObjectCount() const { return m_ObjectCount; }

    /** Finds the object with the given reference
     *  and returns a pointer to it if it is found. Throws a PdfError
     *  exception with error code ePdfError_NoObject if no object was found
     *  \param ref the object to be found
     *  \returns the found object
     *  \throws PdfError(ePdfError_NoObject)
     */
    PdfObject& MustGetObject(const PdfReference& ref) const;

    /** Finds the object with the given reference
     *  and returns a pointer to it if it is found.
     *  \param ref the object to be found
     *  \returns the found object or nullptr if no object was found.
     */
    PdfObject* GetObject(const PdfReference& ref) const;

    /** Remove the object with the given object and generation number from the list
     *  of objects.
     *  The object is returned if it was found. Otherwise nullptr is returned.
     *  The caller has to delete the object by himself.
     *
     *  \param ref the object to be found
     *  \param markAsFree if true the removed object reference is marked as free object
     *                     you will always want to have this true
     *                     as invalid PDF files can be generated otherwise
     *  \returns The removed object.
     */
    std::unique_ptr<PdfObject> RemoveObject(const PdfReference& ref);

    /** Remove the object with the iterator it from the vector and return it
     *  \param ref the reference of the object to remove
     *  \returns the removed object
     */
    std::unique_ptr<PdfObject> RemoveObject(const iterator& it);

    /** Replace the object at the given reference
     *  \param ref the reference of the object to replace
     *  \param obj the object that will be inserted instead, must be non null
     *  \returns the replaced object
     */
    std::unique_ptr<PdfObject> ReplaceObject(const PdfReference& ref, PdfObject* obj);

    /** Creates a new object and inserts it into the vector.
     *  This function assigns the next free object number to the PdfObject.
     *
     *  \param type optional value of the /Type key of the object
     *  \param subtype optional value of the /SubType key of the object
     *  \returns PdfObject pointer to the new PdfObject
     */
    PdfObject& CreateDictionaryObject(const std::string_view& type = { },
        const std::string_view& subtype = { });

    PdfObject& CreateArrayObject();

    /** Creates a new object and inserts it into the vector.
     *  This function assigns the next free object number to the PdfObject.
     *
     *  \param obj value of the PdfObject
     *  \returns PdfObject pointer to the new PdfObject
     */
    PdfObject& CreateObject(const PdfObject& obj);
    PdfObject& CreateObject(PdfObject&& obj);


    /** Attach a new observer
     *  \param observer to attach
     */
    void Attach(Observer& observer);

    /** Detach an observer.
     *
     *  \param observer observer to detach
     */
    void Detach(Observer& observer);

    /** Sets a StreamFactory which is used whenever CreateStream is called.
     *
     *  \param factory a stream factory or nullptr to reset to the default factory
     */
    void SetStreamFactory(StreamFactory* factory);

    /** Creates a stream object
     *  This method is a factory for PdfObjectStream objects.
     *
     *  \param parent parent object
     *
     *  \returns a new stream object
     */
    std::unique_ptr<PdfObjectStreamProvider> CreateStream();

    /** Can be called to force objects to be written to disk.
     *
     *  \param obj a PdfObject that should be written to disk.
     */
    void WriteObject(PdfObject& obj);

    /** Call whenever a document is finished
     */
    void Finish();

    /** Every stream implementation has to call this in BeginAppend
     *  \param stream the stream object that is calling
     */
    void BeginAppendStream(PdfObjectStream& stream);

    /** Every stream implementation has to call this in EndAppend
     *  \param stream the stream object that is calling
     */
    void EndAppendStream(PdfObjectStream& stream);

    /**
     * Set the object count so that the object described this reference
     * is contained in the object count.
     *
     * \param ref reference of newly added object
     */
    void TryIncrementObjectCount(const PdfReference& ref);

private:
    // Use deque as many insertions are here way faster than with using std::list
    // This is especially useful for PDFs like PDFReference17.pdf with
    // lots of free objects.
    using ObjectNumSet = std::set<uint32_t>;
    using ReferenceSet = std::set<PdfReference>;
    using ReferencePointers = std::list<PdfReference*>;
    using ReferencePointersList = std::vector<ReferencePointers>;
    using ObserverList = std::vector<Observer*>;

private:
    PdfIndirectObjectList(PdfDocument& document);
    PdfIndirectObjectList(PdfDocument& document, const PdfIndirectObjectList& rhs);

    PdfIndirectObjectList(const PdfIndirectObjectList&) = delete;
    PdfIndirectObjectList& operator=(const PdfIndirectObjectList&) = delete;

    /** Insert an object into this vector so that
     *  the vector remains sorted w.r.t.
     *  the ordering based on object and generation numbers
     *  m_ObjectCount will be increased for the object.
     *
     *  \param obj pointer to the object you want to insert
     */
    void PushObject(PdfObject* obj);

    /** Mark a reference as unused so that it can be reused for new objects.
     *
     *  Add the object only if the generation is the allowed range
     *
     *  \param rReference the reference to reuse
     *  \returns true if the object was succesfully added
     *
     *  \see AddFreeObject
     */
    bool TryAddFreeObject(const PdfReference& reference);

    /** Mark a reference as unused so that it can be reused for new objects.
     *
     *  Add the object and increment the generation number. Add the object
     *  only if the generation is the allowed range
     *
     *  \param rReference the reference to reuse
     *  \returns the generation of the added free object
     *
     *  \see AddFreeObject
     */
    int32_t SafeAddFreeObject(const PdfReference& reference);

    /** Mark a reference as unused so that it can be reused for new objects.
     *  \param rReference the reference to reuse
     *
     *  \see GetCanReuseObjectNumbers
     */
    void AddFreeObject(const PdfReference& reference);

    /** Add the reference object number as an object stream
     * \remarks These objects are usually compressed and can't be removed,
     * that's why the generation number is irrelevant
     */
    void AddObjectStream(uint32_t objectNum);

    std::unique_ptr<PdfObject> RemoveObject(const PdfReference& ref, bool markAsFree);

    /**
     * Deletes all objects that are not references by other objects
     * besides the trailer (which references the root dictionary, which in
     * turn should reference all other objects).
     */
    void CollectGarbage();

private:
    void pushObject(ObjectList::node_type& it, PdfObject* obj);

    std::unique_ptr<PdfObject> removeObject(const iterator& it, bool markAsFree);

    void addNewObject(PdfObject* obj);

    /**
     * \returns the next free object reference
     */
    PdfReference getNextFreeObject();

    int32_t tryAddFreeObject(uint32_t objnum, uint32_t gennum);

    void visitObject(const PdfObject& obj, std::unordered_set<PdfReference>& referencedObj);

public:
    /** Iterator pointing at the beginning of the vector
     *  \returns beginning iterator
     */
    iterator begin() const;

    /** Iterator pointing at the end of the vector
     *  \returns ending iterator
     */
    iterator end() const;

    reverse_iterator rbegin() const;

    reverse_iterator rend() const;

    size_t size() const;

public:
    /** \returns a pointer to a PdfDocument that is the
     *           parent of this vector.
     *           Might be nullptr if the vector has no parent.
     */
    inline PdfDocument& GetDocument() const { return *m_Document; }

    /**
     *  \returns whether can re-use free object numbers when creating new objects.
     */
    inline bool GetCanReuseObjectNumbers() const { return m_CanReuseObjectNumbers; }

    /** \returns a list of free references in this vector
     */
    inline const ReferenceList& GetFreeObjects() const { return m_FreeObjects; }

private:
    PdfDocument* m_Document;
    bool m_CanReuseObjectNumbers;
    ObjectList m_Objects;
    unsigned m_ObjectCount;
    ReferenceList m_FreeObjects;
    ObjectNumSet m_unavailableObjects;
    ObjectNumSet m_objectStreams;

    ObserverList m_observers;
    StreamFactory* m_StreamFactory;
};

};

#endif // PDF_INDIRECT_OBJECT_LIST_H
