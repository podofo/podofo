
#include "AlgorithmID.h"
#include "../Exception.h"
#include "../NullPointerException.h"
#include "../utils/ByteArray.h"
#include "../openssl/Globals.h"
#include "ObjectID.h"

// -------------- OpenSSL Includes -----------------------
#include <openssl/err.h>

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
		AlgorithmID::AlgorithmID(void) : m_pAlgID(NULL)
		{

		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		AlgorithmID::AlgorithmID(const ObjectID &algorithm, const utils::ByteArray &parameter) : m_pAlgID(NULL)
		{
			m_pAlgID = X509_ALGOR_new();
			if(!m_pAlgID)
			{
				const char *pc = ::ERR_reason_error_string(::ERR_get_error());
				throw Exception(pc);
			}
			m_pAlgID->algorithm = ASN1_OBJECT_dup(algorithm.m_pObjectID);
			m_pAlgID->parameter = ASN1_TYPE_new();
			m_pAlgID->parameter->type = V_ASN1_NULL;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		AlgorithmID::AlgorithmID(const ObjectID &Algorithm) : m_pAlgID(NULL)
		{
			m_pAlgID = X509_ALGOR_new();
			if(!m_pAlgID)
			{
				const char *pc = ::ERR_reason_error_string(::ERR_get_error());
				throw Exception(pc);
			}

			m_pAlgID->algorithm = ASN1_OBJECT_dup(Algorithm.m_pObjectID);
			m_pAlgID->parameter = ASN1_TYPE_new();
			m_pAlgID->parameter->type = V_ASN1_NULL;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		AlgorithmID::~AlgorithmID(void)
		{
			if(m_pAlgID)
			{
				::X509_ALGOR_free(m_pAlgID);
			}
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		AlgorithmID::AlgorithmID(const AlgorithmID &rhs)
		{
			m_pAlgID = X509_ALGOR_dup(rhs.m_pAlgID);
		}



		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		AlgorithmID &AlgorithmID::operator=(const AlgorithmID &rhs)
		{
			// Check for self assignment
			if (this == &rhs) 
				return *this;

			// delete already allocated memory
			if(m_pAlgID)
			{
				::X509_ALGOR_free(m_pAlgID);
			}

			// Assign new values
			m_pAlgID = X509_ALGOR_dup(rhs.m_pAlgID);

			return *this;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ObjectID AlgorithmID::GetAlgorithm() const
		{
			if(!m_pAlgID)
				throw NullPointerException("There is no AlgorithmIdentifier to get Algorithm from.");

			ObjectID Algorithm;
			Algorithm.m_pObjectID = ASN1_OBJECT_dup(m_pAlgID->algorithm); 
			return Algorithm;

		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		utils::ByteArray AlgorithmID::GetParameters() const
		{
			if(!m_pAlgID)
				throw NullPointerException("There is no AlgorithmIdentifier to get Parameters from.");

			return utils::ByteArray();
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		void AlgorithmID::SetAlgorithm(const ObjectID &obj)
		{
			if(!m_pAlgID)
			{
				m_pAlgID = X509_ALGOR_new();
				m_pAlgID->parameter = ASN1_TYPE_new();
				m_pAlgID->parameter->type = V_ASN1_NULL;
			}
			m_pAlgID->algorithm = ASN1_OBJECT_dup( obj.m_pObjectID);
		}
	}
}

