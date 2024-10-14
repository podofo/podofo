/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_REFERENCE_H
#define PDF_REFERENCE_H

#include "PdfBaseDataTypes.h"

namespace PoDoFo {

class OutputStream;

/**
 * A reference is a pointer to a object in the PDF file of the form
 * "4 0 R", where 4 is the object number and 0 is the generation number.
 * Every object in the PDF file can be identified this way.
 *
 * This class is a indirect reference in a PDF file.
 */
class PODOFO_API PdfReference final : private PdfDataMember, public PdfDataProvider<PdfReference>
{
public:
    /**
     * Create a PdfReference with object number and generation number
     * initialized to 0.
     */
    PdfReference();

    /**
     * Create a PdfReference to an object with a given object and generation number.
     *
     * \param nObjectNo the object number
     * \param nGenerationNo the generation number
     */
    PdfReference(const uint32_t objectNo, const uint16_t generationNo);

    /**
     * Create a copy of an existing PdfReference.
     *
     * \param rhs the object to copy
     */
    PdfReference(const PdfReference& rhs) = default;

    /**
      * Assign the value of another object to this PdfReference.
      *
      * \param rhs the object to copy
      */
    PdfReference& operator=(const PdfReference& rhs) = default;

    void Write(OutputStream& stream, PdfWriteFlags flags, const PdfStatefulEncrypt* encrypt, charbuff& buffer) const;

    /**
     * Compare to PdfReference objects.
     * \returns true if both reference the same object
     */
    bool operator==(const PdfReference& rhs) const;

    /**
     * Compare to PdfReference objects.
     * \returns false if both reference the same object
     */
    bool operator!=(const PdfReference& rhs) const;

    /**
     * Compare to PdfReference objects.
     * \returns true if this reference has a smaller object and generation number
     */
    bool operator<(const PdfReference& rhs) const;

    /** Allows to check if a reference points to an indirect
     *  object.
     *
     *  A reference is indirect if object number and generation
     *  number are both not equal 0.
     *
     *  \returns true if this reference is the reference of
     *           an indirect object.
     */
    bool IsIndirect() const;

public:
    /** Set the object number of this object
     *  \param o the new object number
     */
    inline void SetObjectNumber(uint32_t o) { m_ObjectNo = o; }

    /** Get the object number.
     *  \returns the object number of this PdfReference
     */
    inline uint32_t ObjectNumber() const { return m_ObjectNo; }

    /** Set the generation number of this object
     *  \param g the new generation number
     */
    inline void SetGenerationNumber(const uint16_t g) { m_GenerationNo = g; }

    /** Get the generation number.
     *  \returns the generation number of this PdfReference
     */
    inline uint16_t GenerationNumber() const { return m_GenerationNo; }

private:
    uint16_t m_GenerationNo;
    uint32_t m_ObjectNo;
};

};

namespace std
{
    /** Overload hasher for PdfReference
     */
    template<>
    struct hash<PoDoFo::PdfReference>
    {
        std::size_t operator()(const PoDoFo::PdfReference& ref) const noexcept
        {
            return ref.ObjectNumber() ^ (ref.GenerationNumber() << 16);
        }
    };
}

#endif // PDF_REFERENCE_H
