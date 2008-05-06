
#include <cassert>
#include "DistinguishedName.h"
#include "../openssl/Globals.h"

// -------------- OpenSSL Includes -----------------------
#include <openssl/err.h>
#include <openssl/x509.h>

#include "../Exception.h"
#include "../NullPointerException.h"
#include "../InvalidArgumentException.h"
#include "../utils/ByteArray.h"
#include "ObjectID.h"
#include "RelativeDistinguishedName.h"

using namespace std;

char * X509_NAME_oneline_ex(X509_NAME *a, unsigned long flag) 
{ 
	BIO *out=NULL; 
	char *buf = NULL;

	out=BIO_new(BIO_s_mem()); 
	if(X509_NAME_print_ex(out,a,0,flag)>0) 
	{ 
		int size = BIO_number_written(out) + 1;
		buf = (char *) malloc( size * sizeof(char) ); 
		memset(buf,0,size); 
		BIO_read(out,buf,BIO_number_written(out)); 
	} 
	BIO_free(out); 
	return (buf); 
} 


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
		DistinguishedName::DistinguishedName(void) : m_pX509Name(NULL)
		{

		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		DistinguishedName::~DistinguishedName(void)
		{
			if(m_pX509Name)
			{
				::X509_NAME_free(m_pX509Name);
			}
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		DistinguishedName::DistinguishedName(const utils::ByteArray &baDN)
		{
			if(baDN.IsEmpty())
			{
				throw InvalidArgumentException("The provided byte array is empty.");
			}

			unsigned char *puc = const_cast<unsigned char *>( baDN.GetData() );
			unsigned int uiSize = baDN.GetLength();
			Construct(puc, uiSize);
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		void DistinguishedName::Construct(const unsigned char *pbArray, unsigned int cLength) /* throw (Exception) */
		{
			assert(pbArray != NULL);
			assert(cLength > 0);

			m_pX509Name = ::d2i_X509_NAME(NULL, &pbArray, cLength);
			if(!m_pX509Name)
			{
				const char *pc = ::ERR_reason_error_string(::ERR_get_error());
				throw Exception(pc);
			}
			pbArray -= cLength;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		DistinguishedName::DistinguishedName(const DistinguishedName &rhs)
		{
			m_pX509Name = X509_NAME_dup(rhs.m_pX509Name);
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		DistinguishedName &DistinguishedName::operator=(const DistinguishedName &rhs)
		{
			// Check for self assignment
			if (this == &rhs) 
				return *this;

			// delete already allocated memory
			if(m_pX509Name)
			{
				::X509_NAME_free(m_pX509Name);
			}

			// Assign new values
			m_pX509Name = X509_NAME_dup(rhs.m_pX509Name);

			return *this;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		string DistinguishedName::GetRDN(const ObjectID &ObjectID) const
		{
			if(!m_pX509Name)
				throw NullPointerException("There is no DistinguishedName to get RDN from.");

			int index = ::X509_NAME_get_index_by_OBJ(m_pX509Name, ObjectID.m_pObjectID, -1);
			X509_NAME_ENTRY *pEntry = ::X509_NAME_get_entry(m_pX509Name, index);
			if(pEntry)
			{
				ASN1_STRING *pString = ::X509_NAME_ENTRY_get_data(pEntry);
				if(pString)
				{
					unsigned char *pUTF8 = NULL;
					int iRet = ::ASN1_STRING_to_UTF8(&pUTF8, pString);
					string s( (char *) pUTF8);
					free(pUTF8);
					return s;
				}
			}

			return "";
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		int DistinguishedName::GetNumberOfRDNs() const
		{
			if(!m_pX509Name)
				throw NullPointerException("There is no DistinguishedName to get Number of RDNs from.");

			return ::X509_NAME_entry_count(m_pX509Name);
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		std::vector<RelativeDistinguishedName> DistinguishedName::GetRDNs() const
		{
			if(!m_pX509Name)
				throw NullPointerException("There is no DistinguishedName to get RDNs from.");

			vector<RelativeDistinguishedName> vRDNs;
			if(m_pX509Name->entries)
			{
				int cRDNs = sk_num(m_pX509Name->entries);

				for(int i=0; i < cRDNs; ++i)
				{
					RelativeDistinguishedName RDN;
					X509_NAME_ENTRY *pRDN = (X509_NAME_ENTRY *)sk_value(m_pX509Name->entries, i);
					RDN.m_pNameEntry = X509_NAME_ENTRY_dup(pRDN);
					vRDNs.push_back(RDN);
				}

				return vRDNs;
			}
			else
				return vRDNs;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		string DistinguishedName::ToString() const
		{
			if(!m_pX509Name)
				throw NullPointerException("There is no DistinguishedName to get in string form.");

			char *pszDN = ::X509_NAME_oneline_ex(m_pX509Name, XN_FLAG_RFC2253);
			if(!pszDN)
				return "";

			string szDN(pszDN);
			free(pszDN);
			return szDN;
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		utils::ByteArray DistinguishedName::GetEncoded() const
		{
			if(!m_pX509Name)
				throw NullPointerException("There is no distinguished name to get in encoded from.");

			utils::ByteArray ba;
			int iSize = ::i2d_X509_NAME(m_pX509Name, NULL);
			if(iSize == -1)
			{
				const char *pc = ::ERR_reason_error_string(::ERR_get_error());
				throw Exception(pc);
			}

			unsigned char *pEncoded = (unsigned char *) ::malloc(iSize); // Allocate
			iSize = ::i2d_X509_NAME(m_pX509Name, &pEncoded);
			pEncoded -= iSize;

			ba.Set(pEncoded, iSize);

			::free(pEncoded);  // Deallocate

			return ba;
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool DistinguishedName::operator==(const DistinguishedName &rhs) const
		{
			if(m_pX509Name)
			{
				if(rhs.m_pX509Name)
				{
					return ::X509_NAME_cmp(m_pX509Name, rhs.m_pX509Name) == 0;
				}
				else
					throw NullPointerException("There is no DistinguishedName to compare with.");
			}
			else
				throw NullPointerException("There is no DistinguishedName to compare.");

		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool DistinguishedName::operator<=(const DistinguishedName &rhs) const
		{
			if(m_pX509Name)
			{
				if(rhs.m_pX509Name)
				{
					return ::X509_NAME_cmp(m_pX509Name, rhs.m_pX509Name) <= 0;
				}
				else
					throw NullPointerException("There is no DistinguishedName to compare with.");
			}
			else
				throw NullPointerException("There is no DistinguishedName to compare.");
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool DistinguishedName::operator>=(const DistinguishedName &rhs) const 
		{
			if(m_pX509Name)
			{
				if(rhs.m_pX509Name)
				{
					return ::X509_NAME_cmp(m_pX509Name, rhs.m_pX509Name) >= 0;
				}
				else
					throw NullPointerException("There is no DistinguishedName to compare with.");
			}
			else
				throw NullPointerException("There is no DistinguishedName to compare.");
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool DistinguishedName::operator<(const DistinguishedName &rhs) const 
		{
			if(m_pX509Name)
			{
				if(rhs.m_pX509Name)
				{
					return ::X509_NAME_cmp(m_pX509Name, rhs.m_pX509Name) < 0;
				}
				else
					throw NullPointerException("There is no DistinguishedName to compare with.");
			}
			else
				throw NullPointerException("There is no DistinguishedName to compare.");
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool DistinguishedName::operator>(const DistinguishedName &rhs) const
		{
			if(m_pX509Name)
			{
				if(rhs.m_pX509Name)
				{
					return ::X509_NAME_cmp(m_pX509Name, rhs.m_pX509Name) > 0;
				}
				else
					throw NullPointerException("There is no DistinguishedName to compare with.");
			}
			else
				throw NullPointerException("There is no DistinguishedName to compare.");
		}



		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool DistinguishedName::operator!=(const DistinguishedName &rhs) const
		{
			return !operator==(rhs);
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		void DistinguishedName::AddRDN(const ObjectID &ObjectID, const utils::ByteArray &baValue)
		{
			if(!m_pX509Name)
			{
				m_pX509Name = ::X509_NAME_new();
			}

			int iRet = ::X509_NAME_add_entry_by_OBJ(m_pX509Name, ObjectID.m_pObjectID, V_ASN1_APP_CHOOSE,
				const_cast<unsigned char *>(baValue.GetData()), baValue.GetLength(), 0, -1);
			if(iRet != 1)
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
		void DistinguishedName::AddRDN(const RelativeDistinguishedName &RDN)
		{
			if(!RDN.m_pNameEntry)
				throw InvalidArgumentException("Invalid RDN supplied.");

			if(!m_pX509Name)
			{
				m_pX509Name = X509_NAME_new();
				if(!m_pX509Name)
				{
					const char *pc = ::ERR_reason_error_string(::ERR_get_error());
					throw Exception(pc);
				}
			}

			int iRet = ::X509_NAME_add_entry(m_pX509Name, RDN.m_pNameEntry, 0, -1);
			if(iRet != 1)
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
		void DistinguishedName::RemoveRDN(const RelativeDistinguishedName &RDN)
		{
			if(!m_pX509Name)
				return;

			int index = ::X509_NAME_get_index_by_OBJ(m_pX509Name, RDN.m_pNameEntry->object, -1);
			if(index == -1) // -1 is returned if not found
			{
				return;
			}

			X509_NAME_ENTRY *pNameEntry = ::X509_NAME_delete_entry(m_pX509Name, index);

		}
	}
}

