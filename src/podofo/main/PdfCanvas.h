/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_CANVAS_H
#define PDF_CANVAS_H

#include "PdfDeclarations.h"
#include "PdfResources.h"
#include "PdfArray.h"

namespace PoDoFo {

class PdfDictionary;
class PdfObject;
class PdfRect;
class PdfColor;

enum class PdfStreamAppendFlags
{
    None = 0,
    Prepend = 1,
    NoSaveRestorePrior = 2
};

/** A interface that provides the necessary features 
 *  for a painter to draw onto a PdfObject.
 */
class PODOFO_API PdfCanvas
{
public:
    /** Virtual destructor
     *  to avoid compiler warnings
     */
    virtual ~PdfCanvas();

    /** Get access to the contents object of this page.
     *  If you want to draw onto the page, you have to add
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    const PdfObject* GetContentsObject() const;
    PdfObject* GetContentsObject();

    /** Get access an object that you can use to ADD drawing to.
     *  If you want to draw onto the page, you have to add
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    virtual PdfObjectStream& GetStreamForAppending(PdfStreamAppendFlags flags) = 0;

    /** Get an element from the pages resources dictionary,
     *  using a type (category) and a key.
     *
     *  \param type the type of resource to fetch (e.g. /Font, or /XObject)
     *  \param key the key of the resource
     *
     *  \returns the object of the resource or nullptr if it was not found
     */
    PdfObject* GetFromResources(const std::string_view& type, const std::string_view& key);
    const PdfObject* GetFromResources(const std::string_view& type, const std::string_view& key) const;

    /** Get the resource object of this page.
     * \returns a resources object
     */
    PdfResources* GetResources();
    const PdfResources* GetResources() const;

    PdfElement& GetElement();
    const PdfElement& GetElement() const;

    /** Get or create the resource object of this page.
     * \returns a resources object
     */
    virtual PdfResources& GetOrCreateResources() = 0;

    /** Get the current canvas size in PDF Units
     *  \returns a PdfRect containing the page size available for drawing
     */
    virtual PdfRect GetRect() const = 0;

    /** Get the current canvas rotation
     * \param teta counterclockwise rotation in radians
     * \returns true if the canvas has a rotation
     */
    virtual bool HasRotation(double& teta) const = 0;

    /** Get a copy of procset PdfArray.
     *  \returns a procset PdfArray
     */
    static PdfArray GetProcSet();

protected:
    virtual PdfObject* getContentsObject() const = 0;
    virtual PdfResources* getResources() const = 0;
    virtual PdfElement& getElement() const = 0;

private:
    PdfObject* getFromResources(const std::string_view& type, const std::string_view& key) const;
};

};

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfStreamAppendFlags);

#endif // PDF_CANVAS_H
