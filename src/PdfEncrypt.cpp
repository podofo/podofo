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
#include "PdfRijndael.h"
#include "PdfDefinesPrivate.h"

#include <stdlib.h>
#include <string.h>
#include <sstream>

namespace PoDoFo {


/** A class that can encrypt/decrpyt streamed data block wise
 *  This is used in the input and output stream encryption implementation.
 *  Only the RC4 encryption algorithm is supported
 */
class PdfRC4Stream {
public:
    PdfRC4Stream( unsigned char rc4key[256], unsigned char rc4last[256], unsigned char* key, int keylen )
        : m_a( 0 ), m_b( 0 )
    {
        int i;
        int j;
        int t;

        if (memcmp(key,rc4key,keylen) != 0)
        {
            for (i = 0; i < 256; i++)
                m_rc4[i] = i;

            j = 0;
            for (i = 0; i < 256; i++)
            {
                t = m_rc4[i];
                j = (j + t + key[i % keylen]) % 256;
                m_rc4[i] = m_rc4[j];
                m_rc4[j] = t;
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
    virtual pdf_long Read( char* pBuffer, pdf_long lLen )
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


// ----------------
// MD5 by RSA
// ----------------

// C headers for MD5
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MD5_HASHBYTES 16

/// Structure representing an MD5 context while ecrypting. (For internal use only)
typedef struct MD5Context
{
  unsigned int buf[4];
  unsigned int bits[2];
  unsigned char in[64];
} MD5_CTX;

static void  MD5Init(MD5_CTX *context);
static void  MD5Update(MD5_CTX *context, unsigned char const *buf, unsigned len);
static void  MD5Final(unsigned char digest[MD5_HASHBYTES], MD5_CTX *context);
static void  MD5Transform(unsigned int buf[4], unsigned int const in[16]);

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
static void MD5Final(unsigned char digest[MD5_HASHBYTES], MD5_CTX *ctx)
{
  unsigned count;
  unsigned char *p;

  /* Compute number of bytes mod 64 */
  count = (ctx->bits[0] >> 3) & 0x3F; 

  /* Set the first char of padding to 0x80.  This is safe since there is
     always at least one byte free */
  p = ctx->in + count;
  *p++ = 0x80;

  /* Bytes of padding needed to make 64 bytes */
  count = 64 - 1 - count;

  /* Pad out to 56 mod 64 */
  if (count < 8)
  {
    /* Two lots of padding:  Pad the first block to 64 bytes */
    memset(p, 0, count);
    MD5Transform(ctx->buf, reinterpret_cast<unsigned int *>(ctx->in));

    /* Now fill the next block with 56 bytes */
    memset(ctx->in, 0, 56);
  }
  else
  {
    /* Pad block to 56 bytes */
    memset(p, 0, count - 8);   
  }

  /* Append length in bits and transform */
  reinterpret_cast<unsigned int *>(ctx->in)[14] = ctx->bits[0];
  reinterpret_cast<unsigned int *>(ctx->in)[15] = ctx->bits[1];

  MD5Transform(ctx->buf, reinterpret_cast<unsigned int *>(ctx->in));
  memcpy(digest, ctx->buf, MD5_HASHBYTES);
  memset( reinterpret_cast<char *>(ctx), 0, sizeof(ctx));       /* In case it's sensitive */
}

static void MD5Init(MD5_CTX *ctx)
{
  ctx->buf[0] = 0x67452301;
  ctx->buf[1] = 0xefcdab89;
  ctx->buf[2] = 0x98badcfe;
  ctx->buf[3] = 0x10325476;

  ctx->bits[0] = 0;
  ctx->bits[1] = 0;
}

static void MD5Update(MD5_CTX *ctx, unsigned char const *buf, unsigned len)
{
  unsigned int t;

  /* Update bitcount */

  t = ctx->bits[0];
  if ((ctx->bits[0] = t + ( static_cast<unsigned int>(len) << 3)) < t)
  {
        ctx->bits[1]++;         /* Carry from low to high */
  }
  ctx->bits[1] += len >> 29;

  t = (t >> 3) & 0x3f;        /* Bytes already in shsInfo->data */

  /* Handle any leading odd-sized chunks */

  if (t)
  {
    unsigned char *p = static_cast<unsigned char *>(ctx->in) + t;

    t = 64 - t;
    if (len < t)
    {
      memcpy(p, buf, len);
      return;
    }
    memcpy(p, buf, t);
    MD5Transform(ctx->buf, reinterpret_cast<unsigned int *>(ctx->in));
    buf += t;
    len -= t;
  }
  /* Process data in 64-byte chunks */

  while (len >= 64)
  {
    memcpy(ctx->in, buf, 64);
    MD5Transform(ctx->buf, reinterpret_cast<unsigned int *>(ctx->in));
    buf += 64;
    len -= 64;
  }

  /* Handle any remaining bytes of data. */

  memcpy(ctx->in, buf, len);
}


/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))   
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s) \
        ( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
static void MD5Transform(unsigned int buf[4], unsigned int const in[16])
{
  register unsigned int a, b, c, d;

  a = buf[0];
  b = buf[1];
  c = buf[2];
  d = buf[3];

  MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7); 
  MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
  MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
  MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
  MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7); 
  MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
  MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
  MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22); 
  MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);  
  MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12); 
  MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
  MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
  MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7); 
  MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
  MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
  MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

  MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);  
  MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);  
  MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
  MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20); 
  MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);  
  MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9); 
  MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
  MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20); 
  MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);  
  MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9); 
  MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14); 
  MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20); 
  MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
  MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);  
  MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
  MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

  MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
  MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
  MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
  MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
  MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);  
  MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11); 
  MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16); 
  MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
  MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4); 
  MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11); 
  MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16); 
  MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23); 
  MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);  
  MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
  MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
  MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23); 

  MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
  MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
  MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
  MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21); 
  MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6); 
  MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10); 
  MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
  MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21); 
  MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);  
  MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
  MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15); 
  MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
  MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);  
  MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
  MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15); 
  MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21); 

  buf[0] += a;
  buf[1] += b;
  buf[2] += c;
  buf[3] += d;
}
 

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
    PdfEncrypt *pdfEncrypt = 0;
   
    switch (eAlgorithm)
    {
        case ePdfEncryptAlgorithm_AESV2:
            pdfEncrypt = new PdfEncryptAES(userPassword, ownerPassword, protection);
            break;
        case ePdfEncryptAlgorithm_RC4V2:           
        case ePdfEncryptAlgorithm_RC4V1:
        default:
            pdfEncrypt = new PdfEncryptRC4(userPassword, ownerPassword, protection, eAlgorithm, eKeyLength);
            break;
    }
    return pdfEncrypt;
}

PdfEncrypt *
PdfEncrypt::CreatePdfEncrypt( const PdfObject* pObject )
{
    PdfEncrypt *pdfEncrypt = 0;
    if( !pObject->GetDictionary().HasKey( PdfName("Filter") ) ||
        pObject->GetDictionary().GetKey( PdfName("Filter" ) )->GetName() != PdfName("Standard") )
    {
        std::ostringstream oss;
        oss << "Unsupported encryption filter: " << pObject->GetDictionary().GetKey( PdfName("Filter" ) )->GetName().GetName();
        PODOFO_RAISE_ERROR_INFO( ePdfError_UnsupportedFilter, oss.str().c_str() );
    }
        
    long lV;
    long long lLength;
    int rValue;
    int pValue;
    PdfString oValue;
    PdfString uValue;
    
    try {
        PdfString sTmp;
        
        lV           = static_cast<long>(pObject->GetDictionary().GetKey( PdfName("V") )->GetNumber());
        rValue       = static_cast<int>(pObject->GetDictionary().GetKey( PdfName("R") )->GetNumber());

        pValue       = static_cast<int>(pObject->GetDictionary().GetKey( PdfName("P") )->GetNumber());
        oValue       = pObject->GetDictionary().GetKey( PdfName("O") )->GetString();
             
        uValue       = pObject->GetDictionary().GetKey( PdfName("U") )->GetString();
		
        if( pObject->GetDictionary().HasKey( PdfName("Length") ) )
            lLength = pObject->GetDictionary().GetKey( PdfName("Length") )->GetNumber();

    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__, "Invalid key in encryption dictionary" );
        throw e;
    }

    if( lV == 1L && rValue == 2L ) 
    {
        pdfEncrypt = new PdfEncryptRC4(oValue, uValue, pValue, rValue, ePdfEncryptAlgorithm_RC4V1, 40);
    }
    else if( lV == 2L && rValue == 3L ) 
    {
			// [Alexey] - lLength is long long. Please make changes in encryption algorithms
        pdfEncrypt = new PdfEncryptRC4(oValue, uValue, pValue, rValue, ePdfEncryptAlgorithm_RC4V2, static_cast<int>(lLength));
    }
    else if( lV == 4L && rValue == 4L ) 
    {
        pdfEncrypt = new PdfEncryptAES(oValue, uValue, pValue);      
    }
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
    PdfEncrypt *pdfEncrypt = 0;
    if (rhs.m_eAlgorithm == ePdfEncryptAlgorithm_AESV2)
    {
        pdfEncrypt = new PdfEncryptAES(rhs);
    }
    else
    {
        pdfEncrypt = new PdfEncryptRC4(rhs);
    }
    return pdfEncrypt;
}

PdfEncrypt::PdfEncrypt( const PdfEncrypt & rhs )  
{
    this->operator=( rhs );
}

PdfEncrypt::~PdfEncrypt()
{
}

const PdfEncrypt & PdfEncrypt::operator=( const PdfEncrypt & rhs )
{
    m_eAlgorithm = rhs.m_eAlgorithm;
    m_eKeyLength = rhs.m_eKeyLength;

    memcpy( m_uValue, rhs.m_uValue, sizeof(unsigned char) * 32 );
    memcpy( m_oValue, rhs.m_oValue, sizeof(unsigned char) * 32 );

    m_pValue = rhs.m_pValue;
    m_rValue = rhs.m_rValue;

    memcpy( m_encryptionKey, rhs.m_encryptionKey, sizeof(unsigned char) * 16 );

    m_keyLength = rhs.m_keyLength;

    memcpy( m_rc4key, rhs.m_rc4key, sizeof(unsigned char) * 16 );
    memcpy( m_rc4last, rhs.m_rc4last, sizeof(unsigned char) * 256 );

    m_curReference = rhs.m_curReference;
    m_documentId   = rhs.m_documentId;
    m_userPass     = rhs.m_userPass;
    m_ownerPass    = rhs.m_ownerPass;

    return *this;
}

void
PdfEncrypt::PadPassword(const std::string& password, unsigned char pswd[32])
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

void
PdfEncrypt::GenerateEncryptionKey(const PdfString & documentId)
{
    unsigned char userpswd[32];
    unsigned char ownerpswd[32];

    // Pad passwords
    PadPassword( m_userPass,  userpswd  );
    PadPassword( m_ownerPass, ownerpswd );
    
    // Compute O value
    ComputeOwnerKey(userpswd, ownerpswd, m_keyLength*8, m_rValue, false, m_oValue);
    
    // Compute encryption key and U value
    m_documentId = std::string( documentId.GetString(), documentId.GetLength() );
    ComputeEncryptionKey(m_documentId, userpswd,
                         m_oValue, m_pValue, m_keyLength*8, m_rValue, m_uValue);
}

bool PdfEncrypt::Authenticate( const std::string & password, const PdfString & documentId )
{
    bool ok = false;

    m_documentId = std::string( documentId.GetString(), documentId.GetLength() );

    // Pad password
    unsigned char userKey[32];
    unsigned char pswd[32];
    PadPassword( password, pswd );

    // Check password: 1) as user password, 2) as owner password
    ComputeEncryptionKey(m_documentId, pswd, m_oValue, m_pValue, m_keyLength*8, m_rValue, userKey);

    ok = CheckKey(userKey, m_uValue);
    if (!ok)
    {
        unsigned char userpswd[32];
        ComputeOwnerKey( m_oValue, pswd, m_keyLength*8, m_rValue, true, userpswd );
        ComputeEncryptionKey( m_documentId, userpswd, m_oValue, m_pValue, m_keyLength*8, m_rValue, userKey );
        ok = CheckKey( userKey, m_oValue );

        if( ok ) 
            m_ownerPass = password;
    }
    else
        m_userPass = password;

    return ok;
}

bool
PdfEncrypt::Authenticate(const std::string& documentID, const std::string& password,
                         const std::string& uValue, const std::string& oValue,
                         int pValue, int lengthValue, int rValue)
{
  unsigned char userKey[32];
  bool ok = false;
  int j;
  for (j = 0; j < 32; j++)
  {
      m_uValue[j] = static_cast<unsigned char>( uValue[j] );
      m_oValue[j] = static_cast<unsigned char>( oValue[j] );
  }
  m_pValue = pValue;
  m_keyLength = lengthValue / 8;

  // Pad password
  unsigned char pswd[32];
  PadPassword(password, pswd);

  // Check password: 1) as user password, 2) as owner password
  ComputeEncryptionKey(documentID, pswd, m_oValue, pValue, lengthValue, rValue, userKey);
  ok = CheckKey(userKey, m_uValue);
  if (!ok)
  {
    unsigned char userpswd[32];
    ComputeOwnerKey(m_oValue, pswd, lengthValue, rValue, true, userpswd);
    ComputeEncryptionKey(documentID, userpswd, m_oValue, pValue, lengthValue, rValue, userKey);
    ok = CheckKey(userKey, m_uValue);
  }
  return ok;
}

void
PdfEncrypt::ComputeOwnerKey(unsigned char userPad[32], unsigned char ownerPad[32],
                              int keyLength, int revision, bool authenticate,
                              unsigned char ownerKey[32])
{
  unsigned char mkey[MD5_HASHBYTES];
  unsigned char digest[MD5_HASHBYTES];
  int length = keyLength / 8;

  MD5_CTX ctx;
  MD5Init(&ctx);
  MD5Update(&ctx, ownerPad, 32);
  MD5Final(digest,&ctx);

  if (revision == 3 || revision == 4)
  {
    // only use for the input as many bit as the key consists of
    int k;
    for (k = 0; k < 50; ++k)
    {
      MD5Init(&ctx);
      MD5Update(&ctx, digest, length);
      MD5Final(digest,&ctx);
    }
    memcpy(ownerKey, userPad, 32);
    int i;
    int j;
    for (i = 0; i < 20; ++i)
    {
      for (j = 0; j < length ; ++j)
      {
        if (authenticate)
        {
          mkey[j] = (digest[j] ^ (19-i));
        }
        else
        {
          mkey[j] = (digest[j] ^ i);
        }
      }
      RC4(mkey, length, ownerKey, 32, ownerKey);
    }
  }
  else
  {
    RC4(digest, 5, userPad, 32, ownerKey);
  }
}

void
PdfEncrypt::ComputeEncryptionKey(const std::string& documentId,
                                   unsigned char userPad[32], unsigned char ownerKey[32],
                                   int pValue, int keyLength, int revision,
                                   unsigned char userKey[32])
{
  int j;
  int k;
  m_keyLength = keyLength / 8;

  MD5_CTX ctx;
  MD5Init(&ctx);
  MD5Update(&ctx, userPad, 32);
  MD5Update(&ctx, ownerKey, 32);

  unsigned char ext[4];
  ext[0] = static_cast<unsigned char> ( pValue        & 0xff);
  ext[1] = static_cast<unsigned char> ((pValue >>  8) & 0xff);
  ext[2] = static_cast<unsigned char> ((pValue >> 16) & 0xff);
  ext[3] = static_cast<unsigned char> ((pValue >> 24) & 0xff);
  MD5Update(&ctx, ext, 4);

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
    MD5Update(&ctx, docId, docIdLength);
  }
  
  // TODO: (Revision 3 or greater) If document metadata is not being encrypted,
  //       pass 4 bytes with the value 0xFFFFFFFF to the MD5 hash function.

  unsigned char digest[MD5_HASHBYTES];
  MD5Final(digest,&ctx);

  // only use the really needed bits as input for the hash
  if (revision == 3 || revision == 4)
  {
    for (k = 0; k < 50; ++k)
    {
      MD5Init(&ctx);
      MD5Update(&ctx, digest, m_keyLength);
      MD5Final(digest, &ctx);
    }
  }

  memcpy(m_encryptionKey, digest, m_keyLength);

  // Setup user key
  if (revision == 3 || revision == 4)
  {
    MD5Init(&ctx);
    MD5Update(&ctx, padding, 32);
    if (docId != NULL)
    {
      MD5Update(&ctx, docId, docIdLength);
    }
    MD5Final(digest, &ctx);
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
      RC4(digest, m_keyLength, userKey, 16, userKey);
    }
  }
  else
  {
    RC4(m_encryptionKey, m_keyLength, padding, 32, userKey);
  }
  if (docId != NULL)
  {
    delete [] docId;
  }
}

bool
PdfEncrypt::CheckKey(unsigned char key1[32], unsigned char key2[32])
{
  // Check whether the right password had been given
  bool ok = true;
  int k;
  int kmax = (m_rValue == 3) ? 16 : 32;
  for (k = 0; ok && k < kmax; k++)
  {
    ok = ok && (key1[k] == key2[k]);
  }
  return ok;
}

void
PdfEncrypt::Encrypt(std::string& str, pdf_long inputLen) const
{
  size_t len = str.length();
  unsigned char* data = new unsigned char[len];
  size_t j;
  for (j = 0; j < len; j++)
  {
    data[j] = static_cast<unsigned char> ( str[j] );
  }
  Encrypt(data, inputLen);
  for (j = 0; j < len; j++)
  {
    str[j] = data[j];
  }
  delete [] data;
}

void PdfEncrypt::CreateObjKey( unsigned char objkey[16], int* pnKeyLen ) const
{
  int n = m_curReference.ObjectNumber();
  int g = m_curReference.GenerationNumber();

  unsigned char nkey[MD5_HASHBYTES+5+4];
  int nkeylen = m_keyLength + 5;
  int j;
  for (j = 0; j < m_keyLength; j++)
  {
    nkey[j] = m_encryptionKey[j];
  }
  nkey[m_keyLength+0] = 0xff &  n;
  nkey[m_keyLength+1] = 0xff & (n >> 8);
  nkey[m_keyLength+2] = 0xff & (n >> 16);
  nkey[m_keyLength+3] = 0xff &  g;
  nkey[m_keyLength+4] = 0xff & (g >> 8);

  if (m_rValue == 4)
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

/**
* RC4 is the standard encryption algorithm used in PDF format
*/

void
PdfEncrypt::RC4(unsigned char* key, int keylen,
                unsigned char* textin, pdf_long textlen,
                unsigned char* textout)
{
  int i;
  int j;
  int t;
  unsigned char rc4[256];

  if (memcmp(key,m_rc4key,keylen) != 0)
  {
    for (i = 0; i < 256; i++)
    {
      rc4[i] = i;
    }
    j = 0;
    for (i = 0; i < 256; i++)
    {
      t = rc4[i];
      j = (j + t + key[i % keylen]) % 256;
      rc4[i] = rc4[j];
      rc4[j] = t;
    }
    memcpy(m_rc4key,key,keylen);
    memcpy(m_rc4last,rc4,256);
  }
  else
  {
    memcpy(rc4,m_rc4last,256);
  }

  int a = 0;
  int b = 0;
  unsigned char k;
  for (i = 0; i < textlen; i++)
  {
    a = (a + 1) % 256;
    t = rc4[a];
    b = (b + t) % 256;
    rc4[a] = rc4[b];
    rc4[b] = t;
    k = rc4[(rc4[a] + rc4[b]) % 256];
    textout[i] = textin[i] ^ k;
  }
}

void
PdfEncrypt::GetMD5Binary(const unsigned char* data, int length, unsigned char* digest)
{
  MD5_CTX ctx;
  MD5Init(&ctx);
  MD5Update(&ctx, data, length);
  MD5Final(digest,&ctx);
}

void PdfEncrypt::GenerateInitialVector(unsigned char iv[16])
{
    GetMD5Binary(reinterpret_cast<const unsigned char*>(m_documentId.c_str()), 
                 static_cast<unsigned int>(m_documentId.length()), iv);
}

pdf_long
PdfEncrypt::CalculateStreamLength(pdf_long length) const
{
	return length;
}

pdf_long PdfEncrypt::CalculateStreamOffset() const
{
	return 0;
}

PdfString PdfEncrypt::GetMD5String( const unsigned char* pBuffer, int nLength )
{
    char data[MD5_HASHBYTES];

    PdfEncrypt::GetMD5Binary( pBuffer, nLength, reinterpret_cast<unsigned char*>(data) );

    return PdfString( data, MD5_HASHBYTES, true );
}

void
PdfEncryptAES::Encrypt(unsigned char* str, pdf_long len) const
{
  unsigned char objkey[MD5_HASHBYTES];
  int keylen;

  CreateObjKey( objkey, &keylen );

  const_cast<PdfEncryptAES*>(this)->AES(objkey, keylen, str, len, str);  
}
PdfEncryptAES::PdfEncryptAES( const std::string & userPassword, const std::string & ownerPassword, int protection)
{
    // setup object
	m_userPass = userPassword; 
	m_ownerPass = ownerPassword;

    m_rValue = 4;
    m_keyLength = 128 / 8;
    m_eKeyLength = ePdfKeyLength_128;
    m_eAlgorithm = ePdfEncryptAlgorithm_AESV2;
    m_aes = new PdfRijndael();         
    
    int j;
    for (j = 0; j < 16; j++)
    {
        m_rc4key[j] = 0;
    }
    // Compute P value
    m_pValue = -((protection ^ 255) + 1);
}

PdfEncryptAES::PdfEncryptAES(PdfString oValue, PdfString uValue, int pValue)
{
    m_pValue = pValue;
    m_eAlgorithm = ePdfEncryptAlgorithm_AESV2;
    m_eKeyLength = ePdfKeyLength_128;
    m_keyLength  = 128 / 8;
    m_aes        = new PdfRijndael();
    m_rValue	 = 4;
    memcpy( m_oValue, oValue.GetString(), 32 );
    memcpy( m_uValue, uValue.GetString(), 32 );
}

PdfEncryptAES::PdfEncryptAES(const PdfEncrypt &rhs) : PdfEncrypt(rhs)
{
   m_aes = new PdfRijndael();
}

PdfEncryptAES::~PdfEncryptAES()
{
	delete m_aes;   
}

pdf_long
PdfEncryptAES::CalculateStreamLength(pdf_long length) const
{
  pdf_long realLength = length;
//  realLength = (length % 0x7ffffff0) + 32;
  realLength = ((length + 15) & ~15) + 16;
  if (length % 16 == 0)
  {
    realLength += 16;
  }
 
  return realLength;
}

void
PdfEncryptAES::AES(unsigned char* key, int,
                unsigned char* textin, pdf_long textlen,
                unsigned char* textout)
{
  GenerateInitialVector(textout);
  m_aes->init( PdfRijndael::CBC, PdfRijndael::Encrypt, key, PdfRijndael::Key16Bytes, textout);
  pdf_long offset = CalculateStreamOffset();
  pdf_long len = m_aes->padEncrypt(&textin[offset], textlen, &textout[offset]);
  
  // It is a good idea to check the error code
  if (len < 0)
  {
    PdfError::DebugMessage( "PdfEncrypt::AES: Error on encrypting." );
  }
}

pdf_long
PdfEncryptAES::CalculateStreamOffset() const
{ 
  return 16;
}

PdfInputStream* PdfEncryptAES::CreateEncryptionInputStream( PdfInputStream* )
{
  unsigned char objkey[MD5_HASHBYTES];
  int keylen;

  this->CreateObjKey( objkey, &keylen );

  PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "CreateEncryptionInputStream does not yet support AES" );
  
  return NULL;
}

void PdfEncryptAES::CreateEncryptionDictionary( PdfDictionary & rDictionary ) const
{
    rDictionary.AddKey( PdfName("Filter"), PdfName("Standard") );
   
    rDictionary.AddKey( PdfName("V"), static_cast<pdf_int64>(4LL) );
    rDictionary.AddKey( PdfName("R"), static_cast<pdf_int64>(4LL) );
    rDictionary.AddKey( PdfName("Length"), static_cast<pdf_int64>(128LL) );

    PdfDictionary cf;
    PdfDictionary stdCf;
    stdCf.AddKey( PdfName("CFM"), PdfName("AESV2") );
    stdCf.AddKey( PdfName("Length"), static_cast<pdf_int64>(16LL) );
    stdCf.AddKey( PdfName("AuthEvent"), PdfName("DocOpen") );
    cf.AddKey( PdfName("StdCF"), stdCf );

    rDictionary.AddKey( PdfName("CF"), cf );
    rDictionary.AddKey( PdfName("StrF"), PdfName("StdCF") );
    rDictionary.AddKey( PdfName("StmF"), PdfName("StdCF") );

    rDictionary.AddKey( PdfName("O"), PdfString( reinterpret_cast<const char*>(this->GetOValue()), 32, true ) );
    rDictionary.AddKey( PdfName("U"), PdfString( reinterpret_cast<const char*>(this->GetUValue()), 32, true ) );
    rDictionary.AddKey( PdfName("P"), PdfVariant( static_cast<pdf_int64>(this->GetPValue()) ) );
}


PdfOutputStream* PdfEncryptAES::CreateEncryptionOutputStream( PdfOutputStream* )
{
  unsigned char objkey[MD5_HASHBYTES];
  int keylen;

  this->CreateObjKey( objkey, &keylen );

  PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "CreateEncryptionOutputStream does not yet support AES" );
  
  return NULL;
}

void
PdfEncryptRC4::Encrypt(unsigned char* str, pdf_long len) const
{
  unsigned char objkey[MD5_HASHBYTES];
  int keylen;

  this->CreateObjKey( objkey, &keylen );

  const_cast<PdfEncryptRC4*>(this)->RC4(objkey, keylen, str, len, str);  
}

PdfInputStream* PdfEncryptRC4::CreateEncryptionInputStream( PdfInputStream* pInputStream )
{
  unsigned char objkey[MD5_HASHBYTES];
  int keylen;

  this->CreateObjKey( objkey, &keylen );

  return new PdfRC4InputStream( pInputStream, m_rc4key, m_rc4last, objkey, keylen );  
}

PdfEncryptRC4::PdfEncryptRC4(PdfString oValue, PdfString uValue, int pValue, int rValue, EPdfEncryptAlgorithm eAlgorithm, long length)
{
    m_pValue = pValue;
    m_rValue = rValue;
    m_eAlgorithm = eAlgorithm;
    m_eKeyLength = static_cast<EPdfKeyLength>(length);
    m_keyLength  = length/8;
    memcpy( m_oValue, oValue.GetString(), 32 );
    memcpy( m_uValue, uValue.GetString(), 32 );
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
	    break;    }
    
    int j;
    for (j = 0; j < 16; j++)
    {
        m_rc4key[j] = 0;
    }

    // Compute P value
    m_pValue = -((protection ^ 255) + 1);
}

PdfOutputStream* PdfEncryptRC4::CreateEncryptionOutputStream( PdfOutputStream* pOutputStream )
{
  unsigned char objkey[MD5_HASHBYTES];
  int keylen;

  this->CreateObjKey( objkey, &keylen );

  return new PdfRC4OutputStream( pOutputStream, m_rc4key, m_rc4last, objkey, keylen );
}

void PdfEncryptRC4::CreateEncryptionDictionary( PdfDictionary & rDictionary ) const
{
    rDictionary.AddKey( PdfName("Filter"), PdfName("Standard") );

    switch( m_eAlgorithm ) 
    {        
        case ePdfEncryptAlgorithm_RC4V1:
            rDictionary.AddKey( PdfName("V"), static_cast<pdf_int64>(1LL) );
            rDictionary.AddKey( PdfName("R"), static_cast<pdf_int64>(2LL) );
            break;
        case ePdfEncryptAlgorithm_RC4V2:
            rDictionary.AddKey( PdfName("V"), static_cast<pdf_int64>(2LL) );
            rDictionary.AddKey( PdfName("R"), static_cast<pdf_int64>(3LL) );
            rDictionary.AddKey( PdfName("Length"), PdfVariant( static_cast<pdf_int64>(m_eKeyLength) ) );
            break;        
	default:
	case ePdfEncryptAlgorithm_AESV2:
	    break;
    }

    rDictionary.AddKey( PdfName("O"), PdfString( reinterpret_cast<const char*>(this->GetOValue()), 32, true ) );
    rDictionary.AddKey( PdfName("U"), PdfString( reinterpret_cast<const char*>(this->GetUValue()), 32, true ) );
    rDictionary.AddKey( PdfName("P"), PdfVariant( static_cast<pdf_int64>(this->GetPValue()) ) );
}


}
