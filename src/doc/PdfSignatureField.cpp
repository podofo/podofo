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

#include "PdfSignatureField.h"
#include "../base/PdfDictionary.h"
#include "../base/PdfData.h"

#include <string.h>

namespace PoDoFo {

PdfSignatureField::PdfSignatureField( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
	:PdfField(PoDoFo::ePdfField_Signature, pPage, rRect, pDoc)
{
    m_pSignatureObj = NULL;
    Init();
}

void PdfSignatureField::Init()
{
    m_pSignatureObj = this->GetFieldObject()->GetOwner()->CreateObject( "Sig" );
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    GetFieldObject()->GetDictionary().AddKey("V", m_pSignatureObj->Reference());

    PdfDictionary &dict = m_pSignatureObj->GetDictionary();

    dict.AddKey(PdfName::KeyFilter, PdfName("Adobe.PPKLite") );
    dict.AddKey("SubFilter", PdfName("adbe.pkcs7.detached") );
}

void PdfSignatureField::SetSignatureReason(const PdfString & rsText)
{
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    if(m_pSignatureObj->GetDictionary().HasKey(PdfName("Reason")))
    {
        m_pSignatureObj->GetDictionary().RemoveKey(PdfName("Reason"));
    }
    m_pSignatureObj->GetDictionary().AddKey(PdfName("Reason"), rsText);
}

void PdfSignatureField::SetSignatureDate(const PdfDate &sigDate)
{
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    if(m_pSignatureObj->GetDictionary().HasKey(PdfName("M")))
    {
        m_pSignatureObj->GetDictionary().RemoveKey(PdfName("M"));
    }
	PdfString sDate;
	sigDate.ToString(sDate);
    m_pSignatureObj->GetDictionary().AddKey(PdfName("M"), sDate);
}

void PdfSignatureField::SetSignature(const PdfData &sSignatureData)
{
    // Prepare source data
    size_t lSigLen = sSignatureData.data().size();
    char* pData = static_cast<char*>(malloc(lSigLen+2));
    pData[0]='<';
    pData[lSigLen+1]='>';
    memcpy(pData+1, sSignatureData.data().c_str(), lSigLen);
    PdfData signatureData(pData, lSigLen+2);
    free(pData);
    // Content of the signature
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    // Remove old data
    if(m_pSignatureObj->GetDictionary().HasKey("ByteRange"))
    {
        m_pSignatureObj->GetDictionary().RemoveKey("ByteRange");
    }
    if(m_pSignatureObj->GetDictionary().HasKey(PdfName::KeyContents))
    {
        m_pSignatureObj->GetDictionary().RemoveKey(PdfName::KeyContents);
    }	

    // Byte range
    PdfData rangeData("[ 0 1234567890 1234567890 1234567890]");
    m_pSignatureObj->GetDictionary().AddKey("ByteRange", PdfVariant(rangeData) );

    m_pSignatureObj->GetDictionary().AddKey(PdfName::KeyContents, PdfVariant(signatureData) );
}

}
