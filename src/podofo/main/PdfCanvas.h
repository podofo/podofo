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
#include <podofo/auxiliary/Corners.h>

namespace PoDoFo {

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

    /** Get access an object that you can use to ADD drawing to
     *  \returns a contents stream object
     */
    virtual PdfObjectStream& GetOrCreateContentsStream(PdfStreamAppendFlags flags) = 0;

    /** Reset the contents object and create a new stream for appending
     */ 
    virtual PdfObjectStream& ResetContentsStream() = 0;

    charbuff GetContentsCopy() const;

    /**
     * \remarks It clears the buffer before copying
     */
    void CopyContentsTo(charbuff& buffer) const;
    virtual void CopyContentsTo(OutputStream& stream) const = 0;

    /** Get the resource object of this page.
     * \returns a resources object
     */
    PdfResources* GetResources();
    const PdfResources* GetResources() const;

    PdfDictionaryElement& GetElement();
    const PdfDictionaryElement& GetElement() const;

    /** Get or create the resource object of this page.
     * \returns a resources object
     */
    virtual PdfResources& GetOrCreateResources() = 0;

    /** Ensure resources initialized on this canvas
     */
    void EnsureResourcesCreated();

    /** Get the current canvas size in PDF Units
     *  \returns a Rect containing the page size available for drawing
     */
    virtual Corners GetRectRaw() const = 0;

    /** Try getting the current canvas rotation
     * \param teta counterclockwise rotation in radians
     * \returns true if the canvas has a rotation
     */
    virtual bool TryGetRotationRadians(double& teta) const = 0;

protected:
    virtual PdfObject* getContentsObject() = 0;
    virtual PdfResources* getResources() = 0;
    virtual PdfDictionaryElement& getElement() = 0;
};

};

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfStreamAppendFlags);

#endif // PDF_CANVAS_H
