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

#include "ByteArray.h"

namespace PKIBox
{
	namespace utils
	{
		using namespace std;

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ByteArray::ByteArray()
		{
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ByteArray::ByteArray(unsigned int cLength) 
		{
			m_Data.resize(cLength);
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ByteArray::ByteArray(const string &Data) : m_Data(Data)
		{

		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ByteArray::ByteArray(unsigned char *pData, unsigned int cLength) 
			: 	m_Data(reinterpret_cast<const char *>(pData), cLength)
		{

		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		void ByteArray::Set(unsigned char *pData, unsigned int cLength)
		{
			m_Data.assign(reinterpret_cast<char *>(pData), cLength);
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ByteArray &ByteArray::operator=(const string &rhs)
		{
			m_Data = rhs; // Have to test whether this assignment works with null containing strings or not.

			return *this;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		/*virtual*/ ByteArray::~ByteArray()
		{

		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		const unsigned char *ByteArray::GetData() const
		{
			return (unsigned char *)(m_Data.data()); 
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		unsigned int ByteArray::GetLength() const
		{ 
			return m_Data.length(); 
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		string ByteArray::ToString()
		{
			unsigned int uiLengthThis = GetLength();
			unsigned int uiLength = uiLengthThis*3;
			char *pc = new char[uiLength];
			memset(pc, 0, uiLength);

			char *pctmp = pc;
			for(unsigned int i=0; i<uiLengthThis; ++i)
			{
				sprintf(pctmp, "%02X", *(GetData()+i));
				pctmp += 2;
				*pctmp = ' ';
				pctmp++;
			}

			string s(pc, uiLength);
			delete []pc;

			return s;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		std::string ByteArray::ToHexadecimal()
		{
			unsigned int uiLengthThis = GetLength();
			unsigned int uiLength = uiLengthThis*2;
			char *pc = new char[uiLength+1]; // sprintf() writes a null character after last conversion.
			memset(pc, 0, uiLength);

			for( unsigned int i=0; i<uiLengthThis; ++i)
			{
				sprintf( (char *) pc + (i*2), "%02x", *(GetData()+i));
			}
			
			string s(pc, uiLength);
			delete []pc;

			return s;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool ByteArray::operator==(const ByteArray &rhs) const
		{
			return m_Data == rhs.m_Data;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool ByteArray::operator!=(const ByteArray &rhs) const
		{
			return !operator==(rhs);
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		const ByteArray ByteArray::operator+(const string &rhs) const
		{
			ByteArray tmpba(*this);
			tmpba.m_Data += rhs;
			return tmpba;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		const ByteArray ByteArray::operator+(const ByteArray &rhs) const
		{
			ByteArray tmpba(*this);
			tmpba.m_Data += rhs.m_Data;
			return tmpba;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ByteArray &ByteArray::operator+= (const string &rhs)
		{
			m_Data += rhs;
			return *this;
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ByteArray &ByteArray::operator+=(const ByteArray &rhs)
		{
			m_Data += rhs.m_Data;
			return *this;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ByteArray &ByteArray::Insert(unsigned int uiIndex, const string &Data)
		{
			m_Data.insert(uiIndex, Data);
			return *this;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool ByteArray::Erase(const ByteArray &Bytes)
		{
			string::size_type Index = m_Data.find(Bytes.m_Data);
			if ( Index != string::npos )
			{
				m_Data.erase(Index, Bytes.m_Data.size());
				return true;
			}
			else
				return false;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool ByteArray::Erase(unsigned int uiIndex, unsigned int uiLength)
		{
			m_Data.erase(uiIndex, uiLength);
			return true;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ByteArray ByteArray::SubBytes(unsigned int uiIndex, unsigned int uiLength) const
		{
			return ByteArray(m_Data.substr(uiIndex, uiLength));
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		int ByteArray::Find(const ByteArray& Bytes, unsigned int Offset) const
		{
			return m_Data.find(Bytes.m_Data, Offset);
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		int ByteArray::Find(const ByteArray& Bytes) const
		{
			return m_Data.find(Bytes.m_Data);
		}

		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		bool ByteArray::operator<(const ByteArray &rhs) const
		{
			return m_Data < rhs.m_Data;
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         :  

		//---------------------------------------------------------------------------------------
		bool ByteArray::IsEmpty() const
		{
			return m_Data.empty();
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		void ByteArray::Clear()
		{
			m_Data.clear();
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		const ByteArray::Byte ByteArray::operator[](unsigned int Index) const
		{
			return m_Data[Index];
		}


		//---------------------------------------------------------------------------------------
		// Function name	: 
		// Description	    : 
		// Return type		: 
		// Argument         : 
		//---------------------------------------------------------------------------------------
		ByteArray::Byte ByteArray::operator[](unsigned int Index)
		{
			return static_cast<Byte>(m_Data[Index]);
		}
	}
}



