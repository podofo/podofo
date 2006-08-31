/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _PDF_DESTINATION_H_
#define _PDF_DESTINATION_H_

#include "PdfDefines.h"

#include "PdfArray.h"
#include "PdfReference.h"

namespace PoDoFo {

class PdfAction;
class PdfPage;
class PdfRect;

typedef enum EPdfDestinationFit {
    ePdfDestinationFit_Fit,
    ePdfDestinationFit_FitH,
    ePdfDestinationFit_FitV,
    ePdfDestinationFit_FitB,
    ePdfDestinationFit_FitBH,
    ePdfDestinationFit_FitBV,

    ePdfDestinationFit_Unknown = 0xFF
};

/** A destination in a PDF file.
 *  A destination can either be a page or an action.
 *
 *  \see PdfOutlineItem \see PdfAnnotation 
 */
class PdfDestination {
 public:

    /** Create an empty destination
     *  i.e. an destination, pointing to nowhere
     */
    PdfDestination();

    /** Create a new PdfDestination with a page as destination
     *  \param pPage a page which is the destination 
     *  \param eFit fit mode for the page. Must be ePdfDestinationFit_Fit or ePdfDestinationFit_FitB
     */
    PdfDestination( const PdfPage* pPage, EPdfDestinationFit eFit = ePdfDestinationFit_Fit );

    /** Create a destination to a page with its contents magnified to fit into the given rectangle
     *  \param pPage a page which is the destination 
     *  \param rRect magnify the page so that the contents of the rectangle are visible
     */
    PdfDestination( const PdfPage* pPage, const PdfRect & rRect );

    /** Create a new destination to a page with specified left 
     *  and top coordinates and a zoom factor.
     *  \param pPage a page which is the destination 
     *  \param dLeft left coordinate
     *  \param dTop  top coordinate
     *  \param dZoom zoom factor in the viewer
     */
    PdfDestination( const PdfPage* pPage, double dLeft, double dTop, double dZoom );

    /** Create a new destination to a page.
     *  \param pPage a page which is the destination 
     *  \param eFit fit mode for the Page. Allowed values are ePdfDestinationFit_FitH,
     *              ePdfDestinationFit_FitV, ePdfDestinationFit_FitBH, ePdfDestinationFit_FitBV
     *  \param top or left value to focus
     */
    PdfDestination( const PdfPage* pPage, EPdfDestinationFit eFit, double dValue );

    /** Create a new PdfDestination which triggers an action
     *  \param pAction an action
     */
    PdfDestination( const PdfAction* pAction );
    
    /** Copy an existing PdfDestination
     *  \param rhs copy this PdfDestination
     */
    PdfDestination( const PdfDestination & rhs );

    /** Copy an existing PdfDestination
     *  \param rhs copy this PdfDestination
     *  \returns this object
     */
    const PdfDestination & operator=( const PdfDestination & rhs );

    /** Adds this destination to an dictionary.
     *  This method handles the internal difference of actions and pages
     *  in the PDF file format correctly.
     *
     *  \param dictionary the destination will be added to this dictionary
     */
    void AddToDictionary( PdfDictionary & dictionary ) const;

 private:
    static const long  s_lNumDestinations;
    static const char* s_names[];

    bool         m_bIsAction;

    PdfArray     m_array;
    PdfReference m_action;
};

};

#endif // _PDF_DESTINATION_H_

