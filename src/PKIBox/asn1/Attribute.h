
#ifndef PKIBOX_ASN1_ATTRIBUTE_H
#define PKIBOX_ASN1_ATTRIBUTE_H

typedef struct x509_attributes_st X509_ATTRIBUTE;

#include "../utils/clone_ptr.h"
#include <vector>

namespace PKIBox
{
	namespace x509
	{
		namespace attr
		{
			class CAttributeCertificate;
		}
	}

	namespace pkcs7
	{
		class CSignerInfo;
	}

	namespace pkcs8
	{
		class CPrivateKeyInfo;
	}

	namespace pkcs10
	{
		class CCertificateRequest;
	}

	namespace pkcs12
	{
		class CSafeBag;
	}

	namespace cms
	{
		class CSignerInfo;
	}

	namespace asn1
	{
		class ObjectID;
		class AttributeValue;

		//! This class implements the ASN.1 type Attribute. 
		/*!
			An Attribute object consists of an attribute type (specified by an object identifier) and one or more attribute values: 

			The ASN.1 syntax of Attribute is:

			Attribute ::= SEQUENCE {<br>
				type    AttributeType,<br>
				values  SET OF AttributeValue -- at least one value is required --<br>
				}<br>

			AttributeType ::=   OBJECT IDENTIFIER<br>
			AttributeValue :=   ANY DEFINED BY type<br>
		*/
		class Attribute
		{
			friend class x509::attr::CAttributeCertificate;
			friend class pkcs7::CSignerInfo;
			friend class cms::CSignerInfo;
			friend class pkcs8::CPrivateKeyInfo;
			friend class pkcs10::CCertificateRequest;
			friend class pkcs12::CSafeBag;

		public:
			// Default constructor. Initializes m_pAttribute to NULL.
			Attribute(void);

			//! Creates an Attribute from attribute type (ObjectID) and attribute values.
			/*!
				\param const ObjectID &type: the type of the attribute as ObjectID
				\param const AttributeValue &value: the value of the attribute as array of ASN1Objects
			*/
			Attribute(const ObjectID &type, const AttributeValue &value);

			virtual ~Attribute(void);

			//! Copy constructor. 
			/*!
				\param const Attribute &rhs
			*/
			Attribute(const Attribute &rhs);

			//! Copy assignment operator.
			/*!
				\param const Attribute &rhs
				\return Attribute &
			*/
			Attribute &operator=(const Attribute &rhs);

			//! Returns the type of this Attribute. 
			/*!
				\return ObjectID: the type of this Attribute, as ObjectID
			*/
			ObjectID GetType() const;

			//!  Returns the value of this Attribute.
			/*!
				\return std::vector< clone_ptr<AttributeValue> >: the AttributeValue of this (single valued) attribute
			*/
			std::vector< clone_ptr<AttributeValue> > GetValue() const;

		private:
			X509_ATTRIBUTE *m_pAttribute; // Pointer to underlying OpenSSL struct.
		};
	}
}

#endif //!PKIBOX_ASN1_ATTRIBUTE_H

