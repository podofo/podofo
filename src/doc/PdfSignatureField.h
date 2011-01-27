/***************************************************************************
 *   Copyright (C) 2011 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                      by Petr Pytelka                                    *
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

#ifndef _PDF_SIGNATURE_FIELD_H_
#define _PDF_SIGNATURE_FIELD_H_

#include "PdfField.h"
#include "podofo/base/PdfDate.h"

namespace PoDoFo {


/** Signature field
 */
class PODOFO_DOC_API PdfSignatureField :public PdfField
{
protected:
    PdfObject*     m_pSignatureObj;

    void Init();
public:
    /** Create a new PdfSignatureField
     */
    PdfSignatureField( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc );

    /** Create space for signature
     *
     * \param signatureData String used to locate reserved space for signature.
     *   This string will be replaiced with signature.
     *
     * Structure of the PDF file - before signing:
     * <</ByteRange[ 0 1234567890 1234567890 1234567890]/Contents<signatureData>
     * Have to be replaiced with the following structure:
     * <</ByteRange[ 0 count pos count]/Contents<real signature ...0-padding>
     */
    void SetSignature(const PdfData &signatureData);

    /** Set reason of the signature
     *
     *  \param rsText the reason of signature
     */
    void SetSignatureReason(const PdfString & rsText);

	/** Date of signature
	 */
	void SetSignatureDate(const PdfDate &sigDate);
};

}

#endif
