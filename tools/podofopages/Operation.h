/***************************************************************************
 *   Copyright (C) 2009 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef __OPERATION__H__
#define __OPERATION__H__

#include <string>

namespace PoDoFo 
{
class PdfDocument;
}

/**
 * Abstract base class for all operations
 * the podofopages can perform.
 *
 */ 
class Operation {

public:
    virtual ~Operation() { }


    virtual void Perform( PoDoFo::PdfDocument & rDoc ) = 0;

    virtual std::string ToString() const = 0;
};

#endif // __OPERATION__H__
