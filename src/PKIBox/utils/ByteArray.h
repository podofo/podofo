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

#ifndef PKIBOX_UTILS_BYTE_ARRAY_H
#define PKIBOX_UTILS_BYTE_ARRAY_H

#include <string>

namespace PKIBox
{
	//! This namespace provides utility classes for byte array, date/time, file and string handling.
	namespace utils
	{
		//! This class represents an array with byte values. 
		/*!
			This class is used for containing the binary encoded representation of different PKI objects.
		*/
		class ByteArray  
		{
		public:

			//! The unit value this ByteArray holds.
			typedef unsigned char Byte;

			//! Default constructor. Initializes underlying string member with default values.
			ByteArray();

			//! Destructor
			virtual ~ByteArray();

			//! Constructs a ByteArray of cLength size.
			/*!
				\param unsigned int cLength
			*/
			explicit ByteArray(unsigned int cLength);

			//! Constructs a ByteArray from the data in string.
			/*!
				\param const std::string &Data
			*/
			explicit ByteArray(const std::string &Data);

			//! Constructs a ByteArray of user provided buffer and its size.
			/*!
				\param unsigned char *pData
				\param unsigned int cLength
			*/
			ByteArray(unsigned char *pData, unsigned int cLength);

			//! Assignment from a string.
			/*!
				\param const std::string &rhs
				\param ByteArray &
			*/
			ByteArray &operator=(const std::string &rhs);

			//! Sets the data of byte array with user provided buffer and its size
			/*!
				\param unsigned char *pData
				\param unsigned int cLength
			*/
			void Set(unsigned char *pData, unsigned int cLength);

			//! Returns the length of this ByteArray.
			/*!
				\return unsigned int 
			*/
			unsigned int GetLength() const;

			//! Returns the underlying buffer of this ByteArray.
			/*!
				\return const unsigned char *
			*/
			const unsigned char *GetData() const;

			//! Checks whether this ByteArray is empty or not.
			/*!
				return bool
			*/
			bool IsEmpty() const;

			//! Clears the contents of this ByteArray. i.e. makes ByteArray empty.
			void Clear();

			//! Provides a reference to the byte with a specified index in this ByteArray.
			/*!
				\param unsigned int Index
				\return const Byte 
			*/
			const Byte operator[](unsigned int Index) const;

			//! Provides a reference to the byte with a specified index in this ByteArray.
			/*!
				\param unsigned int Index
				\return Byte 
			*/
			Byte operator[](unsigned int Index);

			//! Concatenates string in the byte array.
			/*!
				\param const std::string &rhs
				\return const ByteArray 
			*/
			const ByteArray operator+(const std::string &rhs) const;

			//! Concatenates byte array in the byte array.
			/*!
				\param const ByteArray &rhs
				\return const ByteArray 
			*/
			const ByteArray operator+(const ByteArray &rhs) const;

			//! Concatenates string in the byte array.
			/*!
				\param const std::string &rhs
				\return const ByteArray 
			*/
			ByteArray &operator+=(const std::string &rhs);

			//! Concatenates byte array in the byte array.
			/*!
				\param const ByteArray &rhs
				\return const ByteArray 
			*/
			ByteArray &operator+=(const ByteArray &rhs);

			//! Equality operator.
			/*!
				\param const ByteArray &rhs
				\return bool 
			*/
			bool operator==(const ByteArray &rhs) const;

			//! Inequality operator.
			/*!
				\param const ByteArray &rhs
				\return bool 
			*/
			bool operator!=(const ByteArray &rhs) const;

			//! Less than operator.
			/*!
				\param const ByteArray &rhs
				\return bool 
			*/
			bool operator<(const ByteArray &rhs) const;

			//! Inserts a ByteArray at a specified index into this ByteArray.
			/*!
				\param unsigned int uiIndex
				\param const std::string &Data
				\return ByteArray &
			*/
			ByteArray &Insert(unsigned int uiIndex, const std::string &Data);

			//! Finds the specified ByteArray into this ByteArray and erases it if it is found.
			/*!
				\param const ByteArray &Bytes
				\return bool
			*/
			bool Erase(const ByteArray &Bytes);

			//! Erases the number of bytes specified by uiLength starting from the index uiIndex from this ByteArray.
			/*!
				\param unsigned int uiIndex
				\param unsigned int uiLength
				\return bool
			*/
			bool Erase(unsigned int uiIndex, unsigned int uiLength);

			//! Finds the specified ByteArray starting from the index specified into this ByteArray.
			/*!
				\param const ByteArray& Bytes
				\param unsigned int Offset
				\return int
			*/
			int Find(const ByteArray& Bytes, unsigned int Offset) const;  

			//! Finds the specified ByteArray into this ByteArray.
			/*!
				\param const ByteArray& Bytes
				\return int
			*/
			int Find(const ByteArray& Bytes) const;              

			//! Returns a sub ByteArray begining at the index in this ByteArray and of length 'uiLength'.
			/*!
				\param unsigned int uiIndex
				\param unsigned int uiLength
				\return ByteArray 
			*/
			ByteArray SubBytes(unsigned int uiIndex, unsigned int uiLength) const;

			//! Returns underlying binary data into a string in hexadecimal form.
			/*!
				\return std::string 
			*/
			std::string ToString();

			//! Returns hexadecimal form of this ByteArray.
			/*!
				\return std::string 
			*/
			std::string ToHexadecimal();

		private:
			std::string m_Data;		// Underlying buffer for ByteArray.

		};
	}
}

#endif // PKIBOX_UTILS_BYTE_ARRAY_H


