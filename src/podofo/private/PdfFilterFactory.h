/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FILTER_FACTORY_H
#define PDF_FILTER_FACTORY_H

#include "PdfFilter.h"

#include <podofo/auxiliary/InputStream.h>

namespace PoDoFo {

class PdfObject;

/** A factory to create a filter object for a filter type (as GetType() gives)
 *  from the PdfFilterType enum.
 *  All filters should be created using this factory.
 */
class PdfFilterFactory final
{
public:
    /** Create a filter from an enum.
     *
     *  Ownership is transferred to the caller, who should let the unique_ptr
     *  the filter is returned in take care of freeing it when they're done
     *  with it.
     *
     *  \param filterType return value of GetType() for filter to be created
     *
     *  \returns a new PdfFilter allocated using new, or nullptr if no
     *           filter is available for this type.
     */
    static std::unique_ptr<PdfFilter> Create(PdfFilterType filterType);
    static bool TryCreate(PdfFilterType filterType, std::unique_ptr<PdfFilter>& filter);

    /** Create an OutputStream that applies a list of filters
     *  on all data written to it.
     *
     *  \param filters a list of filters
     *  \param stream write all data to this OutputStream after it has been
     *         encoded
     *  \returns a new OutputStream that has to be deleted by the caller.
     *
     *  \see PdfFilterFactory::CreateFilterList
     */
    static std::unique_ptr<OutputStream> CreateEncodeStream(std::shared_ptr<OutputStream> stream,
        const PdfFilterList& filters);

    /** Create an InputStream that applies a list of filters
     *  on all data written to it.
     *
     *  \param filters a list of filters
     *  \param stream write all data to this OutputStream
     *         after it has been decoded.
     *  \param decodeParms list of additional parameters for stream decoding
     *  \returns a new OutputStream that has to be deleted by the caller.
     *
     *  \see PdfFilterFactory::CreateFilterList
     */
    static std::unique_ptr<InputStream> CreateDecodeStream(std::shared_ptr<InputStream> stream,
        const PdfFilterList& filters, const std::vector<const PdfDictionary*>& decodeParms);

    /** The passed PdfObject has to be a dictionary with a Filters key,
     *  a (possibly empty) array of filter names or a filter name.
     *
     *  \param filtersObj must define a filter or list of filters
     *
     *  \returns a list of filters
     */
    static PdfFilterList CreateFilterList(const PdfObject& filtersObj);

private:
    static void addFilterTo(PdfFilterList& filters, const std::string_view& filterName);

private:
    PdfFilterFactory() = delete;
};

}

#endif // PDF_FILTER_FACTORY_H
