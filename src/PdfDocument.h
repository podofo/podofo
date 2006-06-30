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
#include "PdfObject.h"
#include "PdfParser.h"
#include "PdfWriter.h"

namespace PoDoFo {

class PdfDictionary;
class PdfPage;
class PdfPagesTree;

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

    /** Get the file format version of the pdf
     *  \returns the file format version as string
     */
    const char* GetPdfVersionString() const { return mWriter.GetPdfVersionString(); }
    
    /** \returns whether the parsed document contains linearization tables
     */
    bool IsLinearized() const { if (mParser) return mParser->IsLinearized(); else return false; }
    
    /** \returns the size of a read/parsed PDF
     */
    size_t FileSize() const { if (mParser) return mParser->FileSize(); else return 0; }
    

	/** Retrieve the actual object for a given PdfReference in this document
	*   \param inRef a PdfReference to the object in question
	*   \returns a PdfObject to the reference
	*/
	PdfObject* GetObject( const PdfReference& inRef ) const { return mWriter.GetObjects().GetObject( inRef ); }

	/** Create a PdfObject of type T which must be a subclass of PdfObject
	*  and it does not need a parameter for pszType.
	*  This function assigns the next free object number to the PdfObject
	*  and add is to the internal vector.
	*
	*  \returns a new PdfObject subclass
	*/
	template <class T> T* CreateObject();

	
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

    /** Get access to the StructTreeRoot dictionary
     *  \returns PdfObject the StructTreeRoot dictionary
     */
    PdfObject* GetStructTreeRoot() const { return GetNamedObjectFromCatalog( "StructTreeRoot" ); }

    /** Get access to the Metadata stream
     *  \returns PdfObject the Metadata stream (should be in XML, using XMP grammar)
     */
    PdfObject* GetMetadata() const { return GetNamedObjectFromCatalog( "Metadata" ); }

    /** Get access to the Outlines (Bookmarks) dictionary
     *  \returns PdfObject the Outlines/Bookmarks dictionary
     */
    PdfObject* GetOutlines() const { return GetNamedObjectFromCatalog( "Outlines" ); }

    /** Get access to the AcroForm dictionary
     *  \returns PdfObject the AcroForm dictionary
     */
    PdfObject* GetAcroForm() const { return GetNamedObjectFromCatalog( "AcroForm" ); }

    /** Get the total number of pages in a document
     *  \returns int number of pages
     */
    int GetPageCount() const;

    /** Get the PdfObject for a specific page in a document
     * \param nIndex which page (0-based)
	 *  \returns PdfObject* for the Page
     */
    PdfPage* GetPage( int nIndex ) const;


 private:
    /** Get a dictioary from the catalog dictionary by its name.
     *  \param pszName will be converted into a PdfName
     *  \returns the dictionary if it was found or NULL
     */
    PdfObject* GetNamedObjectFromCatalog( const char* pszName ) const;

   /** Internal method for initializing the pages tree for this document
     */
	void InitPagesTree();

 private:
    PdfParser*	    mParser;
    PdfWriter	    mWriter;
	
	PdfPagesTree*   mPagesTree;
};

template <class T>
T* PdfDocument::CreateObject()
{
	T*         pTemplate = new T( this, mWriter.GetObjects()->GetObjectCount(), 0 );
	PdfObject* pObject   = dynamic_cast<PdfObject*>(pTemplate);

	if( !pObject )
	{
		delete pTemplate;
		return NULL;
	}

	mWriter.GetObjects()->push_back( pObject );
	return pTemplate;
}


};


#endif	// _PDF_DOCUMENT_H_
