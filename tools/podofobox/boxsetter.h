/***************************************************************************
 *   Copyright (C) 2010 by Pierre Marchand   *
 *   pierre@oep-h.com   *
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

#ifndef BOXSETTER_H
#define BOXSETTER_H

#include "podofo.h"
#include <string>

class BoxSetter
{
	BoxSetter(){}
	const std::string m_box;
	const PoDoFo::PdfRect m_rect;
	void SetBox(PoDoFo::PdfPage * page);
	bool CompareBox(const PoDoFo::PdfRect& rect1, const PoDoFo::PdfRect& rect2);

public:
	BoxSetter(const std::string& in, const std::string& out, const std::string& box, const PoDoFo::PdfRect& rect);

};

#endif // BOXSETTER_H
