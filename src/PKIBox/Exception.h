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

#ifndef PKIBOX_EXCEPTION_H
#define PKIBOX_EXCEPTION_H

#include <string>

namespace PKIBox
{
	//! This is the base class for all exceptions thrown from the PKIBox library.
	class Exception
	{
	public:

		//! Default constructor. Initializes members to default values.
		Exception();

		//! Construct an Exception object from a description string.
		/*!
			\param const std::string &sErrMsg
		*/
		explicit Exception(const std::string &sErrMsg);

		//! Construct an Exception object from components.
		/*!
			\param unsigned long ulErrCode: Error message code
			\param const std::string &sErrMsg: String containing the error message
		*/
		Exception(unsigned long ulErrCode, const std::string &sErrMsg);

		virtual ~Exception();

		//! Returns the error code.
		/*!
			\return unsigned long: Error message code
		*/
		virtual unsigned long  GetErrCode() const {	return m_ulErrCode;	}

		//! Returns the error message.
		/*!
			\return const std::string &: string containing the error message
		*/
		virtual const std::string &GetErrorMessage() const {	return m_sErrMsg;	}

		//! Sets the error code.
		/*!
			\param unsigned long dwErrCode: Error message code
		*/
		virtual void SetErrCode(unsigned long dwErrCode);

		//! Sets the error message.
		/*!
			\param const std::string &sErrMsg: Error message string
		*/
		virtual void SetErrorMessage(const std::string &sErrMsg);

	protected:
		std::string	m_sErrMsg;			// Error description.
		unsigned long	m_ulErrCode;	// Error code.
	};
}

#endif // PKIBOX_EXCEPTION_H

