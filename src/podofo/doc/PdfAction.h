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

#ifndef _PDF_ACTION_H_
#define _PDF_ACTION_H_

#include "podofo/base/PdfDefines.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfObject;
class PdfString;
class PdfStreamedDocument;
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
enum EPdfAction {
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
    ePdfAction_RichMediaExecute,
    
    ePdfAction_Unknown = 0xff
};

/** An action that can be performed in a PDF document
 */
class PODOFO_DOC_API PdfAction : public PdfElement {

    friend class PdfAnnotation;

 public:
    /** Create a new PdfAction object
     *  \param eAction type of this action
     *  \param pParent parent of this action
     */
    PdfAction( EPdfAction eAction, PdfVecObjects* pParent );

    /** Create a new PdfAction object
     *  \param eAction type of this action
     *  \param pParent parent of this action
     */
    PdfAction( EPdfAction eAction, PdfDocument* pParent );

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

    void SetScript( const PdfString & sScript );

    PdfString GetScript() const;

    bool HasScript() const;
    
    /** Get the type of this action
     *  \returns the type of this action
     */
    inline EPdfAction GetType() const;

    /** Adds this action to an dictionary.
     *  This method handles the all the complexities of making sure it's added correctly
     *
     *  If this action is empty. Nothing will be added.
     *
     *  \param dictionary the action will be added to this dictionary
     */
    void AddToDictionary( PdfDictionary & dictionary ) const;

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
inline EPdfAction PdfAction::GetType() const
{
    return m_eType;
}

};

#endif // _PDF_ACTION_H_
