/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#ifndef _PDF_ACRO_FORM_H_
#define _PDF_ACRO_FORM_H_

#include "podofo/base/PdfDefines.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfDocument;

enum EPdfAcroFormDefaulAppearance {
  ePdfAcroFormDefaultAppearance_None, ///< Do not add a default appearrance
  ePdfAcroFormDefaultAppearance_BlackText12pt ///< Add a default appearance with Arial embedded and black text 12pt if no other DA key is present
};

class PODOFO_DOC_API PdfAcroForm : public PdfElement {
 public:

    /** Create a new PdfAcroForm dictionary object
     *  \param pDoc parent of this action
     *  \param eDefaultAppearance specifies if a default appearance should be added
     */
    PdfAcroForm( PdfDocument* pDoc, 
                 EPdfAcroFormDefaulAppearance eDefaultAppearance = ePdfAcroFormDefaultAppearance_BlackText12pt );

    /** Create a PdfAcroForm dictionary object from an existing PdfObject
     *  \param pDoc parent document
     *	\param pObject the object to create from
     *  \param eDefaultAppearance specifies if a default appearance should be added
     */
    PdfAcroForm( PdfDocument* pDoc, PdfObject* pObject,
                 EPdfAcroFormDefaulAppearance eDefaultAppearance = ePdfAcroFormDefaultAppearance_BlackText12pt );

    virtual ~PdfAcroForm() { }

    /** Get the document that is associated with this 
     *  acro forms dictionary.
     *
     *  \returns a valid pointer to the parent document
     */
    inline PdfDocument* GetDocument(); 

    /** Set the value of the NeedAppearances key in the interactive forms
     *  dictionary.
     * 
     *  \param bNeedAppearances A flag specifying whether to construct appearance streams
     *                          and appearance dictionaries for all widget annotations in
     *                          the document. Default value is false.
     */
    void SetNeedAppearances( bool bNeedAppearances );

    /** Retrieve the value of the NeedAppearances key in the interactive forms
     *  dictionary.
     *
     *  \returns value of the NeedAppearances key
     *
     *  \see SetNeedAppearances
     */
    bool GetNeedAppearances() const;

 private:
    /** Initialize this object
     *  with a default appearance
     *  \param eDefaultAppearance specifies if a default appearance should be added
     */
    void Init( EPdfAcroFormDefaulAppearance eDefaultAppearance );

 private:
    PdfDocument* m_pDocument;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfDocument* PdfAcroForm::GetDocument()
{
    return m_pDocument;
}

};

#endif // _PDF_ACRO_FORM_H_
