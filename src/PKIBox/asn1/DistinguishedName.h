
#ifndef PKIBOX_ASN1_DISTINGUISHED_NAME_H
#define PKIBOX_ASN1_DISTINGUISHED_NAME_H

//#undef X509_NAME
typedef struct X509_name_st X509_NAME;

#include <vector>
#include <string>

namespace PKIBox
{
	namespace utils
	{
		class ByteArray;
	}

	// Forward declarations.
	namespace x509
	{
		class CX509Certificate;
		class CX509CRL;
		class CX509Store;
	}

	namespace ocsp
	{
		class CResponderID;
		namespace extensions
		{
			class CServiceLocator;
		}

	}

	namespace pkcs7
	{
		class CIssuerAndSerialNumber;
		class CRecipientInfo;
	}

	namespace pkcs10
	{
		class CCertificateRequest;
	}

	namespace asn1
	{
		// Forward declarations.
		class ObjectID;
		class RelativeDistinguishedName;

		//! This class represents an X.501 Name structure. 

		/*! The ASN.1 notation for Name is 

			Name ::= CHOICE { RDNSequence }

			RDNSequence ::= SEQUENCE OF RelativeDistinguishedName

			RelativeDistinguishedName ::= SET OF AttributeValueAssertion

			AttributeValueAssertion ::= SEQUENCE {
				AttributeType,
				AttributeValue }

			AttributeType ::= OBJECT IDENTIFIER

			ATTRIBUTEVALUE ::= ANY
		*/
		class DistinguishedName
		{
			friend class GeneralName;
			friend class x509::CX509Certificate;
			friend class x509::CX509CRL;
			friend class x509::CX509Store;
			friend class ocsp::CResponderID;
			friend class ocsp::extensions::CServiceLocator;
			friend class pkcs7::CIssuerAndSerialNumber;
			friend class pkcs7::CRecipientInfo;
			friend class pkcs10::CCertificateRequest;

		public:
			//! Default constructor. Initializes m_pX509Name to NULL.
			DistinguishedName(void);

			virtual ~DistinguishedName(void);

			//! Copy constructor.
			/*!
				\param const DistinguishedName &rhs
			*/
			DistinguishedName(const DistinguishedName &rhs);

			//! Parameterized constructor
			/*!
				\param const utils::ByteArray &baDN
			*/
			DistinguishedName(const utils::ByteArray &baDN);

			//! Copy assignment operator.
			/*!
				\param const DistinguishedName &rhs
				\return DistinguishedName &
			*/
			DistinguishedName &operator=(const DistinguishedName &rhs);

			//! Returns total number of RDNs contained in this Distinguished Name.
			/*!
				\return int: number of RDNs included in the DN
			*/
			int GetNumberOfRDNs() const;

			//! Returns the all RDNs contained in this Distinguished Name.
			/*!
				\return std::vector<RelativeDistinguishedName>: the values of all included RDNs in the DN
			*/
			std::vector<RelativeDistinguishedName> GetRDNs() const;

			//! Returns the RDN value assigned to a given ObjectID. 
			/*!
				\param const ObjectID &ObjectID: the ObjectID of the RDN (attribute) type to be searched for
				\return std::string: the values of all included RDNs having the requested ObjectID
			*/
			std::string GetRDN(const ObjectID &ObjectID) const;

			//! Adds a RelativeDistinguishedName with given attribute type and value to this Distinguished Name.
			/*!
				\param const ObjectID &ObjectID: the ObjectID (attribute type) of the RDN to be added
				\param const utils::ByteArray &baValue: the value of the RDN to be added
			*/
			void AddRDN(const ObjectID &ObjectID, const utils::ByteArray &baValue);

			//! Adds a RelativeDistinguishedName to this Distinguished Name.
			/*!
				\param const RelativeDistinguishedName &RDN: the RDN to be added
			*/
			void AddRDN(const RelativeDistinguishedName &RDN);

			//! Removes a RelativeDistinguishedNames from this Distinguished Name.
			/*!
				\param const RelativeDistinguishedName &RDN: the RDN to be removed
			*/
			void RemoveRDN(const RelativeDistinguishedName &RDN);

			//! Returns string representation of this Distinguished Name.
			/*!
				\return std::string: the string representation
			*/
			std::string ToString() const ;

			//! Returns the DER encoding of this Distinguished Name
			/*!
				\return ByteArray: byte array holding the DER encoding of this Distinguished name.
			*/
			utils::ByteArray GetEncoded() const;

			//! Equality operator.
			/*!
				\param const DistinguishedName &rhs
				\return bool
			*/
			bool operator==(const DistinguishedName &rhs) const;

			//! Inequality operator.
			/*!
				\param const DistinguishedName &rhs
				\return bool
			*/
			bool operator!=(const DistinguishedName &rhs) const;

			//! Less than operator.
			/*!
				\param const DistinguishedName &rhs
				\return bool
			*/
			bool operator<(const DistinguishedName &rhs) const;

			//! Greater than operator.
			/*!
				\param const DistinguishedName &rhs
				\return bool
			*/
			bool operator>(const DistinguishedName &rhs) const;

			//! Less than operator.
			/*!
				\param const DistinguishedName &rhs
				\return bool
			*/
			bool operator<=(const DistinguishedName &rhs) const;

			//! Greater than operator.
			/*!
				\param const DistinguishedName &rhs
				\return bool
			*/
			bool operator>=(const DistinguishedName &rhs) const;

		private:
			void Construct(const unsigned char *pbArray, unsigned int cLength) /* throw (Exception) */ ;

			X509_NAME *m_pX509Name; // Pointer to underlying OpenSSL struct.
		};			
	}
}

#endif // !PKIBOX_ASN1_DISTINGUISHED_NAME_H
