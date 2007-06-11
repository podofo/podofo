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
 ***************************************************************************/

#ifndef _PDF_ACRO_FORM_H_
#define _PDF_ACRO_FORM_H_

#include "PdfDefines.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfDocument;

class PODOFO_API PdfAcroForm : public PdfElement {
 public:
    /** Create a new PdfAcroForm dictionary object
     *  \param pParent parent of this action
     */
    PdfAcroForm( PdfDocument* pDoc );

    /** Create a PdfAcroForm dictionary object from an existing PdfObject
     *	\param pObject the object to create from
     *  \param pCatalog the Catalog dictionary of the owning PDF
     */
    PdfAcroForm( PdfDocument* pDoc, PdfObject* pObject );

    virtual ~PdfAcroForm() { }

    /** Get the document that is associated with this 
     *  acro forms dictionary.
     *
     *  \returns a valid pointer to the parent document
     */
    inline PdfDocument* GetDocument(); 

 private:
    /** Initialize this object
     *  with a default appearance
     */
    void Init();

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
