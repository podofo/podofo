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
#include <podofo/private/OpenSSLInternal.h>

// Early define PODOFO_CRYPT_CTX to desired type
#define PODOFO_CRYPT_CTX EVP_CIPHER_CTX
#include "PdfEncrypt.h"

#include <openssl/md5.h>
#include <openssl/rand.h>
#include <podofo/private/SASLprep.h>

#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdfEncryptionAlgorithm s_EnabledEncryptionAlgorithms;

// Default value for P (permissions) = no permission
// ISO 32000-2:2020 "Table 22 — Standard security handler user access permissions":
// Bit position 1-2: Reserved. Must be zero (0)
//  ...
// Bit position 7-8: Reserved. Must be 1
//  ...
// Bit position 13-32: (Security handlers of revision 3 or greater) Reserved. Must be 1
constexpr PdfPermissions DefaultPermsMask = ~(PdfPermissions::Default | (PdfPermissions)1 | (PdfPermissions)2);

#define AES_IV_LENGTH 16
#define AES_BLOCK_SIZE 16

constexpr unsigned char padding[] =
"\x28\xBF\x4E\x5E\x4E\x75\x8A\x41\x64\x00\x4E\x56\xFF\xFA\x01\x08\x2E\x2E\x00\xB6\xD0\x68\x3E\x80\x2F\x0C\xA9\xFE\x64\x53\x69\x7A";

static void AESDecrypt(EVP_CIPHER_CTX* ctx, const unsigned char* key, unsigned keylen, const unsigned char* iv,
    const unsigned char* textin, size_t textlen,
    unsigned char* textout, size_t& textoutlen);
static void AESEncrypt(EVP_CIPHER_CTX* ctx, const unsigned char* key, unsigned keylen, const unsigned char* iv,
    const unsigned char* textin, size_t textlen,
    unsigned char* textout, size_t textoutlen);

static void RC4Encrypt(EVP_CIPHER_CTX* ctx, const unsigned char* key, unsigned keylen,
    const unsigned char* textin, size_t textlen,
    unsigned char* textout, size_t textoutlen);

namespace
{
/** A class that can encrypt/decrpyt streamed data block wise
 *  This is used in the input and output stream encryption implementation.
 *  Only the RC4 encryption algorithm is supported
 */
class PdfRC4Stream
{
public:
    PdfRC4Stream(unsigned char rc4key[256], unsigned char rc4last[256],
        const unsigned char* key, unsigned keylen) :
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
        unsigned char rc4last[256], const unsigned char* key, unsigned keylen) :
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
        const unsigned char* key, unsigned keylen) :
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
    PdfAESInputStream(InputStream& inputStream, size_t inputLen, const unsigned char* key, unsigned keylen) :
        m_InputStream(&inputStream),
        m_inputLen(inputLen),
        m_inputEof(false),
        m_init(true),
        m_keyLen(keylen),
        m_drainLeft(0)
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
                    cipher = ssl::Aes128();
                    break;
                }
                case (size_t)PdfKeyLength::L256 / 8:
                {
                    cipher = ssl::Aes256_CBC();
                    break;
                }
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

        eof = false;
        return outlen;

    DrainBuffer:
        drainLen = std::min(len - outlen, m_drainLeft);
        std::memcpy(buffer + outlen, m_tempBuffer.data(), drainLen);
        m_drainLeft -= (int)drainLen;
        if (m_drainLeft == 0)
            eof = true;
        else
            eof = false;

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

struct RC4EncryptContext
{
    unsigned char Rc4key[16];         // last RC4 key
    unsigned char Rc4last[256];       // last RC4 state table
};

}

PdfEncrypt::~PdfEncrypt()
{
    clearSensitiveInfo();
}

void PdfEncrypt::EnsureEncryptionInitialized(const PdfString& documentId, PdfEncryptContext& context)
{
    if (m_initialized)
    {
        if (!context.IsAuthenticated())
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unexpected non-authenticated context");

        // If params are already filled, then it's not necessary
        // (nor possible) to regenerate them
        return;
    }

    GenerateEncryptionKey(documentId.GetRawData(), context.GetAuthResult(), context.GetCryptCtx(),
        m_uValue, m_oValue, context.m_encryptionKey);
    context.m_documentId = documentId.GetRawData();

    PODOFO_INVARIANT(!m_initialized);

    clearSensitiveInfo();
    // When creating an encrypt from scratch we
    // can assume we are the owner of the document
    context.m_AuthResult = PdfAuthResult::Owner;
    m_initialized = true;
}

void PdfEncrypt::Authenticate(const string_view& password, const PdfString& documentId, PdfEncryptContext& context) const
{
    context.m_AuthResult = Authenticate(password, documentId.GetRawData(), context.GetCryptCtx(), context.m_encryptionKey);
    context.m_documentId = documentId.GetRawData();
}

PdfEncryptionAlgorithm PdfEncrypt::GetEnabledEncryptionAlgorithms()
{
    struct Init
    {
        Init()
        {
            s_EnabledEncryptionAlgorithms = PdfEncryptionAlgorithm::AESV2;
            auto hasRc4 = ssl::Rc4() != nullptr;
            if (hasRc4)
            {
                s_EnabledEncryptionAlgorithms |=
                    PdfEncryptionAlgorithm::RC4V1 |
                    PdfEncryptionAlgorithm::RC4V2;
            }
            s_EnabledEncryptionAlgorithms |=
                PdfEncryptionAlgorithm::AESV3R5 |
                PdfEncryptionAlgorithm::AESV3R6;
        }
    } init;

    return s_EnabledEncryptionAlgorithms;
}

bool PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm algorithm)
{
    auto enabledAlgorithms = GetEnabledEncryptionAlgorithms();
    return (enabledAlgorithms & algorithm) != PdfEncryptionAlgorithm::None;
}

unique_ptr<PdfEncrypt> PdfEncrypt::Create(const string_view& userPassword,
    const string_view& ownerPassword,
    PdfPermissions protection,
    PdfEncryptionAlgorithm algorithm,
    PdfKeyLength keyLength)
{
    if (!IsEncryptionEnabled(algorithm))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported encryption");

    switch (algorithm)
    {
        case PdfEncryptionAlgorithm::AESV3R5:
        {
            if (keyLength != PdfKeyLength::Unknown && keyLength != PdfKeyLength::L256)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "Invalid encryption key length for AESV3. Only 256 bit is supported");

            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV3(userPassword, ownerPassword,
                PdfAESV3Revision::R5, protection));
        }
        case PdfEncryptionAlgorithm::AESV3R6:
        {
            if (keyLength != PdfKeyLength::Unknown && keyLength != PdfKeyLength::L256)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "Invalid encryption key length for AESV3. Only 256 bit is supported");

            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV3(userPassword, ownerPassword,
                PdfAESV3Revision::R6, protection));
        }
        case PdfEncryptionAlgorithm::RC4V2:
        case PdfEncryptionAlgorithm::RC4V1:
            return unique_ptr<PdfEncrypt>(new PdfEncryptRC4(userPassword, ownerPassword, protection, algorithm, keyLength));
        case PdfEncryptionAlgorithm::AESV2:
        {
            if (keyLength != PdfKeyLength::Unknown && keyLength != PdfKeyLength::L128)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "Invalid encryption key length for AESV2. Only 128 bit is supported");

            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV2(userPassword, ownerPassword, protection));
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

unique_ptr<PdfEncrypt> PdfEncrypt::CreateFromObject(const PdfObject& encryptObj)
{
    if (!encryptObj.GetDictionary().HasKey("Filter") ||
        encryptObj.GetDictionary().GetKey("Filter")->GetName() != "Standard")
    {
        if (encryptObj.GetDictionary().HasKey("Filter"))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported encryption filter: {}",
                encryptObj.GetDictionary().GetKey("Filter")->GetName().GetString());
        else
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Encryption dictionary does not have a key /Filter");
    }

    unsigned lV;
    int64_t length;
    unsigned rValue;
    PdfPermissions pValue;
    PdfString oValue;
    PdfString uValue;
    PdfName cfmName;
    bool encryptMetadata = true;

    try
    {
        lV = static_cast<unsigned>(encryptObj.GetDictionary().MustGetKey("V").GetNumber());
        rValue = static_cast<unsigned>(encryptObj.GetDictionary().MustGetKey("R").GetNumber());

        // "The value of the P entry shall be interpreted as an unsigned
        // 32-bit quantity containing a set of flags"
        pValue = static_cast<PdfPermissions>(encryptObj.GetDictionary().MustGetKey("P").GetNumber() & 0xFFFFFFFF);

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
        throw;
    }

    if ((lV == 1) && (rValue == 2 || rValue == 3)
        && PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::RC4V1))
    {
        return unique_ptr<PdfEncrypt>(new PdfEncryptRC4(oValue, uValue, pValue, (PdfRC4Revision)rValue,
            PdfEncryptionAlgorithm::RC4V1, (unsigned)PdfKeyLength::L40, encryptMetadata));
    }
    else if ((((lV == 2) && (rValue == 3)) || cfmName == "V2")
        && PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::RC4V2))
    {
        return unique_ptr<PdfEncrypt>(new PdfEncryptRC4(oValue, uValue, pValue, (PdfRC4Revision)rValue,
            PdfEncryptionAlgorithm::RC4V2, (unsigned)length, encryptMetadata));
    }
    else
    {
        if ((lV == 4) && (rValue == 4)
            && PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::AESV2))
        {
            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV2(oValue, uValue, pValue, encryptMetadata));
        }
        else if ((lV == 5) && (
            (rValue == 5 && PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::AESV3R5))
            || (rValue == 6 && PdfEncrypt::IsEncryptionEnabled(PdfEncryptionAlgorithm::AESV3R6))))
        {
            PdfString permsValue = encryptObj.GetDictionary().MustFindKey("Perms").GetString();
            PdfString oeValue = encryptObj.GetDictionary().MustFindKey("OE").GetString();
            PdfString ueValue = encryptObj.GetDictionary().MustFindKey("UE").GetString();

            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV3(oValue, oeValue, uValue,
                ueValue, pValue, permsValue, (PdfAESV3Revision)rValue));
        }
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
        case PdfEncryptionAlgorithm::RC4V1:
        case PdfEncryptionAlgorithm::RC4V2:
            return unique_ptr<PdfEncrypt>(new PdfEncryptRC4(static_cast<const PdfEncryptRC4&>(rhs)));
        case PdfEncryptionAlgorithm::AESV2:
            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV2(static_cast<const PdfEncryptAESV2&>(rhs)));
        case PdfEncryptionAlgorithm::AESV3R5:
        case PdfEncryptionAlgorithm::AESV3R6:
            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV3(static_cast<const PdfEncryptAESV3&>(rhs)));
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Invalid algorithm");
    }
}

void PdfEncrypt::clearSensitiveInfo()
{
    // Clear sensitive information to not leave traces in memory
    std::memset(m_userPass.data(), 0, m_userPass.size());
    std::memset(m_ownerPass.data(), 0, m_ownerPass.size());
}

PdfEncrypt::PdfEncrypt() :
    m_Algorithm(PdfEncryptionAlgorithm::None),
    m_rValue(0),
    m_KeyLength((PdfKeyLength)0),
    m_pValue(PdfPermissions::None),
    m_uValue{ },
    m_oValue{ },
    m_uValueSize(0),
    m_oValueSize(0),
    m_EncryptMetadata(false),
    m_IsParsed(false),
    m_initialized(false)
{
}

int64_t PdfEncrypt::GetPValueForSerialization() const
{
    // NOTE: While "The value of the P entry shall be
    // interpreted as an unsigned 32-bit quantity", PDFs
    // tend to write it a signed integer, which is weird
    // but still acceptable. We convert it first to int32_t
    // again before casting to a PDF 64 bit number to
    // preserve the same form
    return (int64_t)(int32_t)m_pValue;
}

void PdfEncrypt::InitFromValues(PdfEncryptionAlgorithm algorithm, PdfKeyLength keyLength, unsigned char revision, PdfPermissions pValue,
    const bufferview& uValue, const bufferview& oValue, bool encryptedMetadata)
{
    PODOFO_ASSERT((size_t)keyLength / 8 <= std::size(((PdfEncryptContext*)nullptr)->m_encryptionKey));
    m_Algorithm = algorithm;
    m_KeyLength = keyLength;
    m_rValue = revision;
    m_pValue = pValue;
    std::memcpy(m_uValue, uValue.data(), uValue.size());
    std::memcpy(m_oValue, oValue.data(), oValue.size());
    m_EncryptMetadata = encryptedMetadata;
    m_IsParsed = true;
    m_initialized = true;
}

void PdfEncrypt::InitFromScratch(const string_view& userPassword, const string_view& ownerPassword,
    PdfEncryptionAlgorithm algorithm, PdfKeyLength keyLength, unsigned char revision, PdfPermissions pValue, bool encryptedMetadata)
{
    PODOFO_ASSERT((size_t)keyLength / 8 <= std::size(((PdfEncryptContext*)nullptr)->m_encryptionKey));
    m_userPass = userPassword;
    m_ownerPass = ownerPassword;
    m_Algorithm = algorithm;
    m_KeyLength = keyLength;
    m_rValue = revision;
    m_pValue = pValue;
    m_EncryptMetadata = encryptedMetadata;
}

bool PdfEncrypt::CheckKey(const unsigned char key1[32], const unsigned char key2[32]) const
{
    // Check whether the right password had been given
    bool success = true;
    unsigned keyLength = GetKeyLengthBytes();
    PODOFO_INVARIANT(keyLength <= 32);
    for (unsigned k = 0; success && k < keyLength; k++)
        success = success && (key1[k] == key2[k]);

    return success;
}

PdfEncryptContext::PdfEncryptContext() :
    m_encryptionKey{ },
    m_AuthResult(PdfAuthResult::Unkwnon),
    m_cryptCtx(nullptr),
    m_customCtx(nullptr),
    m_customCtxSize(0)
{
}

PdfEncryptContext::~PdfEncryptContext()
{
    // Clear sensitive information to not leave traces in memory
    std::memset(m_encryptionKey, 0, std::size(m_encryptionKey));
    if (m_customCtx != nullptr)
        std::memset(m_customCtx, 0, m_customCtxSize);

    EVP_CIPHER_CTX_free(m_cryptCtx);
    ::operator delete(m_customCtx);
}

PdfEncryptContext::PdfEncryptContext(const PdfEncryptContext& rhs) :
    m_AuthResult(rhs.m_AuthResult),
    m_cryptCtx(nullptr),
    m_customCtx(nullptr),
    m_customCtxSize(0)
{
    std::memcpy(m_encryptionKey, rhs.m_encryptionKey, std::size(m_encryptionKey));
    if (rhs.m_customCtx != nullptr)
    {
        m_customCtx = ::operator new(rhs.m_customCtxSize);
        std::memcpy(m_customCtx, rhs.m_customCtx, rhs.m_customCtxSize);
        m_customCtxSize = rhs.m_customCtxSize;
    }
}

PdfEncryptContext& PdfEncryptContext::operator=(const PdfEncryptContext& rhs)
{
    m_AuthResult = rhs.m_AuthResult;
    std::memcpy(m_encryptionKey, rhs.m_encryptionKey, std::size(m_encryptionKey));
    EVP_CIPHER_CTX_free(m_cryptCtx);
    m_cryptCtx = nullptr;
    ::operator delete(m_customCtx);
    if (rhs.m_customCtx == nullptr)
    {
        m_customCtx = nullptr;
        m_customCtxSize = 0;
    }
    else
    {
        m_customCtx = ::operator new(rhs.m_customCtxSize);
        std::memcpy(m_customCtx, rhs.m_customCtx, rhs.m_customCtxSize);
        m_customCtxSize = rhs.m_customCtxSize;
    }

    return *this;
}

bool PdfEncryptContext::IsAuthenticated() const
{
    return m_AuthResult == PdfAuthResult::User || m_AuthResult == PdfAuthResult::Owner;
}

EVP_CIPHER_CTX* PdfEncryptContext::GetCryptCtx()
{
    if (m_cryptCtx == nullptr)
        m_cryptCtx = EVP_CIPHER_CTX_new();

    return m_cryptCtx;
}

PdfEncryptMD5Base::PdfEncryptMD5Base()
{
}

PdfEncryptMD5Base::PdfEncryptMD5Base(const PdfEncryptMD5Base& rhs)
    : PdfEncrypt(rhs)
{
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

void PdfEncryptMD5Base::ComputeOwnerKey(const unsigned char userPad[32], const unsigned char ownerPad[32],
    unsigned keyLength, unsigned revision, bool authenticate, EVP_CIPHER_CTX* crypt, unsigned char ownerKey[32])
{
    unsigned char mkey[MD5_DIGEST_LENGTH];
    unsigned char digest[MD5_DIGEST_LENGTH];
    int rc;

    PODOFO_INVARIANT(keyLength <= MD5_DIGEST_LENGTH);

    unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> md(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (md == nullptr || (rc = EVP_DigestInit_ex(md.get(), ssl::MD5(), nullptr)) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing MD5 hashing engine");

    rc = EVP_DigestUpdate(md.get(), ownerPad, 32);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

    rc = EVP_DigestFinal_ex(md.get(), digest, nullptr);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

    if ((revision == 3) || (revision == 4))
    {
        // only use for the input as many bit as the key consists of
        for (int k = 0; k < 50; k++)
        {
            rc = EVP_DigestInit_ex(md.get(), ssl::MD5(), nullptr);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing MD5 hashing engine");

            rc = EVP_DigestUpdate(md.get(), digest, keyLength);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

            rc = EVP_DigestFinal_ex(md.get(), digest, nullptr);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");
        }
        std::memcpy(ownerKey, userPad, 32);
        for (unsigned i = 0; i < 20; i++)
        {
            for (unsigned j = 0; j < keyLength; j++)
            {
                if (authenticate)
                    mkey[j] = static_cast<unsigned char>(static_cast<unsigned>(digest[j] ^ (19 - i)));
                else
                    mkey[j] = static_cast<unsigned char>(static_cast<unsigned>(digest[j] ^ i));
            }
            RC4Encrypt(crypt, mkey, keyLength, ownerKey, 32, ownerKey, 32);
        }
    }
    else
    {
        RC4Encrypt(crypt, digest, 5, userPad, 32, ownerKey, 32);
    }
}

void PdfEncryptMD5Base::ComputeEncryptionKey(const string_view& documentId,
    const unsigned char userPad[32], const unsigned char ownerKey[32],
    PdfPermissions pValue, unsigned keyLength, unsigned revision,
    bool encryptMetadata, EVP_CIPHER_CTX* crypt,
    unsigned char userKey[32], unsigned char encryptionKey[32])
{
    unsigned j;
    unsigned k;
    int rc;

    PODOFO_INVARIANT(keyLength <= MD5_DIGEST_LENGTH);

    unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> md(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (md == nullptr || (rc = EVP_DigestInit_ex(md.get(), ssl::MD5(), nullptr)) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing MD5 hashing engine");

    rc = EVP_DigestUpdate(md.get(), userPad, 32);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

    rc = EVP_DigestUpdate(md.get(), ownerKey, 32);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

    unsigned char ext[4];
    ext[0] = static_cast<unsigned char> (((unsigned)pValue >> 0) & 0xFF);
    ext[1] = static_cast<unsigned char> (((unsigned)pValue >> 8) & 0xFF);
    ext[2] = static_cast<unsigned char> (((unsigned)pValue >> 16) & 0xFF);
    ext[3] = static_cast<unsigned char> (((unsigned)pValue >> 24) & 0xFF);
    rc = EVP_DigestUpdate(md.get(), ext, 4);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

    unsigned docIdLength = static_cast<unsigned>(documentId.length());
    vector<unsigned char> docId;
    if (docIdLength > 0)
    {
        docId.resize(docIdLength);
        for (j = 0; j < docIdLength; j++)
        {
            docId[j] = static_cast<unsigned char>(documentId[j]);
        }
        rc = EVP_DigestUpdate(md.get(), docId.data(), docIdLength);
        if (rc != 1)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");
    }

    // If document metadata is not being encrypted,
    // pass 4 bytes with the value 0xFFFFFFFF to the MD5 hash function.
    if (!encryptMetadata)
    {
        unsigned char noMetaAddition[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
        rc = EVP_DigestUpdate(md.get(), noMetaAddition, 4);
        if (rc != 1)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");
    }

    unsigned char digest[MD5_DIGEST_LENGTH];
    rc = EVP_DigestFinal_ex(md.get(), digest, nullptr);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

    // only use the really needed bits as input for the hash
    if (revision == 3 || revision == 4)
    {
        for (k = 0; k < 50; k++)
        {
            rc = EVP_DigestInit_ex(md.get(), ssl::MD5(), nullptr);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing MD5 hashing engine");

            rc = EVP_DigestUpdate(md.get(), digest, keyLength);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

            rc = EVP_DigestFinal_ex(md.get(), digest, nullptr);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");
        }
    }

    std::memcpy(encryptionKey, digest, keyLength);

    // Setup user key
    if (revision == 3 || revision == 4)
    {
        rc = EVP_DigestInit_ex(md.get(), ssl::MD5(), nullptr);
        if (rc != 1)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing MD5 hashing engine");

        rc = EVP_DigestUpdate(md.get(), padding, 32);
        if (rc != 1)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

        if (docId.size() != 0)
        {
            rc = EVP_DigestUpdate(md.get(), docId.data(), docIdLength);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");
        }

        rc = EVP_DigestFinal_ex(md.get(), digest, nullptr);
        if (rc != 1)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error MD5-hashing data");

        std::memcpy(userKey, digest, 16);
        for (k = 16; k < 32; k++)
            userKey[k] = 0;

        for (k = 0; k < 20; k++)
        {
            for (j = 0; j < keyLength; j++)
            {
                digest[j] = static_cast<unsigned char>(encryptionKey[j] ^ k);
            }

            RC4Encrypt(crypt, digest, keyLength, userKey, 16, userKey, 16);
        }
    }
    else
    {
        RC4Encrypt(crypt, encryptionKey, keyLength, padding, 32, userKey, 32);
    }
}

void PdfEncryptMD5Base::CreateObjKey(unsigned char objkey[16], unsigned& pnKeyLen,
    const unsigned char encryptionKey[32], const PdfReference& objref) const
{
    const unsigned n = static_cast<unsigned>(objref.ObjectNumber());
    const unsigned g = static_cast<unsigned>(objref.GenerationNumber());

    unsigned keyLength = GetKeyLengthBytes();
    PODOFO_INVARIANT(keyLength <= MD5_DIGEST_LENGTH);

    unsigned nkeylen = keyLength + 5;
    unsigned char nkey[MD5_DIGEST_LENGTH + 5 + 4];
    for (unsigned j = 0; j < keyLength; j++)
        nkey[j] = encryptionKey[j];

    nkey[keyLength + 0] = static_cast<unsigned char>(0xFF & n);
    nkey[keyLength + 1] = static_cast<unsigned char>(0xFF & (n >> 8));
    nkey[keyLength + 2] = static_cast<unsigned char>(0xFF & (n >> 16));
    nkey[keyLength + 3] = static_cast<unsigned char>(0xFF & g);
    nkey[keyLength + 4] = static_cast<unsigned char>(0xFF & (g >> 8));

    if (m_Algorithm == PdfEncryptionAlgorithm::AESV2)
    {
        // AES encryption needs some 'salt'
        nkeylen += 4;
        nkey[keyLength + 5] = 0x73;
        nkey[keyLength + 6] = 0x41;
        nkey[keyLength + 7] = 0x6C;
        nkey[keyLength + 8] = 0x54;
    }

    ssl::ComputeMD5(bufferview((const char*)nkey, nkeylen), objkey);
    pnKeyLen = (keyLength <= 11) ? keyLength + 5 : 16;
}

void RC4Encrypt(EVP_CIPHER_CTX* ctx, const unsigned char* key, unsigned keylen,
    const unsigned char* textin, size_t textlen,
    unsigned char* textout, size_t textoutlen)
{
    if (textlen != textoutlen)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing RC4 encryption engine");

#if OPENSSL_VERSION_MAJOR >= 3
    PODOFO_ASSERT(ssl::Rc4() != nullptr && "OpenSSL RC4 legacy provider was not found. "
        "Recompile OpenSSL or ensure OPENSSL_MODULES variable is correctly set to load "
        "legacy providers (e.g. legacy.dll)");
#endif // OPENSSL_VERSION_MAJOR >= 3

    // Don't set the key because we will modify the parameters
    int status = EVP_EncryptInit_ex(ctx, ssl::Rc4(), nullptr, nullptr, nullptr);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing RC4 encryption engine");

    status = EVP_CIPHER_CTX_set_key_length(ctx, keylen);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing RC4 encryption engine");

    // We finished modifying parameters so now we can set the key
    status = EVP_EncryptInit_ex(ctx, nullptr, nullptr, key, nullptr);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing RC4 encryption engine");

    int dataOutMoved;
    status = EVP_EncryptUpdate(ctx, textout, &dataOutMoved, textin, (int)textlen);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error RC4-encrypting data");

    status = EVP_EncryptFinal_ex(ctx, &textout[dataOutMoved], &dataOutMoved);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error RC4-encrypting data");
}

void PdfEncryptMD5Base::CreateEncryptionDictionary(PdfDictionary& dictionary) const
{
    dictionary.AddKey("Filter"_n, "Standard"_n);

    if (m_Algorithm == PdfEncryptionAlgorithm::AESV2 || !m_EncryptMetadata)
    {
        PdfDictionary cf;
        PdfDictionary stdCf;

        if (m_Algorithm == PdfEncryptionAlgorithm::RC4V2)
            stdCf.AddKey("CFM"_n, "V2"_n);
        else
            stdCf.AddKey("CFM"_n, "AESV2"_n);
        stdCf.AddKey("Length"_n, static_cast<int64_t>(16));

        dictionary.AddKey("O"_n, PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetOValueRaw()), 32 }));
        dictionary.AddKey("U"_n, PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetUValueRaw()), 32 }));

        stdCf.AddKey("AuthEvent"_n, "DocOpen"_n);
        cf.AddKey("StdCF"_n, stdCf);

        dictionary.AddKey("CF"_n, cf);
        dictionary.AddKey("StrF"_n, "StdCF"_n);
        dictionary.AddKey("StmF"_n, "StdCF"_n);

        dictionary.AddKey("V"_n, static_cast<int64_t>(4));
        dictionary.AddKey("R"_n, static_cast<int64_t>(4));
        dictionary.AddKey("Length"_n, static_cast<int64_t>(128));
        if (!m_EncryptMetadata)
            dictionary.AddKey("EncryptMetadata"_n, PdfVariant(false));
    }
    else if (m_Algorithm == PdfEncryptionAlgorithm::RC4V1)
    {
        dictionary.AddKey("V"_n, static_cast<int64_t>(1));
        // Can be 2 or 3
        dictionary.AddKey("R"_n, static_cast<int64_t>(m_rValue));
    }
    else if (m_Algorithm == PdfEncryptionAlgorithm::RC4V2)
    {
        dictionary.AddKey("V"_n, static_cast<int64_t>(2));
        dictionary.AddKey("R"_n, static_cast<int64_t>(3));
        dictionary.AddKey("Length"_n, PdfVariant(static_cast<int64_t>(m_KeyLength)));
    }

    dictionary.AddKey("O"_n, PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetOValueRaw()), 32 }));
    dictionary.AddKey("U"_n, PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetUValueRaw()), 32 }));
    dictionary.AddKey("P"_n, PdfVariant(GetPValueForSerialization()));
}

void PdfEncryptRC4::GenerateEncryptionKey(
    const string_view& documentId, PdfAuthResult authResult, EVP_CIPHER_CTX* ctx,
    unsigned char uValue[48], unsigned char oValue[48], unsigned char encryptionKey[32])
{
    (void)authResult;

    unsigned char userpswd[32];
    unsigned char ownerpswd[32];

    // Pad passwords
    PadPassword(GetUserPassword(), userpswd);
    PadPassword(GetOwnerPassword(), ownerpswd);

    unsigned keyLength = GetKeyLengthBytes();

    // Compute O value
    ComputeOwnerKey(userpswd, ownerpswd, keyLength, GetRevision(), false, ctx, oValue);

    // Compute encryption key and U value
    ComputeEncryptionKey(documentId, userpswd,
        oValue, GetPValue(), keyLength, GetRevision(), IsMetadataEncrypted(), ctx, uValue, encryptionKey);
}

PdfAuthResult PdfEncryptRC4::Authenticate(const string_view& password, const string_view& documentId,
    EVP_CIPHER_CTX* ctx, unsigned char encryptionKey[32]) const
{
    bool success = false;

    // Pad password
    unsigned char userKey[32];
    unsigned char pswd[32];
    PadPassword(password, pswd);

    unsigned keyLength = GetKeyLengthBytes();

    // Check password: 1) as user password, 2) as owner password
    ComputeEncryptionKey(documentId, pswd, GetOValueRaw(), GetPValue(), keyLength, GetRevision(),
        IsMetadataEncrypted(), ctx, userKey, encryptionKey);

    PdfAuthResult ret = PdfAuthResult::Failed;
    success = CheckKey(userKey, GetUValueRaw());
    if (success)
    {
        ret = PdfAuthResult::User;
    }
    else
    {
        unsigned char userpswd[32];
        ComputeOwnerKey(GetOValueRaw(), pswd, keyLength, GetRevision(), true, ctx, userpswd);
        ComputeEncryptionKey(documentId, userpswd, GetOValueRaw(), GetPValue(), keyLength, GetRevision(),
            IsMetadataEncrypted(), ctx, userKey, encryptionKey);
        success = CheckKey(userKey, GetUValueRaw());

        if (success)
            ret = PdfAuthResult::Owner;
    }

    return ret;
}

unsigned PdfEncryptRC4::normalizeKeyLength(unsigned keyLength)
{
    keyLength = keyLength - keyLength % 8u;
    return std::clamp(keyLength, 40u, 128u);
}

size_t PdfEncryptRC4::CalculateStreamOffset() const
{
    return 0;
}

size_t PdfEncryptRC4::CalculateStreamLength(size_t length) const
{
    return length;
}

void PdfEncryptRC4::Encrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
    const PdfReference& objref, char* outStr, size_t outLen) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    CreateObjKey(objkey, keylen, context.GetEncryptionKey(), objref);
    RC4Encrypt(context.GetCryptCtx(), objkey, keylen, (const unsigned char*)inStr, inLen,
        (unsigned char*)outStr, outLen);
}

void PdfEncryptRC4::Decrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
    const PdfReference& objref, char* outStr, size_t& outLen) const
{
    Encrypt(inStr, inLen, context, objref, outStr, outLen);
}

unique_ptr<InputStream> PdfEncryptRC4::CreateEncryptionInputStream(InputStream& inputStream, size_t inputLen,
    PdfEncryptContext& context, const PdfReference& objref) const
{
    (void)inputLen;
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    this->CreateObjKey(objkey, keylen, context.GetEncryptionKey(), objref);
    auto& rc4Ctx = context.GetCustomCtx<RC4EncryptContext>();
    return unique_ptr<InputStream>(new PdfRC4InputStream(inputStream, inputLen, rc4Ctx.Rc4key, rc4Ctx.Rc4last, objkey, keylen));
}

PdfEncryptRC4::PdfEncryptRC4(PdfString oValue, PdfString uValue, PdfPermissions pValue, PdfRC4Revision revision,
    PdfEncryptionAlgorithm algorithm, unsigned keyLength, bool encryptMetadata)
{
    auto uValueData = uValue.GetRawData();
    if (uValueData.size() < 32)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "/U value is invalid");

    auto oValueData = oValue.GetRawData();
    if (oValueData.size() < 32)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "/O value is invalid");

    InitFromValues(algorithm, (PdfKeyLength)normalizeKeyLength(keyLength), (unsigned char)revision, pValue,
        {uValueData.data(), 32}, {oValueData.data(), 32}, encryptMetadata);
}

PdfEncryptRC4::PdfEncryptRC4(const string_view& userPassword, const string_view& ownerPassword, PdfPermissions protection,
    PdfEncryptionAlgorithm algorithm, PdfKeyLength keyLength)
{
    unsigned char rValue;

    switch (algorithm)
    {
        case PdfEncryptionAlgorithm::RC4V1:
        {
            rValue = 2;
            if (keyLength == PdfKeyLength::Unknown)
            {
                keyLength = PdfKeyLength::L40;
            }
            else
            {
                if (keyLength != PdfKeyLength::L40)
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "Invalid encryption key length for RC4V1. Only 40 bit is supported");
            }

            break;
        }
        case PdfEncryptionAlgorithm::RC4V2:
        {
            rValue = 3;
            if (keyLength == PdfKeyLength::Unknown)
            {
                keyLength = PdfKeyLength::L128;
            }
            else
            {
                switch (keyLength)
                {
                    case PdfKeyLength::L40:
                    case PdfKeyLength::L48:
                    case PdfKeyLength::L56:
                    case PdfKeyLength::L64:
                    case PdfKeyLength::L72:
                    case PdfKeyLength::L80:
                    case PdfKeyLength::L88:
                    case PdfKeyLength::L96:
                    case PdfKeyLength::L104:
                    case PdfKeyLength::L112:
                    case PdfKeyLength::L120:
                    case PdfKeyLength::L128:
                        break;
                    case PdfKeyLength::L256:
                    default:
                        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict,
                            "Invalid encryption key length for RC4V2. Only a multiple of 8 from 40bit to 128bit is supported");;
                }
            }

            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
        }
    }

    InitFromScratch(userPassword, ownerPassword, algorithm, keyLength, rValue, DefaultPermsMask | protection, true);
}

unique_ptr<OutputStream> PdfEncryptRC4::CreateEncryptionOutputStream(OutputStream& outputStream,
    PdfEncryptContext& context, const PdfReference& objref) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    this->CreateObjKey(objkey, keylen, context.GetEncryptionKey(), objref);
    auto& rc4Ctx = context.GetCustomCtx<RC4EncryptContext>();
    return unique_ptr<OutputStream>(new PdfRC4OutputStream(outputStream, rc4Ctx.Rc4key, rc4Ctx.Rc4last, objkey, keylen));
}

void AESDecrypt(EVP_CIPHER_CTX* ctx, const unsigned char* key, unsigned keyLen, const unsigned char* iv,
    const unsigned char* textin, size_t textlen, unsigned char* textout, size_t& outLen)
{
    if ((textlen % 16) != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-decryption data length not a multiple of 16");

    int rc;
    if (keyLen == (int)PdfKeyLength::L128 / 8)
        rc = EVP_DecryptInit_ex(ctx, ssl::Aes128(), nullptr, key, iv);
    else if (keyLen == (int)PdfKeyLength::L256 / 8)
        rc = EVP_DecryptInit_ex(ctx, ssl::Aes256_CBC(), nullptr, key, iv);
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid AES key length");

    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing AES decryption engine");

    int dataOutMoved;
    rc = EVP_DecryptUpdate(ctx, textout, &dataOutMoved, textin, (int)textlen);
    outLen = dataOutMoved;
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-decryption data");

    rc = EVP_DecryptFinal_ex(ctx, textout + outLen, &dataOutMoved);
    outLen += dataOutMoved;
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-decryption data final");
}

void AESEncrypt(EVP_CIPHER_CTX* ctx, const unsigned char* key, unsigned keyLen, const unsigned char* iv,
    const unsigned char* textin, size_t textlen,
    unsigned char* textout, size_t textoutlen)
{
    (void)textoutlen;

    int rc;
    if (keyLen == (int)PdfKeyLength::L128 / 8)
        rc = EVP_EncryptInit_ex(ctx, ssl::Aes128(), nullptr, key, iv);
    else if (keyLen == (int)PdfKeyLength::L256 / 8)
        rc = EVP_EncryptInit_ex(ctx, ssl::Aes256_CBC(), nullptr, key, iv);
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid AES key length");

    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing AES encryption engine");

    int dataOutMoved;
    rc = EVP_EncryptUpdate(ctx, textout, &dataOutMoved, textin, (int)textlen);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");

    rc = EVP_EncryptFinal_ex(ctx, &textout[dataOutMoved], &dataOutMoved);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");
}

void PdfEncryptAESV2::GenerateEncryptionKey(
    const string_view& documentId, PdfAuthResult authResult, EVP_CIPHER_CTX* ctx,
    unsigned char uValue[48], unsigned char oValue[48], unsigned char encryptionKey[32])
{
    (void)authResult;

    unsigned char userpswd[32];
    unsigned char ownerpswd[32];

    // Pad passwords
    PadPassword(GetUserPassword(), userpswd);
    PadPassword(GetOwnerPassword(), ownerpswd);

    unsigned keyLength = GetKeyLengthBytes();

    // Compute O value
    ComputeOwnerKey(userpswd, ownerpswd, keyLength, GetRevision(), false, ctx, oValue);

    // Compute encryption key and U value
    ComputeEncryptionKey(documentId, userpswd,
        oValue, GetPValue(), keyLength, GetRevision(), IsMetadataEncrypted(), ctx, uValue, encryptionKey);
}

PdfAuthResult PdfEncryptAESV2::Authenticate(const string_view& password, const string_view& documentId,
    EVP_CIPHER_CTX* ctx, unsigned char encryptionKey[32]) const
{
    // Pad password
    unsigned char pswd[32];
    PadPassword(password, pswd);

    PdfAuthResult ret = PdfAuthResult::Failed;

    unsigned keyLength = GetKeyLengthBytes();

    // Check password: 1) as user password, 2) as owner password
    unsigned char userKey[32];
    ComputeEncryptionKey(documentId, pswd, GetOValueRaw(), GetPValue(), keyLength, GetRevision(),
        IsMetadataEncrypted(), ctx, userKey, encryptionKey);

    bool success = CheckKey(userKey, GetUValueRaw());
    if (success)
    {
        ret = PdfAuthResult::User;
    }
    else
    {
        unsigned char userpswd[32];
        ComputeOwnerKey(GetOValueRaw(), pswd, keyLength, GetRevision(), true, ctx, userpswd);
        ComputeEncryptionKey(documentId, userpswd, GetOValueRaw(), GetPValue(), keyLength, GetRevision(),
            IsMetadataEncrypted(), ctx, userKey, encryptionKey);
        success = CheckKey(userKey, GetUValueRaw());

        if (success)
            ret = PdfAuthResult::Owner;
    }

    return ret;
}

void PdfEncryptAESV2::generateInitialVector(const string_view& documentId, unsigned char iv[]) const
{
    ssl::ComputeMD5(documentId, iv);
}

size_t PdfEncryptAESV2::CalculateStreamOffset() const
{
    return AES_IV_LENGTH;
}

void PdfEncryptAESV2::Encrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
    const PdfReference& objref, char* outStr, size_t outLen) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    CreateObjKey(objkey, keylen, context.GetEncryptionKey(), objref);
    size_t offset = CalculateStreamOffset();
    generateInitialVector(context.GetDocumentId(), (unsigned char *)outStr);
    AESEncrypt(context.GetCryptCtx(), objkey, keylen, (unsigned char*)outStr, (const unsigned char*)inStr,
        inLen, (unsigned char*)outStr + offset, outLen - offset);
}

void PdfEncryptAESV2::Decrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
    const PdfReference& objref, char* outStr, size_t& outLen) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    CreateObjKey(objkey, keylen, context.GetEncryptionKey(), objref);

    size_t offset = CalculateStreamOffset();
    if (inLen <= offset)
    {
        // Is empty
        outLen = 0;
        return;
    }

    AESDecrypt(context.GetCryptCtx(), objkey, keylen, (const unsigned char*)inStr,
        (const unsigned char*)inStr + offset,
        inLen - offset, (unsigned char*)outStr, outLen);
}

PdfEncryptAESV2::PdfEncryptAESV2(const string_view& userPassword, const string_view& ownerPassword, PdfPermissions protection)
{
    InitFromScratch(userPassword, ownerPassword, PdfEncryptionAlgorithm::AESV2, PdfKeyLength::L128, 4, DefaultPermsMask | protection, true);
}

PdfEncryptAESV2::PdfEncryptAESV2(PdfString oValue, PdfString uValue, PdfPermissions pValue, bool encryptMetadata)
{
    auto oValueData = oValue.GetRawData();
    if (oValueData.size() < 32)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "/O value is invalid");

    auto uValueData = uValue.GetRawData();
    if (uValueData.size() < 32)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "/U value is invalid");

    InitFromValues(PdfEncryptionAlgorithm::AESV2, PdfKeyLength::L128, 4, pValue,
        { uValueData.data(), 32 }, { oValueData.data(), 32 }, encryptMetadata);
}

PdfEncryptAESV2::PdfEncryptAESV2(const PdfEncryptAESV2& rhs)
    : PdfEncryptMD5Base(rhs) { }

size_t PdfEncryptAESV2::CalculateStreamLength(size_t length) const
{
    size_t realLength = ((length + 15) & ~15) + AES_IV_LENGTH;
    if (length % 16 == 0)
        realLength += 16;

    return realLength;
}

unique_ptr<InputStream> PdfEncryptAESV2::CreateEncryptionInputStream(InputStream& inputStream, size_t inputLen,
    PdfEncryptContext& context, const PdfReference& objref) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    this->CreateObjKey(objkey, keylen, context.GetEncryptionKey(), objref);
    return unique_ptr<InputStream>(new PdfAESInputStream(inputStream, inputLen, objkey, keylen));
}

unique_ptr<OutputStream> PdfEncryptAESV2::CreateEncryptionOutputStream(OutputStream& outputStream,
    PdfEncryptContext& context, const PdfReference& objref) const
{
    (void)outputStream;
    (void)context;
    (void)objref;
    // unsigned char objkey[MD5_DIGEST_LENGTH];
    // int keylen;

    // this->CreateObjKey( objkey, &keylen );

    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "CreateEncryptionOutputStream does not yet support AESV2");
}

void PdfEncryptAESV3::computeHash(const unsigned char* pswd, unsigned pswdLen, unsigned revision,
    const unsigned char salt[8], const unsigned char uValue[48], unsigned char hashValue[32])
{
    PODOFO_ASSERT(pswdLen <= 127);

    int rc;
    unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> sha256(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (sha256 == nullptr || (rc = EVP_DigestInit_ex(sha256.get(), ssl::SHA256(), nullptr)) != 1)
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

    if (revision > 5) // AES-256 according to PDF 1.7 Adobe Extension Level 8 (PDF 2.0)
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
            // finalize the context. It may be unnecessary because of some
            // preconditions, but these should be clearly stated
            rc = EVP_EncryptInit_ex(aes.get(), ssl::Aes128(), nullptr, block, block + 16);
            rc = EVP_EncryptUpdate(aes.get(), data, &dataOutMoved, data, dataLen);
            PODOFO_ASSERT((unsigned)dataOutMoved == dataLen);

            unsigned sum = 0;
            for (unsigned j = 0; j < 16; j++)
                sum += data[j];
            blockLen = 32 + (sum % 3) * 16;

            if (blockLen == 32)
            {
                rc = EVP_DigestInit_ex(sha256.get(), ssl::SHA256(), nullptr);
                rc = EVP_DigestUpdate(sha256.get(), data, dataLen);
                rc = EVP_DigestFinal_ex(sha256.get(), block, nullptr);
            }
            else if (blockLen == 48)
            {
                rc = EVP_DigestInit_ex(sha384.get(), ssl::SHA384(), nullptr);
                rc = EVP_DigestUpdate(sha384.get(), data, dataLen);
                rc = EVP_DigestFinal_ex(sha384.get(), block, nullptr);
            }
            else
            {
                rc = EVP_DigestInit_ex(sha512.get(), ssl::SHA512(), nullptr);
                rc = EVP_DigestUpdate(sha512.get(), data, dataLen);
                rc = EVP_DigestFinal_ex(sha512.get(), block, nullptr);
            }
        }
        std::memcpy(hashValue, block, 32);
    }
}

void PdfEncryptAESV3::computeUserKey(const unsigned char* userpswd, unsigned len, unsigned revision,
    unsigned keyLength, const unsigned char encryptionKey[32],
    unsigned char uValue[48], unsigned char ueValue[32])
{
    // ISO 32000-2:2020 "7.6.4.4.7 Algorithm 8: Computing the encryption dictionary’s U
    // (user password) and UE (user encryption) values (Security handlers of revision 6)"
    // "Generate 16 random bytes of data using a strong random number generator.
    // The first 8 bytes are the User Validation Salt. The second 8 bytes are the User Key Salt"

    if (RAND_bytes(uValue + 32, 8) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error generating random user validation salt");
    if (RAND_bytes(uValue + 40, 8) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error generating random user key salt");

    // "Compute the 32-byte hash using algorithm 2.B with an input string consisting
    // of the UTF-8 password concatenated with the User Validation Salt. The 48- byte
    // string consisting of the 32-byte hash followed by the User Validation Salt
    // followed by the User Key Salt is stored as the U key"

    unsigned char hashValue[32];
    computeHash(userpswd, len, revision, uValue + 32, nullptr, hashValue);
    std::memcpy(uValue, hashValue, 32);

    computeHash(userpswd, len, revision, uValue + 40, nullptr, hashValue);

    unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> aes(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (aes == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::OutOfMemory);

    // "Compute the 32-byte hash using algorithm 2.B with an input string consisting
    // of the UTF-8 password concatenated with the User Key Salt. Using this hash
    // as the key, encrypt the file encryption key using AES-256 in CBC mode with
    // no padding and an initialization vector of zero. The resulting 32-byte string
    // is stored as the UE key"
    unsigned char iv[AES_IV_LENGTH] = { 0 };
    if (EVP_EncryptInit_ex(aes.get(), ssl::Aes256_CBC(), nullptr, hashValue, iv) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing AES encryption engine");
    EVP_CIPHER_CTX_set_padding(aes.get(), 0);

    int outLen;
    PODOFO_INVARIANT(keyLength <= 32);
    if (EVP_EncryptUpdate(aes.get(), ueValue, &outLen, encryptionKey, (int)keyLength) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");

    if (EVP_EncryptFinal_ex(aes.get(), ueValue + outLen, &outLen) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");
}

void PdfEncryptAESV3::computeOwnerKey(const unsigned char* ownerpswd, unsigned len, unsigned revision,
    unsigned keyLength, const unsigned char encryptionKey[32], const unsigned char uValue[48],
    unsigned char oValue[48], unsigned char oeValue[32])
{
    // ISO 32000-2:2020 "7.6.4.4.8 Algorithm 9: Computing the encryption dictionary’s O
    // (owner password) and OE (owner encryption) values (Security handlers of revision 6)"
    // "Generate 16 random bytes of data using a strong random number generator. The first
    // 8 bytes are the Owner Validation Salt. The second 8 bytes are the Owner Key Salt"

    if (RAND_bytes(oValue + 32, 8) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error generating random owner validation salt");
    if (RAND_bytes(oValue + 40, 8) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error generating random owner key salt");

    // "Compute the 32-byte hash using algorithm 2.B with an input string consisting of
    // the UTF-8 password concatenated with the Owner Validation Salt and then concatenated
    // with the 48-byte U string as generated in Algorithm 8. The 48-byte string consisting
    // of the 32-byte hash followed by the Owner Validation Salt followed by the Owner Key
    // Salt is stored as the O key"

    unsigned char hashValue[32];
    computeHash(ownerpswd, len, revision, oValue + 32, uValue, hashValue);
    std::memcpy(oValue, hashValue, 32);

    computeHash(ownerpswd, len, revision, oValue + 40, uValue, hashValue);

    unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> aes(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (aes == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::OutOfMemory);

    // "Compute the 32-byte hash using 7.6.4.3.4, "Algorithm 2.B: Computing a hash (revision
    // 6 and later)" with an input string consisting of the UTF-8 password concatenated with
    // the Owner Key Salt and then concatenated with the 48-byte U string as generated in
    // 7.6.4.4.7, "Algorithm 8: Computing the encryption dictionary’s U (user password) and
    // UE (user encryption) values (Security handlers of revision 6)". Using this hash as
    // the key, encrypt the file encryption key using AES-256 in CBC mode with no padding and
    // an initialization vector of zero. The resulting 32-byte string is stored as the OE key"

    unsigned char iv[AES_IV_LENGTH] = { 0 };
    if (EVP_EncryptInit_ex(aes.get(), ssl::Aes256_CBC(), nullptr, hashValue, iv) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing AES encryption engine");
    EVP_CIPHER_CTX_set_padding(aes.get(), 0);

    int outLen;
    PODOFO_INVARIANT(keyLength <= 32);
    if (EVP_EncryptUpdate(aes.get(), oeValue, &outLen, encryptionKey, (int)keyLength) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");

    if (EVP_EncryptFinal_ex(aes.get(), oeValue + outLen, &outLen) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");
}

void PdfEncryptAESV3::preprocessPassword(const string_view& password, unsigned char* outBuf, unsigned& len)
{
    string prepd;
    if (!sprep::TrySASLprep(password, prepd))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidPassword, "Error processing password through SASLprep");

    len = prepd.size() > 127 ? 127 : (unsigned)prepd.size();
    std::memcpy(outBuf, prepd.data(), len);
}

void PdfEncryptAESV3::computeEncryptionKey(unsigned keyLength, unsigned char encryptionKey[32])
{
    PODOFO_INVARIANT(keyLength <= 32);
    if (RAND_bytes(encryptionKey, (int)keyLength) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error generating random encryption key");
}

void PdfEncryptAESV3::CreateEncryptionDictionary(PdfDictionary& dictionary) const
{
    dictionary.AddKey("Filter"_n, "Standard"_n);

    PdfDictionary cf;
    PdfDictionary stdCf;

    dictionary.AddKey("V"_n, static_cast<int64_t>(5));
    dictionary.AddKey("R"_n, static_cast<int64_t>(m_rValue));
    dictionary.AddKey("Length"_n, static_cast<int64_t>(256));

    stdCf.AddKey("CFM"_n, "AESV3"_n);
    stdCf.AddKey("Length"_n, static_cast<int64_t>(32));

    dictionary.AddKey("O"_n, PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetOValueRaw()), 48 }));
    dictionary.AddKey("U"_n, PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetUValueRaw()), 48 }));

    stdCf.AddKey("AuthEvent"_n, "DocOpen"_n);
    cf.AddKey("StdCF"_n, stdCf);

    dictionary.AddKey("CF"_n, cf);
    dictionary.AddKey("StrF"_n, "StdCF"_n);
    dictionary.AddKey("StmF"_n, "StdCF"_n);

    dictionary.AddKey("P"_n, PdfVariant(GetPValueForSerialization()));

    dictionary.AddKey("OE"_n, PdfString::FromRaw(GetOEValue()));
    dictionary.AddKey("UE"_n, PdfString::FromRaw(GetUEValue()));
    dictionary.AddKey("Perms"_n, PdfString::FromRaw(GetPermsValue()));
}

bufferview PdfEncryptAESV3::GetUEValue() const
{
    return bufferview((const char*)m_ueValue, std::size(m_ueValue));
}

bufferview PdfEncryptAESV3::GetOEValue() const
{
    return bufferview((const char*)m_oeValue, std::size(m_oeValue));
}

bufferview PdfEncryptAESV3::GetPermsValue() const
{
    return bufferview((const char*)m_permsValue, std::size(m_permsValue));
}

void PdfEncryptAESV3::GenerateEncryptionKey(
    const string_view& documentId, PdfAuthResult authResult, EVP_CIPHER_CTX* ctx,
    unsigned char uValue[48], unsigned char oValue[48], unsigned char encryptionKey[32])
{
    (void)documentId;
    (void)authResult;
    (void)ctx;  // CHECK-ME: Investigate why we can't reuse
                // the context supplied cipher context here
                // in OpenSSL 3.3. Doing so will break tests

    // ISO 32000-2:2020 7.6.4 Standard security handler
    // "All passwords for revision 6 shall be based on Unicode. Preprocessing
    // of a user-provided password consists first of normalizing its
    // representation by applying the "SASLPrep" profile(Internet RFC 4013)
    // of the "stringprep" algorithm(Internet RFC 3454) to the supplied
    // password using the Normalize and BiDi options.Next, the password
    // string shall be converted to UTF-8 encoding, and then truncated to
    // the first 127 bytes if the string is longer than 127 bytes"

    // Preprocess passwords
    unsigned char userPsw[127];
    unsigned char ownerPsw[127];
    unsigned userPswLen;
    unsigned ownerPswLen;
    preprocessPassword(GetUserPassword(), userPsw, userPswLen);
    preprocessPassword(GetOwnerPassword(), ownerPsw, ownerPswLen);

    // Compute encryption key, /U, /UE, /O, /OE values
    unsigned keyLength = GetKeyLengthBytes();
    computeEncryptionKey(keyLength, encryptionKey);
    computeUserKey(userPsw, userPswLen, GetRevision(), keyLength, encryptionKey, uValue, m_ueValue);
    computeOwnerKey(ownerPsw, ownerPswLen, GetRevision(), keyLength, encryptionKey, uValue, oValue, m_oeValue);

    // ISO 32000-2:2020 "7.6.4.4.9 Algorithm 10: Computing the encryption dictionary’s
    // Perms (permissions) value (Security handlers of revision 6)"
    unsigned char perms[16];
    // "Record the 8 bytes of permission in the bytes 0-7 of the block, low order byte first"
    uint32_t p = static_cast<uint32_t>(GetPValue());
    perms[0] = (unsigned char)(p & 0xFF);
    perms[1] = (unsigned char)((p >> 8) & 0xFF);
    perms[2] = (unsigned char)((p >> 16) & 0xFF);
    perms[3] = (unsigned char)((p >> 24) & 0xFF);
    // "Extend the permissions (contents of the P integer) to 64 bits by setting the upper 32 bits to all 1s."
    perms[4] = 0xFF;
    perms[5] = 0xFF;
    perms[6] = 0xFF;
    perms[7] = 0xFF;
    // "Set byte 8 to the ASCII character "T" or "F" according to the EncryptMetadata boolean"
    perms[8] = IsMetadataEncrypted() ? 'T' : 'F';
    // Set bytes 9 - 11 to the ASCII characters 'a, 'd', 'b'
    perms[9] = 'a';
    perms[10] = 'd';
    perms[11] = 'b';
    // "Set bytes 12-15 to 4 bytes of random data, which will be ignored."
    // TODO: Actually randomize data, and provide a way to make it deterministic
    perms[12] = 0;
    perms[13] = 0;
    perms[14] = 0;
    perms[15] = 0;

    // (From Errata Collection 2) "Encrypt the 16-byte block using AES-256 in ECB mode using the file encryption key
    // as the key. The result (16 bytes) is stored as the Perms string, and checked for validity
    // when the file is opened"
    unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> aes(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (aes == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::OutOfMemory);

    if (EVP_EncryptInit_ex(aes.get(), ssl::Aes256_ECB(), nullptr, encryptionKey, nullptr) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing AES encryption engine");

    // NOTE: Since we the expected results is 16 byte, this implies that we must disable padding,
    // otherwise the output will be 32 byte, corrupting the heap
    EVP_CIPHER_CTX_set_padding(aes.get(), 0);

    int outLen;
    if (EVP_EncryptUpdate(aes.get(), m_permsValue, &outLen, perms, std::size(perms)) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");

    if (EVP_EncryptFinal_ex(aes.get(), m_permsValue + outLen, &outLen) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error AES-encrypting data");
}

PdfAuthResult PdfEncryptAESV3::Authenticate(const string_view& password, const string_view& documentId,
    EVP_CIPHER_CTX* ctx, unsigned char encryptionKey[32]) const
{
    (void)documentId;
    (void)ctx;  // CHECK-ME: Investigate why we can't reuse
                // the context supplied cipher context here
                // in OpenSSL 3.3. Doing so will break tests

    // ISO 32000-2:2020 "7.6.4.3.3 Algorithm 2.A: Retrieving the file encryption key from
    // an encrypted document in order to decrypt it (revision 6 and later)"
    // "a) The UTF - 8 password string shall be generated from Unicode input by processing
    // the input string with the SASLprep(Internet RFC 4013) profile of stringprep (Internet
    // RFC 3454) using the Normalize and BiDi options, and then converting to a UTF-8 representation.
    // b) Truncate the UTF - 8 representation to 127 bytes if it is longer than 127 bytes"
    unsigned char pswd_sasl[127];
    unsigned pswdLen;
    preprocessPassword(password, pswd_sasl, pswdLen);

    // "Test the password against the owner key by computing a hash using algorithm 2.B
    // with an input string consisting of the UTF-8 password concatenated with the 8 bytes
    // of owner Validation Salt, concatenated with the 48-byte U string. If the 32-byte
    // result matches the first 32 bytes of the O string, this is the owner password"
    unsigned char hashValue[32];
    computeHash(pswd_sasl, pswdLen, GetRevision(), GetUValueRaw() + 32, nullptr, hashValue); // user Validation Salt

    bool success = CheckKey(hashValue, GetUValueRaw());
    PdfAuthResult ret;
    const unsigned char* salt;
    const unsigned char* uValue;
    const unsigned char* decryptIn;
    if (success)
    {
        ret = PdfAuthResult::User;
        salt = GetUValueRaw() + 40;
        uValue = nullptr;
        decryptIn = m_ueValue;
    }
    else
    {
        // Secondly, check if it is the owner key
        computeHash(pswd_sasl, pswdLen, GetRevision(), GetOValueRaw() + 32, GetUValueRaw(), hashValue); // owner Validation Salt

        success = CheckKey(hashValue, GetOValueRaw());
        if (!success)
            return PdfAuthResult::Failed;

        ret = PdfAuthResult::Owner;
        salt = GetOValueRaw() + 40;
        uValue = GetUValueRaw();
        decryptIn = m_oeValue;
    }

    // Compute an intermediate user/owner key by computing a hash using algorithm 2.B with an input
    // string consisting of the UTF-8 user/owner password concatenated with the 8 bytes of user/owner Key
    // Salt, concatenated with the 48-byte U/O string"
    computeHash(pswd_sasl, pswdLen, GetRevision(), salt, uValue, hashValue);

    unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> aes(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (aes == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::OutOfMemory);

    // "The 32-byte result is the key used to decrypt the 32-byte UE/OE string using AES-256 in
    // CBC mode with no padding and an initialization vector of zero. The 32-byte result is the
    // file encryption key"
    unsigned char iv[AES_IV_LENGTH] = { 0 };
    if (EVP_DecryptInit_ex(aes.get(), ssl::Aes256_CBC(), nullptr, hashValue, iv) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error initializing AES encryption engine");
    EVP_CIPHER_CTX_set_padding(aes.get(), 0);

    int outLen;
    if (EVP_DecryptUpdate(aes.get(), encryptionKey, &outLen, decryptIn, 32) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error during AES decryption");

    // TODO Validate permissions (or not...)

    return ret;
}

size_t PdfEncryptAESV3::CalculateStreamOffset() const
{
    return AES_IV_LENGTH;
}

void PdfEncryptAESV3::Encrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
    const PdfReference& objref, char* outStr, size_t outLen) const
{
    (void)objref;
    size_t offset = CalculateStreamOffset();
    this->generateInitialVector((unsigned char*)outStr);
    AESEncrypt(context.GetCryptCtx(), context.GetEncryptionKey(), GetKeyLengthBytes(),
        (unsigned char*)outStr, (const unsigned char*)inStr, inLen, (unsigned char*)outStr + offset, outLen - offset);
}

void PdfEncryptAESV3::Decrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
    const PdfReference& objref, char* outStr, size_t& outLen) const
{
    (void)objref;
    size_t offset = CalculateStreamOffset();
    if (inLen <= offset)
    {
        // Is empty
        outLen = 0;
        return;
    }

    AESDecrypt(context.GetCryptCtx(), context.GetEncryptionKey(), GetKeyLengthBytes(),
        (const unsigned char*)inStr, (const unsigned char*)inStr + offset, inLen - offset, (unsigned char*)outStr, outLen);
}

// R5 Support added by P.Zent,
PdfEncryptAESV3::PdfEncryptAESV3(const string_view& userPassword, const string_view& ownerPassword,
        PdfAESV3Revision revision, PdfPermissions protection)
    : m_ueValue{ }, m_oeValue{ }, m_permsValue{ }
{
    InitFromScratch(userPassword, ownerPassword, revision == PdfAESV3Revision::R6 ? PdfEncryptionAlgorithm::AESV3R6 : PdfEncryptionAlgorithm::AESV3R5,
        PdfKeyLength::L256, (unsigned char)revision, DefaultPermsMask | protection, true);
}

PdfEncryptAESV3::PdfEncryptAESV3(PdfString oValue, PdfString oeValue, PdfString uValue,
        PdfString ueValue, PdfPermissions pValue, PdfString permsValue, PdfAESV3Revision revision)
{
    auto uValueData = uValue.GetRawData();
    if (uValueData.size() < 48)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "/U value is invalid");

    auto oValueData = oValue.GetRawData();
    if (oValueData.size() < 48)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "/O value is invalid");

    auto ueValueData = ueValue.GetRawData();
    if (ueValueData.size() < 32)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "/UE value is invalid");

    auto oeValueData = oeValue.GetRawData();
    if (oeValueData.size() < 32)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "/OE value is invalid");

    auto permsValueData = permsValue.GetRawData();
    if (permsValueData.size() < 16)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "/Perms value is invalid");

    InitFromValues(revision == PdfAESV3Revision::R6 ? PdfEncryptionAlgorithm::AESV3R6 : PdfEncryptionAlgorithm::AESV3R5,
        PdfKeyLength::L256, (unsigned char)revision, pValue, { uValueData.data(), 48 }, { oValueData.data(), 48 },
        true);

    std::memcpy(m_ueValue, ueValueData.data(), std::size(m_ueValue));
    std::memcpy(m_oeValue, oeValueData.data(), std::size(m_oeValue));
    std::memcpy(m_permsValue, permsValueData.data(), std::size(m_permsValue));
}

PdfEncryptAESV3::PdfEncryptAESV3(const PdfEncryptAESV3& rhs)
    : PdfEncrypt(rhs)
{
    std::memcpy(m_permsValue, rhs.m_permsValue, std::size(m_permsValue));
    std::memcpy(m_ueValue, rhs.m_ueValue, std::size(m_permsValue));
    std::memcpy(m_oeValue, rhs.m_oeValue, std::size(m_permsValue));
}

size_t PdfEncryptAESV3::CalculateStreamLength(size_t length) const
{
    size_t realLength = ((length + 15) & ~15) + AES_IV_LENGTH;
    if (length % 16 == 0)
        realLength += 16;

    return realLength;
}

unique_ptr<InputStream> PdfEncryptAESV3::CreateEncryptionInputStream(InputStream& inputStream, size_t inputLen,
    PdfEncryptContext& context, const PdfReference& objref) const
{
    (void)objref;
    return unique_ptr<InputStream>(new PdfAESInputStream(inputStream, inputLen, context.GetEncryptionKey(), 32));
}

unique_ptr<OutputStream> PdfEncryptAESV3::CreateEncryptionOutputStream(OutputStream& outputStream,
    PdfEncryptContext& context, const PdfReference& objref) const
{
    (void)outputStream;
    (void)context;
    (void)objref;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "CreateEncryptionOutputStream does not yet support AESV3");
}

void PdfEncryptAESV3::generateInitialVector(unsigned char iv[])
{
    if (RAND_bytes(iv, AES_IV_LENGTH) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Error generating random initialization vector");
}

void PdfEncrypt::EncryptTo(charbuff& out, const bufferview& view, PdfEncryptContext& context, const PdfReference& objref) const
{
    PODOFO_ASSERT(m_initialized);
    size_t outputLen = this->CalculateStreamLength(view.size());
    out.resize(outputLen);
    this->Encrypt(view.data(), view.size(), context, objref, out.data(), outputLen);
}

void PdfEncrypt::DecryptTo(charbuff& out, const bufferview& view, PdfEncryptContext& context, const PdfReference& objref) const
{
    PODOFO_ASSERT(m_initialized);
    // FIX-ME: The following clearly seems hardcoded for AES
    // It was found like this in PdfString and PdfTokenizer
    // Fix it so it will allocate the exact amount of memory
    // needed, including RC4
    size_t outBufferLen = view.size() - this->CalculateStreamOffset();
    out.resize(outBufferLen + 16 - (outBufferLen % 16));
    this->Decrypt(view.data(), view.size(), context, objref, out.data(), outBufferLen);
    out.resize(outBufferLen);
    out.shrink_to_fit();
}

unsigned PdfEncrypt::GetKeyLengthBytes() const
{
    return (unsigned)m_KeyLength / 8;
}

bufferview PdfEncrypt::GetUValue() const
{
    return bufferview((const char*)m_uValue, m_uValueSize);
}

bufferview PdfEncrypt::GetOValue() const
{
    return bufferview((const char*)m_oValue, m_oValueSize);
}

bool PdfEncrypt::IsOwnerPasswordSet() const
{
    return !m_ownerPass.empty();
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
