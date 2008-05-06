
#ifndef PKIBOX_ASN1_ATTRIBUTE_VALUE_H
#define PKIBOX_ASN1_ATTRIBUTE_VALUE_H

typedef struct asn1_type_st ASN1_TYPE;

namespace PKIBox
{
	namespace utils
	{
		class ByteArray;
	}

	namespace pkcs12
	{
		class CKeyBag;
		class CSecretBag;
	}

	namespace asn1
	{
		//! This class is the basic implementation for Attribute Values. Any class which implements some specific Attribute Value must be derived from this class. 
		/*!
			An Attribute consists of an attribute type (specified by an object identifier) and one or 
			more attribute values: 

			Attribute ::= SEQUENCE {
				type    AttributeType,
				values  SET OF AttributeValue -- at least one value is required --}

			AttributeType           ::=   OBJECT IDENTIFIER
			AttributeValue          ::=   ANY DEFINED BY type
		*/
		class AttributeValue
		{
			friend class pkcs12::CKeyBag;
			friend class pkcs12::CSecretBag;
			friend class Attribute;

		public:

			enum ASC_ASN1_TYPE
			{
				ASC_ASN1_BOOLEAN,
				ASC_ASN1_STRING,
				ASC_ASN1_OBJECT,
				ASC_ASN1_INTEGER,
				ASC_ASN1_ENUMERATED,
				ASC_ASN1_BIT_STRING,
				ASC_ASN1_OCTET_STRING,
				ASC_ASN1_PRINTABLESTRING,
				ASC_ASN1_T61STRING,
				ASC_ASN1_IA5STRING,
				ASC_ASN1_GENERALSTRING,
				ASC_ASN1_BMPSTRING,
				ASC_ASN1_UNIVERSALSTRING,
				ASC_ASN1_UTCTIME,
				ASC_ASN1_GENERALIZEDTIME,
				ASC_ASN1_VISIBLESTRING,
				ASC_ASN1_UTF8STRING,
			};

			//! Default constructor.
			AttributeValue(void);

			//! Creates AttributeValue of a certain ASN.1 type.
			/*!
				\param ASC_ASN1_TYPE type: type of an attribute value
				\param void *pValue: value of an attribute
			*/
			AttributeValue(ASC_ASN1_TYPE type, void *pValue);

			virtual ~AttributeValue(void);

			//! Copy constructor.
			/*!
				\param const AttributeValue &rhs
			*/
			AttributeValue(const AttributeValue &rhs);

			//! Copy assignment operator.
			/*!
				\param const AttributeValue &rhs
				\return AttributeValue &
			*/
			AttributeValue &operator=(const AttributeValue &rhs);

			//! Returns bytes of this AttributeValue.
			/*!
				\return utils::ByteArray: binary representation of attribute value
			*/
			utils::ByteArray GetBytes() const;

		protected:
			ASN1_TYPE	*m_pAttValue;
		};
	}
}

#endif // !PKIBOX_ASN1_ATTRIBUTE_VALUE_H

