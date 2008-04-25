/***************************************************************************
 *   Copyright (C) 2008 by Hashim Saleem                                   *
 *   hashim.saleem@gmail.com                                               *
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

#ifndef PKIBOX_ASN1_OBJECT_ID_H
#define PKIBOX_ASN1_OBJECT_ID_H

typedef struct asn1_object_st ASN1_OBJECT;

#include <string>

namespace PKIBox
{
	namespace utils
	{
		class ByteArray;
	}

	namespace x509
	{
		class CX509Certificate;
		class CX509Extension;
		class CX509CRLEntry;
		class CX509CRL;
		namespace extensions
		{
			class CExtendedKeyUsage;
			class CAccessDescription;
			class CPolicyQualifierInfo;
			class CPolicyInformation;
			class CHoldInstructionCode;
		}

		namespace attr
		{
			class CObjectDigestInfo;
		}
	}

	namespace ocsp
	{
		class CCertID;
		class CResponseBytes;
		class COCSPResponse;
		class COCSPRequest;
	}

	namespace pkcs7
	{
		class CContentType;
		class CContentInfo;
		class CSignedData;
		class CEnvelopedData;
		class CEncryptedContentInfo;
		class CSignedAndEnvelopedData;
	}

	namespace pkcs10
	{
		class CCertificateRequest;
	}

	namespace pkcs12
	{
		class CSafeBag;
	}

	namespace tsa
	{
		class CTSTInfo;
		class CTimeStampRequest;
	}

	namespace cms
	{
		class CContentInfo;
		class CEncapsulatedContentInfo;
		class CSignedData;
	}

	namespace security
	{
		class CSignature;
	}

	//! This namespace provides classes for different basic ASN.1 structures.
	namespace asn1
	{
		class CAttribute;
		class CAlgorithmID;
		class CRelativeDistinguishedName;
		class CDistinguishedName;

		//! This class implements the ASN.1 native type "OBJECT IDENTIFIER". 
		/*! 
			OBJECT IDENTIFIER is a simple non-string ASN.1 type identified by the UNIVERSAL TAG number 6. 
			An ASN.1 OBJECT IDENTIFIER object represents an object identifier. It consists of a sequence
			of integer components and is used for identifying some abstract information object 
			(for instance an algorithm, an attribute type, or even a registration authority that defines
			other object identifiers). 
		*/
		class ObjectID
		{
			friend class CAttribute;
			friend class CAlgorithmID;
			friend class CRelativeDistinguishedName;
			friend class CDistinguishedName;
			friend class x509::CX509Certificate;
			friend class x509::CX509Extension;
			friend class x509::CX509CRLEntry;
			friend class x509::CX509CRL;
			friend class x509::extensions::CAccessDescription;
			friend class x509::extensions::CExtendedKeyUsage;
			friend class x509::extensions::CPolicyQualifierInfo;
			friend class x509::extensions::CPolicyInformation;
			friend class x509::extensions::CHoldInstructionCode;
			friend class x509::attr::CObjectDigestInfo;
			friend class ocsp::CCertID;
			friend class ocsp::CResponseBytes;
			friend class ocsp::COCSPResponse;
			friend class ocsp::COCSPRequest;
			friend class pkcs7::CContentInfo;
			friend class pkcs7::CContentType;
			friend class pkcs7::CSignedData;
			friend class pkcs7::CEnvelopedData;
			friend class pkcs7::CEncryptedContentInfo;
			friend class pkcs7::CSignedAndEnvelopedData;
			friend class pkcs10::CCertificateRequest;
			friend class pkcs12::CSafeBag;
			friend class tsa::CTimeStampRequest;
			friend class tsa::CTSTInfo;
			friend class cms::CContentInfo;
			friend class cms::CEncapsulatedContentInfo;
			friend class cms::CSignedData;
			friend class security::CSignature;

		public:
			//! Default constructor. Initializes m_pObjectID to NULL.
			ObjectID(void);

			//! Constructs an ObjectID from its string representation i.e. "1.2.3.4".
			/*!
				\param const std::string &ObjectID: the objectID as String, e.g. "1.2.3.4"
			*/
			explicit ObjectID(const std::string &ObjectID);

			virtual ~ObjectID(void);

			//! Copy constructor.
			/*!
				\param const ObjectID &rhs
			*/
			ObjectID(const ObjectID &rhs);

			//! Copy assignment operator.
			/*!
				\param const ObjectID &rhs
				\return ObjectID &
			*/
			ObjectID &operator=(const ObjectID &rhs);

			//! Returns the objectID as string.
			/*!
				\return std::string: the objectID as String ("1.2.3.4")
			*/
			std::string GetID() const ;

			//! Returns the name registered for this ObjectID.
			/*!
				\return std::string: the name of the ObjectID; if no name is registered, the OID string is returned
			*/
			std::string GetName() const;

			//! Sets object Identifier
			/*!
				\param const std::string &lpzOID: the objectID as String ("1.2.3.4")
			*/
			void SetID(const std::string &lpzOID);

			//! Sets the value of this ObjectID
			/*!
				\param const utils::ByteArray &baValue: the value of this ObjectID as a bytearray object
			*/
			void SetValue(const utils::ByteArray &baValue);

			//! Equality operator.
			/*!
				\param const ObjectID &rhs
				\return bool
			*/
			bool operator==(const ObjectID &rhs) const;

			//! Inequality operator.
			/*!
				\param const ObjectID &rhs
				\return bool
			*/
			bool operator!=(const ObjectID &rhs) const;

		private:
			ASN1_OBJECT *m_pObjectID; // Pointer to underlying OpenSSL struct.
		};
	}
}

#endif //!PKIBOX_ASN1_OBJECT_ID_H

