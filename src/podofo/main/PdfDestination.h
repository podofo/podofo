/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_DESTINATION_H
#define PDF_DESTINATION_H

#include "PdfDeclarations.h"

#include <podofo/auxiliary/Rect.h>
#include "PdfElement.h"

namespace PoDoFo {

class PdfPage;

enum class PdfDestinationFit : uint8_t
{
    Unknown = 0,
    Fit,
    FitH,
    FitV,
    FitB,
    FitBH,
    FitBV,
};

/** Destination type, as per 12.3.2.2 of the Pdf spec.
 *
 *  (see table 151 in the pdf spec)
 */
enum class PdfDestinationType : uint8_t
{
    Unknown = 0,
    XYZ,
    Fit,
    FitH,
    FitV,
    FitR,
    FitB,
    FitBH,
    FitBV,
};

/** A destination in a PDF file.
 *  A destination can either be a page or an action.
 *
 *  \see PdfOutlineItem \see PdfAnnotation \see PdfDocument
 */
class PODOFO_API PdfDestination final : public PdfArrayElement
{
    friend class PdfDocument;
    friend class PdfAnnotationLink;
    friend class PdfOutlineItem;
    friend class PdfNameTreeBase;

private:
    /** Create a new PdfDestination from an existing PdfObject (such as loaded from a doc)
     *  \param obj the object to construct from
     */
    PdfDestination(PdfObject& obj);

    /** Create an empty destination - points to nowhere
     */
    PdfDestination(PdfDocument& doc);

    PdfDestination(const PdfDestination&) = default;

public:
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfDestination>& dest);

    /** Create a new PdfDestination with a page as destination
     *  \param page a page which is the destination
     *  \param fit fit mode for the page. Must be PdfDestinationFit::Fit or PdfDestinationFit::FitB
     */
    void SetDestination(const PdfPage& page, PdfDestinationFit fit = PdfDestinationFit::Fit);

    /** Create a destination to a page with its contents magnified to fit into the given rectangle
     *  \param page a page which is the destination
     *  \param rect magnify the page so that the contents of the rectangle are visible
     */
    void SetDestination(const PdfPage& page, const Rect& rect);

    /** Create a new destination to a page with specified left
     *  and top coordinates and a zoom factor.
     *  \param page a page which is the destination
     *  \param left left coordinate
     *  \param top top coordinate
     *  \param zoom zoom factor in the viewer
     */
    void SetDestination(const PdfPage& page, double left, double top, double zoom);

    /** Create a new destination to a page.
     *  \param page a page which is the destination
     *  \param fit fit mode for the Page. Allowed values are PdfDestinationFit::FitH,
     *              PdfDestinationFit::FitV, PdfDestinationFit::FitBH, PdfDestinationFit::FitBV
     *  \param value value which is a required argument for the selected fit mode
     */
    void SetDestination(const PdfPage& page, PdfDestinationFit fit, double value);

    /** Get the page that this destination points to
     *  Requires that this PdfDestination was somehow
     *  created by or from a PdfDocument. Won't work otherwise.
     *
     *  \returns the referenced PdfPage
     */
    PdfPage* GetPage();

    /** Get the destination fit type
     *
     *  \returns the fit type
     */
    PdfDestinationType GetType() const;

    /** Get the destination zoom
     *  Destination must be of type XYZ
     *  otherwise exception is thrown.
     *
     *  \returns the zoom
     */
    double GetZoom() const;

    /** Get the destination rect
     *  Destination must be of type FirR
     *  otherwise exception is thrown
     *
     *  \returns the destination rect
     */
    Rect GetRect() const;

    /** Get the destination Top position
     *  Destination must be of type XYZ, FitH, FitR, FitBH
     *  otherwise exception is thrown.
     *
     * \returns the Top position
     */
    double GetTop() const;

    /** Get the destination Left position
     *  Destination must be of type XYZ, FitV or FitR
     *  otherwise exception is thrown.
     *
     * \returns the Left position
     */
    double GetLeft() const;

    /** Get the destination Value
     *  Destination must be of type FitH, FitV
     *  or FitBH, otherwise exception is thrown
     *
     *  \returns the destination Value
     */
    double GetDValue() const;

private:
    /** Adds this destination to an dictionary.
     *  This method handles the all the complexities of making sure it's added correctly
     *
     *  If this destination is empty. Nothing will be added.
     *
     *  \param dictionary the destination will be added to this dictionary
     */
    void AddToDictionary(PdfDictionary& dictionary) const;
};

};

#endif // PDF_DESTINATION_H

