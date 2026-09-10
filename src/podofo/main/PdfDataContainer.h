// SPDX-FileCopyrightText: 2006 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef PDF_CONTAINER_DATATYPE_H
#define PDF_CONTAINER_DATATYPE_H

#include "PdfBaseDataTypes.h"
#include "PdfObject.h"

namespace PoDoFo {

class PdfDocument;

/// A PdfDataProvider object with a PdfObject owner, specialized
/// in holding objects
class PODOFO_API PdfDataContainer : public PdfDataProvider<PdfDataContainer>
{
    friend class PdfObject;
    friend class PdfArray;
    friend class PdfDictionary;

private:
    /// Create a new PdfDataOwnedType
    /// Can only be called by subclasses
    /// @remarks We don't define copy/move constructor as the
    /// the owner is not copied/moved
    PdfDataContainer();

public:
    virtual ~PdfDataContainer();

    /// @returns a pointer to a PdfObject that is the
    ///           owner of this data type.
    ///           Might be nullptr if the data type has no owner.
    inline const PdfObject* GetOwner() const { return m_Owner; }
    inline PdfObject* GetOwner() { return m_Owner; }

    /// Write the complete datatype to a file.
    /// @param stream write the object to this device
    /// @param writeMode additional options for writing this object
    /// @param encrypt an encryption object which is used to encrypt this object
    ///                  or nullptr to not encrypt this object
    virtual void Write(OutputStream& stream, PdfWriteFlags writeMode,
        const PdfStatefulEncrypt* encrypt, charbuff& buffer) const = 0;

    /// @returns the document of the owner object, or nullptr if it has no owner
    /// @remarks It's stored here so contained objects can recover it without
    /// keeping a copy of their own
    PdfDocument* GetDocument() const { return m_Document; }

protected:
    virtual void resetDirty() = 0;
    PdfObject* GetIndirectObject(const PdfReference& reference) const;

    [[deprecated("Use GetDocument() instead")]]
    PdfDocument* GetObjectDocument() const { return m_Document; }
    void SetDirty();
    bool IsIndirectReferenceAllowed(const PdfObject& obj);
    virtual void setChildrenParent() = 0;
    void AssertMutable() const;

private:
    /// Set the owner object, along with the document it belongs to
    /// @remarks The document is supplied by the caller, which always knows it
    /// already, so it's not read back from the owner
    void SetOwner(PdfObject& owner, PdfDocument* document);

    /// Set the owner without visiting the children, which is enough when the
    /// owner was just relocated: the children stay attached to this container,
    /// which didn't move itself
    void SetOwnerShallow(PdfObject& owner) { m_Owner = &owner; }

    void ResetDirty();

private:
    PdfObject* m_Owner;
    PdfDocument* m_Document;
};

class PODOFO_API PdfIndirectIterableBase
{
    template <typename TObject, typename TListIterator>
    friend class PdfArrayIndirectIterableBase;

    template <typename TObject, typename TMapIterator>
    friend class PdfDictionaryIndirectIterableBase;

private:
    PdfIndirectIterableBase();

    PdfIndirectIterableBase(PdfDataContainer& container);

protected:
    static PdfObject* GetObject(const PdfIndirectObjectList& list, const PdfReference& ref);

    PdfIndirectObjectList* GetObjects() const { return m_Objects; }

private:
    PdfIndirectObjectList* m_Objects;
};

}

#endif // PDF_CONTAINER_DATATYPE_H
