/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_ELEMENT_H
#define PDF_ELEMENT_H

#include "PdfObject.h"

namespace PoDoFo {

/** PdfElement is a common base class for all elements
 *  in a PDF file. For example pages, action and annotations.
 *
 *  Every PDF element has one PdfObject and provides an easier
 *  interface to modify the contents of the dictionary. 
 *  
 *  A PdfElement base class can be created from an existing PdfObject
 *  or created from scratch. In the later case, the PdfElement creates
 *  a PdfObject and adds it to a vector of objects.
 *
 *  A PdfElement cannot be created directly. Use one
 *  of the subclasses which implement real functionality.
 *
 *  \see PdfPage \see PdfAction \see PdfAnnotation
 */
class PODOFO_API PdfElement
{
public:

    virtual ~PdfElement();

    /** Get access to the internal object
     *  \returns the internal PdfObject
     */
    inline PdfObject& GetObject() { return *m_Object; }

    /** Get access to the internal object
     *  This is an overloaded member function.
     *
     *  \returns the internal PdfObject
     */
    inline const PdfObject& GetObject() const { return *m_Object; }

    PdfDocument& GetDocument() const;

protected:
    PdfElement(PdfObject& obj);

    /** Create a PdfElement from an existing PdfObject
     *  The object might be of any data type,
     *  PdfElement will throw an exception if the PdfObject
     *  if not of the same datatype as the expected one.
     *  This is necessary in rare cases. E.g. in PdfContents.
     *
     *  \param obj refereence to the PdfObject that is modified
     *                 by this PdfElement
     *  \param expectedDataType the expected datatype of this object
     */
    PdfElement(PdfObject& obj, PdfDataType expectedDataType);

    PdfElement(const PdfElement& element) = default;

private:
    PdfElement& operator=(const PdfElement& element) = delete;

private:
    PdfObject* m_Object;
};

class PODOFO_API PdfDictionaryElement : public PdfElement
{
protected:
    /** Creates a new PdfDictionaryElement
     *  \param parent Add a newly created object to this document.
     *  \param type type entry of the elements object
     *  \param subtype optional value of the /SubType key of the object
     */
    PdfDictionaryElement(PdfDocument& parent,
        const PdfName& type = PdfName::Null,
        const PdfName& subtype = PdfName::Null);

    /** Create a PdfDictionaryElement from an existing PdfObject
     *  The object must be a dictionary.
     *
     *  \param obj pointer to the PdfObject that is modified
     *                 by this PdfElement
     */
    PdfDictionaryElement(PdfObject& obj);

    PdfDictionaryElement(const PdfDictionaryElement& element) = default;

public:
    PdfDictionary& GetDictionary();
    const PdfDictionary& GetDictionary() const;
};

class PODOFO_API PdfArrayElement : public PdfElement
{
protected:
    /** Creates a new PdfArrayElement
     *  \param parent Add a newly created object to this document.
     */
    PdfArrayElement(PdfDocument& parent);

    /** Create a PdfArrayElement from an existing PdfObject
     *  The object must be a dictionary.
     *
     *  \param obj reference to the PdfObject that is modified
     *      by this PdfArrayElement
     */
    PdfArrayElement(PdfObject& obj);

    PdfArrayElement(const PdfArrayElement& element) = default;

public:
    PdfArray& GetArray();
    const PdfArray& GetArray() const;
};

};

#endif // PDF_ELEMENT_H
