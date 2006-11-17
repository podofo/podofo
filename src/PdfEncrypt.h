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

namespace PoDoFo {

/// Class representing PDF encryption methods. (For internal use only)
/// Based on code from Ulrich Telle: http://wxcode.sourceforge.net/components/wxpdfdoc/

class PODOFO_API PdfEncrypt
{
public:
	typedef enum {
		PdfKeyLength40 = 40,
		PdfKeyLength56 = 56,
		PdfKeyLength80 = 80
	} PdfKeyLength;

	typedef enum {
		PdfPermPrint		= 0x00000004,
		PdfPermEdit			= 0x00000008,
		PdfPermCopy			= 0x00000010,
		PdfPermEditNotes	= 0x00000020,
		PdfPermFillAndSign	= 0x00000100,
		PdfPermAccessible	= 0x00000200,
		PdfPermDocAssembly	= 0x00000400,
		PdfPermHighPrint	= 0x00000800
	} PdfPerms;

public:
  /// Default constructor
  PdfEncrypt();

  /// Default destructor
  virtual ~PdfEncrypt();

  /// Generate encryption key from user and owner passwords and protection key
  void GenerateEncryptionKey(const std::string& userPassword,
                             const std::string& ownerPassword,
							 PdfKeyLength inKeyLength,
                             int protection);

  /// Get the U object value (user)
  unsigned char* GetUvalue() { return m_Uvalue; }

  /// Get the O object value (owner)
  unsigned char* GetOvalue() { return m_Ovalue; }

  /// Get the P object value (protection)
  int GetPvalue() { return m_Pvalue; }

  /// Encrypt a character string
  void Encrypt(int n, unsigned char* str, int len);

  /** Create a PdfString of MD5 data generated from a buffer in memory.
   *  \param pBuffer the buffer of which to calculate the MD5 sum
   *  \param nLength the length of the buffer
   * 
   *  \returns an MD5 sum as PdfString
   */
  static PdfString GetMD5String( const unsigned char* pBuffer, int nLength );

protected:
  /// Pad a password to 32 characters
  void PadPassword(const std::string& password, unsigned char pswd[32]);

  /// RC4 encryption
  void RC4(unsigned char* key, int keylen,
           unsigned char* textin, int textlen,
           unsigned char* textout);

  /// Calculate the binary MD5 message digest of the given data
  static void GetMD5Binary(const unsigned char* data, int length, unsigned char* digest);

private:
  unsigned char m_Uvalue[32];         ///< U entry in pdf document
  unsigned char m_Ovalue[32];         ///< O entry in pdf document
  int           m_Pvalue;             ///< P entry in pdf document
  unsigned char m_encryptionKey[5];   ///< Encryption key
  unsigned char m_rc4key[5];          ///< last RC4 key
  unsigned char m_rc4last[256];       ///< last RC4 state table
};

};

#endif
