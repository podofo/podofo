
#include "RelativeDistinguishedName.h"
#include "../openssl/Globals.h"

// -------------- OpenSSL Includes -----------------------
#include <openssl/err.h>
#include <openssl/x509.h>

#include "../Exception.h"
#include "../NullPointerException.h"
#include "../utils/ByteArray.h"
#include "ObjectID.h"

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
		RelativeDistinguishedName::RelativeDistinguishedName(void) : m_pNameEntry(NULL)
		{

		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		RelativeDistinguishedName::RelativeDistinguishedName(const ObjectID &objectID,
			const utils::ByteArray &baValue) : m_pNameEntry(NULL)
		{
			int NID = ::OIDtoNID(objectID);
			m_pNameEntry = ::X509_NAME_ENTRY_create_by_NID(NULL, NID, V_ASN1_APP_CHOOSE , 
				const_cast<unsigned char *>(baValue.GetData()), baValue.GetLength());
			if(!m_pNameEntry)
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
		RelativeDistinguishedName::~RelativeDistinguishedName(void)
		{
			if(m_pNameEntry)
				X509_NAME_ENTRY_free(m_pNameEntry);
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		RelativeDistinguishedName::RelativeDistinguishedName(const RelativeDistinguishedName &rhs) : m_pNameEntry(NULL)
		{
			m_pNameEntry = X509_NAME_ENTRY_dup(rhs.m_pNameEntry);
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		RelativeDistinguishedName &RelativeDistinguishedName::operator=(const RelativeDistinguishedName &rhs)
		{
			// Check for self assignment
			if (this == &rhs) 
				return *this;

			// delete already allocated memory
			if(m_pNameEntry)
			{
				::X509_NAME_ENTRY_free(m_pNameEntry);
			}

			// Assign new values
			m_pNameEntry = X509_NAME_ENTRY_dup(rhs.m_pNameEntry);

			return *this;
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ObjectID RelativeDistinguishedName::GetType() const
		{
			if(!m_pNameEntry)
				throw NullPointerException("There is no RelativeDistinguishedName to get type from.");

			ObjectID type;
			type.m_pObjectID = ASN1_OBJECT_dup(m_pNameEntry->object);
			return type;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		utils::ByteArray RelativeDistinguishedName::GetValue() const
		{
			if(!m_pNameEntry)
				throw NullPointerException("There is no RelativeDistinguishedName to get value from.");

			return utils::ByteArray( reinterpret_cast<unsigned char *>(m_pNameEntry->value->data), m_pNameEntry->value->length);
		}
	}
}

