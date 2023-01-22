/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// ---------------------------
// PdfEncrypt implementation
// Based on code from Ulrich Telle: http://wxcode.sourceforge.net/components/wxpdfdoc/
// ---------------------------

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfEncrypt.h"

#include "PdfDictionary.h"
#include "PdfFilter.h"

#ifdef PODOFO_HAVE_LIBIDN
// AES-256 dependencies :
// SASL
#include <stringprep.h>
#include <idn-free.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#endif // PODOFO_HAVE_LIBIDN

#include <openssl/opensslconf.h>
#include <openssl/md5.h>
#include <openssl/evp.h>

using namespace std;
using namespace PoDoFo;

#ifdef PODOFO_HAVE_LIBIDN
PdfEncryptAlgorithm PdfEncrypt::s_EnabledEncryptionAlgorithms =
PdfEncryptAlgorithm::RC4V1 |
PdfEncryptAlgorithm::RC4V2 |
PdfEncryptAlgorithm::AESV2 |
PdfEncryptAlgorithm::AESV3 |
PdfEncryptAlgorithm::AESV3R6;
#else // PODOFO_HAVE_LIBIDN
PdfEncryptAlgorithm PdfEncrypt::s_EnabledEncryptionAlgorithms =
PdfEncryptAlgorithm::RC4V1 |
PdfEncryptAlgorithm::RC4V2 |
PdfEncryptAlgorithm::AESV2;
#endif // PODOFO_HAVE_LIBIDN

#if OPENSSL_VERSION_MAJOR >= 3
#include <openssl/provider.h>
#endif // OPENSSL_VERSION_MAJOR >= 3

class OpenSSLInit
{
public:
    OpenSSLInit()
    {
#if OPENSSL_VERSION_MAJOR >= 3
        m_libCtx = OSSL_LIB_CTX_new();
        if (m_libCtx == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Unable to create OpenSSL library context");

        // NOTE: Load required legacy providers, such as RC4, together regular ones,
        // as explained in https://wiki.openssl.org/index.php/OpenSSL_3.0#Providers
        m_legacyProvider = OSSL_PROVIDER_load(m_libCtx, "legacy");
        if (m_legacyProvider == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Unable to load legacy providers in OpenSSL >= 3.x.x");

        m_defaultProvider = OSSL_PROVIDER_load(m_libCtx, "default");
        if (m_defaultProvider == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Unable to load default providers in OpenSSL >= 3.x.x");

        // https://www.openssl.org/docs/man3.0/man7/crypto.html#FETCHING-EXAMPLES
        Rc4 = EVP_CIPHER_fetch(m_libCtx, "RC4", "provider=legacy");
        Aes128 = EVP_CIPHER_fetch(m_libCtx, "AES-128-CBC", "provider=default");
        Aes256 = EVP_CIPHER_fetch(m_libCtx, "AES-256-CBC", "provider=default");
        MD5 = EVP_MD_fetch(m_libCtx, "MD5", "provider=default");
        SHA256 = EVP_MD_fetch(m_libCtx, "SHA2-256", "provider=default");
        SHA384 = EVP_MD_fetch(m_libCtx, "SHA2-384", "provider=default");
        SHA512 = EVP_MD_fetch(m_libCtx, "SHA2-512", "provider=default");

#else // OPENSSL_VERSION_MAJOR < 3
        Rc4 = EVP_rc4();
        Aes128 = EVP_aes_128_cbc();
        Aes256 = EVP_aes_256_cbc();
        MD5 = EVP_md5();
        SHA256 = EVP_sha256();
        SHA384 = EVP_sha384();
        SHA512 = EVP_sha512();
#endif // OPENSSL_VERSION_MAJOR >= 3
    }

    ~OpenSSLInit()
    {
#if OPENSSL_VERSION_MAJOR >= 3
        OSSL_PROVIDER_unload(m_legacyProvider);
        OSSL_PROVIDER_unload(m_defaultProvider);
        OSSL_LIB_CTX_free(m_libCtx);
#endif // OPENSSL_VERSION_MAJOR >= 3
    }
public:
    const EVP_CIPHER* Rc4;
    const EVP_CIPHER* Aes128;
    const EVP_CIPHER* Aes256;
    const EVP_MD* MD5;
    const EVP_MD* SHA256;
    const EVP_MD* SHA384;
    const EVP_MD* SHA512;
private:
#if OPENSSL_VERSION_MAJOR >= 3
    OSSL_LIB_CTX *m_libCtx;
    OSSL_PROVIDER *m_legacyProvider;
    OSSL_PROVIDER *m_defaultProvider;
#endif // OPENSSL_VERSION_MAJOR >= 3
} s_SSL;

// Default value for P (permissions) = no permission
#define PERMS_DEFAULT (PdfPermissions)0xFFFFF0C0

#define AES_IV_LENGTH 16
#define AES_BLOCK_SIZE 16

namespace PoDoFo
{

// A class that holds the AES Crypto object
class AESCryptoEngine
{
public:
    AESCryptoEngine()
    {
        aes = EVP_CIPHER_CTX_new();
    }
    
    EVP_CIPHER_CTX* getEngine()
    {
        return aes;
    }
    
    ~AESCryptoEngine()
    {
        EVP_CIPHER_CTX_free(aes);
    }

private:
    EVP_CIPHER_CTX *aes;
};

// A class that holds the RC4 Crypto object
// Either CCCrpytor or EVP_CIPHER_CTX
class RC4CryptoEngine
{
public:

    RC4CryptoEngine()
    {
        rc4 = EVP_CIPHER_CTX_new();
    }

    EVP_CIPHER_CTX* getEngine()
    {
        return rc4;
    }

    ~RC4CryptoEngine()
    {
        EVP_CIPHER_CTX_free(rc4);
    }

private:
    EVP_CIPHER_CTX* rc4;
};
    
/** A class that can encrypt/decrpyt streamed data block wise
 *  This is used in the input and output stream encryption implementation.
 *  Only the RC4 encryption algorithm is supported
 */
class PdfRC4Stream
{
public:
    PdfRC4Stream(unsigned char rc4key[256], unsigned char rc4last[256],
        unsigned char* key, unsigned keylen) :
        m_a(0), m_b(0)
    {
        size_t i;
        size_t j;
        size_t t;

        if (std::memcmp(key, rc4key, keylen) != 0)
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

            std::memcpy(rc4key, key, keylen);
            std::memcpy(rc4last, m_rc4, 256);
        }
        else
        {
            std::memcpy(m_rc4, rc4last, 256);
        }
    }

    ~PdfRC4Stream()
    {
    }

    /** Encrypt or decrypt a block
     *
     *  \param buffer the input/output buffer. Data is read from this buffer and also stored here
     *  \param len    the size of the buffer
     */
    size_t Encrypt(char* buffer, size_t len)
    {
        unsigned char k;
        int t;

        // Do not encode data with no length
        if (len == 0)
            return len;

        for (size_t i = 0; i < len; i++)
        {
            m_a = (m_a + 1) % 256;
            t = m_rc4[m_a];
            m_b = (m_b + t) % 256;

            m_rc4[m_a] = m_rc4[m_b];
            m_rc4[m_b] = static_cast<unsigned char>(t);

            k = m_rc4[(m_rc4[m_a] + m_rc4[m_b]) % 256];
            buffer[i] = buffer[i] ^ k;
        }

        return len;
    }

private:
    unsigned char m_rc4[256];

    int m_a;
    int m_b;

};

/** An OutputStream that encrypt all data written
 *  using the RC4 encryption algorithm
 */
class PdfRC4OutputStream : public OutputStream
{
public:
    PdfRC4OutputStream(OutputStream& outputStream, unsigned char rc4key[256],
        unsigned char rc4last[256], unsigned char* key, unsigned keylen) :
        m_OutputStream(&outputStream), m_stream(rc4key, rc4last, key, keylen)
    {
    }

    void writeBuffer(const char* buffer, size_t size) override
    {
        charbuff outputBuffer(size);
        std::memcpy(outputBuffer.data(), buffer, size);

        m_stream.Encrypt(outputBuffer.data(), size);
        m_OutputStream->Write(outputBuffer.data(), size);
    }

private:
    OutputStream* m_OutputStream;
    PdfRC4Stream m_stream;
};

/** An InputStream that decrypts all data read
 *  using the RC4 encryption algorithm
 */
class PdfRC4InputStream : public InputStream
{
public:
    PdfRC4InputStream(InputStream& inputStream, size_t inputLen, unsigned char rc4key[256], unsigned char rc4last[256],
        unsigned char* key, unsigned keylen) :
        m_InputStream(&inputStream),
        m_inputLen(inputLen),
        m_stream(rc4key, rc4last, key, keylen) { }

protected:
    size_t readBuffer(char* buffer, size_t size, bool& eof) override
    {
        // CHECK-ME: The code has never been tested after refactor
        // If it's correct, remove this warning
        bool streameof;
        size_t count = ReadBuffer(*m_InputStream, buffer, std::min(size, m_inputLen), streameof);
        m_inputLen -= count;
        eof = streameof || m_inputLen == 0;
        return m_stream.Encrypt(buffer, count);
    }

private:
    InputStream* m_InputStream;
    size_t m_inputLen;
    PdfRC4Stream m_stream;
};

/** A PdfAESInputStream that decrypts all data read
 *  using the AES encryption algorithm
 */
class PdfAESInputStream : public InputStream
{
public:
    PdfAESInputStream(InputStream& inputStream, size_t inputLen, unsigned char* key, unsigned keylen) :
        m_InputStream(&inputStream),
        m_inputLen(inputLen),
        m_inputEof(false),
        m_init(true),
        m_keyLen(keylen),
        m_drainLeft(-1)
    {
        m_ctx = EVP_CIPHER_CTX_new();
        if (m_ctx == nullptr)
            PODOFO_RAISE_ERROR(PdfErrorCode::OutOfMemory);

        std::memcpy(this->m_key, key, keylen);
    }

    ~PdfAESInputStream()
    {
        EVP_CIPHER_CTX_free(m_ctx);
    }

protected:
    size_t readBuffer(char* buffer, size_t len, bool& eof) override
    {
        int outlen = 0;
        size_t drainLen;
        size_t read;
        if (m_inputEof)
            goto DrainBuffer;

        int rc;
        if (m_init)
        {
            // Read the initialization vector separately first
            char iv[AES_IV_LENGTH];
            bool streameof;
            read = ReadBuffer(*m_InputStream, iv, AES_IV_LENGTH, streameof);
            if (read != AES_IV_LENGTH)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Can't read enough bytes for AES IV");

            const EVP_CIPHER* cipher;
            switch (m_keyLen)
            {
                case (size_t)PdfKeyLength::L128 / 8:
                {
                    cipher = s_SSL.Aes128;
                    break;
                }
#ifdef PODOFO_HAVE_LIBIDN
                case (size_t)PdfKeyLength::L256 / 8:
                {
                    cipher = s_SSL.Aes256;
                    break;
                }
#endif
                default:
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid AES key length");
            }

            rc = EVP_DecryptInit_ex(m_ctx, cipher, nullptr, m_key, (unsigned char*)iv);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing AES encryption engine");

            m_inputLen -= AES_IV_LENGTH;
            m_init = false;
        }

        bool streameof;
        read = ReadBuffer(*m_InputStream, buffer, std::min(len, m_inputLen), streameof);
        m_inputLen -= read;

        // Quote openssl.org: "the decrypted data buffer out passed to EVP_DecryptUpdate() should have sufficient room
        //  for (inl + cipher_block_size) bytes unless the cipher block size is 1 in which case inl bytes is sufficient."
        // So we need to create a buffer that is bigger than len.
        m_tempBuffer.resize(len + AES_BLOCK_SIZE);
        rc = EVP_DecryptUpdate(m_ctx, m_tempBuffer.data(), &outlen, (unsigned char*)buffer, (int)read);

        if (rc != 1)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-decryption data");

        PODOFO_ASSERT((size_t)outlen <= len);
        std::memcpy(buffer, m_tempBuffer.data(), (size_t)outlen);

        if (m_inputLen == 0 || streameof)
        {
            m_inputEof = true;

            int drainLeft;
            rc = EVP_DecryptFinal_ex(m_ctx, m_tempBuffer.data(), &drainLeft);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-decryption data padding");

            m_drainLeft = (size_t)drainLeft;
            goto DrainBuffer;
        }

        return outlen;

    DrainBuffer:
        drainLen = std::min(len - outlen, m_drainLeft);
        std::memcpy(buffer + outlen, m_tempBuffer.data(), drainLen);
        m_drainLeft -= (int)drainLen;
        if (m_drainLeft == 0)
            eof = true;

        return outlen + drainLen;
    }

private:
    EVP_CIPHER_CTX* m_ctx;
    InputStream* m_InputStream;
    size_t m_inputLen;
    bool m_inputEof;
    bool m_init;
    unsigned char m_key[32];
    unsigned m_keyLen;
    vector<unsigned char> m_tempBuffer;
    size_t m_drainLeft;
};

}

PdfEncrypt::~PdfEncrypt() { }

void PdfEncrypt::GenerateEncryptionKey(const PdfString& documentId)
{
    GenerateEncryptionKey(documentId.GetRawData());
}

bool PdfEncrypt::Authenticate(const string_view& password, const PdfString& documentId)
{
    return Authenticate(password, documentId.GetRawData());
}

PdfEncryptAlgorithm PdfEncrypt::GetEnabledEncryptionAlgorithms()
{
    return PdfEncrypt::s_EnabledEncryptionAlgorithms;
}

void PdfEncrypt::SetEnabledEncryptionAlgorithms(PdfEncryptAlgorithm encryptionAlgorithms)
{
    PdfEncrypt::s_EnabledEncryptionAlgorithms = encryptionAlgorithms;
}

bool PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm algorithm)
{
    return (PdfEncrypt::s_EnabledEncryptionAlgorithms & algorithm) != PdfEncryptAlgorithm::None;
}

static unsigned char padding[] =
"\x28\xBF\x4E\x5E\x4E\x75\x8A\x41\x64\x00\x4E\x56\xFF\xFA\x01\x08\x2E\x2E\x00\xB6\xD0\x68\x3E\x80\x2F\x0C\xA9\xFE\x64\x53\x69\x7A";

unique_ptr<PdfEncrypt> PdfEncrypt::Create(const string_view& userPassword,
    const string_view& ownerPassword,
    PdfPermissions protection,
    PdfEncryptAlgorithm algorithm,
    PdfKeyLength keyLength)
{
    switch (algorithm)
    {
#ifdef PODOFO_HAVE_LIBIDN
        case PdfEncryptAlgorithm::AESV3:
            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV3(userPassword, ownerPassword,
                PdfAESV3Revision::R5, protection));
        case PdfEncryptAlgorithm::AESV3R6:
            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV3(userPassword, ownerPassword,
                PdfAESV3Revision::R6, protection));
#endif // PODOFO_HAVE_LIBIDN
        case PdfEncryptAlgorithm::RC4V2:
        case PdfEncryptAlgorithm::RC4V1:
            return unique_ptr<PdfEncrypt>(new PdfEncryptRC4(userPassword, ownerPassword, protection, algorithm, keyLength));
        case PdfEncryptAlgorithm::AESV2:
        default:
            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV2(userPassword, ownerPassword, protection));
    }
}

unique_ptr<PdfEncrypt> PdfEncrypt::CreateFromObject(const PdfObject& encryptObj)
{
    if (!encryptObj.GetDictionary().HasKey(PdfName::KeyFilter) ||
        encryptObj.GetDictionary().GetKey(PdfName::KeyFilter)->GetName() != "Standard")
    {
        if (encryptObj.GetDictionary().HasKey(PdfName::KeyFilter))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported encryption filter: {}",
                encryptObj.GetDictionary().GetKey(PdfName::KeyFilter)->GetName().GetString());
        else
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Encryption dictionary does not have a key /Filter");
    }

    int lV;
    int64_t length;
    int rValue;
    PdfPermissions pValue;
    PdfString oValue;
    PdfString uValue;
    PdfName cfmName;
    bool encryptMetadata = true;

    try
    {
        lV = static_cast<int>(encryptObj.GetDictionary().MustGetKey("V").GetNumber());
        rValue = static_cast<int>(encryptObj.GetDictionary().MustGetKey("R").GetNumber());

        pValue = static_cast<PdfPermissions>(encryptObj.GetDictionary().MustGetKey("P").GetNumber());

        oValue = encryptObj.GetDictionary().MustGetKey("O").GetString();
        uValue = encryptObj.GetDictionary().MustGetKey("U").GetString();

        if (encryptObj.GetDictionary().HasKey("Length"))
            length = encryptObj.GetDictionary().GetKey("Length")->GetNumber();
        else
            length = 0;

        const PdfObject* encryptMetadataObj = encryptObj.GetDictionary().GetKey("EncryptMetadata");
        if (encryptMetadataObj != nullptr && encryptMetadataObj->IsBool())
            encryptMetadata = encryptMetadataObj->GetBool();

        auto stmfObj = encryptObj.GetDictionary().GetKey("StmF");
        if (stmfObj != nullptr && stmfObj->IsName())
        {
            const PdfObject* obj = encryptObj.GetDictionary().GetKey("CF");
            if (obj != nullptr && obj->IsDictionary())
            {
                obj = obj->GetDictionary().GetKey(stmfObj->GetName());
                if (obj != nullptr && obj->IsDictionary())
                {
                    obj = obj->GetDictionary().GetKey("CFM");
                    if (obj != nullptr && obj->IsName())
                        cfmName = obj->GetName();
                }
            }
        }
    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME_INFO(e, "Invalid or missing key in encryption dictionary");
        throw e;
    }

    if ((lV == 1) && (rValue == 2 || rValue == 3)
        && PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::RC4V1))
    {
        return unique_ptr<PdfEncrypt>(new PdfEncryptRC4(oValue, uValue, pValue, rValue, PdfEncryptAlgorithm::RC4V1, (int)PdfKeyLength::L40, encryptMetadata));
    }
    else if ((((lV == 2) && (rValue == 3)) || cfmName == "V2")
        && PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::RC4V2))
    {
        // length is int64_t. Please make changes in encryption algorithms
        // Check key length length here to prevent
        // stack-based buffer over-read later in this file
        if (length > MD5_DIGEST_LENGTH * CHAR_BIT) // length in bits, md5 in bytes
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Given key length too large for MD5");
        }
        return unique_ptr<PdfEncrypt>(new PdfEncryptRC4(oValue, uValue, pValue, rValue, PdfEncryptAlgorithm::RC4V2, static_cast<int>(length), encryptMetadata));
    }
    else
    {
        if ((lV == 4) && (rValue == 4)
            && PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::AESV2))
        {
            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV2(oValue, uValue, pValue, encryptMetadata));
        }
#ifdef PODOFO_HAVE_LIBIDN
        else if ((lV == 5) && (
            (rValue == 5 && PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::AESV3))
            || (rValue == 6 && PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::AESV3R6))))
        {
            PdfString permsValue = encryptObj.GetDictionary().GetKey("Perms")->GetString();
            PdfString oeValue = encryptObj.GetDictionary().GetKey("OE")->GetString();
            PdfString ueValue = encryptObj.GetDictionary().GetKey("UE")->GetString();

            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV3(oValue, oeValue, uValue,
                ueValue, pValue, permsValue, (PdfAESV3Revision)rValue));
        }
#endif // PODOFO_HAVE_LIBIDN
        else
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported encryption method Version={} Revision={}", lV , rValue);
        }
    }
}

unique_ptr<PdfEncrypt> PdfEncrypt::CreateFromEncrypt(const PdfEncrypt& rhs)
{
    switch (rhs.m_Algorithm)
    {
        case PdfEncryptAlgorithm::RC4V1:
        case PdfEncryptAlgorithm::RC4V2:
            return unique_ptr<PdfEncrypt>(new PdfEncryptRC4(rhs));
        case PdfEncryptAlgorithm::AESV2:
            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV2(rhs));
#ifdef PODOFO_HAVE_LIBIDN
        case PdfEncryptAlgorithm::AESV3:
        case PdfEncryptAlgorithm::AESV3R6:
            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV3(rhs));
#endif // PODOFO_HAVE_LIBIDN
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Invalid algorithm");
    }
}

PdfEncrypt::PdfEncrypt() :
    m_Algorithm(PdfEncryptAlgorithm::AESV2),
    m_eKeyLength(PdfKeyLength::L128),
    m_keyLength(0),
    m_rValue(0),
    m_pValue(PdfPermissions::None),
    m_EncryptMetadata(true)
{
    memset(m_uValue, 0, 48);
    memset(m_oValue, 0, 48);
    memset(m_encryptionKey, 0, 32);
};

PdfEncrypt::PdfEncrypt(const PdfEncrypt& rhs)
{
    m_Algorithm = rhs.m_Algorithm;
    m_eKeyLength = rhs.m_eKeyLength;

    m_pValue = rhs.m_pValue;
    m_rValue = rhs.m_rValue;

    m_keyLength = rhs.m_keyLength;

    m_documentId = rhs.m_documentId;
    m_userPass = rhs.m_userPass;
    m_ownerPass = rhs.m_ownerPass;
    m_EncryptMetadata = rhs.m_EncryptMetadata;
}

bool PdfEncrypt::CheckKey(unsigned char key1[32], unsigned char key2[32])
{
    // Check whether the right password had been given
    bool success = true;
    for (unsigned k = 0; success && k < m_keyLength; k++)
        success = success && (key1[k] == key2[k]);

    return success;
}

PdfEncryptMD5Base::PdfEncryptMD5Base()
    : m_rc4key{ }, m_rc4last{ }
{
}

PdfEncryptMD5Base::PdfEncryptMD5Base(const PdfEncrypt& rhs) : PdfEncrypt(rhs)
{
    const PdfEncrypt* ptr = &rhs;

    std::memcpy(m_uValue, rhs.GetUValue(), sizeof(unsigned char) * 32);
    std::memcpy(m_oValue, rhs.GetOValue(), sizeof(unsigned char) * 32);

    std::memcpy(m_encryptionKey, rhs.GetEncryptionKey(), sizeof(unsigned char) * 16);

    std::memcpy(m_rc4key, static_cast<const PdfEncryptMD5Base*>(ptr)->m_rc4key, sizeof(unsigned char) * 16);
    std::memcpy(m_rc4last, static_cast<const PdfEncryptMD5Base*>(ptr)->m_rc4last, sizeof(unsigned char) * 256);
    m_EncryptMetadata = static_cast<const PdfEncryptMD5Base*>(ptr)->m_EncryptMetadata;
}

void PdfEncryptMD5Base::PadPassword(const string_view& password, unsigned char pswd[32])
{
    size_t m = password.length();

    if (m > 32) m = 32;

    size_t j;
    size_t p = 0;
    for (j = 0; j < m; j++)
        pswd[p++] = static_cast<unsigned char>(password[j]);

    for (j = 0; p < 32 && j < 32; j++)
        pswd[p++] = padding[j];
}

bool PdfEncryptMD5Base::Authenticate(const string_view& documentID, const string_view& password,
    const bufferview& uValue, const bufferview& oValue,
    PdfPermissions pValue, int lengthValue, int rValue)
{
    m_pValue = pValue;
    m_keyLength = lengthValue / 8;
    m_rValue = rValue;

    std::memcpy(m_uValue, uValue.data(), 32);
    std::memcpy(m_oValue, oValue.data(), 32);

    return Authenticate(password, documentID);
}

void PdfEncryptMD5Base::ComputeOwnerKey(const unsigned char userPad[32], const unsigned char ownerPad[32],
    int keyLength, int revision, bool authenticate, unsigned char ownerKey[32])
{
    unsigned char mkey[MD5_DIGEST_LENGTH];
    unsigned char digest[MD5_DIGEST_LENGTH];
    int rc;

    unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (ctx == nullptr || (rc = EVP_DigestInit_ex(ctx.get(), s_SSL.MD5, nullptr)) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing MD5 hashing engine");

    rc = EVP_DigestUpdate(ctx.get(), ownerPad, 32);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

    rc = EVP_DigestFinal_ex(ctx.get(), digest, nullptr);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

    if ((revision == 3) || (revision == 4))
    {
        // only use for the input as many bit as the key consists of
        for (int k = 0; k < 50; ++k)
        {
            rc = EVP_DigestInit_ex(ctx.get(), s_SSL.MD5, nullptr);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing MD5 hashing engine");

            rc = EVP_DigestUpdate(ctx.get(), digest, keyLength);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

            rc = EVP_DigestFinal_ex(ctx.get(), digest, nullptr);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");
        }
        std::memcpy(ownerKey, userPad, 32);
        for (unsigned i = 0; i < 20; ++i)
        {
            for (int j = 0; j < keyLength; ++j)
            {
                if (authenticate)
                    mkey[j] = static_cast<unsigned char>(static_cast<unsigned>(digest[j] ^ (19 - i)));
                else
                    mkey[j] = static_cast<unsigned char>(static_cast<unsigned>(digest[j] ^ i));
            }
            RC4(mkey, keyLength, ownerKey, 32, ownerKey, 32);
        }
    }
    else
    {
        RC4(digest, 5, userPad, 32, ownerKey, 32);
    }
}

void PdfEncryptMD5Base::ComputeEncryptionKey(const string_view& documentId,
    const unsigned char userPad[32], const unsigned char ownerKey[32],
    PdfPermissions pValue, PdfKeyLength keyLength, int revision,
    unsigned char userKey[32], bool encryptMetadata)
{
    unsigned j;
    unsigned k;
    m_keyLength = (int)keyLength / 8;
    int rc;

    unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (ctx == nullptr || (rc = EVP_DigestInit_ex(ctx.get(), s_SSL.MD5, nullptr)) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing MD5 hashing engine");

    rc = EVP_DigestUpdate(ctx.get(), userPad, 32);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

    rc = EVP_DigestUpdate(ctx.get(), ownerKey, 32);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

    unsigned char ext[4];
    ext[0] = static_cast<unsigned char> (((unsigned)pValue >> 0) & 0xFF);
    ext[1] = static_cast<unsigned char> (((unsigned)pValue >> 8) & 0xFF);
    ext[2] = static_cast<unsigned char> (((unsigned)pValue >> 16) & 0xFF);
    ext[3] = static_cast<unsigned char> (((unsigned)pValue >> 24) & 0xFF);
    rc = EVP_DigestUpdate(ctx.get(), ext, 4);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

    unsigned docIdLength = static_cast<unsigned>(documentId.length());
    vector<unsigned char> docId;
    if (docIdLength > 0)
    {
        docId.resize(docIdLength);
        size_t j;
        for (j = 0; j < docIdLength; j++)
        {
            docId[j] = static_cast<unsigned char>(documentId[j]);
        }
        rc = EVP_DigestUpdate(ctx.get(), docId.data(), docIdLength);
        if (rc != 1)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");
    }

    // If document metadata is not being encrypted, 
    // pass 4 bytes with the value 0xFFFFFFFF to the MD5 hash function.
    if (!encryptMetadata)
    {
        unsigned char noMetaAddition[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
        rc = EVP_DigestUpdate(ctx.get(), noMetaAddition, 4);
        if (rc != 1)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");
    }

    unsigned char digest[MD5_DIGEST_LENGTH];
    rc = EVP_DigestFinal_ex(ctx.get(), digest, nullptr);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

    // only use the really needed bits as input for the hash
    if (revision == 3 || revision == 4)
    {
        for (k = 0; k < 50; ++k)
        {
            rc = EVP_DigestInit_ex(ctx.get(), s_SSL.MD5, nullptr);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing MD5 hashing engine");

            rc = EVP_DigestUpdate(ctx.get(), digest, m_keyLength);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

            rc = EVP_DigestFinal_ex(ctx.get(), digest, nullptr);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");
        }
    }

    std::memcpy(m_encryptionKey, digest, m_keyLength);

    // Setup user key
    if (revision == 3 || revision == 4)
    {
        rc = EVP_DigestInit_ex(ctx.get(), s_SSL.MD5, nullptr);
        if (rc != 1)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing MD5 hashing engine");

        rc = EVP_DigestUpdate(ctx.get(), padding, 32);
        if (rc != 1)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

        if (docId.size() != 0)
        {
            rc = EVP_DigestUpdate(ctx.get(), docId.data(), docIdLength);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");
        }

        rc = EVP_DigestFinal_ex(ctx.get(), digest, nullptr);
        if (rc != 1)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

        std::memcpy(userKey, digest, 16);
        for (k = 16; k < 32; ++k)
            userKey[k] = 0;

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
}

void PdfEncryptMD5Base::CreateObjKey(unsigned char objkey[16], unsigned& pnKeyLen, const PdfReference& objref) const
{
    const unsigned n = static_cast<unsigned>(objref.ObjectNumber());
    const unsigned g = static_cast<unsigned>(objref.GenerationNumber());

    unsigned nkeylen = m_keyLength + 5;
    unsigned char nkey[MD5_DIGEST_LENGTH + 5 + 4];
    for (unsigned j = 0; j < m_keyLength; j++)
        nkey[j] = m_encryptionKey[j];

    nkey[m_keyLength + 0] = static_cast<unsigned char>(0xFF & n);
    nkey[m_keyLength + 1] = static_cast<unsigned char>(0xFF & (n >> 8));
    nkey[m_keyLength + 2] = static_cast<unsigned char>(0xFF & (n >> 16));
    nkey[m_keyLength + 3] = static_cast<unsigned char>(0xFF & g);
    nkey[m_keyLength + 4] = static_cast<unsigned char>(0xFF & (g >> 8));

    if (m_Algorithm == PdfEncryptAlgorithm::AESV2)
    {
        // AES encryption needs some 'salt'
        nkeylen += 4;
        nkey[m_keyLength + 5] = 0x73;
        nkey[m_keyLength + 6] = 0x41;
        nkey[m_keyLength + 7] = 0x6C;
        nkey[m_keyLength + 8] = 0x54;
    }

    GetMD5Binary(nkey, nkeylen, objkey);
    pnKeyLen = (m_keyLength <= 11) ? m_keyLength + 5 : 16;
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

void PdfEncryptRC4Base::RC4(const unsigned char* key, unsigned keylen,
    const unsigned char* textin, size_t textlen,
    unsigned char* textout, size_t textoutlen) const
{
    EVP_CIPHER_CTX* rc4 = m_rc4->getEngine();

    if (textlen != textoutlen)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing RC4 encryption engine");

    // Don't set the key because we will modify the parameters
    int status = EVP_EncryptInit_ex(rc4, s_SSL.Rc4, nullptr, nullptr, nullptr);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing RC4 encryption engine");

    status = EVP_CIPHER_CTX_set_key_length(rc4, keylen);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing RC4 encryption engine");

    // We finished modifying parameters so now we can set the key
    status = EVP_EncryptInit_ex(rc4, nullptr, nullptr, key, nullptr);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing RC4 encryption engine");

    int dataOutMoved;
    status = EVP_EncryptUpdate(rc4, textout, &dataOutMoved, textin, (int)textlen);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error RC4-encrypting data");

    status = EVP_EncryptFinal_ex(rc4, &textout[dataOutMoved], &dataOutMoved);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error RC4-encrypting data");
}
        
void PdfEncryptMD5Base::GetMD5Binary(const unsigned char* data, unsigned length, unsigned char* digest)
{
    int rc;
    unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (ctx == nullptr || (rc = EVP_DigestInit_ex(ctx.get(), s_SSL.MD5, nullptr)) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing MD5 hashing engine");

    rc = EVP_DigestUpdate(ctx.get(), data, length);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

    rc = EVP_DigestFinal_ex(ctx.get(), digest, nullptr);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");
}

void PdfEncryptMD5Base::GenerateInitialVector(unsigned char iv[]) const
{
    GetMD5Binary(reinterpret_cast<const unsigned char*>(m_documentId.c_str()),
        static_cast<unsigned>(m_documentId.length()), iv);
}
    
PdfString PdfEncryptMD5Base::GetMD5String(const unsigned char* buffer, unsigned length)
{
    char data[MD5_DIGEST_LENGTH];

    GetMD5Binary(buffer, length, reinterpret_cast<unsigned char*>(data));

    return PdfString::FromRaw({ data, MD5_DIGEST_LENGTH });
}
    
void PdfEncryptMD5Base::CreateEncryptionDictionary(PdfDictionary& dictionary) const
{
    dictionary.AddKey(PdfName::KeyFilter, PdfName("Standard"));

    if (m_Algorithm == PdfEncryptAlgorithm::AESV2 || !m_EncryptMetadata)
    {
        PdfDictionary cf;
        PdfDictionary stdCf;

        if (m_Algorithm == PdfEncryptAlgorithm::RC4V2)
            stdCf.AddKey("CFM", PdfName("V2"));
        else
            stdCf.AddKey("CFM", PdfName("AESV2"));
        stdCf.AddKey("Length", static_cast<int64_t>(16));

        dictionary.AddKey("O", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetOValue()), 32 }));
        dictionary.AddKey("U", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetUValue()), 32 }));

        stdCf.AddKey("AuthEvent", PdfName("DocOpen"));
        cf.AddKey("StdCF", stdCf);

        dictionary.AddKey("CF", cf);
        dictionary.AddKey("StrF", PdfName("StdCF"));
        dictionary.AddKey("StmF", PdfName("StdCF"));

        dictionary.AddKey("V", static_cast<int64_t>(4));
        dictionary.AddKey("R", static_cast<int64_t>(4));
        dictionary.AddKey("Length", static_cast<int64_t>(128));
        if (!m_EncryptMetadata)
            dictionary.AddKey("EncryptMetadata", PdfVariant(false));
    }
    else if (m_Algorithm == PdfEncryptAlgorithm::RC4V1)
    {
        dictionary.AddKey("V", static_cast<int64_t>(1));
        // Can be 2 or 3
        dictionary.AddKey("R", static_cast<int64_t>(m_rValue));
    }
    else if (m_Algorithm == PdfEncryptAlgorithm::RC4V2)
    {
        dictionary.AddKey("V", static_cast<int64_t>(2));
        dictionary.AddKey("R", static_cast<int64_t>(3));
        dictionary.AddKey("Length", PdfVariant(static_cast<int64_t>(m_eKeyLength)));
    }

    dictionary.AddKey("O", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetOValue()), 32 }));
    dictionary.AddKey("U", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetUValue()), 32 }));
    dictionary.AddKey("P", PdfVariant(static_cast<int64_t>(this->GetPValue())));
}
    
void PdfEncryptRC4::GenerateEncryptionKey(const string_view& documentId)
{
    unsigned char userpswd[32];
    unsigned char ownerpswd[32];

    // Pad passwords
    PadPassword(m_userPass, userpswd);
    PadPassword(m_ownerPass, ownerpswd);

    // Compute O value
    ComputeOwnerKey(userpswd, ownerpswd, m_keyLength, m_rValue, false, m_oValue);

    // Compute encryption key and U value
    m_documentId = documentId;
    ComputeEncryptionKey(m_documentId, userpswd,
        m_oValue, m_pValue, m_eKeyLength, m_rValue, m_uValue, m_EncryptMetadata);
}
    
bool PdfEncryptRC4::Authenticate(const string_view& password, const string_view& documentId)
{
    bool success = false;

    m_documentId = documentId;

    // Pad password
    unsigned char userKey[32];
    unsigned char pswd[32];
    PadPassword(password, pswd);

    // Check password: 1) as user password, 2) as owner password
    ComputeEncryptionKey(m_documentId, pswd, m_oValue, m_pValue, m_eKeyLength, m_rValue, userKey, m_EncryptMetadata);

    success = CheckKey(userKey, m_uValue);
    if (!success)
    {
        unsigned char userpswd[32];
        ComputeOwnerKey(m_oValue, pswd, m_keyLength, m_rValue, true, userpswd);
        ComputeEncryptionKey(m_documentId, userpswd, m_oValue, m_pValue, m_eKeyLength, m_rValue, userKey, m_EncryptMetadata);
        success = CheckKey(userKey, m_uValue);

        if (success)
            m_ownerPass = password;
    }
    else
        m_userPass = password;

    return success;
}

size_t PdfEncryptRC4::CalculateStreamOffset() const
{
    return 0;
}

size_t PdfEncryptRC4::CalculateStreamLength(size_t length) const
{
    return length;
}

void PdfEncryptRC4::Encrypt(const char* inStr, size_t inLen, const PdfReference& objref,
    char* outStr, size_t outLen) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    CreateObjKey(objkey, keylen, objref);
    this->RC4(objkey, keylen, (const unsigned char *)inStr, inLen,
        (unsigned char*)outStr, outLen);
}

void PdfEncryptRC4::Decrypt(const char* inStr, size_t inLen, const PdfReference& objref,
    char* outStr, size_t& outLen) const
{
    Encrypt(inStr, inLen, objref, outStr, outLen);
}

unique_ptr<InputStream> PdfEncryptRC4::CreateEncryptionInputStream(InputStream& inputStream, size_t inputLen, const PdfReference& objref)
{
    (void)inputLen;
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    this->CreateObjKey(objkey, keylen, objref);
    return unique_ptr<InputStream>(new PdfRC4InputStream(inputStream, inputLen, m_rc4key, m_rc4last, objkey, keylen));
}

PdfEncryptRC4::PdfEncryptRC4(PdfString oValue, PdfString uValue, PdfPermissions pValue, int rValue,
    PdfEncryptAlgorithm algorithm, int length, bool encryptMetadata)
{
    m_pValue = pValue;
    m_rValue = rValue;
    m_Algorithm = algorithm;
    m_eKeyLength = static_cast<PdfKeyLength>(length);
    m_keyLength = length / 8;
    m_EncryptMetadata = encryptMetadata;
    std::memcpy(m_oValue, oValue.GetRawData().data(), 32);
    std::memcpy(m_uValue, uValue.GetRawData().data(), 32);

    // Init buffers
    std::memset(m_rc4key, 0, 16);
    std::memset(m_rc4last, 0, 256);
    std::memset(m_encryptionKey, 0, 32);
}

PdfEncryptRC4::PdfEncryptRC4(const string_view& userPassword, const string_view& ownerPassword, PdfPermissions protection,
    PdfEncryptAlgorithm algorithm, PdfKeyLength keyLength)
{
    // setup object
    m_userPass = userPassword;
    m_ownerPass = ownerPassword;
    m_Algorithm = algorithm;
    m_eKeyLength = keyLength;
    int intKeyLength = static_cast<int>(keyLength);

    switch (algorithm)
    {
        case PdfEncryptAlgorithm::RC4V2:
        {
            intKeyLength = intKeyLength - intKeyLength % 8;
            intKeyLength = (intKeyLength >= 40) ? ((intKeyLength <= 128) ? intKeyLength : 128) : 40;
            m_rValue = 3;
            m_keyLength = intKeyLength / 8;
            break;
        }
        case PdfEncryptAlgorithm::RC4V1:
        default:
        {
            m_rValue = 2;
            m_keyLength = 40 / 8;
            break;
        }
        case PdfEncryptAlgorithm::AESV2:
#ifdef PODOFO_HAVE_LIBIDN
        case PdfEncryptAlgorithm::AESV3:
        case PdfEncryptAlgorithm::AESV3R6:
#endif // PODOFO_HAVE_LIBIDN
        {
            break;
        }
    }

    // Init buffers
    std::memset(m_rc4key, 0, 16);
    std::memset(m_oValue, 0, 48);
    std::memset(m_uValue, 0, 48);
    std::memset(m_rc4last, 0, 256);
    std::memset(m_encryptionKey, 0, 32);

    // Compute P value
    m_pValue = PERMS_DEFAULT | protection;
}

PdfEncryptRC4::PdfEncryptRC4(const PdfEncrypt& rhs)
    : PdfEncryptMD5Base(rhs) {}

unique_ptr<OutputStream> PdfEncryptRC4::CreateEncryptionOutputStream(OutputStream& outputStream, const PdfReference& objref)
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    this->CreateObjKey(objkey, keylen, objref);
    return unique_ptr<OutputStream>(new PdfRC4OutputStream(outputStream, m_rc4key, m_rc4last, objkey, keylen));
}
    
PdfEncryptAESBase::PdfEncryptAESBase()
{
    m_aes = new AESCryptoEngine();
}

PdfEncryptAESBase::~PdfEncryptAESBase()
{
    delete m_aes;
}

void PdfEncryptAESBase::BaseDecrypt(const unsigned char* key, unsigned keyLen, const unsigned char* iv,
    const unsigned char* textin, size_t textlen,
    unsigned char* textout, size_t& outLen) const
{
    if ((textlen % 16) != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-decryption data length not a multiple of 16");

    EVP_CIPHER_CTX* aes = m_aes->getEngine();

    int rc;
    if (keyLen == (int)PdfKeyLength::L128 / 8)
        rc = EVP_DecryptInit_ex(aes, s_SSL.Aes128, nullptr, key, iv);
#ifdef PODOFO_HAVE_LIBIDN
    else if (keyLen == (int)PdfKeyLength::L256 / 8)
        rc = EVP_DecryptInit_ex(aes, s_SSL.Aes256, nullptr, key, iv);
#endif
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid AES key length");

    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing AES decryption engine");

    int dataOutMoved;
    rc = EVP_DecryptUpdate(aes, textout, &dataOutMoved, textin, (int)textlen);
    outLen = dataOutMoved;
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-decryption data");

    rc = EVP_DecryptFinal_ex(aes, textout + outLen, &dataOutMoved);
    outLen += dataOutMoved;
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-decryption data final");
}

void PdfEncryptAESBase::BaseEncrypt(const unsigned char* key, unsigned keyLen, const unsigned char* iv,
    const unsigned char* textin, size_t textlen,
    unsigned char* textout, size_t textoutlen) const
{
    (void)textoutlen;
    EVP_CIPHER_CTX* aes = m_aes->getEngine();

    int rc;
    if (keyLen == (int)PdfKeyLength::L128 / 8)
        rc = EVP_EncryptInit_ex(aes, s_SSL.Aes128, nullptr, key, iv);
#ifdef PODOFO_HAVE_LIBIDN
    else if (keyLen == (int)PdfKeyLength::L256 / 8)
        rc = EVP_EncryptInit_ex(aes, s_SSL.Aes256, nullptr, key, iv);
#endif
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid AES key length");

    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing AES encryption engine");

    int dataOutMoved;
    rc = EVP_EncryptUpdate(aes, textout, &dataOutMoved, textin, (int)textlen);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");

    rc = EVP_EncryptFinal_ex(aes, &textout[dataOutMoved], &dataOutMoved);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");
}
    
void PdfEncryptAESV2::GenerateEncryptionKey(const string_view& documentId)
{
    unsigned char userpswd[32];
    unsigned char ownerpswd[32];

    // Pad passwords
    PadPassword(m_userPass, userpswd);
    PadPassword(m_ownerPass, ownerpswd);

    // Compute O value
    ComputeOwnerKey(userpswd, ownerpswd, m_keyLength, m_rValue, false, m_oValue);

    // Compute encryption key and U value
    m_documentId = documentId;
    ComputeEncryptionKey(m_documentId, userpswd,
        m_oValue, m_pValue, m_eKeyLength, m_rValue, m_uValue, m_EncryptMetadata);
}

bool PdfEncryptAESV2::Authenticate(const string_view& password, const string_view& documentId)
{
    m_documentId = documentId;

    // Pad password
    unsigned char pswd[32];
    PadPassword(password, pswd);

    // Check password: 1) as user password, 2) as owner password
    unsigned char userKey[32];
    ComputeEncryptionKey(m_documentId, pswd, m_oValue, m_pValue, m_eKeyLength, m_rValue, userKey, m_EncryptMetadata);

    bool success = CheckKey(userKey, m_uValue);
    if (success)
    {
        m_userPass = password;
    }
    else
    {
        unsigned char userpswd[32];
        ComputeOwnerKey(m_oValue, pswd, m_keyLength, m_rValue, true, userpswd);
        ComputeEncryptionKey(m_documentId, userpswd, m_oValue, m_pValue, m_eKeyLength, m_rValue, userKey, m_EncryptMetadata);
        success = CheckKey(userKey, m_uValue);

        if (success)
            m_ownerPass = password;
    }

    return success;
}
    
size_t PdfEncryptAESV2::CalculateStreamOffset() const
{
    return AES_IV_LENGTH;
}
    
void PdfEncryptAESV2::Encrypt(const char* inStr, size_t inLen, const PdfReference& objref,
    char* outStr, size_t outLen) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    CreateObjKey(objkey, keylen, objref);
    size_t offset = CalculateStreamOffset();
    this->GenerateInitialVector((unsigned char *)outStr);
    this->BaseEncrypt(objkey, keylen, (unsigned char*)outStr, (const unsigned char*)inStr,
        inLen, (unsigned char*)outStr + offset, outLen - offset);
}

void PdfEncryptAESV2::Decrypt(const char* inStr, size_t inLen, const PdfReference& objref,
    char* outStr, size_t& outLen) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    CreateObjKey(objkey, keylen, objref);

    size_t offset = CalculateStreamOffset();
    if (inLen <= offset)
    {
        // Is empty
        outLen = 0;
        return;
    }

    this->BaseDecrypt(objkey, keylen, (const unsigned char*)inStr,
        (const unsigned char*)inStr + offset,
        inLen - offset, (unsigned char*)outStr, outLen);
}
    
PdfEncryptAESV2::PdfEncryptAESV2(const string_view& userPassword, const string_view& ownerPassword, PdfPermissions protection) : PdfEncryptAESBase()
{
    // setup object
    m_userPass = userPassword;
    m_ownerPass = ownerPassword;
    m_Algorithm = PdfEncryptAlgorithm::AESV2;

    m_rValue = 4;
    m_eKeyLength = PdfKeyLength::L128;
    m_keyLength = (int)PdfKeyLength::L128 / 8;

    // Init buffers
    std::memset(m_rc4key, 0, 16);
    std::memset(m_rc4last, 0, 256);
    std::memset(m_oValue, 0, 48);
    std::memset(m_uValue, 0, 48);
    std::memset(m_encryptionKey, 0, 32);

    // Compute P value
    m_pValue = PERMS_DEFAULT | protection;
}

PdfEncryptAESV2::PdfEncryptAESV2(const PdfEncrypt& rhs)
    : PdfEncryptMD5Base(rhs) { }
    
PdfEncryptAESV2::PdfEncryptAESV2(PdfString oValue, PdfString uValue, PdfPermissions pValue, bool encryptMetadata) : PdfEncryptAESBase()
{
    m_pValue = pValue;
    m_Algorithm = PdfEncryptAlgorithm::AESV2;

    m_eKeyLength = PdfKeyLength::L128;
    m_keyLength = (int)PdfKeyLength::L128 / 8;
    m_rValue = 4;
    m_EncryptMetadata = encryptMetadata;
    std::memcpy(m_oValue, oValue.GetRawData().data(), 32);
    std::memcpy(m_uValue, uValue.GetRawData().data(), 32);

    // Init buffers
    std::memset(m_rc4key, 0, 16);
    std::memset(m_rc4last, 0, 256);
    std::memset(m_encryptionKey, 0, 32);
}

size_t PdfEncryptAESV2::CalculateStreamLength(size_t length) const
{
    size_t realLength = ((length + 15) & ~15) + AES_IV_LENGTH;
    if (length % 16 == 0)
        realLength += 16;

    return realLength;
}
    
unique_ptr<InputStream> PdfEncryptAESV2::CreateEncryptionInputStream(InputStream& inputStream, size_t inputLen, const PdfReference& objref)
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    this->CreateObjKey(objkey, keylen, objref);
    return unique_ptr<InputStream>(new PdfAESInputStream(inputStream, inputLen, objkey, keylen));
}
    
unique_ptr<OutputStream> PdfEncryptAESV2::CreateEncryptionOutputStream(OutputStream& outputStream, const PdfReference& objref)
{
    (void)outputStream;
    (void)objref;
    /*unsigned char objkey[MD5_DIGEST_LENGTH];
     int keylen;

     this->CreateObjKey( objkey, &keylen );*/

    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "CreateEncryptionOutputStream does not yet support AESV2");
}
    
#ifdef PODOFO_HAVE_LIBIDN
    
PdfEncryptSHABase::PdfEncryptSHABase()
    : m_ueValue{ }, m_oeValue{ }, m_permsValue{ }
{
}

PdfEncryptSHABase::PdfEncryptSHABase(const PdfEncrypt& rhs) : PdfEncrypt(rhs)
{
    const PdfEncrypt* ptr = &rhs;

    std::memcpy(m_uValue, rhs.GetUValue(), sizeof(unsigned char) * 48);
    std::memcpy(m_oValue, rhs.GetOValue(), sizeof(unsigned char) * 48);

    std::memcpy(m_encryptionKey, rhs.GetEncryptionKey(), sizeof(unsigned char) * 32);

    std::memcpy(m_permsValue, static_cast<const PdfEncryptSHABase*>(ptr)->m_permsValue, sizeof(unsigned char) * 16);

    std::memcpy(m_ueValue, static_cast<const PdfEncryptSHABase*>(ptr)->m_ueValue, sizeof(unsigned char) * 32);
    std::memcpy(m_oeValue, static_cast<const PdfEncryptSHABase*>(ptr)->m_oeValue, sizeof(unsigned char) * 32);
}

void PdfEncryptSHABase::ComputeHash(const unsigned char* pswd, unsigned pswdLen, unsigned char salt[8], unsigned char uValue[48], unsigned char hashValue[32])
{
    PODOFO_ASSERT(pswdLen <= 127);

    int rc;
    unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> sha256(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (sha256 == nullptr || (rc = EVP_DigestInit_ex(sha256.get(), s_SSL.SHA256, nullptr)) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing sha256 hashing engine");

    if (pswdLen != 0)
    {
        rc = EVP_DigestUpdate(sha256.get(), pswd, pswdLen);
    }

    rc = EVP_DigestUpdate(sha256.get(), salt, 8);
    if (uValue != nullptr)
    {
        rc = EVP_DigestUpdate(sha256.get(), uValue, 48);
    }

    rc = EVP_DigestFinal_ex(sha256.get(), hashValue, nullptr);

    if (m_rValue > 5) // AES-256 according to PDF 1.7 Adobe Extension Level 8 (PDF 2.0)
    {
        unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> sha384(EVP_MD_CTX_new(), EVP_MD_CTX_free);
        if (sha384 == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing sha384 hashing engine");

        unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> sha512(EVP_MD_CTX_new(), EVP_MD_CTX_free);
        if (sha512 == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing sha512 hashing engine");

        unsigned dataLen = 0;
        unsigned blockLen = 32; // Start with current SHA256 hash
        unsigned char data[(127 + 64 + 48) * 64]; // 127 for password, 64 for hash up to SHA512, 48 for uValue
        unsigned char block[64];
        std::memcpy(block, hashValue, 32);

        unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> aes(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
        if (aes == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing AES encryption engine");

        int dataOutMoved;
        for (unsigned i = 0; i < 64 || i < (unsigned)(32 + data[dataLen - 1]); i++)
        {
            dataLen = pswdLen + blockLen;
            std::memcpy(data, pswd, pswdLen);
            std::memcpy(data + pswdLen, block, blockLen);
            if (uValue)
            {
                std::memcpy(data + dataLen, uValue, 48);
                dataLen += 48;
            }
            for (unsigned j = 1; j < 64; j++)
                std::memcpy(data + j * dataLen, data, dataLen);

            dataLen *= 64;

            // CHECK-ME: The following was converted to new EVP_Encrypt API
            // from old internal API which is deprecated in OpenSSL 3.0 but
            // I'm not 100% sure the conversion is correct, since we don't
            // finalize the context. It may be unecessary because of some
            // preconditions, but these should be clearly stated
            rc = EVP_EncryptInit_ex(aes.get(), s_SSL.Aes128, nullptr, block, block + 16);
            rc = EVP_EncryptUpdate(aes.get(), data, &dataOutMoved, data, dataLen);
            PODOFO_ASSERT((unsigned)dataOutMoved == dataLen);

            unsigned sum = 0;
            for (unsigned j = 0; j < 16; j++)
                sum += data[j];
            blockLen = 32 + (sum % 3) * 16;

            if (blockLen == 32)
            {
                rc = EVP_DigestInit_ex(sha256.get(), s_SSL.SHA256, nullptr);
                rc = EVP_DigestUpdate(sha256.get(), data, dataLen);
                rc = EVP_DigestFinal_ex(sha256.get(), block, nullptr);
            }
            else if (blockLen == 48)
            {
                rc = EVP_DigestInit_ex(sha384.get(), s_SSL.SHA384, nullptr);
                rc = EVP_DigestUpdate(sha384.get(), data, dataLen);
                rc = EVP_DigestFinal_ex(sha384.get(), block, nullptr);
            }
            else
            {
                rc = EVP_DigestInit_ex(sha512.get(), s_SSL.SHA512, nullptr);
                rc = EVP_DigestUpdate(sha512.get(), data, dataLen);
                rc = EVP_DigestFinal_ex(sha512.get(), block, nullptr);
            }
        }
        std::memcpy(hashValue, block, 32);
    }
}

void PdfEncryptSHABase::ComputeUserKey(const unsigned char* userpswd, unsigned len)
{
    // Generate User Salts
    unsigned char vSalt[8];
    unsigned char kSalt[8];

    for (unsigned i = 0; i < 8; i++)
    {
        vSalt[i] = rand() % 255;
        kSalt[i] = rand() % 255;
    }

    // Generate hash for U
    unsigned char hashValue[32];

    ComputeHash(userpswd, len, vSalt, 0, hashValue);

    // U = hash + validation salt + key salt
    std::memcpy(m_uValue, hashValue, 32);
    std::memcpy(m_uValue + 32, vSalt, 8);
    std::memcpy(m_uValue + 32 + 8, kSalt, 8);

    // Generate hash for UE
    ComputeHash(userpswd, len, kSalt, 0, hashValue);

    // UE = AES-256 encoded file encryption key with key=hash
    // CBC mode, no padding, init vector=0

    int rc;
    unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> aes(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (aes == nullptr || (rc = EVP_EncryptInit_ex(aes.get(), s_SSL.Aes256, nullptr, hashValue, nullptr)) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing AES encryption engine");

    EVP_CIPHER_CTX_set_padding(aes.get(), 0); // disable padding

    int dataOutMoved;
    rc = EVP_EncryptUpdate(aes.get(), m_ueValue, &dataOutMoved, m_encryptionKey, m_keyLength);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");

    rc = EVP_EncryptFinal_ex(aes.get(), &m_ueValue[dataOutMoved], &dataOutMoved);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");
}

void PdfEncryptSHABase::ComputeOwnerKey(const unsigned char* ownerpswd, unsigned len)
{
    // Generate User Salts
    unsigned char vSalt[8];
    unsigned char kSalt[8];

    for (unsigned i = 0; i < 8; i++)
    {
        vSalt[i] = rand() % 255;
        kSalt[i] = rand() % 255;
    }

    // Generate hash for O
    unsigned char hashValue[32];
    ComputeHash(ownerpswd, len, vSalt, m_uValue, hashValue);

    // O = hash + validation salt + key salt
    std::memcpy(m_oValue, hashValue, 32);
    std::memcpy(m_oValue + 32, vSalt, 8);
    std::memcpy(m_oValue + 32 + 8, kSalt, 8);

    // Generate hash for OE
    ComputeHash(ownerpswd, len, kSalt, m_uValue, hashValue);

    // OE = AES-256 encoded file encryption key with key=hash
    // CBC mode, no padding, init vector=0

    int rc;
    unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> aes(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (aes == nullptr || (rc = EVP_EncryptInit_ex(aes.get(), s_SSL.Aes256, nullptr, hashValue, nullptr)) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing AES encryption engine");

    EVP_CIPHER_CTX_set_padding(aes.get(), 0); // disable padding

    int dataOutMoved;
    rc = EVP_EncryptUpdate(aes.get(), m_oeValue, &dataOutMoved, m_encryptionKey, m_keyLength);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");

    rc = EVP_EncryptFinal_ex(aes.get(), &m_oeValue[dataOutMoved], &dataOutMoved);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");
}

void PdfEncryptSHABase::PreprocessPassword(const string_view& password, unsigned char* outBuf, unsigned& len)
{
    char* password_sasl;
    // NOTE: password view may be unterminated. Wrap it for stringprep_profile
    int rc = stringprep_profile(string(password).data(), &password_sasl, "SASLprep", STRINGPREP_NO_UNASSIGNED);
    if (rc != STRINGPREP_OK)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidPassword, "Error processing password through SASLprep");

    size_t l = std::strlen(password_sasl);
    len = l > 127 ? 127 : l;

    std::memcpy(outBuf, password_sasl, len);
    // password_sasl is allocated by stringprep_profile (libidn), so use idn_free
    // (in Windows the libidn.dll could use an other heap and then the normal free or podofo_free will crash while debugging podofo.)
    idn_free(password_sasl);
}

void PdfEncryptSHABase::ComputeEncryptionKey()
{
    // Seed once for all
    srand((unsigned)time(nullptr));

    for (unsigned i = 0; i < m_keyLength; i++)
        m_encryptionKey[i] = rand() % 255;
}
    
bool PdfEncryptSHABase::Authenticate(const std::string_view& documentID, const std::string_view& password,
    const bufferview& uValue, const std::string_view& ueValue,
    const bufferview& oValue, const std::string_view& oeValue,
    PdfPermissions pValue, const std::string_view& permsValue,
    int lengthValue, int rValue)
{
    m_pValue = pValue;
    m_keyLength = lengthValue / 8;
    m_rValue = rValue;

    std::memcpy(m_uValue, uValue.data(), 48);
    std::memcpy(m_ueValue, ueValue.data(), 32);
    std::memcpy(m_oValue, oValue.data(), 48);
    std::memcpy(m_oeValue, oeValue.data(), 32);
    std::memcpy(m_permsValue, permsValue.data(), 16);

    return Authenticate(password, documentID);
}
    
void PdfEncryptSHABase::GenerateInitialVector(unsigned char iv[]) const
{
    for (unsigned i = 0; i < AES_IV_LENGTH; i++)
        iv[i] = rand() % 255;
}
    
void PdfEncryptSHABase::CreateEncryptionDictionary(PdfDictionary& dictionary) const
{
    dictionary.AddKey(PdfName::KeyFilter, PdfName("Standard"));

    PdfDictionary cf;
    PdfDictionary stdCf;

    dictionary.AddKey("V", static_cast<int64_t>(5));
    dictionary.AddKey("R", static_cast<int64_t>(m_rValue));
    dictionary.AddKey("Length", static_cast<int64_t>(256));

    stdCf.AddKey("CFM", PdfName("AESV3"));
    stdCf.AddKey("Length", static_cast<int64_t>(32));

    dictionary.AddKey("O", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetOValue()), 48 }));
    dictionary.AddKey("OE", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetOEValue()), 32 }));
    dictionary.AddKey("U", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetUValue()), 48 }));
    dictionary.AddKey("UE", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetUEValue()), 32 }));
    dictionary.AddKey("Perms", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetPermsValue()), 16 }));

    stdCf.AddKey("AuthEvent", PdfName("DocOpen"));
    cf.AddKey("StdCF", stdCf);

    dictionary.AddKey("CF", cf);
    dictionary.AddKey("StrF", PdfName("StdCF"));
    dictionary.AddKey("StmF", PdfName("StdCF"));

    dictionary.AddKey("P", PdfVariant(static_cast<int64_t>(this->GetPValue())));
}
    
void PdfEncryptAESV3::GenerateEncryptionKey(const string_view& documentId)
{
    (void)documentId;

    // Prepare passwords
    unsigned char userpswd[127];
    unsigned char ownerpswd[127];
    unsigned userpswdLen;
    unsigned ownerpswdLen;
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
    perms[3] = ((unsigned)m_pValue >> 24) & 0xFF;
    perms[2] = ((unsigned)m_pValue >> 16) & 0xFF;
    perms[1] = ((unsigned)m_pValue >> 8) & 0xFF;
    perms[0] = ((unsigned)m_pValue >> 0) & 0xFF;
    // Placeholder for future versions that may need 64-bit permissions
    perms[4] = 0xFF;
    perms[5] = 0xFF;
    perms[6] = 0xFF;
    perms[7] = 0xFF;
    // if EncryptMetadata is false, this value should be set to 'F'
    perms[8] = m_EncryptMetadata ? 'T' : 'F';
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

    int rc;
    unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> aes(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (aes == nullptr || (rc = EVP_EncryptInit_ex(aes.get(), s_SSL.Aes256, nullptr, m_encryptionKey, nullptr)) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing AES encryption engine");

    EVP_CIPHER_CTX_set_padding(aes.get(), 0); // disable padding

    int dataOutMoved;
    rc = EVP_EncryptUpdate(aes.get(), m_permsValue, &dataOutMoved, perms, 16);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");

    rc = EVP_EncryptFinal_ex(aes.get(), &m_permsValue[dataOutMoved], &dataOutMoved);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");
}

bool PdfEncryptAESV3::Authenticate(const string_view& password, const string_view& documentId)
{
    (void)documentId;
    bool success = false;

    // Prepare password
    unsigned char pswd_sasl[127];
    unsigned pswdLen;
    PreprocessPassword(password, pswd_sasl, pswdLen);

    // Test 1: is it the user key ?
    unsigned char hashValue[32];
    ComputeHash(pswd_sasl, pswdLen, m_uValue + 32, 0, hashValue); // user Validation Salt

    success = CheckKey(hashValue, m_uValue);
    if (!success)
    {
        // Test 2: is it the owner key ?
        ComputeHash(pswd_sasl, pswdLen, m_oValue + 32, m_uValue, hashValue); // owner Validation Salt

        success = CheckKey(hashValue, m_oValue);

        if (success)
        {
            m_ownerPass = password;
            // ISO 32000: "Compute an intermediate owner key by computing the SHA-256 hash of
            // the UTF-8 password concatenated with the 8 bytes of owner Key Salt, concatenated with the 48-byte U string."
            ComputeHash(pswd_sasl, pswdLen, m_oValue + 40, m_uValue, hashValue); // owner Key Salt

            // ISO 32000: "The 32-byte result is the key used to decrypt the 32-byte OE string using
            // AES-256 in CBC mode with no padding and an initialization vector of zero.
            // The 32-byte result is the file encryption key"
            EVP_CIPHER_CTX* aes = m_aes->getEngine();
            EVP_DecryptInit_ex(aes, s_SSL.Aes256, nullptr, hashValue, 0); // iv zero
            EVP_CIPHER_CTX_set_padding(aes, 0); // no padding
            int lOutLen;
            EVP_DecryptUpdate(aes, m_encryptionKey, &lOutLen, m_oeValue, 32);
        }
    }
    else
    {
        m_userPass = password;
        // ISO 32000: "Compute an intermediate user key by computing the SHA-256 hash of
        // the UTF-8 password concatenated with the 8 bytes of user Key Salt"
        ComputeHash(pswd_sasl, pswdLen, m_uValue + 40, 0, hashValue); // user Key Salt

        // ISO 32000: "The 32-byte result is the key used to decrypt the 32-byte UE string using
        // AES-256 in CBC mode with no padding and an initialization vector of zero.
        // The 32-byte result is the file encryption key"
        EVP_CIPHER_CTX* aes = m_aes->getEngine();
        EVP_DecryptInit_ex(aes, s_SSL.Aes256, nullptr, hashValue, 0); // iv zero
        EVP_CIPHER_CTX_set_padding(aes, 0); // no padding
        int lOutLen;
        EVP_DecryptUpdate(aes, m_encryptionKey, &lOutLen, m_ueValue, 32);
    }

    // TODO Validate permissions (or not...)

    return success;
}

size_t PdfEncryptAESV3::CalculateStreamOffset() const
{
    return AES_IV_LENGTH;
}

void PdfEncryptAESV3::Encrypt(const char* inStr, size_t inLen, const PdfReference& objref,
    char* outStr, size_t outLen) const
{
    (void)objref;
    size_t offset = CalculateStreamOffset();
    this->GenerateInitialVector((unsigned char*)outStr);
    this->BaseEncrypt(m_encryptionKey, m_keyLength, (unsigned char*)outStr, (const unsigned char*)inStr, inLen, (unsigned char*)outStr + offset, outLen - offset);
}

void PdfEncryptAESV3::Decrypt(const char* inStr, size_t inLen, const PdfReference& objref,
    char* outStr, size_t& outLen) const
{
    (void)objref;
    size_t offset = CalculateStreamOffset();
    if (inLen <= offset)
    {
        // Is empty
        outLen = 0;
        return;
    }

    this->BaseDecrypt(const_cast<unsigned char*>(m_encryptionKey), m_keyLength, (const unsigned char*)inStr, (const unsigned char*)inStr + offset, inLen - offset, (unsigned char*)outStr, outLen);
}

PdfEncryptAESV3::PdfEncryptAESV3(const string_view& userPassword, const string_view& ownerPassword,
    PdfAESV3Revision revision, PdfPermissions protection) : PdfEncryptAESBase()
{
    // setup object
    m_userPass = userPassword;
    m_ownerPass = ownerPassword;
    m_Algorithm = revision == PdfAESV3Revision::R6 ? PdfEncryptAlgorithm::AESV3R6 : PdfEncryptAlgorithm::AESV3;

    m_rValue = (int)revision;
    m_eKeyLength = PdfKeyLength::L256;
    m_keyLength = (int)PdfKeyLength::L256 / 8;

    // Init buffers
    std::memset(m_oValue, 0, 48);
    std::memset(m_uValue, 0, 48);
    std::memset(m_encryptionKey, 0, 32);
    std::memset(m_ueValue, 0, 32);
    std::memset(m_oeValue, 0, 32);

    // Compute P value
    m_pValue = PERMS_DEFAULT | protection;
}

PdfEncryptAESV3::PdfEncryptAESV3(const PdfEncrypt& rhs)
    : PdfEncryptSHABase(rhs) {}

PdfEncryptAESV3::PdfEncryptAESV3(PdfString oValue, PdfString oeValue, PdfString uValue,
    PdfString ueValue, PdfPermissions pValue, PdfString permsValue, PdfAESV3Revision revision) : PdfEncryptAESBase()
{
    m_pValue = pValue;
    m_Algorithm = revision == PdfAESV3Revision::R6 ? PdfEncryptAlgorithm::AESV3R6 : PdfEncryptAlgorithm::AESV3;

    m_eKeyLength = PdfKeyLength::L256;
    m_keyLength = (int)PdfKeyLength::L256 / 8;
    m_rValue = (int)revision;
    std::memcpy(m_oValue, oValue.GetRawData().data(), 48);
    std::memcpy(m_oeValue, oeValue.GetRawData().data(), 32);
    std::memcpy(m_uValue, uValue.GetRawData().data(), 48);
    std::memcpy(m_ueValue, ueValue.GetRawData().data(), 32);
    std::memcpy(m_permsValue, permsValue.GetRawData().data(), 16);
    std::memset(m_encryptionKey, 0, 32);
}

size_t PdfEncryptAESV3::CalculateStreamLength(size_t length) const
{
    size_t realLength = ((length + 15) & ~15) + AES_IV_LENGTH;
    if (length % 16 == 0)
        realLength += 16;

    return realLength;
}

unique_ptr<InputStream> PdfEncryptAESV3::CreateEncryptionInputStream(InputStream& inputStream, size_t inputLen, const PdfReference& objref)
{
    (void)objref;
    return unique_ptr<InputStream>(new PdfAESInputStream(inputStream, inputLen, m_encryptionKey, 32));
}

unique_ptr<OutputStream> PdfEncryptAESV3::CreateEncryptionOutputStream(OutputStream& outputStream, const PdfReference& objref)
{
    (void)outputStream;
    (void)objref;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "CreateEncryptionOutputStream does not yet support AESV3");
}
    
#endif // PODOFO_HAVE_LIBIDN

int PdfEncrypt::GetKeyLength() const
{
    return m_keyLength * 8;
}

void PdfEncrypt::EncryptTo(charbuff& out, const bufferview& view, const PdfReference& objref) const
{
    size_t outputLen = this->CalculateStreamLength(view.size());
    out.resize(outputLen);
    this->Encrypt(view.data(), view.size(), objref, out.data(), outputLen);
}

void PdfEncrypt::DecryptTo(charbuff& out, const bufferview& view, const PdfReference& objref) const
{
    // FIX-ME: The following clearly seems hardcoded for AES
    // It was found like this in PdfString and PdfTokenizer
    // Fix it so it will allocate the exact amount of memory
    // needed, including RC4
    size_t outBufferLen = view.size() - this->CalculateStreamOffset();
    out.resize(outBufferLen + 16 - (outBufferLen % 16));
    this->Decrypt(view.data(), view.size(), objref, out.data(), outBufferLen);
    out.resize(outBufferLen);
    out.shrink_to_fit();
}

bool PdfEncrypt::IsPrintAllowed() const
{
    return (m_pValue & PdfPermissions::Print) == PdfPermissions::Print;
}

bool PdfEncrypt::IsEditAllowed() const
{
    return (m_pValue & PdfPermissions::Edit) == PdfPermissions::Edit;
}

bool PdfEncrypt::IsCopyAllowed() const
{
    return (m_pValue & PdfPermissions::Copy) == PdfPermissions::Copy;
}

bool PdfEncrypt::IsEditNotesAllowed() const
{
    return (m_pValue & PdfPermissions::EditNotes) == PdfPermissions::EditNotes;
}

bool PdfEncrypt::IsFillAndSignAllowed() const
{
    return (m_pValue & PdfPermissions::FillAndSign) == PdfPermissions::FillAndSign;
}

bool PdfEncrypt::IsAccessibilityAllowed() const
{
    return (m_pValue & PdfPermissions::Accessible) == PdfPermissions::Accessible;
}

bool PdfEncrypt::IsDocAssemblyAllowed() const
{
    return (m_pValue & PdfPermissions::DocAssembly) == PdfPermissions::DocAssembly;
}

bool PdfEncrypt::IsHighPrintAllowed() const
{
    return (m_pValue & PdfPermissions::HighPrint) == PdfPermissions::HighPrint;
}
