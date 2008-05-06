
#ifndef PKIBOX_ASN1_RELATIVE_DISTINGUISHED_NAME_H
#define PKIBOX_ASN1_RELATIVE_DISTINGUISHED_NAME_H

typedef struct X509_name_entry_st X509_NAME_ENTRY;

namespace PKIBox
{
	namespace utils
	{
		class ByteArray;
	}

	namespace asn1
	{
		// Forward declarations.
		class ObjectID;

		//! This class represents an X.501 RelativeDistinguishedName (RDN) structure. 
		/*! 
			The ASN.1 notation for Name is 

			RelativeRelativeDistinguishedName ::= SET OF AttributeValueAssertion

			AttributeValueAssertion ::= SEQUENCE {
				AttributeType,
				AttributeValue }

			AttributeType ::= OBJECT IDENTIFIER

			ATTRIBUTEVALUE ::= ANY
		*/
		class RelativeDistinguishedName
		{
			friend class DistinguishedName;

		public:
			//! Default constructor. Initializes m_pNameEntry to NULL.
			RelativeDistinguishedName(void);

			//! Constructs a RelativeDistinguishedName from an AVA. 
			/*!
				\param const OIDs &ObjectID: the type (object ID) of the AVA to be added
				\param const utils::ByteArray &baValue: the value of the AVA to be added
			*/
			RelativeDistinguishedName(const ObjectID &objectID, const utils::ByteArray &baValue);

			virtual ~RelativeDistinguishedName(void);

			//! Copy constructor.
			/*!
				\param const RelativeDistinguishedName &rhs
			*/
			RelativeDistinguishedName(const RelativeDistinguishedName &rhs);

			//! Copy assignment operator.
			/*!
				\param const RelativeDistinguishedName &rhs
			*/
			RelativeDistinguishedName &operator=(const RelativeDistinguishedName &rhs);

			//! Returns the type of AttributeValueAssertion contained in this RDN. 
			/*!
				\return OIDs: ObjectID representing the type of RDN
			*/
			ObjectID GetType() const;
				
			//! Returns the value of AttributeValueAssertion contained in this RDN.
			/*!
				\return utils::ByteArray: the value of the RDN
			*/
			utils::ByteArray GetValue() const;
				
		private:
			X509_NAME_ENTRY *m_pNameEntry; // Pointer to underlying OpenSSL struct.
		};
	}
}

#endif //!PKIBOX_ASN1_RELATIVE_DISTINGUISHED_NAME_H

