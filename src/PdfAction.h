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
#include "PdfObject.h"

namespace PoDoFo {

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
    ePdfAction_GoTo,
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
class PdfAction : public PdfObject {
 public:
    PdfAction( unsigned int nObjectNo, unsigned int nGenerationNo );
    
    /** Initalize the PdfAction object
     *  \param eAction type of this action
     *
     *  \see EPdfAction
     */
    PdfError Init( EPdfAction eAction );


    /** Set the URI of an ePdfAction_URI
     *  \param sUri must be a correct URI as PdfString
     */
    void SetURI( const PdfString & sUri );

 protected:
    /** Convert an action enum to its string representation
     *  which can be written to the PDF file.
     *  \returns the string representation or NULL for unsupported annotation types
     */
    const char* ActionKey( EPdfAction eAnnot );

 private:
    EPdfAction m_type;
    

};

};

#endif // _PDF_ACTION_H_
