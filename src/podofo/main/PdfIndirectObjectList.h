/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_INDIRECT_OBJECT_LIST_H
#define PDF_INDIRECT_OBJECT_LIST_H

#include "PdfObject.h"

namespace PoDoFo {

class PdfObjectStreamProvider;
using PdfFreeObjectList = std::deque<PdfReference>;

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
    friend class PdfDocument;
    friend class PdfObject;
    friend class PdfObjectOutputStream;
    PODOFO_PRIVATE_FRIEND(class PdfObjectStreamParser);
    PODOFO_PRIVATE_FRIEND(class PdfImmediateWriter);
    PODOFO_PRIVATE_FRIEND(class PdfParser);
    PODOFO_PRIVATE_FRIEND(class PdfWriter);
    PODOFO_PRIVATE_FRIEND(class PdfParserTest);
    PODOFO_PRIVATE_FRIEND(class PdfEncodingTest);
    PODOFO_PRIVATE_FRIEND(class PdfEncryptTest);

private:
    // NOTE: For testing only
    PdfIndirectObjectList();

public:
    ~PdfIndirectObjectList();

public:
    /** Finds the object with the given reference
     *  and returns a pointer to it if it is found. Throws a PdfError
     *  exception with error code PdfErrorCode::NoObject if no object was found
     *  \param ref the object to be found
     *  \returns the found object
     *  \throws PdfError(PdfErrorCode::NoObject)
     */
    PdfObject& MustGetObject(const PdfReference& ref) const;

    /** Finds the object with the given reference
     *  and returns a pointer to it if it is found.
     *  \param ref the object to be found
     *  \returns the found object or nullptr if no object was found.
     */
    PdfObject* GetObject(const PdfReference& ref) const;

    /** Creates a new object and inserts it into the vector.
     *  This function assigns the next free object number to the PdfObject.
     *
     *  \param type optional value of the /Type key of the object
     *  \param subtype optional value of the /SubType key of the object
     *  \returns PdfObject pointer to the new PdfObject
     */
    PdfObject& CreateDictionaryObject(const PdfName& type = PdfName::Null,
        const PdfName& subtype = PdfName::Null);

    PdfObject& CreateArrayObject();

    /** Creates a new object and inserts it into the vector.
     *  This function assigns the next free object number to the PdfObject.
     *
     *  \param obj value of the PdfObject
     *  \returns PdfObject pointer to the new PdfObject
     */
    PdfObject& CreateObject(const PdfObject& obj);
    PdfObject& CreateObject(PdfObject&& obj);

    /**
     * Deletes all objects that are not references by other objects
     * besides the trailer (which references the root dictionary, which in
     * turn should reference all other objects).
     */
    void CollectGarbage();

public:
    /**
     * \returns the size of the internal object list
     * \remarks It may differ from GetObjectCount()
     */
    unsigned GetSize() const;

    /**
     * \returns the logical object count in the document.
     * \remarks It corresponds to the highest object number
     *   ever used and it never decreases. This value is used
     *   to determine the next available object number, when
     *   the free object list is empty
     */
    inline unsigned GetObjectCount() const { return m_ObjectCount; }

    /** \returns a list of free references in this vector
     */
    inline const PdfFreeObjectList& GetFreeObjects() const { return m_FreeObjects; }

    /** \returns a reference to the owner document
     */
    inline PdfDocument& GetDocument() const { return *m_Document; }

private:
    /** Every observer of PdfIndirectObjectList has to implement this interface.
     */
    class PODOFO_API Observer
    {
        friend class PdfIndirectObjectList;
    public:
        virtual ~Observer() { }

        /** Called whenever appending to a stream is started.
         *  \param stream the stream object the user currently writes to.
         */
        virtual void BeginAppendStream(PdfObjectStream& stream) = 0;

        /** Called whenever appending to a stream has ended.
         *  \param stream the stream object the user currently writes to.
         */
        virtual void EndAppendStream(PdfObjectStream& stream) = 0;
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

    using ObjectNumSet = std::set<uint32_t>;
    using ReferenceSet = std::set<PdfReference>;
    using ObserverList = std::vector<Observer*>;
    using ObjectList = std::set<PdfObject*, PdfObjectInequality>;

public:

    // An incomplete set of container typedefs, just enough to handle
    // the begin() and end() methods we wrap from the internal vector.
    // TODO: proper wrapper iterator class.
    using iterator = ObjectList::const_iterator;
    using reverse_iterator = ObjectList::const_reverse_iterator;

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

private:
    PdfIndirectObjectList(PdfDocument& document);
    PdfIndirectObjectList(PdfDocument& document, const PdfIndirectObjectList& rhs);

    PdfIndirectObjectList(const PdfIndirectObjectList&) = delete;
    PdfIndirectObjectList& operator=(const PdfIndirectObjectList&) = delete;

private:
    /** Creates a stream object
     *  This method is a factory for PdfObjectStream objects.
     *
     *  \param parent parent object
     *
     *  \returns a new stream object
     */
    std::unique_ptr<PdfObjectStreamProvider> CreateStream();

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

    /** Attach a new observer
     *  \param observer to attach
     */
    void AttachObserver(Observer& observer);

    /** Detach an observer.
     *
     *  \param observer observer to detach
     */
    void DetachObserver(Observer& observer);

    /** Every stream implementation has to call this in BeginAppend
     *  \param stream the stream object that is calling
     */
    void BeginAppendStream(PdfObjectStream& stream);

    /** Every stream implementation has to call this in EndAppend
     *  \param stream the stream object that is calling
     */
    void EndAppendStream(PdfObjectStream& stream);

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
     *  \returns true if the object was successfully added
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

    /** Sets a StreamFactory which is used whenever CreateStream is called.
     *
     *  \param factory a stream factory or nullptr to reset to the default factory
     */
    void SetStreamFactory(StreamFactory* factory);

private:
    void pushObject(const ObjectList::const_iterator& hintpos, ObjectList::node_type& node, PdfObject* obj);

    std::unique_ptr<PdfObject> removeObject(const iterator& it, bool markAsFree);

    void addNewObject(PdfObject* obj);

    /**
     * \returns the next free object reference
     */
    PdfReference getNextFreeObject();

    int32_t tryAddFreeObject(uint32_t objnum, uint32_t gennum);

    void visitObject(const PdfObject& obj, std::unordered_set<PdfReference>& referencedObj);

    /**
     * Set the object count so that the object described this reference
     * is contained in the object count.
     *
     * \param ref reference of newly added object
     */
    void tryIncrementObjectCount(const PdfReference& ref);

private:
    PdfDocument* m_Document;
    ObjectList m_Objects;
    unsigned m_ObjectCount;
    PdfFreeObjectList m_FreeObjects;
    ObjectNumSet m_unavailableObjects;
    ObjectNumSet m_objectStreams;

    ObserverList m_observers;
    StreamFactory* m_StreamFactory;
};

};

#endif // PDF_INDIRECT_OBJECT_LIST_H
