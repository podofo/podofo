/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_ENCRYPT_H
#define PDF_ENCRYPT_H

#include "PdfString.h"
#include "PdfReference.h"

// Define an opaque type for the internal PoDoFo encryption context
#ifndef PODOFO_CRYPT_CTX
#define PODOFO_CRYPT_CTX void
#endif // PODOFO_CRYPT_CTX

namespace PoDoFo
{

class PdfDictionary;
class InputStream;
class PdfObject;
class OutputStream;

/* Class representing PDF encryption methods. (For internal use only)
 * Based on code from Ulrich Telle: http://wxcode.sourceforge.net/components/wxpdfdoc/
 * Original Copyright header:
 * Name:        pdfencrypt.h
 * Purpose:
 * Author:      Ulrich Telle
 * Modified by:
 * Created:     2005-08-16
 * Copyright:   (c) Ulrich Telle
 * Licence:     wxWindows licence
 */

/** A enum specifying a valid keylength for a PDF encryption key
 */
enum class PdfKeyLength : uint16_t
{
    Unknown = 0,
    L40 = 40,
    L48 = 48,
    L56 = 56,
    L64 = 64,
    L72 = 72,
    L80 = 80,
    L88 = 88,
    L96 = 96,
    L104 = 104,
    L112 = 112,
    L120 = 120,
    L128 = 128,
    L256 = 256
};

/** Set user permissions/restrictions on a document
 */
// ISO 32000-2:2020 7.6.4.2 "Standard encryption dictionary":
// "The value of the P entry shall be interpreted as an unsigned
// 32-bit quantity containing a set of flags."
enum class PdfPermissions : uint32_t
{
    None = 0,
    Print = 0x00000004,  ///< Allow printing the document
    Edit = 0x00000008,  ///< Allow modifying the document besides annotations, form fields or changing pages
    Copy = 0x00000010,  ///< Allow text and graphic extraction
    EditNotes = 0x00000020,  ///< Add or modify text annotations or form fields (if PdfPermissions::Edit is set also allow to create interactive form fields including signature)
    FillAndSign = 0x00000100,  ///< Fill in existing form or signature fields 
    Accessible = 0x00000200,  ///< Extract text and graphics to support user with disabilities
    DocAssembly = 0x00000400,  ///< Assemble the document: insert, create, rotate delete pages or add bookmarks
    HighPrint = 0x00000800,   ///< Print a high resolution version of the document
    Default = Print
        | Edit
        | Copy
        | EditNotes
        | FillAndSign
        | Accessible
        | DocAssembly
        | HighPrint
};

/**
 * The encryption algorithm.
 */
enum class PdfEncryptionAlgorithm : uint8_t
{
    None = 0,
    RC4V1 = 1,      ///< RC4 Version 1 encryption using a 40bit key
    RC4V2 = 2,      ///< RC4 Version 2 encryption using a key with 40-128bit
    AESV2 = 4,      ///< AES encryption with a 128 bit key (PDF1.6)
    AESV3R5 = 8,    ///< AES encryption with a 256 bit key (PDF1.7 extension 3, deprecated in PDF 2.0)
    AESV3R6 = 16,   ///< AES encryption with a 256 bit key, Revision 6 (PDF1.7 extension 8, PDF 2.0)
};

enum class PdfAuthResult : uint8_t
{
    Unkwnon = 0,
    Failed,     ///< Failed to authenticate to this PDF
    User,       ///< Success authenticating a user for this PDF
    Owner,      ///< Success authenticating the owner for this PDF
};

class PdfEncryptContext;

/** A class that is used to encrypt a PDF file and
 *  set document permissions on the PDF file.
 *
 *  As a user of this class, you have only to instantiate a
 *  object of this class and pass it to PdfWriter, PdfMemDocument,
 *  PdfStreamedDocument or PdfImmediateWriter.
 *  You do not have to call any other method of this class. The above
 *  classes know how to handle encryption using PdfEncrypt.
 *
 */
class PODOFO_API PdfEncrypt
{
    friend class PdfEncryptMD5Base;
    friend class PdfEncryptAESV3;
    PODOFO_PRIVATE_FRIEND(class PdfEncryptSession);

public:
    virtual ~PdfEncrypt();

    /** Create a PdfEncrypt object which can be used to encrypt a PDF file.
     *
     *  \param userPassword the user password (if empty the user does not have
     *                      to enter a password to open the document)
     *  \param ownerPassword the owner password
     *  \param protection several PdfPermissions values or'ed together to set
     *                    the users permissions for this document
     *  \param algorithm the revision of the encryption algorithm to be used
     *  \param keyLength the length of the encryption key ranging from 40 to 128 bits
     *                    (only used if algorithm == PdfEncryptAlgorithm::RC4V2)
     *
     *  \see EnsureEncryptionInitialized with the documentID to generate the real
     *       encryption key using this information
     */
    static std::unique_ptr<PdfEncrypt> Create(const std::string_view& userPassword,
        const std::string_view& ownerPassword,
        PdfPermissions protection = PdfPermissions::Default,
        PdfEncryptionAlgorithm algorithm = PdfEncryptionAlgorithm::AESV3R6,
        PdfKeyLength keyLength = PdfKeyLength::Unknown);

    /** Initialize a PdfEncrypt object from an encryption dictionary in a PDF file.
     *
     *  This is required for encrypting a PDF file, but handled internally in PdfParser
     *  for you.
     *
     *  Will use only encrypting algorithms that are enabled.
     *
     *  \param obj a PDF encryption dictionary
     *
     *  \see GetEnabledEncryptionAlgorithms
     */
    static std::unique_ptr<PdfEncrypt> CreateFromObject(const PdfObject& obj);

    /**
     * Retrieve the list of encryption algorithms that are used
     * when loading a PDF document.
     *
     * By default all algorithms are enabled.
     *
     * \see IsEncryptionEnabled
     * \see SetEnabledEncryptionAlgorithms
     *
     * \return an or'ed together list of all enabled encryption algorithms
     */
    static PdfEncryptionAlgorithm GetEnabledEncryptionAlgorithms();

    /**
     * Test if a certain encryption algorithm is enabled for loading PDF documents.
     *
     * \returns true if the encryption algorithm is enabled
     * \see GetEnabledEncryptionAlgorithms
     * \see SetEnabledEncryptionAlgorithms
     */
    static bool IsEncryptionEnabled(PdfEncryptionAlgorithm algorithm);

    /** Ensure encryption key and /O, /U, /OE, /UE values are initialized
     *
     *  \param documentId the documentId of the current document
     */
    void EnsureEncryptionInitialized(const PdfString& documentId, PdfEncryptContext& context);

    /**
     * Tries to authenticate a user using either the user or owner password
     *
     * \param password owner or user password
     * \param documentId the documentId of the PDF file
     *
     * \returns true if either the owner or user password matches password
     */
    void Authenticate(const std::string_view& password, const PdfString& documentId, PdfEncryptContext& context) const;

    /** Fill all keys into a encryption dictionary.
     *  This dictionary is usually added to the PDF files trailer
     *  under the /Encryption key.
     *
     *  \param dictionary an empty dictionary which is filled with information about
     *                     the used encryption algorithm
     */
    virtual void CreateEncryptionDictionary(PdfDictionary& dictionary) const = 0;

    /** Create an InputStream that decrypts all data read from
     *  it using the current settings of the PdfEncrypt object.
     *
     *  Warning: Currently only RC4 based encryption is supported using output streams!
     *
     *  \param inputStream the created InputStream reads all decrypted
     *         data to this input stream.
     *
     *  \returns an InputStream that decrypts all data.
     */
    virtual std::unique_ptr<InputStream> CreateEncryptionInputStream(InputStream& inputStream, size_t inputLen,
        PdfEncryptContext& context, const PdfReference& objref) const = 0;

    /** Create an OutputStream that encrypts all data written to
     *  it using the current settings of the PdfEncrypt object.
     *
     *  Warning: Currently only RC4 based encryption is supported using output streams!
     *
     *  \param outputStream the created OutputStream writes all encrypted
     *         data to this output stream.
     *
     *  \returns a OutputStream that encrypts all data.
     */
    virtual std::unique_ptr<OutputStream> CreateEncryptionOutputStream(OutputStream& outputStream,
        PdfEncryptContext& context, const PdfReference& objref) const = 0;

    /** Get the encryption algorithm of this object.
     * \returns the PdfEncryptAlgorithm of this object
     */
    inline PdfEncryptionAlgorithm GetEncryptAlgorithm() const { return m_Algorithm; }

    /** Checks if an owner password is set.
     *  An application reading PDF must adhere to permissions for printing,
     *  copying, etc., unless the owner password was used to open it.
     *
     *  \returns true if the owner password is set, in which case
     *  IsPrintAllowed etc. should be ignored
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsOwnerPasswordSet() const;

    /** Checks if printing this document is allowed.
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to print this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsPrintAllowed() const;

    /** Checks if modifying this document (besides annotations, form fields or changing pages) is allowed.
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to modify this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsEditAllowed() const;

    /** Checks if text and graphics extraction is allowed.
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to extract text and graphics from this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsCopyAllowed() const;

    /** Checks if it is allowed to add or modify annotations or form fields
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to add or modify annotations or form fields
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsEditNotesAllowed() const;

    /** Checks if it is allowed to fill in existing form or signature fields
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to fill in existing form or signature fields
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsFillAndSignAllowed() const;

    /** Checks if it is allowed to extract text and graphics to support users with disabilities
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to extract text and graphics to support users with disabilities
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsAccessibilityAllowed() const;

    /** Checks if it is allowed to insert, create, rotate, delete pages or add bookmarks
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed  to insert, create, rotate, delete pages or add bookmarks
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsDocAssemblyAllowed() const;

    /** Checks if it is allowed to print a high quality version of this document
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to print a high quality version of this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsHighPrintAllowed() const;

    /** Encrypt a character span
     */
    void EncryptTo(charbuff& out, const bufferview& view, PdfEncryptContext& context, const PdfReference& objref) const;

    /** Decrypt a character span
     */
    void DecryptTo(charbuff& out, const bufferview& view, PdfEncryptContext& context, const PdfReference& objref) const;

    /** Calculate stream size
     */
    virtual size_t CalculateStreamLength(size_t length) const = 0;

    /** Calculate stream offset
     */
    virtual size_t CalculateStreamOffset() const = 0;

    /** Get the encryption key length in bytes
     * \remarks The maximum is 32 bytes
     */
    unsigned GetKeyLengthBytes() const;

    /** Get the U object value (user)
     */
    bufferview GetUValue() const;

    /** Get the O object value (owner)
     */
    bufferview GetOValue() const;

public:
    /** Get the length of the encryption key in bits
     */
    PdfKeyLength GetKeyLength() const { return m_KeyLength; }

    /** Get the P object value (protection)
     */
    inline PdfPermissions GetPValue() const { return m_pValue; }

    /** Get the revision number of the encryption method
     */
    inline unsigned GetRevision() const { return m_rValue; }

    inline bool IsMetadataEncrypted() const { return m_EncryptMetadata; }

    inline bool IsParsed() const { return m_IsParsed; }

protected:
    inline const unsigned char* GetUValueRaw() const { return m_uValue; }

    inline const unsigned char* GetOValueRaw() const { return m_oValue; }

    inline const std::string& GetUserPassword() const { return m_userPass; }

    inline const std::string& GetOwnerPassword() const { return m_ownerPass; }

    int64_t GetPValueForSerialization() const;

protected:
    /**
     * Init from parsed values
     */
    void InitFromValues(PdfEncryptionAlgorithm algorithm, PdfKeyLength keyLength, unsigned char revision,
        PdfPermissions pValue, const bufferview& uValue, const bufferview& oValue,
        bool encryptedMetadata);

    /**
     * Init from scratch with user/owner password. Requires full
     * initialization with EnsureEncryptionInitialized
     */
    void InitFromScratch(const std::string_view& userPassword, const std::string_view& ownerPassword,
        PdfEncryptionAlgorithm algorithm, PdfKeyLength keyLength, unsigned char revision,
        PdfPermissions pValue, bool encryptedMetadata);

    virtual void Decrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
        const PdfReference& objref, char* outStr, size_t& outLen) const = 0;

    virtual void Encrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
        const PdfReference& objref, char* outStr, size_t outLen) const = 0;

    virtual PdfAuthResult Authenticate(const std::string_view& password, const std::string_view& documentId,
        PODOFO_CRYPT_CTX* ctx, unsigned char encryptionKey[32]) const = 0;

    virtual void GenerateEncryptionKey(
        const std::string_view& documentId, PdfAuthResult authResult, PODOFO_CRYPT_CTX* ctx,
        unsigned char uValue[48], unsigned char oValue[48], unsigned char encryptionKey[32]) = 0;

    // Check two keys for equality
    bool CheckKey(const unsigned char key1[32], const unsigned char key2[32]) const;

    enum class PdfRC4Revision : uint8_t
    {
        R2 = 2,
        R3 = 3,
    };

    enum class PdfAESV3Revision : uint8_t
    {
        R5 = 5,
        R6 = 6,
    };

private:
    PdfEncrypt();

    PdfEncrypt(const PdfEncrypt& rhs) = default;

    PdfEncrypt& operator=(PdfEncrypt& rhs) = delete;

private:
    static std::unique_ptr<PdfEncrypt> CreateFromEncrypt(const PdfEncrypt& rhs);

    void clearSensitiveInfo();

private:
    PdfEncryptionAlgorithm m_Algorithm;   // The used encryption algorithm
    unsigned char m_rValue;            // Revision
    PdfKeyLength m_KeyLength;          // The encryption key length, as enum value
    PdfPermissions m_pValue;           // P entry in pdf document
    unsigned char m_uValue[48];        // U entry in pdf document
    unsigned char m_oValue[48];        // O entry in pdf document
    unsigned char m_uValueSize;
    unsigned char m_oValueSize;
    bool m_EncryptMetadata;            // Is metadata encrypted
    bool m_IsParsed;                   // True if the object is created from parsed values
    bool m_initialized;               // True if the object O/U values were filled
    std::string m_userPass;            // User password
    std::string m_ownerPass;           // Owner password

};

class PODOFO_API PdfEncryptContext final
{
    friend class PdfEncrypt;
    friend class PdfEncryptRC4;
    friend class PdfEncryptAESV2;
    friend class PdfEncryptAESV3;

public:
    PdfEncryptContext();

    ~PdfEncryptContext();

    PdfEncryptContext(const PdfEncryptContext&);

    PdfEncryptContext& operator=(const PdfEncryptContext&);

public:
    inline PdfAuthResult GetAuthResult() { return m_AuthResult; }

    inline const std::string GetDocumentId() { return m_documentId; }

    bool IsAuthenticated() const;

private:
    inline const unsigned char* GetEncryptionKey() const { return m_encryptionKey; }

    PODOFO_CRYPT_CTX* GetCryptCtx();

    template <typename T>
    T& GetCustomCtx()
    {
        if (m_customCtx == nullptr)
        {
            m_customCtx = ::operator new(sizeof(T));
            m_customCtxSize = sizeof(T);
        }

        return *(T*)m_customCtx;
    }

private:
    unsigned char m_encryptionKey[32]; // Encryption key
    std::string m_documentId;          // DocumentID of the current document
    PdfAuthResult m_AuthResult;
    PODOFO_CRYPT_CTX* m_cryptCtx;
    void* m_customCtx;
    size_t m_customCtxSize;
};


/** A pure virtual class that is used to encrypt a PDF file (RC4, AES-128)
 *  This class is the base for classes that implement algorithms based on MD5 hashes
 *
 *  Client code is working only with PdfEncrypt class and knows nothing
 *	about PdfEncrypt*, it is created through CreatePdfEncrypt factory method
 *
 */
class PdfEncryptMD5Base : public PdfEncrypt
{
    friend class PdfEncryptRC4;
    friend class PdfEncryptAESV2;

private:
    PdfEncryptMD5Base();

    PdfEncryptMD5Base(const PdfEncryptMD5Base& rhs);

public:
    void CreateEncryptionDictionary(PdfDictionary& dictionary) const override;

    // NOTE: We must declare again without body otherwise the other Authenticate overload hides it
    PdfAuthResult Authenticate(const std::string_view& password, const std::string_view& documentId,
        PODOFO_CRYPT_CTX* ctx, unsigned char encryptionKey[32]) const override = 0;

protected:
    // Compute owner key
    static void ComputeOwnerKey(const unsigned char userPad[32], const unsigned char ownerPad[32],
        unsigned keylength, unsigned revision, bool authenticate, PODOFO_CRYPT_CTX* ctx, unsigned char ownerKey[32]);

    // Pad a password to 32 characters
    static void PadPassword(const std::string_view& password, unsigned char pswd[32]);

    // Compute encryption key and user key
    static void ComputeEncryptionKey(const std::string_view& documentID,
        const unsigned char userPad[32], const unsigned char ownerKey[32],
        PdfPermissions pValue, unsigned keyLength, unsigned revision,
        bool encryptMetadata, PODOFO_CRYPT_CTX* ctx,
        unsigned char userKey[32], unsigned char encryptionKey[32]);

    /** Create the encryption key for the current object.
     *
     *  \param objkey pointer to an array of at least MD5_HASHBYTES (=16) bytes length
     *  \param pnKeyLen pointer to an integer where the actual keylength is stored.
     */
    void CreateObjKey(unsigned char objkey[16], unsigned& pnKeyLen,
        const unsigned char m_encryptionKey[32], const PdfReference& objref) const;
};

/** A class that is used to encrypt a PDF file (AES-128)
 *
 *  Client code is working only with PdfEncrypt class and knows nothing
 *	about PdfEncryptAES*, it is created through CreatePdfEncrypt factory method
 *
 */
class PdfEncryptAESV2 final : public PdfEncryptMD5Base
{
    friend class PdfEncrypt;

private:
    PdfEncryptAESV2(PdfString oValue, PdfString uValue, PdfPermissions pValue, bool encryptMetadata);
    PdfEncryptAESV2(const std::string_view& userPassword, const std::string_view& ownerPassword,
        PdfPermissions protection);
    PdfEncryptAESV2(const PdfEncryptAESV2& rhs);

public:
    std::unique_ptr<InputStream> CreateEncryptionInputStream(InputStream& inputStream, size_t inputLen,
        PdfEncryptContext& context, const PdfReference& objref) const override;
    std::unique_ptr<OutputStream> CreateEncryptionOutputStream(OutputStream& outputStream,
        PdfEncryptContext& context, const PdfReference& objref) const override;

    size_t CalculateStreamOffset() const override;

    size_t CalculateStreamLength(size_t length) const override;

protected:
    void Encrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
        const PdfReference& objref, char* outStr, size_t outLen) const override;
    void Decrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
        const PdfReference& objref, char* outStr, size_t& outLen) const override;

    void GenerateEncryptionKey(const std::string_view& documentId, PdfAuthResult authResult, PODOFO_CRYPT_CTX* ctx,
        unsigned char uValue[48], unsigned char oValue[48], unsigned char encryptionKey[32]) override;

    PdfAuthResult Authenticate(const std::string_view& password, const std::string_view& documentId,
        PODOFO_CRYPT_CTX* ctx, unsigned char encryptionKey[32]) const override;

private:
    void generateInitialVector(const std::string_view& documentId, unsigned char iv[]) const;
};

/** A class that is used to encrypt a PDF file (AES-256)
 *
 *  Client code is working only with PdfEncrypt class and knows nothing
 *	about PdfEncryptAES*, it is created through CreatePdfEncrypt factory method
 *
 */
class PdfEncryptAESV3 final : public PdfEncrypt
{
    friend class PdfEncrypt;

private:
    PdfEncryptAESV3(PdfString oValue, PdfString oeValue, PdfString uValue, PdfString ueValue,
        PdfPermissions pValue, PdfString permsValue, PdfAESV3Revision rev);
    PdfEncryptAESV3(const std::string_view& userPassword, const std::string_view& ownerPassword,
        PdfAESV3Revision rev, PdfPermissions protection);
    PdfEncryptAESV3(const PdfEncryptAESV3& rhs);

public:
    std::unique_ptr<InputStream> CreateEncryptionInputStream(InputStream& inputStream, size_t inputLen,
        PdfEncryptContext& context, const PdfReference& objref) const override;
    std::unique_ptr<OutputStream> CreateEncryptionOutputStream(OutputStream& outputStream,
        PdfEncryptContext& context, const PdfReference& objref) const override;

    size_t CalculateStreamOffset() const override;

    size_t CalculateStreamLength(size_t length) const override;

    void CreateEncryptionDictionary(PdfDictionary& dictionary) const override;

    // Get the UE object value (user)
    bufferview GetUEValue() const;

    // Get the OE object value (owner)
    bufferview GetOEValue() const;

    // Get the Perms object value (encrypted protection)
    bufferview GetPermsValue() const;

protected:
    // Encrypt a character string
    void Encrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
        const PdfReference& objref, char* outStr, size_t outLen) const override;
    void Decrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
        const PdfReference& objref, char* outStr, size_t& outLen) const override;

    PdfAuthResult Authenticate(const std::string_view& password, const std::string_view& documentId,
        PODOFO_CRYPT_CTX* ctx, unsigned char encryptionKey[32]) const override;

    void GenerateEncryptionKey(const std::string_view& documentId, PdfAuthResult authResult, PODOFO_CRYPT_CTX* ctx,
        unsigned char uValue[48], unsigned char oValue[48], unsigned char encryptionKey[32]) override;

private:
    // Generate initial vector
    static void generateInitialVector(unsigned char iv[]);

    // Preprocess password for use in EAS-256 Algorithm
    // outBuf needs to be at least 127 bytes long
    static void preprocessPassword(const std::string_view& password, unsigned char* outBuf, unsigned& len);

    // Compute encryption key to be used with AES-256
    static void computeEncryptionKey(unsigned keyLength, unsigned char encryptionKey[32]);

    // Compute hash for password and salt with optional uValue
    static void computeHash(const unsigned char* pswd, unsigned pswdLen, unsigned revision,
        const unsigned char salt[8], const unsigned char uValue[48], unsigned char hashValue[32]);

    // Generate the U and UE entries
    static void computeUserKey(const unsigned char* userpswd, unsigned len, unsigned revision,
        unsigned keyLength, const unsigned char encryptionKey[32],
        unsigned char uValue[48], unsigned char ueValue[32]);

    // Generate the O and OE entries
    static void computeOwnerKey(const unsigned char* userpswd, unsigned len, unsigned revision,
        unsigned keyLength, const unsigned char encryptionKey[32], const unsigned char uValue[48],
        unsigned char oValue[48], unsigned char oeValue[32]);

private:
    unsigned char m_ueValue[32];        // UE entry in pdf document
    unsigned char m_oeValue[32];        // OE entry in pdf document
    unsigned char m_permsValue[16];     // Perms entry in pdf document
};

/** A class that is used to encrypt a PDF file (RC4 40-bit and 128-bit)
 *
 *  Client code is working only with PdfEncrypt class and knows nothing
 *	about PdfEncryptRC4, it is created through CreatePdfEncrypt factory method
 *
 */
class PdfEncryptRC4 final : public PdfEncryptMD5Base
{
    friend class PdfEncrypt;

private:
    PdfEncryptRC4(PdfString oValue, PdfString uValue, PdfPermissions pValue,
        PdfRC4Revision revision, PdfEncryptionAlgorithm algorithm,
        unsigned keyLength, bool encryptMetadata);
    PdfEncryptRC4(const std::string_view& userPassword, const std::string_view& ownerPassword,
        PdfPermissions protection,
        PdfEncryptionAlgorithm algorithm,
        PdfKeyLength keyLength);
    PdfEncryptRC4(const PdfEncryptRC4& rhs) = default;

public:
    std::unique_ptr<InputStream> CreateEncryptionInputStream(InputStream& inputStream, size_t inputLen,
        PdfEncryptContext& context, const PdfReference& objref) const override;

    std::unique_ptr<OutputStream> CreateEncryptionOutputStream(OutputStream& outputStream,
        PdfEncryptContext& context, const PdfReference& objref) const override;

    size_t CalculateStreamOffset() const override;

    size_t CalculateStreamLength(size_t length) const override;

protected:
    void Encrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
        const PdfReference& objref, char* outStr, size_t outLen) const override;

    void Decrypt(const char* inStr, size_t inLen, PdfEncryptContext& context,
        const PdfReference& objref, char* outStr, size_t& outLen) const override;

    void GenerateEncryptionKey(const std::string_view& documentId, PdfAuthResult authResult, PODOFO_CRYPT_CTX* ctx,
        unsigned char uValue[48], unsigned char oValue[48], unsigned char encryptionKey[32]) override;

    PdfAuthResult Authenticate(const std::string_view& password, const std::string_view& documentId,
        PODOFO_CRYPT_CTX* ctx, unsigned char encryptionKey[32]) const override;

private:
    static unsigned normalizeKeyLength(unsigned keyLength);
};

}
ENABLE_BITMASK_OPERATORS(PoDoFo::PdfPermissions);
ENABLE_BITMASK_OPERATORS(PoDoFo::PdfEncryptionAlgorithm);

#endif // PDF_ENCRYPT_H
