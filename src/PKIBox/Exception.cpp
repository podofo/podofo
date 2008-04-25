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

#include "Exception.h"

using namespace std;

namespace PKIBox
{
	//---------------------------------------------------------------------------------------
	// Function name	:
	// Description	    :
	// Return type		:
	// Argument         :
	//---------------------------------------------------------------------------------------
	Exception::Exception(): m_ulErrCode(0)
	{

	}


	//---------------------------------------------------------------------------------------
	// Function name	:
	// Description	    :
	// Return type		:
	// Argument         :
	//---------------------------------------------------------------------------------------
	Exception::Exception(const string &sErrMsg)
		: m_sErrMsg(sErrMsg), m_ulErrCode(0)
	{

	}

	//---------------------------------------------------------------------------------------
	// Function name	:
	// Description	    :
	// Return type		:
	// Argument         :
	//---------------------------------------------------------------------------------------
	Exception::Exception(unsigned long ulErrCode, const string &sErrMsg)
		: m_sErrMsg(sErrMsg), m_ulErrCode(ulErrCode)
	{

	}


	//---------------------------------------------------------------------------------------
	// Function name	:
	// Description	    :
	// Return type		:
	// Argument         :
	//---------------------------------------------------------------------------------------
	/*virtual*/ Exception::~Exception()
	{

	}


	//---------------------------------------------------------------------------------------
	// Function name	:
	// Description	    :
	// Return type		:
	// Argument         :
	//---------------------------------------------------------------------------------------
	void Exception::SetErrCode(unsigned long ulErrCode)
	{
		m_ulErrCode = ulErrCode;
	}


	//---------------------------------------------------------------------------------------
	// Function name	:
	// Description	    :
	// Return type		:
	// Argument         :
	//---------------------------------------------------------------------------------------
	void Exception::SetErrorMessage(const string &sErrMsg)
	{
		m_sErrMsg = sErrMsg;
	}

}




