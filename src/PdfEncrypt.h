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

#ifndef _PDFENCRYPT_H_
#define _PDFENCRYPT_H_

#include "PdfDefines.h"
#include "PdfString.h"
#include "PdfReference.h"


namespace PoDoFo {

class PdfDictionary;
class PdfRijndael;

/// Class representing PDF encryption methods. (For internal use only)
/// Based on code from Ulrich Telle: http://wxcode.sourceforge.net/components/wxpdfdoc/
/// Original Copyright header:
///////////////////////////////////////////////////////////////////////////////
// Name:        pdfencrypt.h
// Purpose:     
// Author:      Ulrich Telle
// Modified by:
// Created:     2005-08-16
// Copyright:   (c) Ulrich Telle
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////


/** A class that is used to encrypt a PDF file and 
 *  set document permisions on the PDF file.
 *
 *  As a user of this class, you have only to instanciate a
 *  object of this class and pass it to PdfWriter, PdfMemDocument,
 *  PdfStreamedDocument or PdfImmediateWriter.
 *  You do not have to call any other method of this class. The above
 *  classes know how to handle encryption using Pdfencrypt.
 *
 */
class PODOFO_API PdfEncrypt
{
public:

    /** A enum specifying a valid keylength for a PDF encryption key.
     *  Keys must be in the range 40 to 128 bit and have to be a 
     *  multiple of 8.
     *
     *  Adobe Reader supports only keys with 40 or 128bit!
     */
    typedef enum {
        ePdfKeyLength_40  = 40,
        ePdfKeyLength_56  = 56,
        ePdfKeyLength_80  = 80,
        ePdfKeyLength_96  = 96,
        ePdfKeyLength_128 = 128
    } EPdfKeyLength;

    /** Set user permissions/restrictions on a document
     */
    typedef enum {
        ePdfPermissions_Print		= 0x00000004,  ///< Allow printing the document
        ePdfPermissions_Edit		= 0x00000008,  ///< Allow modifying the document besides annotations, form fields or chaning pages
        ePdfPermissions_Copy		= 0x00000010,  ///< Allow text and graphic extraction
        ePdfPermissions_EditNotes	= 0x00000020,  ///< Add or modify text annoations or form fields (if ePdfPermissions_Edit is set also allow to create interactive form fields including signature)
        ePdfPermissions_FillAndSign	= 0x00000100,  ///< Fill in existing form or signature fields 
        ePdfPermissions_Accessible	= 0x00000200,  ///< Extract text and graphics to support user with disabillities
        ePdfPermissions_DocAssembly	= 0x00000400,  ///< Assemble the document: insert, create, rotate delete pages or add bookmarks
        ePdfPermissions_HighPrint	= 0x00000800   ///< Print a high resolution version of the document
    } EPdfPermissions;

    typedef enum {
        ePdfEncryptAlgorithm_RC4V1, ///< RC4 Version 1 encryption using a 40bit key
        ePdfEncryptAlgorithm_RC4V2, ///< RC4 Version 2 encryption using a key with 40-128bit
        ePdfEncryptAlgorithm_AESV2  ///< AES encryption with an 128 bit key (PDF1.6)
    } EPdfEncryptAlgorithm;

    /** Create a PdfEncrypt object which can be used to encrypt a PDF file.
     * 
     *  \param userPassword the user password (if empty the user does not have 
     *                      to enter a password to open the document)
     *  \param ownerPassword the owner password
     *  \param protection several EPdfPermissions values or'ed together to set 
     *                    the users permissions for this document
     *  \param eRevision the revision of the encryption algorithm to be used
     *  \param eKeyLength the length of the encryption key ranging from 40 to 128 bits 
     *                    (only used if eAlgorithm == ePdfEncryptAlgorithm_RC4V2)
     *
     *  \see GenerateEncryptionKey with the documentID to generate the real
     *       encryption key using this information
     */
    PdfEncrypt( const std::string & userPassword,
                const std::string & ownerPassword, 
                int protection = ePdfPermissions_Print | 
                                 ePdfPermissions_Edit |
                                 ePdfPermissions_Copy |
                                 ePdfPermissions_EditNotes | 
                                 ePdfPermissions_FillAndSign |
                                 ePdfPermissions_Accessible |
                                 ePdfPermissions_DocAssembly |
                                 ePdfPermissions_HighPrint,
                EPdfEncryptAlgorithm eAlgorithm = ePdfEncryptAlgorithm_RC4V1, 
                EPdfKeyLength eKeyLength = ePdfKeyLength_40 );

    /** Copy constructor
     *
     *  \param rhs another PdfEncrypt object which is copied
     */
    PdfEncrypt( const PdfEncrypt & rhs );

    /** Destruct the PdfEncrypt object
     */
    virtual ~PdfEncrypt();

    /** Assignment operator
     *
     *  \param rhs another PdfEncrypt object which is copied
     */
    const PdfEncrypt & operator=( const PdfEncrypt & rhs );

    /** Generate encryption key from user and owner passwords and protection key
     *  
     *  \param documentId the documentId of the current document
     */
    void GenerateEncryptionKey(const PdfString & documentId);

    /** Fill all keys into a encryption dictionary.
     *  This dictionary is usually added to the PDF files trailer
     *  under the /Encryption key.
     *
     *  \param rDictionary an empty dictionary which is filled with information about
     *                     the used encryption algorithm
     */
    void CreateEncryptionDictionary( PdfDictionary & rDictionary ) const;
    


  bool Authenticate(const std::string & documentID, const std::string & password,
                    const std::string & uValue, const std::string & oValue,
                    int pValue, int lengthValue, int rValue);

  /// Get the U object value (user)
  const unsigned char* GetUValue() const { return m_uValue; }

  /// Get the O object value (owner)
  const unsigned char* GetOValue() const { return m_oValue; }

  /// Get the P object value (protection)
  int GetPValue() const { return m_pValue; }

  /// Get the revision number of the encryption method
  int GetRevision() const { return m_rValue; }

  /// Get the key length of the encryption key in bits
  int GetKeyLength() const { return m_keyLength*8; }

  /// Encrypt a wxString
  void Encrypt( std::string & str ) const;

  /// Encrypt a character string
  void Encrypt(unsigned char* str, int len) const;

  /// Calculate stream size
  int CalculateStreamLength(int length);

  /// Calculate stream offset
  int CalculateStreamOffset();

  /** Create a PdfString of MD5 data generated from a buffer in memory.
   *  \param pBuffer the buffer of which to calculate the MD5 sum
   *  \param nLength the length of the buffer
   * 
   *  \returns an MD5 sum as PdfString
   */
  static PdfString GetMD5String( const unsigned char* pBuffer, int nLength );

  /** Set the reference of the object that is currently encrypted.
   *
   *  This value will be used in following calls of Encrypt
   *  to encrypt the object.
   *
   *  \see Encrypt 
   */
  inline void SetCurrentReference( const PdfReference & rRef );

protected:
  /// Pad a password to 32 characters
  void PadPassword(const std::string& password, unsigned char pswd[32]);

  /// Compute owner key
  void ComputeOwnerKey(unsigned char userPad[32], unsigned char ownerPad[32],
                       int keylength, int revision, bool authenticate,
                       unsigned char ownerKey[32]);

  /// Compute encryption key and user key
  void ComputeEncryptionKey(const std::string & documentID,
                            unsigned char userPad[32], unsigned char ownerKey[32],
                            int pValue, int keyLength, int revision,
                            unsigned char userKey[32]);

  /// Check two keys for equality
  bool CheckKey(unsigned char key1[32], unsigned char key2[32]);

  /// RC4 encryption
  void RC4(unsigned char* key, int keylen,
           unsigned char* textin, int textlen,
           unsigned char* textout);

  /// Calculate the binary MD5 message digest of the given data
  static void GetMD5Binary(const unsigned char* data, int length, unsigned char* digest);

  /// AES encryption
  void AES(unsigned char* key, int keylen,
           unsigned char* textin, int textlen,
           unsigned char* textout);

  /// Generate initial vector
  void GenerateInitialVector(unsigned char iv[16]);

private:
  EPdfEncryptAlgorithm m_eAlgorithm;    ///< The used encryption algorithm
  EPdfKeyLength        m_eKeyLength;    ///< The key length

  unsigned char  m_uValue[32];         ///< U entry in pdf document
  unsigned char  m_oValue[32];         ///< O entry in pdf document
  int            m_pValue;             ///< P entry in pdf document
  int            m_rValue;             ///< Revision
  unsigned char  m_encryptionKey[16];  ///< Encryption key
  int            m_keyLength;          ///< Length of encryption key
  unsigned char  m_rc4key[16];         ///< last RC4 key
  unsigned char  m_rc4last[256];       ///< last RC4 state table

  PdfRijndael*   m_aes;                ///< AES encryptor
  PdfReference   m_curReference;       ///< Reference of the current PdfObject

  std::string    m_documentId;         ///< DocumentID of the current document
  std::string    m_userPass;           ///< User password
  std::string    m_ownerPass;          ///< Owner password
};

void PdfEncrypt::SetCurrentReference( const PdfReference & rRef )
{
    m_curReference = rRef;
}

};

#endif
