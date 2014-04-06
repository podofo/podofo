/*
 **********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved. **
 **                                                                  **
 ** License to copy and use this software is granted provided that   **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message     **
 ** Digest Algorithm" in all material mentioning or referencing this **
 ** software or this function.                                       **
 **                                                                  **
 ** License is also granted to make and use derivative works         **
 ** provided that such works are identified as "derived from the RSA **
 ** Data Security, Inc. MD5 Message Digest Algorithm" in all         **
 ** material mentioning or referencing the derived work.             **
 **                                                                  **
 ** RSA Data Security, Inc. makes no representations concerning      **
 ** either the merchantability of this software or the suitability   **
 ** of this software for any particular purpose.  It is provided "as **
 ** is" without express or implied warranty of any kind.             **
 **                                                                  **
 ** These notices must be retained in any copies of any part of this **
 ** documentation and/or software.                                   **
 **********************************************************************
 */

// includes
#include "PdfEncrypt.h"

#include "PdfDictionary.h"
#include "PdfFilter.h"
#include "PdfDefinesPrivate.h"

#include <stdlib.h>
#include <string.h>
#include <sstream>

// SHA-256
#ifdef PODOFO_HAVE_LIBIDN
// AES-256 dependencies :
// SASL
#include <stringprep.h>
#include <openssl/sha.h>
#endif // PODOFO_HAVE_LIBIDN

#include <openssl/md5.h>
#include <openssl/evp.h>

namespace
{
    //RG: TODO Could we name literal 256 and use the literal name e.g.
    //const size_t KEY_SIZE = 256;
}

namespace PoDoFo {

#ifdef PODOFO_HAVE_LIBIDN
int PdfEncrypt::s_nEnabledEncryptionAlgorithms = 
ePdfEncryptAlgorithm_RC4V1 |
ePdfEncryptAlgorithm_RC4V2 |
ePdfEncryptAlgorithm_AESV2 |
ePdfEncryptAlgorithm_AESV3;
#else // PODOFO_HAVE_LIBIDN
int PdfEncrypt::s_nEnabledEncryptionAlgorithms =
ePdfEncryptAlgorithm_RC4V1 |
ePdfEncryptAlgorithm_RC4V2 |
ePdfEncryptAlgorithm_AESV2;
#endif // PODOFO_HAVE_LIBIDN

// Default value for P (permissions) = no permission
#define PERMS_DEFAULT 0xFFFFF0C0

#define AES_IV_LENGTH 16

// A class that holds the AES Crypto object
class AESCryptoEngine {
    public:
    
        AESCryptoEngine()
        {
            EVP_CIPHER_CTX_init(&aes);
        }
    
        EVP_CIPHER_CTX* getEngine() {return &aes;}
    
        ~AESCryptoEngine()
        {
            EVP_CIPHER_CTX_cleanup(&aes);
        }
    
    private:
    
        EVP_CIPHER_CTX aes;
};

// A class that holds the RC4 Crypto object
// Either CCCrpytor or EVP_CIPHER_CTX
class RC4CryptoEngine {
public:
    
    RC4CryptoEngine()
    {
        EVP_CIPHER_CTX_init(&rc4);
    }
    
    EVP_CIPHER_CTX* getEngine() {return &rc4;}
    
    ~RC4CryptoEngine()
    {
        EVP_CIPHER_CTX_cleanup(&rc4);
    }
    
private:
    
    EVP_CIPHER_CTX rc4;
};
    
/** A class that can encrypt/decrpyt streamed data block wise
 *  This is used in the input and output stream encryption implementation.
 *  Only the RC4 encryption algorithm is supported
 */
class PdfRC4Stream {
public:
    PdfRC4Stream( unsigned char rc4key[256], unsigned char rc4last[256], unsigned char* key, const size_t keylen )
    : m_a( 0 ), m_b( 0 )
    {
        size_t i;
        size_t j;
        size_t t;
        
        if (memcmp(key,rc4key,keylen) != 0)
        {
            for (i = 0; i < 256; i++)
                m_rc4[i] = static_cast<unsigned char>(i);
            
            j = 0;
            for (i = 0; i < 256; i++)
            {
                t = static_cast<size_t>(m_rc4[i]);
                j = (j + t + static_cast<size_t>(key[i % keylen])) % 256;
                m_rc4[i] = m_rc4[j];
                m_rc4[j] = static_cast<unsigned char>(t);
            }
            
            memcpy(rc4key, key, keylen);
            memcpy(rc4last, m_rc4, 256);
        }
        else
        {
            memcpy(m_rc4, rc4last, 256);
        }
    }
    
    ~PdfRC4Stream()
    {
    }
    
    /** Encrypt or decrypt a block
     *  
     *  \param pBuffer the input/output buffer. Data is read from this buffer and also stored here
     *  \param lLen    the size of the buffer 
     */
    pdf_long Encrypt( char* pBuffer, pdf_long lLen )
    {
        unsigned char k;
        pdf_long t, i;
        
        // Do not encode data with no length
        if( !lLen )
            return lLen;
        
        for (i = 0; i < lLen; i++ )
        {
            m_a = (m_a + 1) % 256;
            t   = m_rc4[m_a];
            m_b = (m_b + t) % 256;
            
            m_rc4[m_a] = m_rc4[m_b];
            m_rc4[m_b] = static_cast<unsigned char>(t);
            
            k = m_rc4[(m_rc4[m_a] + m_rc4[m_b]) % 256];
            pBuffer[i] = pBuffer[i] ^ k;
        }
        
        return lLen;
    }
    
private:
    unsigned char m_rc4[256];
    
    int           m_a;
    int           m_b;
    
};

/** A PdfOutputStream that encrypt all data written
 *  using the RC4 encryption algorithm
 */
class PdfRC4OutputStream : public PdfOutputStream {
public:
    PdfRC4OutputStream( PdfOutputStream* pOutputStream, unsigned char rc4key[256], unsigned char rc4last[256], unsigned char* key, int keylen )
    : m_pOutputStream( pOutputStream ), m_stream( rc4key, rc4last, key, keylen )
    {
    }
    
    virtual ~PdfRC4OutputStream()
    {
    }
    
    /** Write data to the output stream
     *  
     *  \param pBuffer the data is read from this buffer
     *  \param lLen    the size of the buffer 
     */
    virtual pdf_long Write( const char* pBuffer, pdf_long lLen )
    {
        // Do not encode data with no length
        if( !lLen )
            return lLen;
        
        char* pOutputBuffer = static_cast<char*>(malloc( sizeof(char) * lLen ));
        if( !pOutputBuffer )
        {
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }
        
        memcpy(pOutputBuffer, pBuffer, lLen);
        
        m_stream.Encrypt( pOutputBuffer, lLen );
        m_pOutputStream->Write( pOutputBuffer, lLen );
        
        free( pOutputBuffer );
        return lLen;
    }
    
    /** Close the PdfOutputStream.
     *  This method may throw exceptions and has to be called 
     *  before the descructor to end writing.
     *
     *  No more data may be written to the output device
     *  after calling close.
     */
    virtual void Close() 
    {
    }
    
private:
    PdfOutputStream* m_pOutputStream;
    PdfRC4Stream     m_stream;
};

/** A PdfInputStream that decrypts all data read
 *  using the RC4 encryption algorithm
 */
class PdfRC4InputStream : public PdfInputStream {
public:
    PdfRC4InputStream( PdfInputStream* pInputStream, unsigned char rc4key[256], unsigned char rc4last[256], 
                      unsigned char* key, int keylen )
    : m_pInputStream( pInputStream ), m_stream( rc4key, rc4last, key, keylen )
    {
    }
    
    virtual ~PdfRC4InputStream() 
    {
    }
    
    /** Read data from the input stream
     *  
     *  \param pBuffer the data will be stored into this buffer
     *  \param lLen    the size of the buffer and number of bytes
     *                 that will be read
     *
     *  \returns the number of bytes read, -1 if an error ocurred
     *           and zero if no more bytes are available for reading.
     */
    virtual pdf_long Read( char* pBuffer, pdf_long lLen, pdf_long* )
    {
        // Do not encode data with no length
        if( !lLen )
            return lLen;
        
        m_pInputStream->Read( pBuffer, lLen );
        m_stream.Encrypt( pBuffer, lLen );
        
        return lLen;
    }
    
private:
    PdfInputStream* m_pInputStream;
    PdfRC4Stream    m_stream;
};

/** A class that can encrypt/decrpyt streamed data block wise
 *  This is used in the input and output stream encryption implementation.
 */
class PdfAESStream : public PdfEncryptAESBase {
public:
    PdfAESStream( unsigned char* key, const size_t keylen )
		: keyLen( keylen ), bFirstRead( true ), bOnlyFinalLeft( false )
    {
		memcpy( this->key, key, keylen );
    }
    
    ~PdfAESStream() {}
   
    /** Decrypt a block
     *  
     *  \param pBuffer    the input/output buffer. Data is read from this buffer and also stored here
     *  \param lLen       the size of the buffer 
     *  \param pTotalLeft total bytes left (needed for AES IV and padding)
     */
    pdf_long Decrypt( unsigned char* pBuffer, pdf_long lLen, pdf_long* pTotalLeft )
    {
		if (pTotalLeft == 0)
			PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-decryption needs pTotalLeft" );
		if( lLen % keyLen != 0 )
			PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-decryption data length not a multiple of the key length" );
		EVP_CIPHER_CTX* aes = m_aes->getEngine();
		int lOutLen = 0, lStepOutLen;
		int status = 1;
		if( bFirstRead ) {
			bFirstRead = false;
			if( keyLen == PdfEncrypt::ePdfKeyLength_128/8 ) {
				status = EVP_DecryptInit_ex( aes, EVP_aes_128_cbc(), NULL, key, pBuffer );
#ifdef PODOFO_HAVE_LIBIDN
			} else if( keyLen == PdfEncrypt::ePdfKeyLength_256/8 ) {
				status = EVP_DecryptInit_ex( aes, EVP_aes_256_cbc(), NULL, key, pBuffer );
#endif
			} else {
				PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Invalid AES key length" );
			}
			if(status != 1)
				PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing AES encryption engine" );
			status = EVP_DecryptUpdate( aes, pBuffer, &lOutLen, pBuffer + keyLen, lLen - keyLen );
		} else if( !bOnlyFinalLeft ) {
			// Quote openssl.org: "the decrypted data buffer out passed to EVP_DecryptUpdate() should have sufficient room
			//  for (inl + cipher_block_size) bytes unless the cipher block size is 1 in which case inl bytes is sufficient."
			// So we need to create a buffer that is bigger than lLen.
			unsigned char* tempBuffer = new unsigned char[lLen + keyLen];
			status = EVP_DecryptUpdate( aes, tempBuffer, &lOutLen, pBuffer, lLen );
			memcpy( pBuffer, tempBuffer, lOutLen );
			delete[] tempBuffer;
		}
		if( status != 1 )
			PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-decryption data" );
		if( lLen == *pTotalLeft ) {
			// Last chunk of the stream
			if( lLen == lOutLen ) {
				// Buffer is full, so we need an other round for EVP_DecryptFinal_ex.
				bOnlyFinalLeft = true;
				*pTotalLeft += keyLen;
			} else {
				status = EVP_DecryptFinal_ex( aes, pBuffer + lOutLen, &lStepOutLen );
				if( status != 1 )
					PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-decryption data padding" );
				lOutLen += lStepOutLen;
			}
		}
		*pTotalLeft -= lLen - lOutLen; // AES makes the resulting buffer shorter (IV and padding)
        return lOutLen;
    }
    
private:
	unsigned char key[32];
	const size_t keyLen;
	bool bFirstRead;
	bool bOnlyFinalLeft;
};

/** A PdfAESInputStream that decrypts all data read
 *  using the AES encryption algorithm
 */
class PdfAESInputStream : public PdfInputStream {
public:
    PdfAESInputStream( PdfInputStream* pInputStream, unsigned char* key, int keylen )
    : m_pInputStream( pInputStream ), m_stream( key, keylen )
    {
    }
    
    virtual ~PdfAESInputStream() 
    {
    }
    
    /** Read data from the input stream
     *  
     *  \param pBuffer    the data will be stored into this buffer
     *  \param lLen       the size of the buffer and number of bytes
     *                    that will be read
     *  \param pTotalLeft total bytes left (needed for AES IV and padding)
     *
     *  \returns the number of bytes read, -1 if an error ocurred
     *           and zero if no more bytes are available for reading.
     */
    virtual pdf_long Read( char* pBuffer, pdf_long lLen, pdf_long *pTotalLeft )
    {
        // Do not encode data with no length
        if( !lLen )
            return lLen;
        
		m_pInputStream->Read( pBuffer, lLen );
        return m_stream.Decrypt( (unsigned char*)pBuffer, lLen, pTotalLeft );
    }
    
private:
    PdfInputStream* m_pInputStream;
    PdfAESStream m_stream;
};

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

// ---------------------------
// PdfEncrypt implementation
// Based on code from Ulrich Telle: http://wxcode.sourceforge.net/components/wxpdfdoc/
// ---------------------------

static unsigned char padding[] =
"\x28\xBF\x4E\x5E\x4E\x75\x8A\x41\x64\x00\x4E\x56\xFF\xFA\x01\x08\x2E\x2E\x00\xB6\xD0\x68\x3E\x80\x2F\x0C\xA9\xFE\x64\x53\x69\x7A";

PdfEncrypt *
PdfEncrypt::CreatePdfEncrypt( const std::string & userPassword, 
                             const std::string & ownerPassword, 
                             int protection,
                             EPdfEncryptAlgorithm eAlgorithm, 
                             EPdfKeyLength eKeyLength )
{
    PdfEncrypt *pdfEncrypt = NULL;
    
    switch (eAlgorithm)
    {
        case ePdfEncryptAlgorithm_AESV2:
            pdfEncrypt = new PdfEncryptAESV2(userPassword, ownerPassword, protection);
            break;
#ifdef PODOFO_HAVE_LIBIDN
        case ePdfEncryptAlgorithm_AESV3:
            pdfEncrypt = new PdfEncryptAESV3(userPassword, ownerPassword, protection);
            break;
#endif // PODOFO_HAVE_LIBIDN
        case ePdfEncryptAlgorithm_RC4V2:           
        case ePdfEncryptAlgorithm_RC4V1:
        default:
            pdfEncrypt = new PdfEncryptRC4(userPassword, ownerPassword, protection, eAlgorithm, eKeyLength);
            break;
    }
    return pdfEncrypt;
}

PdfEncrypt* PdfEncrypt::CreatePdfEncrypt( const PdfObject* pObject )
{
    PdfEncrypt* pdfEncrypt = NULL;
    if( !pObject->GetDictionary().HasKey( PdfName("Filter") ) ||
       pObject->GetDictionary().GetKey( PdfName("Filter" ) )->GetName() != PdfName("Standard") )
    {
        std::ostringstream oss;
        if( pObject->GetDictionary().HasKey( PdfName("Filter") ) )
        {
            oss << "Unsupported encryption filter: " << pObject->GetDictionary().GetKey( PdfName("Filter" ) )->GetName().GetName();
        }
        else
        {
            oss << "Encryption dictionary does not have a key /Filter.";
        }
        
        PODOFO_RAISE_ERROR_INFO( ePdfError_UnsupportedFilter, oss.str().c_str() );
    }
    
    long lV;
    long long lLength;
    int rValue;
    int pValue;
    PdfString oValue;
    PdfString uValue;
	PdfName cfmName;
	bool encryptMetadata = true;
    
    try {
        PdfString sTmp;
        
        lV           = static_cast<long>(pObject->GetDictionary().GetKey( PdfName("V") )->GetNumber());
        rValue       = static_cast<int>(pObject->GetDictionary().GetKey( PdfName("R") )->GetNumber());
        
        pValue       = static_cast<int>(pObject->GetDictionary().GetKey( PdfName("P") )->GetNumber());
        
        oValue       = pObject->GetDictionary().GetKey( PdfName("O") )->GetString();        
        uValue       = pObject->GetDictionary().GetKey( PdfName("U") )->GetString();
        
        if( pObject->GetDictionary().HasKey( PdfName("Length") ) )
        {
            lLength = pObject->GetDictionary().GetKey( PdfName("Length") )->GetNumber();
        }
        else
        {
            lLength = 0;
        }
		const PdfObject *encryptMetadataObj = pObject->GetDictionary().GetKey( PdfName("EncryptMetadata") );
		if( encryptMetadataObj && encryptMetadataObj->IsBool() )
			encryptMetadata = encryptMetadataObj->GetBool();
		const PdfObject *stmfObj = pObject->GetDictionary().GetKey( PdfName("StmF") );
		if( stmfObj && stmfObj->IsName() ) {
			const PdfObject *obj = pObject->GetDictionary().GetKey( PdfName("CF") );
			if( obj && obj->IsDictionary() ) {
				obj = obj->GetDictionary().GetKey( stmfObj->GetName() );
				if( obj && obj->IsDictionary() ) {
					obj = obj->GetDictionary().GetKey( PdfName("CFM") );
					if( obj && obj->IsName() )
						cfmName = obj->GetName();
				}
			}
		}
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__, "Invalid key in encryption dictionary" );
        throw e;
    }
    
    if( (lV == 1L) && (rValue == 2L || rValue == 3L)
       && PdfEncrypt::IsEncryptionEnabled( ePdfEncryptAlgorithm_RC4V1 ) ) 
    {
        pdfEncrypt = new PdfEncryptRC4(oValue, uValue, pValue, rValue, ePdfEncryptAlgorithm_RC4V1, 40, encryptMetadata);
    }
    else if( (((lV == 2L) && (rValue == 3L)) || cfmName == "V2")
            && PdfEncrypt::IsEncryptionEnabled( ePdfEncryptAlgorithm_RC4V2 ) ) 
    {
        // [Alexey] - lLength is long long. Please make changes in encryption algorithms
        pdfEncrypt = new PdfEncryptRC4(oValue, uValue, pValue, rValue, ePdfEncryptAlgorithm_RC4V2, static_cast<int>(lLength), encryptMetadata);
    }
    else if( (lV == 4L) && (rValue == 4L)
            && PdfEncrypt::IsEncryptionEnabled( ePdfEncryptAlgorithm_AESV2 ) ) 
    {
        pdfEncrypt = new PdfEncryptAESV2(oValue, uValue, pValue, encryptMetadata);      
    }
#ifdef PODOFO_HAVE_LIBIDN
    else if( (lV == 5L) && (rValue == 5L) 
            && PdfEncrypt::IsEncryptionEnabled( ePdfEncryptAlgorithm_AESV3 ) ) 
    {
        PdfString permsValue   = pObject->GetDictionary().GetKey( PdfName("Perms") )->GetString();
        PdfString oeValue      = pObject->GetDictionary().GetKey( PdfName("OE") )->GetString();
        PdfString ueValue      = pObject->GetDictionary().GetKey( PdfName("UE") )->GetString();
        
        pdfEncrypt = new PdfEncryptAESV3(oValue, oeValue, uValue, ueValue, pValue, permsValue);     
    }
#endif // PODOFO_HAVE_LIBIDN
    else
    {
        std::ostringstream oss;
        oss << "Unsupported encryption method Version=" << lV << " Revision=" << rValue;
        PODOFO_RAISE_ERROR_INFO( ePdfError_UnsupportedFilter, oss.str().c_str() );
    }
    return pdfEncrypt;
}

PdfEncrypt *
PdfEncrypt::CreatePdfEncrypt(const PdfEncrypt & rhs )  
{
    PdfEncrypt *pdfEncrypt = NULL;
    
    if (rhs.m_eAlgorithm == ePdfEncryptAlgorithm_AESV2)
        pdfEncrypt = new PdfEncryptAESV2(rhs);
#ifdef PODOFO_HAVE_LIBIDN
    else if (rhs.m_eAlgorithm == ePdfEncryptAlgorithm_AESV3)
        pdfEncrypt = new PdfEncryptAESV3(rhs);
#endif // PODOFO_HAVE_LIBIDN
    else
        pdfEncrypt = new PdfEncryptRC4(rhs);
    return pdfEncrypt;
}

PdfEncrypt::~PdfEncrypt()
{
}
    
int PdfEncrypt::GetEnabledEncryptionAlgorithms()
{
    return PdfEncrypt::s_nEnabledEncryptionAlgorithms;
}

void PdfEncrypt::SetEnabledEncryptionAlgorithms(int nEncryptionAlgorithms)
{
    PdfEncrypt::s_nEnabledEncryptionAlgorithms = nEncryptionAlgorithms;
}

bool PdfEncrypt::IsEncryptionEnabled(EPdfEncryptAlgorithm eAlgorithm)
{
    return (PdfEncrypt::s_nEnabledEncryptionAlgorithms & eAlgorithm) != 0;
}

PdfEncrypt::PdfEncrypt( const PdfEncrypt & rhs )
{
    m_eAlgorithm = rhs.m_eAlgorithm;
    m_eKeyLength = rhs.m_eKeyLength;
    
    m_pValue = rhs.m_pValue;
    m_rValue = rhs.m_rValue;
    
    m_keyLength = rhs.m_keyLength;
    
    m_curReference = rhs.m_curReference;
    m_documentId   = rhs.m_documentId;
    m_userPass     = rhs.m_userPass;
    m_ownerPass    = rhs.m_ownerPass;
	m_bEncryptMetadata = rhs.m_bEncryptMetadata;
}
    
bool
PdfEncrypt::CheckKey(unsigned char key1[32], unsigned char key2[32])
{
    // Check whether the right password had been given
    bool ok = true;
    int k;
    for (k = 0; ok && k < m_keyLength; k++)
    {
        ok = ok && (key1[k] == key2[k]);
    }
    
    return ok;
}

PdfEncryptMD5Base::PdfEncryptMD5Base( const PdfEncrypt & rhs ) : PdfEncrypt(rhs)
{
    const PdfEncrypt* ptr = &rhs;
    
    memcpy( m_uValue, rhs.GetUValue(), sizeof(unsigned char) * 32 );
    memcpy( m_oValue, rhs.GetOValue(), sizeof(unsigned char) * 32 );
    
    memcpy( m_encryptionKey, rhs.GetEncryptionKey(), sizeof(unsigned char) * 16 );
    
    memcpy( m_rc4key, static_cast<const PdfEncryptMD5Base*>(ptr)->m_rc4key, sizeof(unsigned char) * 16 );
    memcpy( m_rc4last, static_cast<const PdfEncryptMD5Base*>(ptr)->m_rc4last, sizeof(unsigned char) * 256 );
	m_bEncryptMetadata = static_cast<const PdfEncryptMD5Base*>(ptr)->m_bEncryptMetadata;
}

void
PdfEncryptMD5Base::PadPassword(const std::string& password, unsigned char pswd[32])
{
    size_t m = password.length();
    
    if (m > 32) m = 32;
    
    size_t j;
    size_t p = 0;
    for (j = 0; j < m; j++)
    {
        pswd[p++] = static_cast<unsigned char>( password[j] );
    }
    for (j = 0; p < 32 && j < 32; j++)
    {
        pswd[p++] = padding[j];
    }
}

bool
PdfEncryptMD5Base::Authenticate(const std::string& documentID, const std::string& password,
                                const std::string& uValue, const std::string& oValue,
                                int pValue, int lengthValue, int rValue)
{
    m_pValue = pValue;
    m_keyLength = lengthValue / 8;
    m_rValue = rValue;
    
    memcpy(m_uValue, uValue.c_str(), 32);
    memcpy(m_oValue, oValue.c_str(), 32);
    
    return Authenticate(password, documentID);
}

void
PdfEncryptMD5Base::ComputeOwnerKey(unsigned char userPad[32], unsigned char ownerPad[32],
                                   int keyLength, int revision, bool authenticate,
                                   unsigned char ownerKey[32])
{
    unsigned char mkey[MD5_DIGEST_LENGTH];
    unsigned char digest[MD5_DIGEST_LENGTH];
    
    MD5_CTX ctx;
    int status = MD5_Init(&ctx);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing MD5 hashing engine" );
    status = MD5_Update(&ctx, ownerPad, 32);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
    status = MD5_Final(digest,&ctx);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
    
    if ((revision == 3) || (revision == 4))
    {
        // only use for the input as many bit as the key consists of
        for (int k = 0; k < 50; ++k)
        {
            status = MD5_Init(&ctx);
            if(status != 1)
                PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing MD5 hashing engine" );
            status = MD5_Update(&ctx, digest, keyLength);
            if(status != 1)
                PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
            status = MD5_Final(digest,&ctx);
            if(status != 1)
                PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
        }
        memcpy(ownerKey, userPad, 32);
        for (unsigned int i = 0; i < 20; ++i)
        {
            for (int j = 0; j < keyLength ; ++j)
            {
                if (authenticate)
                    mkey[j] = static_cast<unsigned char>(static_cast<unsigned int>(digest[j] ^ (19-i)));
                else
                    mkey[j] = static_cast<unsigned char>(static_cast<unsigned int>(digest[j] ^ i));
            }
            RC4(mkey, keyLength, ownerKey, 32, ownerKey, 32);
        }
    }
    else
    {
        RC4(digest, 5, userPad, 32, ownerKey, 32);
    }
}

void
PdfEncryptMD5Base::ComputeEncryptionKey(const std::string& documentId,
                                        unsigned char userPad[32], unsigned char ownerKey[32],
                                        int pValue, int keyLength, int revision,
                                        unsigned char userKey[32], bool encryptMetadata)
{
    int j;
    int k;
    m_keyLength = keyLength / 8;
    
    MD5_CTX ctx;
    int status = MD5_Init(&ctx);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing MD5 hashing engine" );
    status = MD5_Update(&ctx, userPad, 32);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
    status = MD5_Update(&ctx, ownerKey, 32);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
    
    unsigned char ext[4];
    ext[0] = static_cast<unsigned char> ( pValue        & 0xff);
    ext[1] = static_cast<unsigned char> ((pValue >>  8) & 0xff);
    ext[2] = static_cast<unsigned char> ((pValue >> 16) & 0xff);
    ext[3] = static_cast<unsigned char> ((pValue >> 24) & 0xff);
    status = MD5_Update(&ctx, ext, 4);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
    
    unsigned int docIdLength = static_cast<unsigned int>(documentId.length());
    unsigned char* docId = NULL;
    if (docIdLength > 0)
    {
        docId = new unsigned char[docIdLength];
        size_t j;
        for (j = 0; j < docIdLength; j++)
        {
            docId[j] = static_cast<unsigned char>( documentId[j] );
        }
        status = MD5_Update(&ctx, docId, docIdLength);
        if(status != 1)
            PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
    }
    
    // If document metadata is not being encrypted, 
	// pass 4 bytes with the value 0xFFFFFFFF to the MD5 hash function.
    if( !encryptMetadata ) {
		unsigned char noMetaAddition[4] = { 0xff, 0xff, 0xff, 0xff };
        status = MD5_Update(&ctx, noMetaAddition, 4);
	}

    unsigned char digest[MD5_DIGEST_LENGTH];
    status = MD5_Final(digest,&ctx);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
    
    // only use the really needed bits as input for the hash
    if (revision == 3 || revision == 4)
    {
        for (k = 0; k < 50; ++k)
        {
            status = MD5_Init(&ctx);
            if(status != 1)
                PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing MD5 hashing engine" );
            status = MD5_Update(&ctx, digest, m_keyLength);
            if(status != 1)
                PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
            status = MD5_Final(digest, &ctx);
            if(status != 1)
                PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
        }
    }
    
    memcpy(m_encryptionKey, digest, m_keyLength);
    
    // Setup user key
    if (revision == 3 || revision == 4)
    {
        status = MD5_Init(&ctx);
        if(status != 1)
            PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing MD5 hashing engine" );
        status = MD5_Update(&ctx, padding, 32);
        if(status != 1)
            PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
        if (docId != NULL)
        {
            status = MD5_Update(&ctx, docId, docIdLength);
            if(status != 1)
                PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
        }
        status = MD5_Final(digest, &ctx);
        if(status != 1)
            PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
        memcpy(userKey, digest, 16);
        for (k = 16; k < 32; ++k)
        {
            userKey[k] = 0;
        }
        for (k = 0; k < 20; k++)
        {
            for (j = 0; j < m_keyLength; ++j)
            {
                digest[j] = static_cast<unsigned char>(m_encryptionKey[j] ^ k);
            }
            
            RC4(digest, m_keyLength, userKey, 16, userKey, 16);
        }
    }
    else
    {
        RC4(m_encryptionKey, m_keyLength, padding, 32, userKey, 32);
    }
    if (docId != NULL)
    {
        delete [] docId;
    }
}

void PdfEncryptMD5Base::CreateObjKey( unsigned char objkey[16], int* pnKeyLen ) const
{
    const unsigned int n = static_cast<unsigned int>(m_curReference.ObjectNumber());
    const unsigned int g = static_cast<unsigned int>(m_curReference.GenerationNumber());
    
    unsigned char nkey[MD5_DIGEST_LENGTH+5+4];
    int nkeylen = m_keyLength + 5;
    const size_t KEY_LENGTH_SIZE_T = static_cast<size_t>(m_keyLength);
    
    for (size_t j = 0; j < KEY_LENGTH_SIZE_T; j++)
    {
        nkey[j] = m_encryptionKey[j];
    }
    nkey[m_keyLength+0] = static_cast<unsigned char>(0xff &  n);
    nkey[m_keyLength+1] = static_cast<unsigned char>(0xff & (n >> 8));
    nkey[m_keyLength+2] = static_cast<unsigned char>(0xff & (n >> 16));
    nkey[m_keyLength+3] = static_cast<unsigned char>(0xff &  g);
    nkey[m_keyLength+4] = static_cast<unsigned char>(0xff & (g >> 8));
    
	if (m_eAlgorithm == ePdfEncryptAlgorithm_AESV2
#ifdef PODOFO_HAVE_LIBIDN
		|| m_eAlgorithm == ePdfEncryptAlgorithm_AESV3
#endif
	)
    {
        // AES encryption needs some 'salt'
        nkeylen += 4;
        nkey[m_keyLength+5] = 0x73;
        nkey[m_keyLength+6] = 0x41;
        nkey[m_keyLength+7] = 0x6c;
        nkey[m_keyLength+8] = 0x54;
    }
    
    GetMD5Binary(nkey, nkeylen, objkey);
    *pnKeyLen = (m_keyLength <= 11) ? m_keyLength+5 : 16;
}

PdfEncryptRC4Base::PdfEncryptRC4Base()
{
    m_rc4 = new RC4CryptoEngine();
}

PdfEncryptRC4Base::~PdfEncryptRC4Base()
{
    delete m_rc4;
}
    
/**
 * RC4 is the standard encryption algorithm used in PDF format
 */

void
PdfEncryptRC4Base::RC4(const unsigned char* key, int keylen,
                       const unsigned char* textin, pdf_long textlen,
                       unsigned char* textout, pdf_long textoutlen)
{
    EVP_CIPHER_CTX* rc4 = m_rc4->getEngine();
    
    if(textlen != textoutlen)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing RC4 encryption engine" );
    
    // Don't set the key because we will modify the parameters
    int status = EVP_EncryptInit_ex(rc4, EVP_rc4(), NULL, NULL, NULL);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing RC4 encryption engine" );
    
    status = EVP_CIPHER_CTX_set_key_length(rc4, keylen);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing RC4 encryption engine" );
    
    // We finished modifying parameters so now we can set the key
    status = EVP_EncryptInit_ex(rc4, NULL, NULL, key, NULL);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing RC4 encryption engine" );
    
    int dataOutMoved;
    status = EVP_EncryptUpdate(rc4, textout, &dataOutMoved, textin, textlen);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error RC4-encrypting data" );
    
    status = EVP_EncryptFinal_ex(rc4, &textout[dataOutMoved], &dataOutMoved);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error RC4-encrypting data" );
}
        
void
PdfEncryptMD5Base::GetMD5Binary(const unsigned char* data, int length, unsigned char* digest)
{
    int status;
    MD5_CTX ctx;
    status = MD5_Init(&ctx);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing MD5 hashing engine" );
    status = MD5_Update(&ctx, data, length);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
    status = MD5_Final(digest,&ctx);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error MD5-hashing data" );
}

void PdfEncryptMD5Base::GenerateInitialVector(unsigned char iv[AES_IV_LENGTH])
{
    GetMD5Binary(reinterpret_cast<const unsigned char*>(m_documentId.c_str()), 
                 static_cast<unsigned int>(m_documentId.length()), iv);
}
    
PdfString PdfEncryptMD5Base::GetMD5String( const unsigned char* pBuffer, int nLength )
{
    char data[MD5_DIGEST_LENGTH];
    
    GetMD5Binary( pBuffer, nLength, reinterpret_cast<unsigned char*>(data) );
    
    return PdfString( data, MD5_DIGEST_LENGTH, true );
}
    
void PdfEncryptMD5Base::CreateEncryptionDictionary( PdfDictionary & rDictionary ) const
{
    rDictionary.AddKey( PdfName("Filter"), PdfName("Standard") );
    
    if(m_eAlgorithm == ePdfEncryptAlgorithm_AESV2 || !m_bEncryptMetadata)
    {
        PdfDictionary cf;
        PdfDictionary stdCf;
        
		if(m_eAlgorithm == ePdfEncryptAlgorithm_RC4V2)
			stdCf.AddKey( PdfName("CFM"), PdfName("V2") );
		else
			stdCf.AddKey( PdfName("CFM"), PdfName("AESV2") );
        stdCf.AddKey( PdfName("Length"), static_cast<pdf_int64>(16LL) );
        
        rDictionary.AddKey( PdfName("O"), PdfString( reinterpret_cast<const char*>(this->GetOValue()), 32, true ) );
        rDictionary.AddKey( PdfName("U"), PdfString( reinterpret_cast<const char*>(this->GetUValue()), 32, true ) );
        
        stdCf.AddKey( PdfName("AuthEvent"), PdfName("DocOpen") );
        cf.AddKey( PdfName("StdCF"), stdCf );
        
        rDictionary.AddKey( PdfName("CF"), cf );
        rDictionary.AddKey( PdfName("StrF"), PdfName("StdCF") );
        rDictionary.AddKey( PdfName("StmF"), PdfName("StdCF") );
        
        rDictionary.AddKey( PdfName("V"), static_cast<pdf_int64>(4LL) );
        rDictionary.AddKey( PdfName("R"), static_cast<pdf_int64>(4LL) );
        rDictionary.AddKey( PdfName("Length"), static_cast<pdf_int64>(128LL) );
		if(!m_bEncryptMetadata)
			rDictionary.AddKey( PdfName("EncryptMetadata"), PdfVariant( false ) );
	}
    else if(m_eAlgorithm == ePdfEncryptAlgorithm_RC4V1)
    {
        rDictionary.AddKey( PdfName("V"), static_cast<pdf_int64>(1LL) );
        // Can be 2 or 3
        rDictionary.AddKey( PdfName("R"), static_cast<pdf_int64>(m_rValue) );
    }
    else if(m_eAlgorithm == ePdfEncryptAlgorithm_RC4V2)
    {
        rDictionary.AddKey( PdfName("V"), static_cast<pdf_int64>(2LL) );
        rDictionary.AddKey( PdfName("R"), static_cast<pdf_int64>(3LL) );
		rDictionary.AddKey( PdfName("Length"), PdfVariant( static_cast<pdf_int64>(m_eKeyLength) ) );
    }
    
    rDictionary.AddKey( PdfName("O"), PdfString( reinterpret_cast<const char*>(this->GetOValue()), 32, true ) );
    rDictionary.AddKey( PdfName("U"), PdfString( reinterpret_cast<const char*>(this->GetUValue()), 32, true ) );
    rDictionary.AddKey( PdfName("P"), PdfVariant( static_cast<pdf_int64>(this->GetPValue()) ) );
}
    
void
PdfEncryptRC4::GenerateEncryptionKey(const PdfString & documentId)
{
    unsigned char userpswd[32];
    unsigned char ownerpswd[32];
    
    // Pad passwords
    PadPassword( m_userPass,  userpswd  );
    PadPassword( m_ownerPass, ownerpswd );
    
    // Compute O value
    ComputeOwnerKey(userpswd, ownerpswd, m_keyLength, m_rValue, false, m_oValue);
    
    // Compute encryption key and U value
    m_documentId = std::string( documentId.GetString(), documentId.GetLength() );
    ComputeEncryptionKey(m_documentId, userpswd,
                         m_oValue, m_pValue, m_eKeyLength, m_rValue, m_uValue, m_bEncryptMetadata);
}
    
bool PdfEncryptRC4::Authenticate( const std::string & password, const PdfString & documentId )
{
    bool ok = false;
    
    m_documentId = std::string( documentId.GetString(), documentId.GetLength() );
    
    // Pad password
    unsigned char userKey[32];
    unsigned char pswd[32];
    PadPassword( password, pswd );
    
    // Check password: 1) as user password, 2) as owner password
    ComputeEncryptionKey(m_documentId, pswd, m_oValue, m_pValue, m_eKeyLength, m_rValue, userKey, m_bEncryptMetadata);
    
    ok = CheckKey(userKey, m_uValue);
    if (!ok)
    {
        unsigned char userpswd[32];
        ComputeOwnerKey( m_oValue, pswd, m_keyLength, m_rValue, true, userpswd );
        ComputeEncryptionKey( m_documentId, userpswd, m_oValue, m_pValue, m_eKeyLength, m_rValue, userKey, m_bEncryptMetadata );
        ok = CheckKey( userKey, m_uValue );
        
        if( ok )
            m_ownerPass = password;
    }
    else
        m_userPass = password;
    
    return ok;
}

pdf_long PdfEncryptRC4::CalculateStreamOffset() const
{
    return 0;
}

pdf_long
PdfEncryptRC4::CalculateStreamLength(pdf_long length) const
{
    return length;
}

void
PdfEncryptRC4::Encrypt(const unsigned char* inStr, pdf_long inLen,
                       unsigned char* outStr, pdf_long outLen) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    int keylen;
    
    CreateObjKey( objkey, &keylen );
    
    const_cast<PdfEncryptRC4*>(this)->RC4(objkey, keylen, inStr, inLen, outStr, outLen);
}

void
PdfEncryptRC4::Decrypt(const unsigned char* inStr, pdf_long inLen,
                       unsigned char* outStr, pdf_long &outLen) const
{
    Encrypt(inStr, inLen, outStr, outLen);
}

PdfInputStream* PdfEncryptRC4::CreateEncryptionInputStream( PdfInputStream* pInputStream )
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    int keylen;
    
    this->CreateObjKey( objkey, &keylen );
    
    return new PdfRC4InputStream( pInputStream, m_rc4key, m_rc4last, objkey, keylen );
}

PdfEncryptRC4::PdfEncryptRC4(PdfString oValue, PdfString uValue, int pValue, int rValue, EPdfEncryptAlgorithm eAlgorithm, long length, bool encryptMetadata)
{
    m_pValue = pValue;
    m_rValue = rValue;
    m_eAlgorithm = eAlgorithm;
    m_eKeyLength = static_cast<EPdfKeyLength>(length);
    m_keyLength  = length/8;
	m_bEncryptMetadata = encryptMetadata;
    memcpy( m_oValue, oValue.GetString(), 32 );
    memcpy( m_uValue, uValue.GetString(), 32 );
    
    // Init buffers
    memset(m_rc4key, 0, 16);
    memset(m_rc4last, 0, 256);
    memset(m_encryptionKey, 0, 32);
}

PdfEncryptRC4::PdfEncryptRC4( const std::string & userPassword, const std::string & ownerPassword, int protection,
                             EPdfEncryptAlgorithm eAlgorithm, EPdfKeyLength eKeyLength )
{
    // setup object
    int keyLength = static_cast<int>(eKeyLength);
    
    m_userPass = userPassword;
    m_ownerPass =  ownerPassword;
    m_eAlgorithm = eAlgorithm;
    m_eKeyLength = eKeyLength;
    
    switch (eAlgorithm)
    {
        case ePdfEncryptAlgorithm_RC4V2:
            keyLength = keyLength - keyLength % 8;
            keyLength = (keyLength >= 40) ? ((keyLength <= 128) ? keyLength : 128) : 40;
            m_rValue = 3;
            m_keyLength = keyLength / 8;
            break;
        case ePdfEncryptAlgorithm_RC4V1:
        default:
            m_rValue = 2;
            m_keyLength = 40 / 8;
            break;
        case ePdfEncryptAlgorithm_AESV2:
#ifdef PODOFO_HAVE_LIBIDN
        case ePdfEncryptAlgorithm_AESV3:
#endif // PODOFO_HAVE_LIBIDN
            break;
    }
    
    // Init buffers
    memset(m_rc4key, 0, 16);
    memset(m_oValue, 0, 48);
    memset(m_uValue, 0, 48);
    memset(m_rc4last, 0, 256);
    memset(m_encryptionKey, 0, 32);
    
    // Compute P value
    m_pValue = PERMS_DEFAULT | protection;
}

PdfOutputStream* PdfEncryptRC4::CreateEncryptionOutputStream( PdfOutputStream* pOutputStream )
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    int keylen;
    
    this->CreateObjKey( objkey, &keylen );
    
    return new PdfRC4OutputStream( pOutputStream, m_rc4key, m_rc4last, objkey, keylen );
}
    
PdfEncryptAESBase::PdfEncryptAESBase()
{
    m_aes = new AESCryptoEngine();
}

PdfEncryptAESBase::~PdfEncryptAESBase()
{
    delete m_aes;
}

void
PdfEncryptAESBase::BaseDecrypt(const unsigned char* key, int keyLen, const unsigned char* iv,
                       const unsigned char* textin, pdf_long textlen,
                       unsigned char* textout, pdf_long &outLen )
{
	if (textlen % keyLen != 0)
		PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-decryption data length not a multiple of the key length" );

    EVP_CIPHER_CTX* aes = m_aes->getEngine();
    
    int status;
    if(keyLen == PdfEncrypt::ePdfKeyLength_128/8)
        status = EVP_DecryptInit_ex(aes, EVP_aes_128_cbc(), NULL, key, iv);
#ifdef PODOFO_HAVE_LIBIDN
    else if (keyLen == PdfEncrypt::ePdfKeyLength_256/8)
        status = EVP_DecryptInit_ex(aes, EVP_aes_256_cbc(), NULL, key, iv);
#endif
    else
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Invalid AES key length" );
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing AES decryption engine" );
    
	int dataOutMoved;
    status = EVP_DecryptUpdate(aes, textout, &dataOutMoved, textin, textlen);
	outLen = dataOutMoved;
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-decryption data" );
    
    status = EVP_DecryptFinal_ex(aes, textout + outLen, &dataOutMoved);
	outLen += dataOutMoved;
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-decryption data final" );
}

void
PdfEncryptAESBase::BaseEncrypt(const unsigned char* key, int keyLen, const unsigned char* iv,
                       const unsigned char* textin, pdf_long textlen,
                       unsigned char* textout, pdf_long ) // To avoid Wunused-parameter
{
    EVP_CIPHER_CTX* aes = m_aes->getEngine();
    
    int status;
    if(keyLen == PdfEncrypt::ePdfKeyLength_128/8)
        status = EVP_EncryptInit_ex(aes, EVP_aes_128_cbc(), NULL, key, iv);
#ifdef PODOFO_HAVE_LIBIDN
    else if (keyLen == PdfEncrypt::ePdfKeyLength_256/8)
        status = EVP_EncryptInit_ex(aes, EVP_aes_256_cbc(), NULL, key, iv);
#endif
    else
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Invalid AES key length" );
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing AES encryption engine" );
    
    int dataOutMoved;
    status = EVP_EncryptUpdate(aes, textout, &dataOutMoved, textin, textlen);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-encrypting data" );
    
    status = EVP_EncryptFinal_ex(aes, &textout[dataOutMoved], &dataOutMoved);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-encrypting data" );
}
    
void
PdfEncryptAESV2::GenerateEncryptionKey(const PdfString & documentId)
{
    unsigned char userpswd[32];
    unsigned char ownerpswd[32];
    
    // Pad passwords
    PadPassword( m_userPass,  userpswd  );
    PadPassword( m_ownerPass, ownerpswd );
    
    // Compute O value
    ComputeOwnerKey(userpswd, ownerpswd, m_keyLength, m_rValue, false, m_oValue);
    
    // Compute encryption key and U value
    m_documentId = std::string( documentId.GetString(), documentId.GetLength() );
    ComputeEncryptionKey(m_documentId, userpswd,
                         m_oValue, m_pValue, m_eKeyLength, m_rValue, m_uValue, m_bEncryptMetadata);
}

bool PdfEncryptAESV2::Authenticate( const std::string & password, const PdfString & documentId )
{
    bool ok = false;
    
    m_documentId = std::string( documentId.GetString(), documentId.GetLength() );
    
    // Pad password
    unsigned char userKey[32];
    unsigned char pswd[32];
    PadPassword( password, pswd );
    
    // Check password: 1) as user password, 2) as owner password
    ComputeEncryptionKey(m_documentId, pswd, m_oValue, m_pValue, m_eKeyLength, m_rValue, userKey, m_bEncryptMetadata);
    
    ok = CheckKey(userKey, m_uValue);
    if (!ok)
    {
        unsigned char userpswd[32];
        ComputeOwnerKey( m_oValue, pswd, m_keyLength, m_rValue, true, userpswd );
        ComputeEncryptionKey( m_documentId, userpswd, m_oValue, m_pValue, m_eKeyLength, m_rValue, userKey, m_bEncryptMetadata );
        ok = CheckKey( userKey, m_uValue );
        
        if( ok )
            m_ownerPass = password;
    }
    else
        m_userPass = password;
    
    return ok;
}
    
pdf_long PdfEncryptAESV2::CalculateStreamOffset() const
{
    return AES_IV_LENGTH;
}
    
void
PdfEncryptAESV2::Encrypt(const unsigned char* inStr, pdf_long inLen,
                         unsigned char* outStr, pdf_long outLen) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    int keylen;
    
    CreateObjKey( objkey, &keylen );
    
    pdf_long offset = CalculateStreamOffset();
    const_cast<PdfEncryptAESV2*>(this)->GenerateInitialVector(outStr);
    
    const_cast<PdfEncryptAESV2*>(this)->BaseEncrypt(objkey, keylen, outStr, inStr, inLen, &outStr[offset], outLen-offset);
}

void
PdfEncryptAESV2::Decrypt(const unsigned char* inStr, pdf_long inLen,
                         unsigned char* outStr, pdf_long &outLen) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    int keylen;
    
    CreateObjKey( objkey, &keylen );
    
    pdf_long offset = CalculateStreamOffset();
	if( inLen <= offset ) { // Is empty
		outLen = 0;
		return;
	}
    
    const_cast<PdfEncryptAESV2*>(this)->BaseDecrypt(objkey, keylen, inStr, &inStr[offset], inLen-offset, outStr, outLen);
}
    
PdfEncryptAESV2::PdfEncryptAESV2( const std::string & userPassword, const std::string & ownerPassword, int protection) : PdfEncryptAESBase()
{
    // setup object
    m_userPass = userPassword;
    m_ownerPass = ownerPassword;
    m_eAlgorithm = ePdfEncryptAlgorithm_AESV2;
    
    m_rValue = 4;
    m_eKeyLength = ePdfKeyLength_128;
    m_keyLength = ePdfKeyLength_128 / 8;
    
    // Init buffers
    memset(m_rc4key, 0 ,16);
    memset(m_rc4last, 0 ,256);
    memset(m_oValue, 0 ,48);
    memset(m_uValue, 0 ,48);
    memset(m_encryptionKey, 0 ,32);
    
    // Compute P value
    m_pValue = PERMS_DEFAULT | protection;
}
    
PdfEncryptAESV2::PdfEncryptAESV2(PdfString oValue, PdfString uValue, int pValue, bool encryptMetadata) : PdfEncryptAESBase()
{
    m_pValue = pValue;
    m_eAlgorithm = ePdfEncryptAlgorithm_AESV2;
    
    m_eKeyLength = ePdfKeyLength_128;
    m_keyLength  = ePdfKeyLength_128 / 8;
    m_rValue	 = 4;
	m_bEncryptMetadata = encryptMetadata;
    memcpy( m_oValue, oValue.GetString(), 32 );
    memcpy( m_uValue, uValue.GetString(), 32 );
    
    // Init buffers
    memset(m_rc4key, 0 ,16);
    memset(m_rc4last, 0 ,256);
    memset(m_encryptionKey, 0 ,32);
}

pdf_long
PdfEncryptAESV2::CalculateStreamLength(pdf_long length) const
{
    pdf_long realLength = ((length + 15) & ~15) + AES_IV_LENGTH;
    if (length % 16 == 0)
    {
        realLength += 16;
    }
    
    return realLength;
}
    
PdfInputStream* PdfEncryptAESV2::CreateEncryptionInputStream( PdfInputStream* pInputStream )
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    int keylen;
     
    this->CreateObjKey( objkey, &keylen );

	return new PdfAESInputStream( pInputStream, objkey, keylen );
}
    
PdfOutputStream* PdfEncryptAESV2::CreateEncryptionOutputStream( PdfOutputStream* )
{
    /*unsigned char objkey[MD5_DIGEST_LENGTH];
     int keylen;
     
     this->CreateObjKey( objkey, &keylen );*/
    
    PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "CreateEncryptionOutputStream does not yet support AESV2" );
}
    
#ifdef PODOFO_HAVE_LIBIDN
    
PdfEncryptSHABase::PdfEncryptSHABase( const PdfEncrypt & rhs ) : PdfEncrypt(rhs)
{
    const PdfEncrypt* ptr = &rhs;
    
    memcpy( m_uValue, rhs.GetUValue(), sizeof(unsigned char) * 48 );
    memcpy( m_oValue, rhs.GetOValue(), sizeof(unsigned char) * 48 );
    
    memcpy( m_encryptionKey, rhs.GetEncryptionKey(), sizeof(unsigned char) * 32 );
    
    memcpy( m_permsValue, static_cast<const PdfEncryptSHABase*>(ptr)->m_permsValue, sizeof(unsigned char) * 16 );
    
    memcpy( m_ueValue, static_cast<const PdfEncryptSHABase*>(ptr)->m_ueValue, sizeof(unsigned char) * 32 );
    memcpy( m_oeValue, static_cast<const PdfEncryptSHABase*>(ptr)->m_oeValue, sizeof(unsigned char) * 32 );
}
    
void PdfEncryptSHABase::ComputeUserKey(const unsigned char * userpswd, int len)
{
    // Generate User Salts
    unsigned char vSalt[8];
    unsigned char kSalt[8];
    
    for(int i=0; i< 8 ; i++)
    {
        vSalt[i] = rand()%255;
        kSalt[i] = rand()%255;
    }
    
    // Generate hash for U
    unsigned char hashValue[32];
    
    SHA256_CTX context;
    SHA256_Init(&context);
    
    SHA256_Update(&context, userpswd, len);
    SHA256_Update(&context, vSalt, 8);
    SHA256_Final(hashValue, &context);
    
    // U = hash + validation salt + key salt
    memcpy(m_uValue, hashValue, 32);
    memcpy(m_uValue+32, vSalt, 8);
    memcpy(m_uValue+32+8, kSalt, 8);
    
    // Generate hash for UE
    SHA256_Init(&context);
    SHA256_Update(&context, userpswd, len);
    SHA256_Update(&context, kSalt, 8);
    SHA256_Final(hashValue, &context);
    
    // UE = AES-256 encoded file encryption key with key=hash
    // CBC mode, no padding, init vector=0
    
    EVP_CIPHER_CTX aes;
    EVP_CIPHER_CTX_init(&aes);
    
    int status = EVP_EncryptInit_ex(&aes, EVP_aes_256_cbc(), NULL, hashValue, NULL);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing AES encryption engine" );
    EVP_CIPHER_CTX_set_padding(&aes, 0); // disable padding
    
    int dataOutMoved;
    status = EVP_EncryptUpdate(&aes, m_ueValue, &dataOutMoved, m_encryptionKey, m_keyLength);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-encrypting data" );
    
    status = EVP_EncryptFinal_ex(&aes, &m_ueValue[dataOutMoved], &dataOutMoved);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-encrypting data" );
    
    EVP_CIPHER_CTX_cleanup(&aes);
}

void PdfEncryptSHABase::ComputeOwnerKey(const unsigned char * ownerpswd, int len)
{
    // Generate User Salts
    unsigned char vSalt[8];
    unsigned char kSalt[8];
    
    for(int i=0; i< 8 ; i++)
    {
        vSalt[i] = rand()%255;
        kSalt[i] = rand()%255;
    }
    
    // Generate hash for O
    unsigned char hashValue[32];
    SHA256_CTX context;
    SHA256_Init(&context);
    SHA256_Update(&context, ownerpswd, len);
    SHA256_Update(&context, vSalt, 8);
    SHA256_Update(&context, m_uValue, 48);
    SHA256_Final(hashValue, &context);
    
    // O = hash + validation salt + key salt
    memcpy(m_oValue, hashValue, 32);
    memcpy(m_oValue+32, vSalt, 8);
    memcpy(m_oValue+32+8, kSalt, 8);
    
    // Generate hash for OE
    SHA256_Init(&context);
    SHA256_Update(&context, ownerpswd, len);
    SHA256_Update(&context, kSalt, 8);
    SHA256_Update(&context, m_uValue, 48);
    SHA256_Final(hashValue, &context);
    
    // OE = AES-256 encoded file encryption key with key=hash
    // CBC mode, no padding, init vector=0
    
    EVP_CIPHER_CTX aes;
    EVP_CIPHER_CTX_init(&aes);
    
    int status = EVP_EncryptInit_ex(&aes, EVP_aes_256_cbc(), NULL, hashValue, NULL);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing AES encryption engine" );
    EVP_CIPHER_CTX_set_padding(&aes, 0); // disable padding
    
    int dataOutMoved;
    status = EVP_EncryptUpdate(&aes, m_oeValue, &dataOutMoved, m_encryptionKey, m_keyLength);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-encrypting data" );
    
    status = EVP_EncryptFinal_ex(&aes, &m_oeValue[dataOutMoved], &dataOutMoved);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-encrypting data" );
    
    EVP_CIPHER_CTX_cleanup(&aes);
}

void PdfEncryptSHABase::PreprocessPassword( const std::string &password, unsigned char* outBuf, int &len)
{
    char* password_sasl;
    
    if (stringprep_profile(password.c_str(), &password_sasl, "SASLprep", STRINGPREP_NO_UNASSIGNED) != STRINGPREP_OK)
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidPassword, "Error processing password through SASLprep" );
    }
    
    int l = strlen(password_sasl);
    len = l > 127 ? 127 : l;
    
    memcpy(outBuf, password_sasl, len);
    free(password_sasl);
}

void
PdfEncryptSHABase::ComputeEncryptionKey()
{
    // Seed once for all
    srand ( time(NULL) );
    
    for(int i=0; i< m_keyLength ; i++)
        m_encryptionKey[i] = rand()%255;
}
    
bool
PdfEncryptSHABase::Authenticate(const std::string& documentID, const std::string& password,
                                const std::string& uValue, const std::string& ueValue,
                                const std::string& oValue, const std::string& oeValue,
                                int pValue, const std::string& permsValue,
                                int lengthValue, int rValue)
{
    m_pValue = pValue;
    m_keyLength = lengthValue / 8;
    m_rValue = rValue;
    
    memcpy(m_uValue, uValue.c_str(), 48);
    memcpy(m_ueValue, ueValue.c_str(), 32);
    memcpy(m_oValue, oValue.c_str(), 48);
    memcpy(m_oeValue, oeValue.c_str(), 32);
    memcpy(m_permsValue, permsValue.c_str(), 16);
    
    return Authenticate(password, documentID);
}
    
void PdfEncryptSHABase::GenerateInitialVector(unsigned char iv[AES_IV_LENGTH])
{
    for (int i=0; i<AES_IV_LENGTH; i++)
        iv[i] = rand()%255;
}
    
void PdfEncryptSHABase::CreateEncryptionDictionary( PdfDictionary & rDictionary ) const
{
    rDictionary.AddKey( PdfName("Filter"), PdfName("Standard") );
    
    PdfDictionary cf;
    PdfDictionary stdCf;
    
    rDictionary.AddKey( PdfName("V"), static_cast<pdf_int64>(5LL) );
    rDictionary.AddKey( PdfName("R"), static_cast<pdf_int64>(5LL) );
    rDictionary.AddKey( PdfName("Length"), static_cast<pdf_int64>(256LL) );
    
    stdCf.AddKey( PdfName("CFM"), PdfName("AESV3") );
    stdCf.AddKey( PdfName("Length"), static_cast<pdf_int64>(32LL) );
    
    rDictionary.AddKey( PdfName("O"), PdfString( reinterpret_cast<const char*>(this->GetOValue()), 48, true ) );
    rDictionary.AddKey( PdfName("OE"), PdfString( reinterpret_cast<const char*>(this->GetOEValue()), 32, true ) );
    rDictionary.AddKey( PdfName("U"), PdfString( reinterpret_cast<const char*>(this->GetUValue()), 48, true ) );
    rDictionary.AddKey( PdfName("UE"), PdfString( reinterpret_cast<const char*>(this->GetUEValue()), 32, true ) );
    rDictionary.AddKey( PdfName("Perms"), PdfString( reinterpret_cast<const char*>(this->GetPermsValue()), 16, true ) );
    
    stdCf.AddKey( PdfName("AuthEvent"), PdfName("DocOpen") );
    cf.AddKey( PdfName("StdCF"), stdCf );
    
    rDictionary.AddKey( PdfName("CF"), cf );
    rDictionary.AddKey( PdfName("StrF"), PdfName("StdCF") );
    rDictionary.AddKey( PdfName("StmF"), PdfName("StdCF") );
    
    rDictionary.AddKey( PdfName("P"), PdfVariant( static_cast<pdf_int64>(this->GetPValue()) ) );
}
    
void
PdfEncryptAESV3::GenerateEncryptionKey(const PdfString &)
{
    // Prepare passwords
    unsigned char userpswd[127];
    unsigned char ownerpswd[127];
    int userpswdLen;
    int ownerpswdLen;
    PreprocessPassword(m_userPass, userpswd, userpswdLen);
    PreprocessPassword(m_ownerPass, ownerpswd, ownerpswdLen);
    
    // Compute encryption key
    ComputeEncryptionKey();
    
    // Compute U and UE values
    ComputeUserKey(userpswd, userpswdLen);
    
    // Compute O and OE value
    ComputeOwnerKey(ownerpswd, ownerpswdLen);
    
    // Compute Perms value
    unsigned char perms[16];
    // First 4 bytes = 32bits permissions
    perms[3] = (m_pValue >> 24) & 0xff;
    perms[2] = (m_pValue >> 16) & 0xff;
    perms[1] = (m_pValue >> 8) & 0xff;
    perms[0] = m_pValue & 0xff;
    // Placeholder for future versions that may need 64-bit permissions
    perms[4] = 0xff;
    perms[5] = 0xff;
    perms[6] = 0xff;
    perms[7] = 0xff;
    // if EncryptMetadata is false, this value should be set to 'F'
	perms[8] = m_bEncryptMetadata ? 'T' : 'F';
    // Next 3 bytes are mandatory
    perms[9] = 'a';
    perms[10] = 'd';
    perms[11] = 'b';
    // Next 4 bytes are ignored
    perms[12] = 0;
    perms[13] = 0;
    perms[14] = 0;
    perms[15] = 0;
    
    // Encrypt Perms value
    
    EVP_CIPHER_CTX aes;
    EVP_CIPHER_CTX_init(&aes);
    
    int status = EVP_EncryptInit_ex(&aes, EVP_aes_256_ecb(), NULL, m_encryptionKey, NULL);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error initializing AES encryption engine" );
    EVP_CIPHER_CTX_set_padding(&aes, 0); // disable padding
    
    int dataOutMoved;
    status = EVP_EncryptUpdate(&aes, m_permsValue, &dataOutMoved, perms, 16);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-encrypting data" );
    
    status = EVP_EncryptFinal_ex(&aes, &m_permsValue[dataOutMoved], &dataOutMoved);
    if(status != 1)
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Error AES-encrypting data" );
    
    EVP_CIPHER_CTX_cleanup(&aes);
}

bool PdfEncryptAESV3::Authenticate( const std::string & password, const PdfString & )
{
    bool ok = false;
    
    // Prepare password
    unsigned char pswd_sasl[127];
    int pswdLen;
    PreprocessPassword(password, pswd_sasl, pswdLen);
    
    unsigned char valSalt[8];
    unsigned char keySalt[8];
    
    // Test 1: is it the user key ?
    memcpy(valSalt, &m_uValue[32], 8);
    memcpy(keySalt, &m_uValue[40], 8);
    unsigned char hashValue[32];
    SHA256_CTX context;
    SHA256_Init(&context);
    SHA256_Update(&context, pswd_sasl, pswdLen);
    SHA256_Update(&context, valSalt, 8);
    SHA256_Final(hashValue, &context);
    
    ok = CheckKey(hashValue, m_uValue);
    if(!ok)
    {
        // Test 2: is it the owner key ?
        memcpy(valSalt, &m_oValue[32], 8);
        memcpy(keySalt, &m_oValue[40], 8);
        SHA256_Init(&context);
        SHA256_Update(&context, pswd_sasl, pswdLen);
        SHA256_Update(&context, valSalt, 8);
        SHA256_Update(&context, m_uValue, 48);
        SHA256_Final(hashValue, &context);
        
        ok = CheckKey(hashValue, m_oValue);
        
        if(ok)
            m_ownerPass = password;
    }
    else
        m_userPass = password;
    
    // TODO Validate permissions (or not...)
    
    return ok;
}

pdf_long PdfEncryptAESV3::CalculateStreamOffset() const
{
    return AES_IV_LENGTH;
}

void
PdfEncryptAESV3::Encrypt(const unsigned char* inStr, pdf_long inLen,
                         unsigned char* outStr, pdf_long outLen) const
{
    pdf_long offset = CalculateStreamOffset();
    const_cast<PdfEncryptAESV3*>(this)->GenerateInitialVector(outStr);
    
    const_cast<PdfEncryptAESV3*>(this)->BaseEncrypt(const_cast<unsigned char*>(m_encryptionKey), m_keyLength, outStr, inStr, inLen, &outStr[offset], outLen-offset);
}

void
PdfEncryptAESV3::Decrypt(const unsigned char* inStr, pdf_long inLen,
                         unsigned char* outStr, pdf_long &outLen) const
{
    pdf_long offset = CalculateStreamOffset();
    
    const_cast<PdfEncryptAESV3*>(this)->BaseDecrypt(const_cast<unsigned char*>(m_encryptionKey), m_keyLength, inStr, &inStr[offset], inLen-offset, outStr, outLen);
}

PdfEncryptAESV3::PdfEncryptAESV3( const std::string & userPassword, const std::string & ownerPassword, int protection) : PdfEncryptAESBase()
{
    // setup object
    m_userPass = userPassword;
    m_ownerPass = ownerPassword;
    m_eAlgorithm = ePdfEncryptAlgorithm_AESV3;
    
    m_rValue = 5;
    m_eKeyLength = ePdfKeyLength_256;
    m_keyLength = ePdfKeyLength_256 / 8;
    
    // Init buffers
    memset(m_oValue, 0 ,48);
    memset(m_uValue, 0 ,48);
    memset(m_encryptionKey, 0 ,32);
    memset(m_ueValue, 0 ,32);
    memset(m_oeValue, 0 ,32);
    
    // Compute P value
    m_pValue = PERMS_DEFAULT | protection;
}

PdfEncryptAESV3::PdfEncryptAESV3(PdfString oValue,PdfString oeValue, PdfString uValue, PdfString ueValue, int pValue, PdfString permsValue) : PdfEncryptAESBase()
{
    m_pValue = pValue;
    m_eAlgorithm = ePdfEncryptAlgorithm_AESV3;
    
    m_eKeyLength = ePdfKeyLength_256;
    m_keyLength  = ePdfKeyLength_256 / 8;
    m_rValue	 = 5;
    memcpy( m_oValue, oValue.GetString(), 48 );
    memcpy( m_oeValue, oeValue.GetString(), 32 );
    memcpy( m_uValue, uValue.GetString(), 48 );
    memcpy( m_ueValue, ueValue.GetString(), 32 );
    memcpy( m_permsValue, permsValue.GetString(), 16 );
    memset(m_encryptionKey, 0 ,32);
}

pdf_long
PdfEncryptAESV3::CalculateStreamLength(pdf_long length) const
{
    pdf_long realLength = ((length + 15) & ~15) + AES_IV_LENGTH;
    if (length % 16 == 0)
    {
        realLength += 16;
    }
    
    return realLength;
}

PdfInputStream* PdfEncryptAESV3::CreateEncryptionInputStream( PdfInputStream* )
{
    PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "CreateEncryptionInputStream does not yet support AESV3" );
}

PdfOutputStream* PdfEncryptAESV3::CreateEncryptionOutputStream( PdfOutputStream* )
{
    PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "CreateEncryptionOutputStream does not yet support AESV3" );
}
    
#endif // PODOFO_HAVE_LIBIDN
}
