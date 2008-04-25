/***************************************************************************
 *   Copyright (C) 2008 by Hashim Saleem                                   *
 *   hashim.saleem@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "ObjectID.h"
#include "../openssl/Globals.h"

// -------------- OpenSSL Includes -----------------------
#include <openssl/err.h>
#include <openssl/x509.h>

#include "../Exception.h"
#include "../utils/ByteArray.h"

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
		ObjectID::ObjectID(void): m_pObjectID(NULL)
		{
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ObjectID::ObjectID(const string &ObjectID) : m_pObjectID(NULL)
		{
			m_pObjectID = ::OBJ_txt2obj(ObjectID.c_str(), 1);
			if(!m_pObjectID)
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
		ObjectID::~ObjectID(void)
		{
			if(m_pObjectID)
				::ASN1_OBJECT_free(m_pObjectID);
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ObjectID::ObjectID(const ObjectID &rhs)
		{
			m_pObjectID = ASN1_OBJECT_dup(rhs.m_pObjectID);
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ObjectID &ObjectID::operator=(const ObjectID &rhs)
		{
			// Check for self assignment
			if (this == &rhs) 
				return *this;

			// delete already allocated memory
			if(m_pObjectID)
			{
				::ASN1_OBJECT_free(m_pObjectID);
			}

			// Assign new values
			m_pObjectID = ASN1_OBJECT_dup(rhs.m_pObjectID);

			return *this;
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		string ObjectID::GetID() const
		{
			if(m_pObjectID)
			{
				char buffer[128] = {0};
				int iRet = ::OBJ_obj2txt(buffer, 128, m_pObjectID, 1);
				if(!iRet)
				{
					const char *pc = ::ERR_reason_error_string(::ERR_get_error());
					throw Exception(pc);
				}

				return buffer;
			}
			else
				throw Exception("There isnt any ObjectID to get ID from.");
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		string ObjectID::GetName() const
		{
			if(!m_pObjectID)
				throw Exception("There is no ObjectID to get Name from.");

			char buffer[128] = {0};
			int iRet = ::i2t_ASN1_OBJECT(buffer, 128, m_pObjectID);
			if(!iRet)
			{
				const char *pc = ::ERR_reason_error_string(::ERR_get_error());
				throw Exception(pc);
			}

			return buffer;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		void ObjectID::SetID(const string &lpzOID)
		{
			if(m_pObjectID)
				::ASN1_OBJECT_free(m_pObjectID);

			m_pObjectID = OBJ_txt2obj(lpzOID.c_str(),1);

			if(!m_pObjectID)
			{
				const char *pc = ::ERR_reason_error_string(::ERR_get_error());
				throw Exception(pc);
			}

		}

		//---------------------------------------------------------------------------------------
		// Function name	: SetValue
		// Description	    : Sets the value of this ObjectID
		// Return type		: void
		// Argument         : const CAscByteArray &baValue
		//---------------------------------------------------------------------------------------
		void ObjectID::SetValue(const utils::ByteArray &baValue)
		{
			if(!m_pObjectID)
			{
				m_pObjectID = ::ASN1_OBJECT_new();
				if(!m_pObjectID)
				{
					const char *pc = ::ERR_reason_error_string(::ERR_get_error());
					throw Exception(pc);
				}
			}

			m_pObjectID->data = (unsigned char *)malloc( baValue.GetLength() );

			strcpy( (char *)m_pObjectID->data, (const char *)baValue.GetData() );
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool ObjectID::operator==(const ObjectID &rhs) const
		{
			if(m_pObjectID)
			{
				if(rhs.m_pObjectID)
				{
					return ::OBJ_cmp(m_pObjectID, rhs.m_pObjectID) == 0;
				}
				else
					throw Exception("There isnt any ObjecID to compare with.");
			}
			else
				throw Exception("There isnt any ObjecID to compare from.");
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool ObjectID::operator!=(const ObjectID &rhs) const
		{
			return !operator==(rhs);
		}
	}
}

