
#include "AttributeValue.h"

// -------------- OpenSSL Includes -----------------------
#include <openssl/asn1.h>
#include "../openssl/Globals.h"
#include "../Exception.h"
#include "../NullPointerException.h"
#include "../utils/ByteArray.h"

using namespace PKIBox::utils;

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
		AttributeValue::AttributeValue(void) : m_pAttValue(NULL)
		{

		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		AttributeValue::AttributeValue(ASC_ASN1_TYPE type, void *pValue) : m_pAttValue(NULL)
		{
			switch(type)
			{
				// Cases will be added by time. Here we have just provided a default implementation.
			default:
				{
					ByteArray *pbaValue = (ByteArray *)pValue;
					ASN1_OCTET_STRING *pOctetString = ASN1_OCTET_STRING_new();
					int iRet = ASN1_OCTET_STRING_set(pOctetString, (unsigned char *)pbaValue->GetData(), 
						pbaValue->GetLength() );
					m_pAttValue = ASN1_TYPE_new();
					ASN1_TYPE_set(m_pAttValue, V_ASN1_OCTET_STRING, pOctetString);
				}

			}

		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		AttributeValue::~AttributeValue(void)
		{
			if(m_pAttValue)
				ASN1_TYPE_free(m_pAttValue);
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		AttributeValue::AttributeValue(const AttributeValue &rhs)
		{
			m_pAttValue = ASN1_TYPE_dup(rhs.m_pAttValue);
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		AttributeValue &AttributeValue::operator=(const AttributeValue &rhs)
		{
			// Check for self assignment
			if (this == &rhs) 
				return *this;

			// delete already allocated memory
			if(m_pAttValue)
				ASN1_TYPE_free(m_pAttValue);

			// Assign new values
			m_pAttValue = ASN1_TYPE_dup(rhs.m_pAttValue);

			return *this;
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ByteArray AttributeValue::GetBytes() const
		{
			if(!m_pAttValue)
				throw NullPointerException("There is no AttributeValue to get bytes from.");

			return ByteArray(m_pAttValue->value.asn1_string->data, m_pAttValue->value.asn1_string->length);  
		}
	}
}


