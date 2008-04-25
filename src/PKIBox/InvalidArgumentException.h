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

#ifndef PKIBOX_INVALID_ARGUMENT_EXCEPTION_H
#define PKIBOX_INVALID_ARGUMENT_EXCEPTION_H

#include "Exception.h"

namespace PKIBox
{
	//! Thrown when an invalid argument is passed to a PKIBox method
	class InvalidArgumentException : public Exception
	{
	public:
		//! Constructs an InvalidArgumentException without an error description.
		InvalidArgumentException(void);

		//! Constructs an InvalidArgumentException from an error description.
		/*!
			\param const std::string &sErrorMsg: string containing the error message
		*/
		explicit InvalidArgumentException(const std::string &sErrorMsg);

		virtual ~InvalidArgumentException(void);
	};
}

#endif // !PKIBOX_INVALID_ARGUMENT_EXCEPTION_H

