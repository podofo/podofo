
#include "GeneralName.h"

// -------------- OpenSSL Includes -----------------------
#include <openssl/x509v3.h>

#include "../Exception.h"
#include "../NullPointerException.h"
#include "DistinguishedName.h"
#include "../openssl/Globals.h"
using namespace std;

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
		GeneralName::GeneralName(void) : m_pName(NULL)
		{

		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		GeneralName::~GeneralName(void)
		{
			if(m_pName)
			{
				::GENERAL_NAME_free(m_pName);
			}
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		GeneralName::GeneralName(const GeneralName &rhs)
		{
			m_pName = GENERAL_NAME_dup(rhs.m_pName);

		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		GeneralName &GeneralName::operator=(const GeneralName &rhs)
		{
			// Check for self assignment
			if (this == &rhs) 
				return *this;

			// delete already allocated memory
			if(m_pName)
			{
				::GENERAL_NAME_free(m_pName);
			}

			// Assign new values
			m_pName = GENERAL_NAME_dup(rhs.m_pName);

			return *this;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		GeneralName::NameType GeneralName::GetType() const
		{
			if(!m_pName)
				throw NullPointerException("There is no GeneralName to get type from.");

			switch(m_pName->type)
			{
			case GEN_OTHERNAME:
				return otherName;

			case GEN_EMAIL:
				return rfc822Name;

			case GEN_DNS:
				return dNSName;

			case GEN_X400:
				return x400Address;

			case GEN_DIRNAME:
				return directoryName;

			case GEN_EDIPARTY:
				return ediPartyName;

			case GEN_URI:
				return uniformResourceIdentifier;

			case GEN_IPADD:
				return iPAddress;

			case GEN_RID:
				return registeredID;

			default:
				return unInitialized;
			}

		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		string GeneralName::GetRFC822Name() const
		{
			if(!m_pName)
				throw NullPointerException("There is no GeneralName to get RFC822Name from.");

			if(m_pName->type != GEN_EMAIL)
				throw Exception("This GeneralName does not contain a RFC822Name. Try checking type of this GeneralName.");

			return string( (char *) m_pName->d.rfc822Name->data, m_pName->d.rfc822Name->length );

		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		string GeneralName::GetDNSName() const
		{
			if(!m_pName)
				throw NullPointerException("There is no GeneralName to get DNSName from.");

			if(m_pName->type != GEN_DNS)
				throw Exception("This GeneralName does not contain a DNSName. Try checking type of this GeneralName.");

			return string( reinterpret_cast<char *>( ASN1_STRING_data(m_pName->d.dNSName) ),
				ASN1_STRING_length(m_pName->d.dNSName) );
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		void GeneralName::SetDNSName(const std::string &DNS)
		{
			if(!m_pName)
			{
				m_pName = ::GENERAL_NAME_new();
			}

			m_pName->type = GEN_DNS;
			m_pName->d.dNSName = M_ASN1_IA5STRING_new();
			int iRet = ASN1_STRING_set(m_pName->d.uniformResourceIdentifier, DNS.c_str(), DNS.size());
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		DistinguishedName GeneralName::GetDirectoryName() const
		{
			if(!m_pName)
				throw NullPointerException("There is no GeneralName to get DirectoryName from.");

			if(m_pName->type != GEN_DIRNAME)
				throw Exception("This GeneralName does not contain a DirectoryName. Try checking type of this GeneralName.");

			DistinguishedName DN;
			DN.m_pX509Name = X509_NAME_dup(m_pName->d.directoryName);
			return DN;


		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		void GeneralName::SetDirectoryName(const DistinguishedName &dirName)
		{
			if(!m_pName)
			{
				m_pName = ::GENERAL_NAME_new();
			}

			m_pName->type = GEN_DIRNAME;
			m_pName->d.directoryName =  X509_NAME_dup(dirName.m_pX509Name);
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		string GeneralName::GetUniformResourceIdentifier() const
		{
			if(!m_pName)
				throw NullPointerException("There is no GeneralName to get URI from.");

			if(m_pName->type != GEN_URI)
				throw Exception("This GeneralName does not contain a URI. Try checking type of this GeneralName.");

			return string( reinterpret_cast<char *>( ASN1_STRING_data(m_pName->d.uniformResourceIdentifier) ),
				ASN1_STRING_length(m_pName->d.uniformResourceIdentifier) );

		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		void GeneralName::SetUniformResourceIdentifier(const string &sURL)
		{
			if(!m_pName)
			{
				m_pName = ::GENERAL_NAME_new();
			}

			m_pName->type = GEN_URI;
			m_pName->d.uniformResourceIdentifier = M_ASN1_IA5STRING_new();
			int iRet = ASN1_STRING_set(m_pName->d.uniformResourceIdentifier, sURL.c_str(), sURL.size());
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool GeneralName::operator==(const GeneralName &rhs) const
		{
			if(m_pName)
			{
				if(rhs.m_pName)
				{
					return ::GENERAL_NAME_cmp(m_pName, rhs.m_pName) == 0;
				}
				else
					throw NullPointerException("There is no GeneralName to compare with.");
			}
			else
				throw NullPointerException("There is no GeneralName to compare from.");
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool GeneralName::operator!=(const GeneralName &rhs) const
		{
			return !operator==(rhs);
		}
	}
}

