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

#include "../../src/podofo.h"

#include "../PdfTest.h"

#include "SignatureGenerator.h"
#include "SimpleSignatureGenerator.h"
//#include "NSSSignatureGenerator.h"

//#include <cert.h>
//#include <certt.h>

#include <iostream>
#include <cstdio>
#include <fstream>

using namespace PoDoFo;

#define CONVERSION_CONSTANT 0.002834645669291339
/*
static CERTCertificate* read_cert()
{
    CERTCertificate* pCert = NULL;
    SECStatus status;
    CERTCertDBHandle* pDbHandle;
    const char* pszDbName = "/home/dominik/Desktop/Documents";
    / *
    status = CERT_InitCertDB( pDbHandle );
    printf("Init database: %i\n", status );

    status = CERT_OpenCertDBFilename( pDbHandle,
                                      strdup(pszDbName), 0 );
    printf("Open database: %i\n", status );
    */
    /*
    pCert = CERT_FindCertByName (
        pDbHandle, "domseichter@web.de" );
    * /
    return pCert;
}
*/
void CreateSimpleForm( PdfPage* pPage, PdfStreamedDocument* pDoc, const PdfData &signatureData )
{
    PdfPainter painter;
    PdfFont*   pFont = pDoc->CreateFont( "Courier" );

    painter.SetPage( pPage );
    painter.SetFont( pFont );
    painter.DrawText( 10000 * CONVERSION_CONSTANT, 280000 * CONVERSION_CONSTANT, "PoDoFo Sign Test" );
    painter.FinishPage();

	PdfSignatureField signField( pPage, PdfRect( 70000 * CONVERSION_CONSTANT, 10000 * CONVERSION_CONSTANT,
                                       50000 * CONVERSION_CONSTANT, 50000 * CONVERSION_CONSTANT ), pDoc );
    signField.SetFieldName("SignatureFieldName");
	signField.SetSignature(signatureData);
	signField.SetSignatureReason("I agree");
	// Set time of signing
	signField.SetSignatureDate( PdfDate() );
}


int main( int argc, char* argv[] ) 
{
    PdfPage*            pPage;

    if( argc != 2  )
    {
        printf("Usage: SignTest [output_filename]\n");
        printf("       - Create a PDF ready to be signed\n");
        return 0;
    }

    PdfSignOutputDevice signer(argv[1]);
	// Reserve space for signature
    signer.SetSignatureSize(1024);

	PdfStreamedDocument writer( &signer, PoDoFo::ePdfVersion_1_5 );
    // Disable default appearance
    writer.GetAcroForm(ePdfCreateObject, ePdfAcroFormDefaultAppearance_None);

    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    TEST_SAFE_OP( CreateSimpleForm( pPage, &writer, *signer.GetSignatureBeacon() ) );

    TEST_SAFE_OP( writer.Close() );

    // Check if position of signature was found
    if(signer.HasSignaturePosition()) {
		// Adjust ByteRange for signature
        signer.AdjustByteRange();
		
		// Read data for signature and count it
		// We have to seek at the beginning of the file
		signer.Seek(0);

		// Generate digest and count signature
		// use NSS, MS Crypto API or OpenSSL 
		// to generate signature in DER format

		// This is example of generation process
		// with dummy generator. Check example for 
		// NSS generator
		/*
        SimpleSignatureGenerator sg;

		// Read data to be signed and send them to the
		// signature generator
		char buff[65536];
		size_t len;
		while( (len = signer.ReadForSignature(buff, 65536))>0 )
		{
			sg.appendData(buff, len);
		}
		sg.finishData();		

		// Paste signature to the file
		const PdfData *pSignature = sg.getSignature();
        */
        /*
        CERTCertificate* pCert = read_cert();        
        NSSSignatureGenerator ng(pCert);
		char buff[65536];
		size_t len;
		while( (len = signer.ReadForSignature(buff, 65536))>0 )
		{
			ng.appendData(buff, len);
		}
		ng.finishData();		

		// Paste signature to the file
		const PdfData *pSignature = ng.getSignature();

        CERT_DestroyCertificate(pCert);

		if(pSignature!=NULL) {
			signer.SetSignature(*pSignature);
		}
        */
    }

	signer.Flush();


    return 0;
}
