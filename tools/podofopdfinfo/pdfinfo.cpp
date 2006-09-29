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
#include "PdfNamesTree.h"

PdfInfo::PdfInfo( const std::string& inPathname )
{
    mDoc = new PdfDocument( inPathname.c_str() );
}

PdfInfo::~PdfInfo()
{
    if ( mDoc ) {
        delete mDoc;
        mDoc = NULL;
    }
}

void PdfInfo::OutputDocumentInfo( std::ostream& sOutStream )
{
    sOutStream << "PDF Version: " << s_szPdfVersionNums[(int)mDoc->GetPdfVersion()] << std::endl;
    sOutStream << "Page Count: " << mDoc->GetPageCount() << std::endl;
    sOutStream << std::endl;
    sOutStream << "Fast Web View Enabled: " << (mDoc->IsLinearized() ? "Yes" : "No") << std::endl;
    sOutStream << "Tagged: " << (mDoc->GetStructTreeRoot() != NULL ? "Yes" : "No") << std::endl;

/*
// print encryption info
printf("Encrypted:      ");
if (doc->isEncrypted()) {
printf("yes (print:%s copy:%s change:%s addNotes:%s)\n",
doc->okToPrint(gTrue) ? "yes" : "no",
doc->okToCopy(gTrue) ? "yes" : "no",
doc->okToChange(gTrue) ? "yes" : "no",
doc->okToAddNotes(gTrue) ? "yes" : "no");
} else {
printf("no\n");
}
*/
}

void PdfInfo::OutputInfoDict( std::ostream& sOutStream )
{
    if( !mDoc->GetInfo() )
        sOutStream << "No info dictionary in this PDF file!" << std::endl;
    else
    {
        sOutStream << "\tAuthor: "   << ( mDoc->GetAuthor().GetString()   ? mDoc->GetAuthor().GetString()  : "" ) << std::endl;
        sOutStream << "\tCreator: "  << ( mDoc->GetCreator().GetString()  ? mDoc->GetCreator().GetString() : "" ) << std::endl;
        sOutStream << "\tSubject: "  << ( mDoc->GetSubject().GetString()  ? mDoc->GetSubject().GetString() : "" ) << std::endl;
        sOutStream << "\tTitle: "    << ( mDoc->GetTitle().GetString()    ? mDoc->GetTitle().GetString()  : "" ) << std::endl;
        sOutStream << "\tKeywords: " << ( mDoc->GetKeywords().GetString() ? mDoc->GetKeywords().GetString()  : "" ) << std::endl;
    }
}

void PdfInfo::OutputPageInfo( std::ostream& sOutStream )
{
    PdfPage*    curPage;
    PdfVariant  var;
    std::string str;

    int	pgCount = mDoc->GetPageCount();
    for ( int pg=0; pg<pgCount; pg++ ) 
    {
        sOutStream << "Page " << pg << ":" << std::endl;
        
        curPage = mDoc->GetPage( pg );
        
        curPage->GetMediaBox().ToVariant( var );
        var.ToString( str );

        sOutStream << "\tMediaBox: " << str << std::endl;
        sOutStream << "\tRotation: " << curPage->GetRotation() << std::endl;
        sOutStream << "\t# of Annotations: " << curPage->GetNumAnnots() << std::endl;
    }
}

void PdfInfo::OutputOutlines( std::ostream& sOutStream, PdfOutlineItem* pItem, int level )
{
    PdfOutlines* pOutlines;
    int          i;

    if( !pItem ) 
    {
        pOutlines = mDoc->GetOutlines( ePdfDontCreateObject );
		if ( !pOutlines ) {
			sOutStream << "\tNone Found" << std::endl;
			return;
		}
        pItem     = pOutlines->First();
    }

    for( i=0;i<level;i++ )
        sOutStream << "-";

    sOutStream << ">" << pItem->GetTitle().GetString();
	PdfDestination* pDest = pItem->GetDestination();
	if ( pDest ) {	// then it's a destination
		PdfPage* pPage = pDest->GetPage();
		sOutStream << "\tDestination: Page #" << "???";
	} else {		// then it's one or more actions
		sOutStream << "\tAction: " << "???";
	}
	sOutStream << std::endl;

    if( pItem->First() )
        this->OutputOutlines( sOutStream, pItem->First(), level+1 );
    
    if( pItem->Next() )
        this->OutputOutlines( sOutStream, pItem->Next(), level );
}

void PdfInfo::OutputOneName( std::ostream& sOutStream, PdfNamesTree* inTreeObj, 
							 const std::string& inTitle, const std::string& inKey )
{
    sOutStream << "\t" << inTitle << std::endl;
    PdfObject* arrObj = inTreeObj->GetOneArrayOfNames( PdfName( inKey ), ePdfDontCreateObject );
    if ( arrObj ) {
        PdfArray&	arr = arrObj->GetArray();
        
        // a names array is a set of PdfString/PdfObject pairs
        // so we loop in sets of two - getting each pair
        for ( unsigned int i=0; i<arr.size(); i+=2 ) {	
            const PdfString&	theName = arr[i].GetString();
            const PdfObject&	theVal = arr[i+1];
            
            sOutStream << "\t\t" << theName.GetString() << std::endl;
        }
    } else {
        sOutStream << "\t\tNone Found" << std::endl;
    }
}

void PdfInfo::OutputNames( std::ostream& sOutStream )
{
    PdfNamesTree*	namesObj = mDoc->GetNamesTree( ePdfDontCreateObject );
    if ( namesObj ) {
        OutputOneName( sOutStream, namesObj, "Destinations", "Dests" );
        OutputOneName( sOutStream, namesObj, "JavaScripts", "JavaScript" );
        OutputOneName( sOutStream, namesObj, "Embedded Files", "EmbeddedFiles" );
    } else {
        sOutStream << "\t\tNone Found" << std::endl;
    }
}
