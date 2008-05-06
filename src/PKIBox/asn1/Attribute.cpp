
#include "Attribute.h"

// -------------- OpenSSL Includes -----------------------
#include <openssl/x509.h>
#include <openssl/err.h>
#include "../openssl/Globals.h"
#include "../Exception.h"
#include "../NullPointerException.h"
#include "../asn1/ObjectID.h"
#include "../asn1/OIDs.h"
#include "AttributeValue.h"
//#include "../pkcs7/ContentType.h"
//#include "../cms/ess/SigningCertificate.h"
//#include "../cms/attributes/SigningTime.h"

using namespace std;
//using namespace PKIBox::pkcs7;
//using namespace PKIBox::cms::ess;
//using namespace PKIBox::cms::attributes;


namespace PKIBox
{
	namespace asn1
	{
		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		Attribute::Attribute(void) : m_pAttribute(NULL)
		{

		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		// Code Added By	: GA
		//---------------------------------------------------------------------------------------
		Attribute::Attribute(const ObjectID &type, const AttributeValue &value) : m_pAttribute(NULL)
		{
			void *pValue = NULL;
			switch(value.m_pAttValue->type)
			{
			case V_ASN1_BOOLEAN: pValue = &value.m_pAttValue->value.boolean; break;
			case V_ASN1_OBJECT: pValue = value.m_pAttValue->value.object; break;
			case V_ASN1_INTEGER: pValue = value.m_pAttValue->value.integer; break;
			case V_ASN1_ENUMERATED: pValue = value.m_pAttValue->value.enumerated; break;
			case V_ASN1_BIT_STRING: pValue = value.m_pAttValue->value.bit_string; break;
			case V_ASN1_OCTET_STRING: pValue = value.m_pAttValue->value.octet_string; break;
			case V_ASN1_PRINTABLESTRING: pValue = value.m_pAttValue->value.printablestring; break;
			case V_ASN1_T61STRING: pValue = value.m_pAttValue->value.t61string; break;
			case V_ASN1_IA5STRING: pValue = value.m_pAttValue->value.ia5string; break;
			case V_ASN1_GENERALSTRING: pValue = value.m_pAttValue->value.generalstring; break;
			case V_ASN1_BMPSTRING: pValue = value.m_pAttValue->value.bmpstring; break;
			case V_ASN1_UNIVERSALSTRING: pValue = value.m_pAttValue->value.universalstring; break;
			case V_ASN1_UTCTIME: pValue = value.m_pAttValue->value.utctime; break;
			case V_ASN1_GENERALIZEDTIME: pValue = value.m_pAttValue->value.generalizedtime; break;
			case V_ASN1_VISIBLESTRING: pValue = value.m_pAttValue->value.visiblestring; break;
			case V_ASN1_UTF8STRING: pValue = value.m_pAttValue->value.utf8string; break;
			default: pValue = value.m_pAttValue->value.octet_string; break;

			}

			m_pAttribute = X509_ATTRIBUTE_create( OIDtoNID(type), value.m_pAttValue->type, pValue);
			if(!m_pAttribute)
			{
				const char *pc = ::ERR_reason_error_string(::ERR_get_error());
				throw Exception(pc);			
			}
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		Attribute::~Attribute(void)
		{
			if(m_pAttribute)
				X509_ATTRIBUTE_free(m_pAttribute);
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		Attribute::Attribute(const Attribute &rhs) : m_pAttribute(NULL)
		{
			m_pAttribute = X509_ATTRIBUTE_dup(rhs.m_pAttribute);
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		Attribute &Attribute::operator=(const Attribute &rhs)
		{
			// Check for self assignment
			if (this == &rhs) 
				return *this;

			// delete already allocated memory
			if(m_pAttribute)
				X509_ATTRIBUTE_free(m_pAttribute);

			// Assign new values
			m_pAttribute = X509_ATTRIBUTE_dup(rhs.m_pAttribute);

			return *this;
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ObjectID Attribute::GetType() const
		{
			if(!m_pAttribute)
				throw NullPointerException("There is no Attribute to get Type from.");

			ObjectID ObjectID;
			ObjectID.m_pObjectID = ASN1_OBJECT_dup(m_pAttribute->object);
			return ObjectID;


		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		vector< clone_ptr<AttributeValue> > Attribute::GetValue() const
		{
			if(!m_pAttribute)
				throw NullPointerException("There is no Attribute to get Value from.");

			vector< clone_ptr<AttributeValue> > vValues;
			//if( GetType() == OIDs::id_signingTime )
			//{
			//	if(!m_pAttribute->single) // There is not a single value but a "Set"
			//	{
			//		int cValues = sk_ASN1_TYPE_num(m_pAttribute->value.set);

			//		for(int i=0; i < cValues; ++i)
			//		{
			//			ASN1_TYPE *pASNType = sk_ASN1_TYPE_value(m_pAttribute->value.set, i);
			//			CSigningTime *pAttValue = new CSigningTime;
			//			pAttValue->m_pAttValue = ASN1_TYPE_dup(pASNType);
			//			vValues.push_back( pAttValue  );
			//		}

			//		return vValues;
			//	}
			//}
			//else if( GetType() == OIDs::id_contentType )
			//{
			//	if(!m_pAttribute->single) // There is not a single value but a "Set"
			//	{
			//		int cValues = sk_ASN1_TYPE_num(m_pAttribute->value.set);

			//		for(int i=0; i < cValues; ++i)
			//		{
			//			ASN1_TYPE *pASNType = sk_ASN1_TYPE_value(m_pAttribute->value.set, i);
			//			CContentType *pAttValue = new CContentType;
			//			pAttValue->m_pAttValue = ASN1_TYPE_dup(pASNType);
			//			vValues.push_back( pAttValue  );
			//		}

			//		return vValues;
			//	}
			//}
			//else if( GetType() == OIDs::id_aa_signingCertificate )
			//{
			//	if(!m_pAttribute->single) // There is not a single value but a "Set"
			//	{
			//		int cValues = sk_ASN1_TYPE_num(m_pAttribute->value.set);

			//		for(int i=0; i < cValues; ++i)
			//		{
			//			ASN1_TYPE *pASNType = sk_ASN1_TYPE_value(m_pAttribute->value.set, i);
			//			CSigningCertificate *pAttValue = new CSigningCertificate;
			//			pAttValue->m_pAttValue = ASN1_TYPE_dup(pASNType);
			//			vValues.push_back( pAttValue  );
			//		}

			//		return vValues;
			//	}
			//}
			//else
			//{
			//	if(!m_pAttribute->single) // There is not a single value but a "Set"
			//	{
			//		int cValues = sk_ASN1_TYPE_num(m_pAttribute->value.set);

			//		for(int i=0; i < cValues; ++i)
			//		{
			//			ASN1_TYPE *pASNType = sk_ASN1_TYPE_value(m_pAttribute->value.set, i);
			//			AttributeValue *pAttValue = new AttributeValue;
			//			pAttValue->m_pAttValue = ASN1_TYPE_dup(pASNType);
			//			vValues.push_back( clone_ptr<AttributeValue>(pAttValue) );
			//		}

			//		return vValues;
			//	}
			//	else if(m_pAttribute->single == 1) // There is a single value
			//	{
			//		AttributeValue *pAttValue = new AttributeValue;
			//		pAttValue->m_pAttValue = ASN1_TYPE_dup(m_pAttribute->value.single);
			//		vValues.push_back( clone_ptr<AttributeValue>(pAttValue) );

			//		return vValues;
			//	}
			//}

			return vValues;

		}
	}
}

