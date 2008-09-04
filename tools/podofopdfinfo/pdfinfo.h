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

#ifndef _PDFINFO_H_
#define _PDFINFO_H_

#include <ostream>
#include <podofo.h>

class PdfInfo {
public:
	PdfInfo( const std::string& inPathname );
	virtual ~PdfInfo();

	void OutputDocumentInfo( std::ostream& sOutStream );
	void OutputInfoDict( std::ostream& sOutStream );
	void OutputPageInfo( std::ostream& sOutStream );
	void OutputOutlines( std::ostream& sOutStream, PoDoFo::PdfOutlineItem* pFirst = NULL, int level = 0 );
	void OutputNames( std::ostream& sOutStream );

private:
	PoDoFo::PdfDocument*	mDoc;

	void OutputOneName( std::ostream& sOutStream, PoDoFo::PdfNamesTree* inTreeObj, 
                            const std::string& inTitle, const std::string& inKey );
	std::string GuessFormat();
};


#endif	// _PDFINFO_H

