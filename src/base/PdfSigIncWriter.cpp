/***************************************************************************
 *   Copyright (C) 2014 by Dominik Seichter                                *
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

#include "PdfSigIncWriter.h"

#include "PdfData.h"
#include "PdfDate.h"
#include "PdfDictionary.h"
#include "PdfObject.h"
#include "PdfParser.h"
#include "PdfStream.h"
#include "PdfVariant.h"
#include "PdfXRef.h"
#include "PdfXRefStream.h"
#include "PdfDefinesPrivate.h"


#include <iostream>
#include <stdlib.h>

namespace PoDoFo {


PdfSigIncWriter::PdfSigIncWriter( PdfVecObjects* pVecObjects, const PdfObject* pTrailer )
    : PdfWriter(pVecObjects, pTrailer)
{

}


PdfSigIncWriter::~PdfSigIncWriter()
{
}


void PdfSigIncWriter::Write( PdfOutputDevice* pDevice, pdf_int64 prevOffset)
{
    //CreateFileIdentifier( m_identifier, m_pTrailer );
    if( m_pTrailer->GetDictionary().HasKey( "ID" ) ) {
       PdfObject *idObj =  m_pTrailer->GetDictionary().GetKey("ID");
       
       TCIVariantList it = idObj->GetArray().begin();
		 while( it != idObj->GetArray().end() ) {
			if( (*it).GetDataType() == ePdfDataType_HexString ) {
				 PdfVariant var = (*it);
             m_identifier = var.GetString();
			}
				
			++it;
		}
    } else {
       PdfDate   date;
       PdfString dateString;
       PdfObject*      pInfo;
       PdfOutputDevice length;

       date.ToString( dateString );

       pInfo = new PdfObject();
       pInfo->GetDictionary().AddKey( "CreationDate", dateString );
       pInfo->GetDictionary().AddKey( "Creator", PdfString("PoDoFo") );
       pInfo->GetDictionary().AddKey( "Producer", PdfString("PoDoFo") );
       
      pInfo->GetDictionary().AddKey( "Location", PdfString("SOMEFILENAME") );

      pInfo->WriteObject( &length, ePdfWriteMode_Clean, NULL );

      char *pBuffer = static_cast<char*>(podofo_calloc( length.GetLength(), sizeof(char) ));
      if( !pBuffer )  {
        delete pInfo;
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
      } 

      PdfOutputDevice device( pBuffer, length.GetLength() );
      pInfo->WriteObject( &device, ePdfWriteMode_Clean, NULL );

      // calculate the MD5 Sum
      m_identifier = PdfEncryptMD5Base::GetMD5String( reinterpret_cast<unsigned char*>(pBuffer),
                                           static_cast<unsigned int>(length.GetLength()) );
      podofo_free( pBuffer );

      delete pInfo;
    }

    if( !pDevice )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // setup encrypt dictionary
    if( m_pEncrypt )
    {
        m_pEncrypt->GenerateEncryptionKey( m_identifier );

        // Add our own Encryption dictionary
        m_pEncryptObj = m_vecObjects->CreateObject();
        m_pEncrypt->CreateEncryptionDictionary( m_pEncryptObj->GetDictionary() );
    }

    if( GetLinearized() ) 
    {
        this->WriteLinearized( pDevice );
    }
    else
    {
        PdfXRef* pXRef = m_bXRefStream ? new PdfXRefStream( m_vecObjects, this ) : new PdfXRef();

        try {
//            WritePdfHeader  ( pDevice );
            WritePdfObjects ( pDevice, *m_vecObjects, pXRef );
            pXRef->SetFirstEmptyBlock();

            pXRef->Write( pDevice );
            
            // XRef streams contain the trailer in the XRef
            if( !m_bXRefStream ) 
            {
                PdfObject  trailer;
                
                // if we have a dummy offset we write also a prev entry to the trailer
                FillTrailerObject( &trailer, pXRef->GetSize(), false, false );

                PdfObject prevOffsetObj(prevOffset);
                trailer.GetDictionary().AddKey( "Prev", prevOffsetObj);
                pDevice->Print("trailer\n");
                trailer.WriteObject( pDevice, ePdfWriteMode_Clean, NULL ); // Do not encrypt the trailer dicionary!!!
            }
            
            pDevice->Print( "startxref\n%li\n%%%%EOF\n", pXRef->GetOffset());
            delete pXRef;
        } catch( PdfError & e ) {
            // Make sure pXRef is always deleted
            delete pXRef;
            e.AddToCallstack( __FILE__, __LINE__ );
            throw e;
        }
    }
}


};

