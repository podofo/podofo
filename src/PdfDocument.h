/***************************************************************************
*   Copyright (C) 2006 by Dominik Seichter                                *
*   domseichter@web.de                                                    *
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

#ifndef _PDF_DOCUMENT_H_
#define _PDF_DOCUMENT_H_

#include "PdfDefines.h"
#include "PdfParser.h"
#include "PdfWriter.h"

namespace PoDoFo {

	class PdfDocument {
	public:

		/** Construct a new (empty) PdfDocument
		*/
		PdfDocument();

		/** Construct a PdfDocument from an existing PDF (on disk)
		*  \param sFilename filename of the file which is going to be parsed/opened
		*/
		PdfDocument( const std::string& sPathname );

		/** Close down/destruct the PdfDocument
		*/
		virtual ~PdfDocument();

		/** Set the PDF Version of the document. Has to be called before Write() to
		*  have an effect.
		*  \param eVersion  version of the pdf document
		*/
		void SetPdfVersion( EPdfVersion eVersion )	{ return mWriter.SetPdfVersion( eVersion ); }

		/** Get the PDF version of the document
		*  \returns EPdfVersion version of the pdf document
		*/
		EPdfVersion GetPdfVersion() const	{ return mWriter.GetPdfVersion(); }

		/** Get access to the internal Catalog dictionary
		*  or root object.
		*  
		*  \returns PdfObject the documents catalog or NULL 
		*                     if no catalog is available
		*/
		PdfObject* GetCatalog() const { return mWriter.GetCatalog(); }

		/** Get access to the internal Info dictionary
		*  \returns PdfObject the info dictionary
		*/
		PdfObject* GetInfo() const { return mWriter.GetInfo(); }

	private:
		PdfParser*	mParser;
		PdfWriter	mWriter;
	};

};


#endif	// _PDF_DOCUMENT_H_