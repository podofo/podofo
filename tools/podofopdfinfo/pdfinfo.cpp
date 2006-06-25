/***************************************************************************
*   Copyright (C) 2005 by Dominik Seichter                                *
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

#include "pdfinfo.h"


PdfInfo::PdfInfo( const std::string& inPathname )
{
	mDoc = new PdfDocument( inPathname );
}

PdfInfo::~PdfInfo()
{
	if ( mDoc ) {
		delete mDoc;
		mDoc = NULL;
	}
}

void PdfInfo::OutputInfoDict( std::ostream& sOutStream )
{
	PdfObject	*infoObj = mDoc->GetInfo();
	if ( infoObj ) {
		if ( infoObj->HasSingleValue() ) {
			PdfVariant	infoVar = infoObj->GetSingleValueVariant();
			std::string	varVal;
			infoVar.ToString( varVal );
			sOutStream << varVal;
		} else {
			TKeyMap	infoKeyMap = infoObj->GetKeys();
			TIKeyMap	keyItor = infoKeyMap.begin();
			while ( keyItor != infoKeyMap.end() ) {
				sOutStream << keyItor->first.Name() << ": ";

				std::string	varVal;
				keyItor->second.ToString( varVal );
				sOutStream << varVal << std::endl;
			
				keyItor++;
			}
		}
	}
}