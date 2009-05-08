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

#ifndef __MOVE_OPERATION__H__
#define __MOVE_OPERATION__H__

#include "Operation.h"

namespace PoDoFo 
{
class PdfDocument;
}

class MoveOperation : public Operation {

public:
    MoveOperation( int nFrom, int nTo ); 

    virtual ~MoveOperation() { }


    virtual void Perform( PoDoFo::PdfDocument & rDoc );
    virtual std::string ToString() const;

private:
    int m_nFrom;
    int m_nTo;
};

#endif // __MOVE_OPERATION__H__
