/***************************************************************************
 *   Copyright (C) 2016 by zyx at litePDF dot cz                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "podofo.h"

#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#if defined(_WIN64)
#define fseeko _fseeki64
#define ftello _ftelli64
#else
#define fseeko fseek
#define ftello ftell
#endif

using namespace PoDoFo;

static int print_errors_string( const char *str, size_t len, void *u )
{
    std::string *pstr = reinterpret_cast<std::string *>( u );

    if( !pstr || !len || !str )
        return 0;

    if( !pstr->empty() && (*pstr)[pstr->length() - 1] != '\n' )
        *pstr += "\n";

    *pstr += std::string( str, len );

    // to continue
    return 1;
}

static void raise_podofo_error_with_opensslerror( const char *detail )
{
    std::string err;

    ERR_print_errors_cb( print_errors_string, &err );

    if( err.empty() )
        err = "Unknown OpenSSL error";

    err = ": " + err;
    err = detail + err;

    PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, err.c_str() );
}

static int pkey_password_cb( char *buf, int bufsize, int PODOFO_UNUSED_PARAM( rwflag ), void *userdata )
{
    const char *password = reinterpret_cast<const char *>(userdata);

    if (!password)
        return 0;

    int res = strlen( password );

    if (res > bufsize)
        res = bufsize;

    memcpy( buf, password, res );

    return res;
}

static bool load_cert_and_key( const char *certfile, const char *pkeyfile, const char *pkey_password, X509 **out_cert, EVP_PKEY **out_pkey, pdf_int32 &min_signature_size )
{
    min_signature_size = 0;

    if( !certfile || !*certfile )
    {
        std::cerr << "Certificate file not specified" << std::endl;
        return false;
    }

    if( !pkeyfile || !*pkeyfile )
    {
        std::cerr << "Private key file not specified" << std::endl;
        return false;
    }

    // should not happen, but let's be paranoid
    if( !out_cert || !out_pkey )
    {
        std::cerr << "Invalid call of load_cert_and_key" << std::endl;
        return false;
    }

    FILE *fp;

    fp = fopen( certfile, "rb" );

    if( !fp )
    {
        std::cerr << "Failed to open certificate file '" << certfile << "'" << std::endl;
        return false;
    }

    *out_cert = PEM_read_X509 (fp, NULL, NULL, NULL);

    if( fseeko( fp, 0, SEEK_END ) != -1 )
        min_signature_size += ftello( fp );
    else
        min_signature_size += 3072;

    fclose( fp );

    if( !*out_cert )
    {
        std::cerr << "Failed to decode certificate file '" << certfile << "'" << std::endl;
        std::string err;

        ERR_print_errors_cb( print_errors_string, &err );

        if( !err.empty() )
            std::cerr << err.c_str() << std::endl;

        return false;
    }

    fp = fopen( pkeyfile, "rb" );

    if( !fp )
    {
        X509_free( *out_cert );
        *out_cert = NULL;

        std::cerr << "Failed to private key file '" << pkeyfile << "'" << std::endl;
        return false;
    }

    *out_pkey = PEM_read_PrivateKey( fp, NULL, pkey_password_cb, const_cast<char *>( pkey_password ) );

    if( fseeko( fp, 0, SEEK_END ) != -1 )
        min_signature_size += ftello( fp );
    else
        min_signature_size += 1024;

    fclose( fp );

    if( !*out_pkey )
    {
        X509_free( *out_cert );
        *out_cert = NULL;

        std::cerr << "Failed to decode private key file '" << pkeyfile << "'" << std::endl;
        std::string err;

        ERR_print_errors_cb( print_errors_string, &err );

        if( !err.empty() )
            std::cerr << err.c_str() << std::endl;

        return false;
    }

    return true;
}

static void sign_with_signer( PdfSignOutputDevice &signer, X509 *cert, EVP_PKEY *pkey )
{
    if( !cert )
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "cert == NULL" );
    if( !pkey )
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "pkey == NULL" );

    unsigned int uBufferLen = 65535, len;
    char *pBuffer;

    while( pBuffer = reinterpret_cast<char *>( podofo_malloc( sizeof( char ) * uBufferLen) ), !pBuffer )
    {
        uBufferLen = uBufferLen / 2;
        if( !uBufferLen )
            break;
    }

    if( !pBuffer )
        PODOFO_RAISE_ERROR (ePdfError_OutOfMemory);

    int rc;
    BIO *mem = BIO_new( BIO_s_mem() );
    if( !mem )
    {
        podofo_free( pBuffer );
        raise_podofo_error_with_opensslerror( "Failed to create input BIO" );
    }

    unsigned int flags = PKCS7_DETACHED | PKCS7_BINARY;
    PKCS7 *pkcs7 = PKCS7_sign( cert, pkey, NULL, mem, flags );
    if( !pkcs7 )
    {
        BIO_free( mem );
        podofo_free( pBuffer );
        raise_podofo_error_with_opensslerror( "PKCS7_sign failed" );
    }

    while( len = signer.ReadForSignature( pBuffer, uBufferLen ), len > 0 )
    {
        rc = BIO_write( mem, pBuffer, len );
        if( static_cast<unsigned int>( rc ) != len )
        {
            PKCS7_free( pkcs7 );
            BIO_free( mem );
            podofo_free( pBuffer );
            raise_podofo_error_with_opensslerror( "BIO_write failed" );
        }
    }

    podofo_free( pBuffer );

    if( PKCS7_final( pkcs7, mem, flags ) <= 0 )
    {
        PKCS7_free( pkcs7 );
        BIO_free( mem );
        raise_podofo_error_with_opensslerror( "PKCS7_final failed" );
    }

    bool success = false;
    BIO *out = BIO_new( BIO_s_mem() );
    if( !out )
    {
        PKCS7_free( pkcs7 );
        BIO_free( mem );
        raise_podofo_error_with_opensslerror( "Failed to create output BIO" );
    }

    char *outBuff = NULL;
    long outLen;

    i2d_PKCS7_bio( out, pkcs7 );

    outLen = BIO_get_mem_data( out, &outBuff );

    if( outLen > 0 && outBuff )
    {
        if( static_cast<size_t>( outLen ) > signer.GetSignatureSize() )
        {
            PKCS7_free( pkcs7 );
            BIO_free( out );
            BIO_free( mem );

            std::ostringstream oss;
            oss << "Requires at least " << outLen << " bytes for the signature, but reserved is only " << signer.GetSignatureSize() << " bytes";
            PODOFO_RAISE_ERROR_INFO( ePdfError_ValueOutOfRange, oss.str().c_str() );
        }

        PdfData signature( outBuff, outLen );
        signer.SetSignature( signature );
        success = true;
    }

    PKCS7_free( pkcs7 );
    BIO_free( out );
    BIO_free( mem );

    if( !success )
        raise_podofo_error_with_opensslerror( "Failed to get data from the output BIO" );
}

static void print_help( bool bOnlyUsage )
{
    if( !bOnlyUsage )
    {
        std::cout << "Digitally signs existing PDF file with the given certificate and private key." << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Usage: podofosign [arguments]" << std::endl;
    std::cout << "The required arguments:" << std::endl;
    std::cout << "  -in [inputfile] ... an input file to sign; if no -out is set, updates the input file" << std::endl;
    std::cout << "  -cert [certfile] ... a file with a PEM-encoded certificate to include in the document" << std::endl;
    std::cout << "  -pkey [pkeyfile] ... a file with a PEM-encoded private key to sign the document with" << std::endl;
    std::cout << "The optional arguments:" << std::endl;
    std::cout << "  -out [outputfile] ... an output file to save the signed document to; cannot be the same as the input file" << std::endl;
    std::cout << "  -password [password] ... a password to unlock the private key file" << std::endl;
    std::cout << "  -reason [utf8-string] ... a UTF-8 encoded string with the reason of the signature; default reason is \"I agree\"" << std::endl;
    std::cout << "  -sigsize [size] ... how many bytes to allocate for the signature; the default is derived from the certificate and private key file size" << std::endl;
    std::cout << "  -field-name [name] ... field name to use; defaults to 'PoDoFoSignatureFieldXXX', where XXX is the object number" << std::endl;
    std::cout << "  -field-use-existing ... whether to use existing signature field, if such named exists; the field type should be a signature" << std::endl;
    std::cout << "  -annot-units [mm|inch] ... set units for the annotation positions; default is mm" << std::endl;
    std::cout << "  -annot-position [page,left,top,width,height] ... where to place the annotation" << std::endl;
    std::cout << "       page ... a 1-based page index (integer), where '1' means the first page, '2' the second, and so on" << std::endl;
    std::cout << "       left,top,width,height ... a rectangle (in annot-units) where to place the annotation on the page (double)" << std::endl;
    std::cout << "  -annot-print ... use that to have the annotation printable, otherwise it's not printed (the default is not to print it)" << std::endl;
    std::cout << "  -annot-font [size,rrggbb,name] ... sets a font for the following annot-text; default is \"5,000000,Helvetica\" in mm" << std::endl;
    std::cout << "       size ... the font size, in annot-units" << std::endl;
    std::cout << "       rrggbb ... the font color, where rr is for red, gg for green and bb for blue, all two-digit hexa values between 00 and ff" << std::endl;
    std::cout << "       name ... the font name to use; if a Base14 font is recognized, then it is used, instead of embedding a new font" << std::endl;
    std::cout << "  -annot-text [left,top,utf8-string] ... a UTF-8 encoded string to add to the annotation" << std::endl;
    std::cout << "       left,top ... the position (in annot-units, relative to annot-position) where to place the text (double)" << std::endl;
    std::cout << "       text ... the actual UTF-8 encoded string to add to the annotation" << std::endl;
    std::cout << "  -annot-image [left,top,width,height,filename] ... an image to add to the annotation" << std::endl;
    std::cout << "       left,top,width,height ... a rectangle (in annot-units) where to place the image (double), relative to annot-position" << std::endl;
    std::cout << "       filename ... a filname of the image to add" << std::endl;
    std::cout << "The annotation arguments can be repeated, except of the -annot-position and -annot-print, which can appear up to once." << std::endl;
    std::cout << "The -annot-print, -annot-font, -annot-text and -annot-image can appear only after -annot-position." << std::endl;
    std::cout << "All the left,top positions are treated with 0,0 being at the left-top of the page." << std::endl;
    std::cout << "No drawing is done when using existing field." << std::endl;
}

static double convert_to_pdf_units( const char *annot_units, double value )
{
    if( strcmp( annot_units, "mm" ) == 0 )
    {
        return 72.0 * value / 25.4;
    }
    else if(  strcmp( annot_units, "inch" ) == 0 )
    {
        return 72.0 * value;
    }
    else
    {
        std::string err = "Unknown annotation unit '";
        err += annot_units;
        err += "'";

        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidEnumValue, err.c_str() );
    }
}

static bool parse_annot_position( const char *annot_position,
                                  const char *annot_units,
                                  int &annot_page,
                                  double &annot_left,
                                  double &annot_top,
                                  double &annot_width,
                                  double &annot_height )
{
    float fLeft, fTop, fWidth, fHeight;

    if( sscanf( annot_position, "%d,%f,%f,%f,%f", &annot_page, &fLeft, &fTop, &fWidth, &fHeight ) != 5 )
    {
        return false;
    }

    annot_left = convert_to_pdf_units( annot_units, fLeft );
    annot_top = convert_to_pdf_units( annot_units, fTop );
    annot_width = convert_to_pdf_units( annot_units, fWidth );
    annot_height = convert_to_pdf_units( annot_units, fHeight );

    if( annot_page < 1)
        return false;

    annot_page--;

    return true;
}

static const char* skip_commas( const char* text, int ncommas )
{
    if( !text )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    const char *res = text;

    while( *res && ncommas > 0 )
    {
        if( *res == ',' )
            ncommas--;

        res++;
    }

    if( ncommas > 0 )
    {
        std::string err = "The text '";
        err += text;
        err += "' does not conform to the specified format (no enougt commas)";

        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, err.c_str() );
    }

    return res;
}

static void draw_annotation( PdfDocument& document,
                             PdfPainter& painter,
                             int argc,
                             char *argv[],
                             const PdfRect &annot_rect )
{
    const char *annot_units = "mm";
    double font_size = convert_to_pdf_units( "mm", 5.0 );
    PdfColor font_color( 0.0, 0.0, 0.0 );
    const char *font_name = "Helvetica";
    bool bUpdateFont = true;
    int ii;

    for( ii = 1; ii < argc; ii++ )
    {
        if( strcmp( argv[ii], "-annot-units" ) == 0 )
        {
            annot_units = argv[ii + 1];
        }
        else if( strcmp( argv[ii], "-annot-font" ) == 0 )
        {
            float fSize;
            int rr, gg, bb;

            if( sscanf( argv[ii + 1], "%f,%02x%02x%02x,", &fSize, &rr, &gg, &bb ) != 4 )
            {
                std::string err = "The value for -annot-font '";
                err += argv[ii + 1];
                err += "' doesn't conform to format 'size,rrggbb,name'";

                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, err.c_str() );
            }

            font_size = convert_to_pdf_units( annot_units, fSize );
            font_color = PdfColor( static_cast<double>( rr ) / 255.0, static_cast<double>( gg ) / 255.0, static_cast<double>( bb ) / 255.0 );
            font_name = skip_commas( argv[ii + 1], 2 );
            bUpdateFont = true;
        }
        else if( strcmp( argv[ii], "-annot-text" ) == 0 )
        {
            float fLeft, fTop;

            if( sscanf( argv[ii + 1], "%f,%f,", &fLeft, &fTop ) != 2 )
            {
                std::string err = "The value for -annot-text '";
                err += argv[ii + 1];
                err += "' doesn't conform to format 'left,top,text'";

                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, err.c_str() );
            }

            const char *text = skip_commas( argv[ii + 1], 2 );

            if( bUpdateFont )
            {
                PdfFont* pFont;

                pFont = document.CreateFont( font_name, false, false, false );
                if( !pFont )
                {
                    std::string err = "Failed to create font '";
                    err += font_name;
                    err += "'";

                    PODOFO_RAISE_ERROR_INFO( ePdfError_OutOfMemory, err.c_str() );
                }

                pFont->SetFontSize( font_size );
                painter.SetFont( pFont );
                painter.SetColor( font_color );
            }

            fLeft = convert_to_pdf_units( annot_units, fLeft );
            fTop = convert_to_pdf_units( annot_units, fTop );

            painter.DrawMultiLineText( fLeft,
                                       0.0,
                                       annot_rect.GetWidth() - fLeft,
                                       annot_rect.GetHeight() - fTop,
                                       PdfString( reinterpret_cast<const pdf_utf8 *>( text ) ));
        }
        else if( strcmp( argv[ii], "-annot-image" ) == 0 )
        {
            float fLeft, fTop, fWidth, fHeight;

            if( sscanf( argv[ii + 1], "%f,%f,%f,%f,", &fLeft, &fTop, &fWidth, &fHeight ) != 4 )
            {
                std::string err = "The value for -annot-image '";
                err += argv[ii + 1];
                err += "' doesn't conform to format 'left,top,width,height,filename'";

                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, err.c_str() );
            }

            const char *filename = skip_commas( argv[ii + 1], 4 );

            fLeft = convert_to_pdf_units( annot_units, fLeft );
            fTop = convert_to_pdf_units( annot_units, fTop );
            fWidth = convert_to_pdf_units( annot_units, fWidth );
            fHeight = convert_to_pdf_units( annot_units, fHeight );

            PdfImage image( &document );
            image.LoadFromFile( filename );

            double dScaleX = fWidth / image.GetWidth();
            double dScaleY = fHeight / image.GetHeight();

            painter.DrawImage( fLeft, annot_rect.GetHeight() - fTop - fHeight, &image, dScaleX, dScaleY );
        }

        // these are the only parameters without additional value
        if( strcmp( argv[ii], "-annot-print" ) != 0 &&
            strcmp( argv[ii], "-field-use-existing" ) != 0 )
            ii++;
    }
}

static PdfObject* find_existing_signature_field( PdfAcroForm* pAcroForm, const PdfString& name )
{
    if( !pAcroForm )
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );

    PdfObject* pFields = pAcroForm->GetObject()->GetDictionary().GetKey( PdfName( "Fields" ) );
    if( pFields )
    {
        if( pFields->GetDataType() == ePdfDataType_Reference )
            pFields = pAcroForm->GetDocument()->GetObjects()->GetObject( pFields->GetReference() );

        if( pFields && pFields->GetDataType() == ePdfDataType_Array )
        {
            PdfArray &rArray = pFields->GetArray();
            PdfArray::iterator it, end = rArray.end();
            for( it = rArray.begin(); it != end; it++ )
            {
                // require references in the Fields array
                if( it->GetDataType() == ePdfDataType_Reference )
                {
                    PdfObject *item = pAcroForm->GetDocument()->GetObjects()->GetObject( it->GetReference() );

                    if( item && item->GetDictionary().HasKey( PdfName( "T" ) ) &&
                        item->GetDictionary().GetKey( PdfName( "T" ) )->GetString() == name )
                    {
                        // found a field with the same name
                        const PdfObject *pFT = item->GetDictionary().GetKey( PdfName( "FT" ) );
                        if( !pFT && item->GetDictionary().HasKey( PdfName( "Parent" ) ) )
                        {
                            const PdfObject *pTemp = item->GetIndirectKey( PdfName( "Parent" ) );
                            if( !pTemp )
                            {
                                PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
                            }

                            pFT = pTemp->GetDictionary().GetKey( PdfName( "FT" ) );
                        }

                        if( !pFT )
                        {
                            PODOFO_RAISE_ERROR( ePdfError_NoObject );
                        }

                        const PdfName fieldType = pFT->GetName();
                        if( fieldType != PdfName( "Sig" ) )
                        {
                            std::string err = "Existing field '";
                            err += name.GetString();
                            err += "' isn't of a signature type, but '";
                            err += fieldType.GetName().c_str();
                            err += "' instead";

                            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidName, err.c_str() );
                        }

                        return item;
                    }
                }
            }
        }
    }

    return NULL;
}

#if 0 /* TODO */
static void update_default_appearance_streams( PdfAcroForm* pAcroForm )
{
    if( !pAcroForm ||
        !pAcroForm->GetObject()->GetDictionary().HasKey( "Fields" ) ||
        pAcroForm->GetObject()->GetDictionary().GetKey( "Fields" )->GetDataType() != ePdfDataType_Array )
        return;

    PdfArray& rFields = pAcroForm->GetObject()->GetDictionary().GetKey( "Fields" )->GetArray();

    PdfArray::iterator it, end = rFields.end();
    for( it = rFields.begin(); it != end; it++ )
    {
        if( it->GetDataType() == ePdfDataType_Reference )
        {
            PdfObject *pObject = pAcroForm->GetDocument()->GetObjects()->GetObject( it->GetReference() );
            if( !pObject || pObject->GetDataType() != ePdfDataType_Dictionary )
                continue;

            PdfDictionary &rFielDict = pObject->GetDictionary();
            if( rFielDict.HasKey( "FT" ) &&
                rFielDict.GetKey( "FT" )->GetDataType() == ePdfDataType_Name &&
                (rFielDict.GetKey( "FT" )->GetName() == PdfName( "Tx" ) || rFielDict.GetKey( "FT" )->GetName() == PdfName( "Ch" ) ) )
            {
                PdfString rDA, rV, rDV;

                if( rFielDict.HasKey( "V" ) &&
                    ( rFielDict.GetKey( "V" )->GetDataType() == ePdfDataType_String || rFielDict.GetKey( "V" )->GetDataType() == ePdfDataType_HexString ) )
                {
                    rV = rFielDict.GetKey( "V" )->GetString();
                }

                if( rFielDict.HasKey( "DV" ) &&
                    ( rFielDict.GetKey( "DV" )->GetDataType() == ePdfDataType_String || rFielDict.GetKey( "DV" )->GetDataType() == ePdfDataType_HexString ) )
                {
                    rDV = rFielDict.GetKey( "DV" )->GetString();
                }

                if( rV.IsValid() && rV.GetCharacterLength() > 0 )
                {
                    rDV = rV;
                }

                if( !rDV.IsValid() || rDV.GetCharacterLength() <= 0 )
                    continue;

                if( rDV.GetLength() >= 2 && rDV.GetString()[0] == static_cast<char>( 0xFE ) && rDV.GetString()[1] == static_cast<char>( 0xFF ) )
                {
                    if( rDV.GetLength() == 2 )
                        continue;
                }

                if( rFielDict.HasKey( "DA" ) &&
                    rFielDict.GetKey( "DA" )->GetDataType() == ePdfDataType_String )
                {
                    rDA = rFielDict.GetKey( "DA" )->GetString();
                }

                if( rFielDict.HasKey( "AP" ) &&
                    rFielDict.GetKey( "AP" )->GetDataType() == ePdfDataType_Dictionary &&
                    rFielDict.GetKey( "AP" )->GetDictionary().HasKey( "N" ) &&
                    rFielDict.GetKey( "AP" )->GetDictionary().GetKey( "N" )->GetDataType() == ePdfDataType_Reference )
                {
                    pObject = pAcroForm->GetDocument()->GetObjects()->GetObject( rFielDict.GetKey( "AP" )->GetDictionary().GetKey( "N" )->GetReference() );
                    if( pObject->GetDataType() == ePdfDataType_Dictionary &&
                        pObject->GetDictionary().HasKey( "Type" ) &&
                        pObject->GetDictionary().GetKey( "Type" )->GetDataType() == ePdfDataType_Name &&
                        pObject->GetDictionary().GetKey( "Type" )->GetName() == PdfName( "XObject" ) )
                    {
                        PdfXObject xObject(pObject);
                        PdfStream *pCanvas = xObject.GetContentsForAppending()->GetStream();

                        if( rFielDict.GetKey( "FT" )->GetName() == PdfName( "Tx" ) )
                        {
                            pCanvas->BeginAppend(true);

                            PdfRefCountedBuffer rBuffer;
                            PdfOutputDevice rOutputDevice( &rBuffer );

                            rDV.Write( &rOutputDevice, ePdfWriteMode_Compact );

                            std::ostringstream oss;

                            oss << "/Tx BMC" << std::endl;
                            oss << "BT" << std::endl;
                            if( rDA.IsValid() )
                                oss << rDA.GetString() << std::endl;
                            oss << "2.0 2.0 Td" << std::endl;
                            oss << rBuffer.GetBuffer() << " Tj" << std::endl;
                            oss << "ET" << std::endl;
                            oss << "EMC" << std::endl;

                            pCanvas->Append( oss.str() );

                            pCanvas->EndAppend();
                        }
                        else if( rFielDict.GetKey( "FT" )->GetName() == PdfName( "Ch" ) )
                        {
                        }
                    }
                }
            }
        }
    }
}
#endif

int main( int argc, char* argv[] )
{
    const char *inputfile = NULL;
    const char *outputfile = NULL;
    const char *certfile = NULL;
    const char *pkeyfile = NULL;
    const char *password = NULL;
    const char *reason = "I agree";
    const char *sigsizestr = NULL;
    const char *annot_units = "mm";
    const char *annot_position = NULL;
    const char *field_name = NULL;
    int annot_page = 0;
    double annot_left = 0.0, annot_top = 0.0, annot_width = 0.0, annot_height = 0.0;
    bool annot_print = false;
    bool field_use_existing = false;
    int ii;

    PdfError::EnableDebug( false );

    for( ii = 1; ii < argc; ii++ )
    {
        const char **value = NULL;

        if( strcmp( argv[ii], "-in" ) == 0 )
        {
            value = &inputfile;
        }
        else if( strcmp( argv[ii], "-out" ) == 0 )
        {
            value = &outputfile;
        }
        else if( strcmp( argv[ii], "-cert" ) == 0 )
        {
            value = &certfile;
        }
        else if( strcmp( argv[ii], "-pkey" ) == 0 )
        {
            value = &pkeyfile;
        }
        else if( strcmp( argv[ii], "-password" ) == 0 )
        {
            value = &password;
        }
        else if( strcmp( argv[ii], "-reason" ) == 0 )
        {
            value = &reason;
        }
        else if( strcmp( argv[ii], "-sigsize" ) == 0 )
        {
            value = &sigsizestr;
        }
        else if( strcmp( argv[ii], "-annot-units" ) == 0 )
        {
            value = &annot_units;
        }
        else if( strcmp( argv[ii], "-annot-position" ) == 0 )
        {
            if( annot_position )
            {
                std::cerr << "Only one -annot-position can be specified" << std::endl;

                return -1;
            }

            value = &annot_position;
        }
        else if( strcmp( argv[ii], "-annot-print" ) == 0 )
        {
            if( !annot_position )
            {
                std::cerr << "Missing -annot-position argument, which should be defined before '" << argv[ii] << "'" << std::endl;

                return -2;
            }

            if( annot_print )
            {
                std::cerr << "Only one -annot-print can be specified" << std::endl;

                return -1;
            }

            annot_print = !annot_print;
            continue;
        }
        else if( strcmp( argv[ii], "-annot-font" ) == 0 ||
                 strcmp( argv[ii], "-annot-text" ) == 0 ||
                 strcmp( argv[ii], "-annot-image" ) == 0 )
        {
            if( !annot_position )
            {
                std::cerr << "Missing -annot-position argument, which should be defined before '" << argv[ii] << "'" << std::endl;

                return -2;
            }
            // value is left NULL, these are parsed later
        }
        else if( strcmp( argv[ii], "-field-name" ) == 0 )
        {
            value = &field_name;
        }
        else if( strcmp( argv[ii], "-field-use-existing" ) == 0 )
        {
            if( field_use_existing )
            {
                std::cerr << "Only one -field-use-existing can be specified" << std::endl;

                return -1;
            }

            field_use_existing = !field_use_existing;
            continue;
        }
        else
        {
            std::cerr << "Unknown argument '" << argv[ii] << "'" << std::endl;
            print_help( true );

            return -3;
        }

        if( ii + 1 >= argc)
        {
            std::cerr << "Missing value for argument '" << argv[ii] << "'" << std::endl;
            print_help( true );

            return -4;
        }

        if( value )
        {
            *value = argv[ii + 1];

            if( *value == annot_units && strcmp( annot_units, "mm" ) != 0 && strcmp( annot_units, "inch" ) != 0 )
            {
                std::cerr << "Invalid -annot-units value '" << *value << "', only 'mm' and 'inch' are supported" << std::endl;

                return -5;
            }

            try {
                if( *value == annot_position && !parse_annot_position( annot_position, annot_units, annot_page, annot_left, annot_top, annot_width, annot_height ) )
                {
                    std::cerr << "Invalid -annot-position value '" << *value << "', expected format \"page,left,top,width,height\"" << std::endl;

                    return -6;
                }
            } catch( PdfError & e ) {
                std::cerr << "Invalid -annot-position value '" << *value << "', expected format \"page,left,top,width,height\"" << std::endl;

                return -6;
            }
        }
        ii++;
    }

    if( !inputfile || !certfile || !pkeyfile )
    {
        if( argc != 1 )
            std::cerr << "Not all required arguments specified." << std::endl;
        print_help( true );

        return -7;
    }

    int sigsize = -1;

    if( sigsizestr )
    {
        sigsize = atoi( sigsizestr );

        if( sigsize <= 0 )
        {
            std::cerr << "Invalid value for signature size specified (" << sigsizestr << "), use a positive integer, please" << std::endl;
            return -8;
        }
    }

    if( outputfile && strcmp( outputfile, inputfile ) == 0)
    {
        // even I told you not to do it, you still specify the same output file
        // as the input file. Just ignore that.
        outputfile = NULL;
    }

#ifdef PODOFO_HAVE_OPENSSL_1_1
    OPENSSL_init_crypto(0, NULL);
#else
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    ERR_load_PEM_strings();
    ERR_load_ASN1_strings();
    ERR_load_EVP_strings();
#endif

    X509* cert = NULL;
    EVP_PKEY* pkey = NULL;
    pdf_int32 min_signature_size = 0;

    if( !load_cert_and_key( certfile, pkeyfile, password, &cert, &pkey, min_signature_size ) )
    {
        return -9;
    }

    if( sigsize > 0 )
        min_signature_size = sigsize;

    int result = 0;
    PdfSignatureField *pSignField = NULL;
    PdfAnnotation *pTemporaryAnnot = NULL; // for existing signature fields

    try
    {
        PdfMemDocument document;

        document.Load( inputfile, true );

        if( !document.GetPageCount() )
            PODOFO_RAISE_ERROR_INFO( ePdfError_PageNotFound, "The document has no page. Only documents with at least one page can be signed" );

        PdfAcroForm* pAcroForm = document.GetAcroForm();
        if( !pAcroForm )
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "acroForm == NULL" );

        if( !pAcroForm->GetObject()->GetDictionary().HasKey( PdfName( "SigFlags" ) ) ||
            !pAcroForm->GetObject()->GetDictionary().GetKey( PdfName( "SigFlags" ) )->IsNumber() ||
            pAcroForm->GetObject()->GetDictionary().GetKeyAsLong( PdfName( "SigFlags" ) ) != 3 )
        {
            if( pAcroForm->GetObject()->GetDictionary().HasKey( PdfName( "SigFlags" ) ) )
                pAcroForm->GetObject()->GetDictionary().RemoveKey( PdfName( "SigFlags" ) );

            pdf_int64 val = 3;
            pAcroForm->GetObject()->GetDictionary().AddKey( PdfName( "SigFlags" ), PdfObject( val ) );
        }

        if( pAcroForm->GetNeedAppearances() )
        {
            #if 0 /* TODO */
            update_default_appearance_streams( pAcroForm );
            #endif

            pAcroForm->SetNeedAppearances( false );
        }

        PdfOutputDevice outputDevice( outputfile ? outputfile : inputfile, outputfile != NULL );
        PdfSignOutputDevice signer( &outputDevice );

        PdfString name;
        PdfObject* pExistingSigField = NULL;

        if( field_name )
        {
            name = PdfString( field_name );

            pExistingSigField = find_existing_signature_field( pAcroForm, name );
            if( pExistingSigField && !field_use_existing)
            {
                std::string err = "Signature field named '";
                err += name.GetString();
                err += "' already exists";

                PODOFO_RAISE_ERROR_INFO( ePdfError_WrongDestinationType, err.c_str() );
            }
        }
        else
        {
            char fldName[96]; // use bigger buffer to make sure sprintf does not overflow
            sprintf( fldName, "PodofoSignatureField%" PDF_FORMAT_INT64, static_cast<pdf_int64>( document.GetObjects().GetObjectCount() ) );

            name = PdfString( fldName );
        }

        if( pExistingSigField )
        {
            if( !pExistingSigField->GetDictionary().HasKey( "P" ) )
            {
                std::string err = "Signature field named '";
                err += name.GetString();
                err += "' doesn't have a page reference";

                PODOFO_RAISE_ERROR_INFO( ePdfError_PageNotFound, err.c_str() );
            }

            PdfPage* pPage;

            pPage = document.GetPagesTree()->GetPage( pExistingSigField->GetDictionary().GetKey( "P" )->GetReference() );
            if( !pPage )
                PODOFO_RAISE_ERROR( ePdfError_PageNotFound );

            pTemporaryAnnot = new PdfAnnotation( pExistingSigField, pPage );
            if( !pTemporaryAnnot )
                PODOFO_RAISE_ERROR_INFO( ePdfError_OutOfMemory, "Cannot allocate annotation object for existing signature field" );

            pSignField = new PdfSignatureField( pTemporaryAnnot );
            if( !pSignField )
                PODOFO_RAISE_ERROR_INFO( ePdfError_OutOfMemory, "Cannot allocate existing signature field object" );

            pSignField->EnsureSignatureObject();
        }
        else
        {
            PdfPage* pPage = document.GetPage( annot_page );
            if( !pPage )
                PODOFO_RAISE_ERROR( ePdfError_PageNotFound );

            PdfRect annot_rect;
            if( annot_position )
            {
                annot_rect = PdfRect( annot_left, pPage->GetPageSize().GetHeight() - annot_top - annot_height, annot_width, annot_height );
            }

            PdfAnnotation* pAnnot = pPage->CreateAnnotation( ePdfAnnotation_Widget, annot_rect );
            if( !pAnnot )
                PODOFO_RAISE_ERROR_INFO( ePdfError_OutOfMemory, "Cannot allocate annotation object" );

            if( annot_position && annot_print )
                pAnnot->SetFlags( ePdfAnnotationFlags_Print );
            else if( !annot_position && ( !field_name || !field_use_existing ) )
                pAnnot->SetFlags( ePdfAnnotationFlags_Invisible | ePdfAnnotationFlags_Hidden );

            pSignField = new PdfSignatureField( pAnnot, pAcroForm, &document );
            if( !pSignField )
                PODOFO_RAISE_ERROR_INFO( ePdfError_OutOfMemory, "Cannot allocate signature field object" );

            if( annot_position )
            {
                PdfRect annotSize( 0.0, 0.0, annot_rect.GetWidth(), annot_rect.GetHeight() );
                PdfXObject sigXObject( annotSize, &document );
                PdfPainter painter;

                try
                {
                    painter.SetPage( &sigXObject );

                    /* Workaround Adobe's reader error 'Expected a dict object.' when the stream
                       contains only one object which does Save()/Restore() on its own, like
                       the image XObject. */
                    painter.Save();
                    painter.Restore();

                    draw_annotation( document, painter, argc, argv, annot_rect );

                    pSignField->SetAppearanceStream( &sigXObject );
                }
                catch( PdfError & e )
                {
                    if( painter.GetPage() )
                    {
                        try
                        {
                            painter.FinishPage();
                        }
                        catch( ... )
                        {
                        }
                    }
                }

                painter.FinishPage();
            }
        }

        // use large-enough buffer to hold the signature with the certificate
        signer.SetSignatureSize( min_signature_size );

        pSignField->SetFieldName( name );
        pSignField->SetSignatureReason( PdfString( reinterpret_cast<const pdf_utf8 *>( reason ) ) );
        pSignField->SetSignatureDate( PdfDate() );
        pSignField->SetSignature( *signer.GetSignatureBeacon() );

        // The outputfile != NULL means that the write happens to a new file,
        // which will be truncated first and then the content of the inputfile
        // will be copied into the document, follwed by the changes.
        document.WriteUpdate( &signer, outputfile != NULL );

        if( !signer.HasSignaturePosition() )
            PODOFO_RAISE_ERROR_INFO( ePdfError_SignatureError, "Cannot find signature position in the document data" );

        // Adjust ByteRange for signature
        signer.AdjustByteRange();

        // Read data for signature and count it
        // We seek at the beginning of the file
        signer.Seek( 0 );

        sign_with_signer( signer, cert, pkey );

        signer.Flush();
    }
    catch( PdfError & e )
    {
        std::cerr << "Error: An error " << e.GetError() << " occurred during the sign of the pdf file:" << std::endl;
        e.PrintErrorMsg();

        result = e.GetError();
    }

#ifndef PODOFO_HAVE_OPENSSL_1_1
    ERR_free_strings();
#endif

    if( pSignField )
        delete pSignField;

    if( pTemporaryAnnot )
        delete pTemporaryAnnot;

    if( pkey )
        EVP_PKEY_free( pkey );

    if( cert )
        X509_free( cert );

    return result;
}
