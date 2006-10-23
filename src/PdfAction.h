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

#ifndef _PDF_ACTION_H_
#define _PDF_ACTION_H_

#include "PdfDefines.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfObject;
class PdfString;
class PdfVecObjects;

/** The type of the action.
 *  PDF supports different action types, each of 
 *  them has different keys and propeties.
 *  
 *  Not all action types listed here are supported yet.
 *
 *  Please make also sure that the action type you use is
 *  supported by the PDF version you are using.
 */
typedef enum EPdfAction {
    ePdfAction_GoTo = 0,
    ePdfAction_GoToR,
    ePdfAction_GoToE,
    ePdfAction_Launch,    
    ePdfAction_Thread,
    ePdfAction_URI,
    ePdfAction_Sound,
    ePdfAction_Movie,
    ePdfAction_Hide,
    ePdfAction_Named,
    ePdfAction_SubmitForm,
    ePdfAction_ResetForm,
    ePdfAction_ImportData,
    ePdfAction_JavaScript,
    ePdfAction_SetOCGState,
    ePdfAction_Rendition,
    ePdfAction_Trans,
    ePdfAction_GoTo3DView,
    
    ePdfAction_Unknown = 0xff
};

/** An action that can be performed in a PDF document
 */
class PODOFO_API PdfAction : public PdfElement {

    friend class PdfAnnotation;

 public:
    /** Create a new PdfAction object
     *  \param eAction type of this action
     *  \param pParent parent of this action
     */
    PdfAction( EPdfAction eAction, PdfVecObjects* pParent );

    virtual ~PdfAction() { }

    /** Create a PdfAction object from an existing 
     *  PdfObject
     */
    PdfAction( PdfObject* pObject );

    /** Set the URI of an ePdfAction_URI
     *  \param sUri must be a correct URI as PdfString
     */
    void SetURI( const PdfString & sUri );

    /** Get the URI of an ePdfAction_URI
     *  \returns an URI
     */
    PdfString GetURI() const;

    /** 
     *  \returns true if this action has an URI
     */
    bool HasURI() const;

    /** Get the type of this action
     *  \returns the type of this action
     */
    inline const EPdfAction GetType() const;

 private:
    PdfAction( const PdfAction & rhs );

 private:

    static const long  s_lNumActions;
    static const char* s_names[];

 private:
    EPdfAction m_eType;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const EPdfAction PdfAction::GetType() const
{
    return m_eType;
}

};

#endif // _PDF_ACTION_H_
