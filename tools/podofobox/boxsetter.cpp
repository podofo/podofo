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

#include "boxsetter.h"

BoxSetter::BoxSetter(const std::string& in, const std::string& out, const std::string& box, const PoDoFo::PdfRect& rect)
	:m_box(box), m_rect(rect)
{
	PoDoFo::PdfMemDocument* source = new PoDoFo::PdfMemDocument(in.c_str());
	int pcount(source->GetPageCount());
	for ( int i = 0; i < pcount ; ++i )
	{
		SetBox(source->GetPage ( i ));
	}

	source->Write(out.c_str());

}

void BoxSetter::SetBox(PoDoFo::PdfPage *page)
{
	if(!page)
		return;
	PoDoFo::PdfObject r;
	m_rect.ToVariant( r );
	if(m_box.find("media") != std::string::npos)
	{
		page->GetObject()->GetDictionary().AddKey ( PoDoFo::PdfName ( "MediaBox" ), r );
	}
	else if(m_box.find("crop") != std::string::npos)
	{
		page->GetObject()->GetDictionary().AddKey ( PoDoFo::PdfName ( "CropBox" ), r );
	}
	else if(m_box.find("bleed") != std::string::npos)
	{
		page->GetObject()->GetDictionary().AddKey ( PoDoFo::PdfName ( "BleedBox" ), r );
	}
	else if(m_box.find("trim") != std::string::npos)
	{
		page->GetObject()->GetDictionary().AddKey ( PoDoFo::PdfName ( "TrimBox" ), r );
	}
	else if(m_box.find("art") != std::string::npos)
	{
		page->GetObject()->GetDictionary().AddKey ( PoDoFo::PdfName ( "ArtBox" ), r );
	}

	// TODO check that box sizes are ordered
}

bool BoxSetter::CompareBox(const PoDoFo::PdfRect &rect1, const PoDoFo::PdfRect &rect2)
{
	return rect1.ToString() == rect2.ToString();
}

