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
     */
    PdfDestination( const PdfPage* pPage );

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
    bool         m_bIsEmpty;
    bool         m_bIsAction;
    PdfArray     m_array;
    PdfReference m_action;
};

};

#endif // _PDF_DESTINATION_H_

