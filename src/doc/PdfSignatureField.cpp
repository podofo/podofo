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

#include "PdfSignatureField.h"
#include "../base/PdfDictionary.h"
#include "../base/PdfData.h"

#include "PdfXObject.h"

#include <string.h>

namespace PoDoFo {

PdfSignatureField::PdfSignatureField( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
	:PdfField(PoDoFo::ePdfField_Signature, pPage, rRect, pDoc)
{
    m_pSignatureObj = NULL;
    Init();
}

//begin L.K
PdfSignatureField::PdfSignatureField( PdfAnnotation* pWidget, PdfAcroForm* pParent, PdfDocument* pDoc)
	:PdfField(PoDoFo::ePdfField_Signature, pWidget,  pParent, pDoc)
{
    m_pSignatureObj = NULL;
    Init();
}

void PdfSignatureField::SetAppearanceStream( PdfXObject* pObject )
{
    if( !pObject )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( !m_pObject->GetDictionary().HasKey( PdfName("AP") ) )
        m_pObject->GetDictionary().AddKey( PdfName("AP"), PdfDictionary() );

    if( m_pObject->GetDictionary().GetKey( PdfName("AP") )->GetDictionary().HasKey( PdfName("N") ) )
       m_pObject->GetDictionary().GetKey( PdfName("AP") )->GetDictionary().RemoveKey(PdfName("N"));
    
    m_pObject->GetDictionary().GetKey( PdfName("AP") )->GetDictionary().AddKey( PdfName("N"), pObject->GetObject()->Reference() );
    
    this->GetAppearanceCharacteristics(true);
}
//end L.K

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
    char* pData = static_cast<char*>(podofo_malloc(lSigLen+2));
 	if (!pData)
 	{
 		PODOFO_RAISE_ERROR(ePdfError_OutOfMemory);
    }
    
    pData[0]='<';
    pData[lSigLen+1]='>';
    memcpy(pData+1, sSignatureData.data().c_str(), lSigLen);
    PdfData signatureData(pData, lSigLen+2);
    podofo_free(pData);
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

void PdfSignatureField::SetSignatureLocation( const PdfString & rsText )
{
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    if(m_pSignatureObj->GetDictionary().HasKey(PdfName("Location")))
    {
        m_pSignatureObj->GetDictionary().RemoveKey(PdfName("Location"));
    }
    m_pSignatureObj->GetDictionary().AddKey(PdfName("Location"), rsText);
}

void PdfSignatureField::AddCertificationReference( PdfObject* pDocumentCatalog, EPdfCertPermission perm )
{
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if (m_pSignatureObj->GetDictionary().HasKey(PdfName("Reference")))
    {
        m_pSignatureObj->GetDictionary().RemoveKey(PdfName("Reference"));
    }

    PdfObject *pSigRef = this->GetFieldObject()->GetOwner()->CreateObject( "SigRef" );
    pSigRef->GetDictionary().AddKey(PdfName("TransformMethod"), PdfName("DocMDP"));

    PdfObject *pTransParams = this->GetFieldObject()->GetOwner()->CreateObject( "TransformParams" );
    pTransParams->GetDictionary().AddKey(PdfName("V"), PdfName("1.2"));
    pTransParams->GetDictionary().AddKey(PdfName("P"), PdfVariant((pdf_int64)perm));
    pSigRef->GetDictionary().AddKey(PdfName("TransformParams"), pTransParams);

    if (pDocumentCatalog != NULL)
    {
        PdfObject permObject;
        permObject.GetDictionary().AddKey("DocMDP", this->GetFieldObject()->GetDictionary().GetKey("V")->GetReference());

        if (pDocumentCatalog->GetDictionary().HasKey(PdfName("Perms")))
        {
            pDocumentCatalog->GetDictionary().RemoveKey(PdfName("Perms"));
        }

        pDocumentCatalog->GetDictionary().AddKey(PdfName("Perms"), permObject);
    }

    PdfArray refers;
    refers.push_back(*pSigRef);

    m_pSignatureObj->GetDictionary().AddKey(PdfName("Reference"), PdfVariant(refers));
}


}
