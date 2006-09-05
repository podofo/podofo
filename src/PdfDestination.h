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
 *  \see PdfOutlineItem \see PdfAnnotation \see PdfDocument
 */
class PdfDestination {
 public:

	 /** Create an empty destination - points to nowhere
	 */
	 PdfDestination( PdfVecObjects* pParent );

    /** Create a new PdfDestination from an existing PdfObject (such as loaded from a doc)
     *  \param pObject the object to construct from 
     */
	 PdfDestination( PdfObject* pObject );

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
    
    /** Copy an existing PdfDestination
     *  \param rhs copy this PdfDestination
     */
    PdfDestination( const PdfDestination & rhs );

    /** Copy an existing PdfDestination
     *  \param rhs copy this PdfDestination
     *  \returns this object
     */
    const PdfDestination & operator=( const PdfDestination & rhs );

	/** Get the page that this destination points to
	*  \returns the referenced PdfPage
	*/
	PdfPage* GetPage();

	/** Get access to the internal object
	*  \returns the internal PdfObject
	*/
	inline PdfObject* Object();

	/** Get access to the internal object
	*  This is an overloaded member function.
	*
	*  \returns the internal PdfObject
	*/
	inline const PdfObject* Object() const;

    /** Adds this destination to an dictionary.
     *  This method handles the all the complexities of making sure it's added correctly
     *
     *  \param dictionary the destination will be added to this dictionary
     */
    void AddToDictionary( PdfDictionary & dictionary ) const;

 private:
    static const long  s_lNumDestinations;
    static const char* s_names[];

	PdfArray	m_array;
    PdfObject*	m_pObject;

	/** Create an empty destination - NOT ALLOWED
	*/
	PdfDestination();

};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfObject* PdfDestination::Object()
{
	return m_pObject;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfObject* PdfDestination::Object() const
{
	return m_pObject;
}

};

#endif // _PDF_DESTINATION_H_

