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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_DESTINATION_H_
#define _PDF_DESTINATION_H_

#include "podofo/base/PdfDefines.h"

#include "podofo/base/PdfArray.h"
#include "podofo/base/PdfRect.h"
#include "podofo/base/PdfReference.h"

namespace PoDoFo {

class PdfAction;
class PdfPage;
class PdfRect;

enum EPdfDestinationFit {
    ePdfDestinationFit_Fit,
    ePdfDestinationFit_FitH,
    ePdfDestinationFit_FitV,
    ePdfDestinationFit_FitB,
    ePdfDestinationFit_FitBH,
    ePdfDestinationFit_FitBV,

    ePdfDestinationFit_Unknown = 0xFF
};

/** Destination type, as per 12.3.2.2 of the Pdf spec.
 *
 *  (see table 151 in the pdf spec)
 */
enum EPdfDestinationType {
  ePdfDestinationType_XYZ,
  ePdfDestinationType_Fit,
  ePdfDestinationType_FitH,
  ePdfDestinationType_FitV,
  ePdfDestinationType_FitR,
  ePdfDestinationType_FitB,
  ePdfDestinationType_FitBH,
  ePdfDestinationType_FitBV,
  
  ePdfDestinationType_Unknown = 0xFF
};

/** A destination in a PDF file.
 *  A destination can either be a page or an action.
 *
 *  \see PdfOutlineItem \see PdfAnnotation \see PdfDocument
 */
class PODOFO_DOC_API PdfDestination {
 public:

    /** Create an empty destination - points to nowhere
     */
    PdfDestination( PdfVecObjects* pParent );

    /** Create a new PdfDestination from an existing PdfObject (such as loaded from a doc)
     *  \param pObject the object to construct from 
     *  \param pDocument a PDF document owning this destination, needed to resolve pages
     */
    PdfDestination( PdfObject* pObject, PdfDocument* pDocument );

    /** Create a new PdfDestination from an existing PdfObject (such as loaded from a doc)
     *  \param pObject the object to construct from 
     *  \param pVecObjects a PdfVecObjects owning this destination, needed to resolve pages
     */
    PdfDestination( PdfObject* pObject, PdfVecObjects* pVecObjects );

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
     *  \param dValue value which is a required argument for the selected fit mode
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
     *  Requires that this PdfDestination was somehow
     *  created by or from a PdfDocument. Won't work otherwise.
     *  \param pDoc a PDF document owning this destination, needed to resolve pages
     * 
     *  \returns the referenced PdfPage
     */
    PdfPage* GetPage( PdfDocument* pDoc ); 

    /** Get the page that this destination points to
     *  Requires that this PdfDestination was somehow
     *  created by or from a PdfDocument. Won't work otherwise.
     *  \param pVecObjects a PdfVecObjects owning this destination, needed to resolve pages
     * 
     *  \returns the referenced PdfPage
     */
    PdfPage* GetPage( PdfVecObjects* pVecObjects );
    
    /** Get the destination fit type
     *
     *  \returns the fit type
     */
    inline EPdfDestinationType GetType() const;
    
    /** Get the destination zoom 
     *  Destination must be of type XYZ
     *  otherwise exception is thrown.
     *
     *  \returns the zoom
     */
    inline double GetZoom() const;
    
    /** Get the destination rect 
     *  Destination must be of type FirR
     *  otherwise exception is thrown
     *
     *  \returns the destination rect
     */
    inline PdfRect GetRect() const;
     
    /** Get the destination Top position
     *  Destination must be of type XYZ, FitH, FitR, FitBH
     *  otherwise exception is thrown.
     * 
     * \returns the Top position
     */
    inline double GetTop() const; 
    
    /** Get the destination Left position 
     *  Destination must be of type XYZ, FitV or FitR
     *  otherwise exception is thrown.
     * 
     * \returns the Left position
     */
    inline double GetLeft() const;
    
    /** Get the destination Value 
     *  Destination must be of type FitH, FitV
     *  or FitBH, otherwise exception is thrown 
     * 
     *  \returns the destination Value
     */
    inline double GetDValue() const;
    
    /** Get access to the internal object
     *
     *  \returns the internal PdfObject
     */
    inline PdfObject* GetObject();
    
    /** Get access to the internal object
     *  This is an overloaded member function.
     *
     *  \returns the internal PdfObject
     */
    inline const PdfObject* GetObject() const;

    /** Get access to the internal array
     *  \returns the internal PdfArray
     */
    inline PdfArray &GetArray();
    
    /** Get access to the internal array
     *  This is an overloaded member function.
     *
     *  \returns the internal PdfArray
     */
    inline const PdfArray &GetArray() const;

    
    /** Adds this destination to an dictionary.
     *  This method handles the all the complexities of making sure it's added correctly
     *
     *  If this destination is empty. Nothing will be added.
     *
     *  \param dictionary the destination will be added to this dictionary
     */
    void AddToDictionary( PdfDictionary & dictionary ) const;

 private:
    /** Initialize a new PdfDestination from an existing PdfObject (such as loaded from a doc)
     *  and a document.
     *
     *  \param pObject the object to construct from 
     *  \param pDoc a PDF document owning this destination, needed to resolve pages
     */
    void Init( PdfObject* pObject, PdfDocument* pDocument );

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
inline PdfObject* PdfDestination::GetObject()
{
    return m_pObject;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfObject* PdfDestination::GetObject() const
{
    return m_pObject;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfArray &PdfDestination::GetArray()
{
    return m_array;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfArray &PdfDestination::GetArray() const
{
    return m_array;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline EPdfDestinationType PdfDestination::GetType() const 
{
    if ( !m_array.size() ) 
        return ePdfDestinationType_Unknown;  
    
    PdfName tp = m_array[1].GetName();
    
    if ( tp == PdfName("XYZ") ) return ePdfDestinationType_XYZ;
    if ( tp == PdfName("Fit") ) return ePdfDestinationType_Fit;
    if ( tp == PdfName("FitH") ) return ePdfDestinationType_FitH;
    if ( tp == PdfName("FitV") ) return ePdfDestinationType_FitV;   
    if ( tp == PdfName("FitR") ) return ePdfDestinationType_FitR; 
    if ( tp == PdfName("FitB") ) return ePdfDestinationType_FitB; 
    if ( tp == PdfName("FitBH") ) return ePdfDestinationType_FitBH; 
    if ( tp == PdfName("FitBV") ) return ePdfDestinationType_FitBV; 
    
    return ePdfDestinationType_Unknown; 
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline double PdfDestination::GetDValue() const 
{
    EPdfDestinationType tp = GetType();
    
    if ( tp != ePdfDestinationType_FitH
         && tp != ePdfDestinationType_FitV
         && tp != ePdfDestinationType_FitBH )
    {
        PODOFO_RAISE_ERROR( ePdfError_WrongDestinationType );
    }
    
    return m_array[2].GetReal();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline double PdfDestination::GetLeft() const
{
    EPdfDestinationType tp = GetType();
    
    if ( tp != ePdfDestinationType_FitV
         && tp != ePdfDestinationType_XYZ
         && tp != ePdfDestinationType_FitR )
    {
        PODOFO_RAISE_ERROR( ePdfError_WrongDestinationType );
    }
    
    return m_array[2].GetReal();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfRect PdfDestination::GetRect() const
{
    if ( GetType() != ePdfDestinationType_FitR )
    {
        PODOFO_RAISE_ERROR( ePdfError_WrongDestinationType );
    }
    
    return PdfRect(m_array[2].GetReal(), m_array[3].GetReal(),
                   m_array[4].GetReal(), m_array[5].GetReal());
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline double PdfDestination::GetTop() const
{
    EPdfDestinationType tp = GetType();
    
    switch (tp) 
    { 
        case ePdfDestinationType_XYZ:
            return m_array[3].GetReal();
        case ePdfDestinationType_FitH:
        case ePdfDestinationType_FitBH:
            return m_array[2].GetReal();
        case ePdfDestinationType_FitR:
            return m_array[5].GetReal();
        case ePdfDestinationType_Fit:
        case ePdfDestinationType_FitV:
        case ePdfDestinationType_FitB:
        case ePdfDestinationType_FitBV:
        case ePdfDestinationType_Unknown:
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_WrongDestinationType );
        }
    };
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline double PdfDestination::GetZoom() const
{
    if ( GetType() != ePdfDestinationType_XYZ )
    {
        PODOFO_RAISE_ERROR( ePdfError_WrongDestinationType );
    }
  
    return m_array[4].GetReal();
}

};




#endif // _PDF_DESTINATION_H_

