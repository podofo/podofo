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
    mDoc = new PoDoFo::PdfMemDocument( inPathname.c_str() );
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
	sOutStream << "\tPDF Version: " << PoDoFo::s_szPdfVersionNums[static_cast<int>(mDoc->GetPdfVersion())] << std::endl;
	sOutStream << "\tPage Count: " << mDoc->GetPageCount() << std::endl;
	sOutStream << "\tPage Size: " << GuessFormat() << std::endl; 
    sOutStream << std::endl;
    sOutStream << "\tFast Web View Enabled: " << (mDoc->IsLinearized() ? "Yes" : "No") << std::endl;
    sOutStream << "\tTagged: " << (static_cast<PoDoFo::PdfMemDocument*>(mDoc)->GetStructTreeRoot() != NULL ? "Yes" : "No") << std::endl;
    sOutStream << "\tEncrypted: " << (static_cast<PoDoFo::PdfMemDocument*>(mDoc)->GetEncrypted() ? "Yes" : "No") << std::endl;
    sOutStream << "\tPrinting Allowed: "  << (mDoc->IsPrintAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "\tModification Allowed: "  << (mDoc->IsEditAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "\tCopy&Paste Allowed: "  << (mDoc->IsCopyAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "\tAdd/Modify Annotations Allowed: "  << (mDoc->IsEditNotesAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "\tFill&Sign Allowed: "  << (mDoc->IsFillAndSignAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "\tAccessibility Allowed: "  << (mDoc->IsAccessibilityAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "\tDocument Assembly Allowed: "  << (mDoc->IsDocAssemblyAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "\tHigh Quality Print Allowed: "  << (mDoc->IsHighPrintAllowed() ? "Yes" : "No") << std::endl;
}

void PdfInfo::OutputInfoDict( std::ostream& sOutStream )
{
    if( !mDoc->GetInfo() )
        sOutStream << "No info dictionary in this PDF file!" << std::endl;
    else
    {

        sOutStream << "\tAuthor: "   << mDoc->GetInfo()->GetAuthor().GetStringUtf8() << std::endl;
        sOutStream << "\tCreator: "  << mDoc->GetInfo()->GetCreator().GetStringUtf8() << std::endl;
        sOutStream << "\tSubject: "  << mDoc->GetInfo()->GetSubject().GetStringUtf8() << std::endl;
        sOutStream << "\tTitle: "    << mDoc->GetInfo()->GetTitle().GetStringUtf8() << std::endl;
        sOutStream << "\tKeywords: " << mDoc->GetInfo()->GetKeywords().GetStringUtf8() << std::endl;
	sOutStream << "\tTrapped: "  << mDoc->GetInfo()->GetTrapped().GetEscapedName() << std::endl;
    }
}

void PdfInfo::OutputPageInfo( std::ostream& sOutStream )
{
    PoDoFo::PdfPage*       curPage;
    PoDoFo::PdfAnnotation* curAnnot;

    PoDoFo::PdfVariant  var;
    std::string str;

    int annotCount;
    int	pgCount = mDoc->GetPageCount();
    sOutStream << "Page Count: " << pgCount << std::endl;
    for ( int pg=0; pg<pgCount; pg++ ) 
    {
        sOutStream << "Page " << pg << ":" << std::endl;
        
        curPage = mDoc->GetPage( pg );
        sOutStream << "->Internal Number:" << curPage->GetPageNumber() << std::endl;
        sOutStream << "->Object Number:" << curPage->GetObject()->Reference().ObjectNumber() 
                   << " " <<  curPage->GetObject()->Reference().GenerationNumber() << " R" << std::endl;
        
        curPage->GetMediaBox().ToVariant( var );
        var.ToString( str );

        annotCount = curPage->GetNumAnnots();
        sOutStream << "\tMediaBox: " << str << std::endl;
        sOutStream << "\tRotation: " << curPage->GetRotation() << std::endl;
        sOutStream << "\t# of Annotations: " << annotCount << std::endl;

        for( int i=0; i < annotCount; i++ ) 
        {
            curAnnot = curPage->GetAnnotation( i );

            curAnnot->GetRect().ToVariant( var );
            var.ToString( str );

            sOutStream << std::endl;
            sOutStream << "\tAnnotation "  << i << std::endl;
            sOutStream << "\t\tType: "     << curAnnot->GetType() << std::endl;
            sOutStream << "\t\tContents: " << curAnnot->GetContents().GetStringUtf8() << std::endl;
            sOutStream << "\t\tTitle: "    << curAnnot->GetTitle().GetStringUtf8() << std::endl;
            sOutStream << "\t\tFlags: "    << curAnnot->GetFlags() << std::endl;
            sOutStream << "\t\tRect: "     << str << std::endl;
            sOutStream << "\t\tOpen: "     << (curAnnot->GetOpen() ? "true" : "false" ) << std::endl;

            if( curAnnot->GetType() == PoDoFo::ePdfAnnotation_Link ) 
            {
                sOutStream << "\t\tLink Target: " << curAnnot->GetType() << std::endl;
                if( curAnnot->HasAction() && curAnnot->GetAction()->HasURI() )
                    sOutStream << "\t\tAction URI: " << curAnnot->GetAction()->GetURI().GetStringUtf8()  << std::endl;
            }
        }        
    }
}

void PdfInfo::OutputOutlines( std::ostream& sOutStream, PoDoFo::PdfOutlineItem* pItem, int level )
{
    PoDoFo::PdfOutlines* pOutlines;
    int          i;

    if( !pItem ) 
    {
        pOutlines = mDoc->GetOutlines( PoDoFo::ePdfDontCreateObject );
        if ( !pOutlines || !pOutlines->First() ) {
            sOutStream << "\tNone Found" << std::endl;
            return;
        }
        pItem     = pOutlines->First();
    }

    for( i=0;i<level;i++ )
        sOutStream << "-";

    sOutStream << ">" << pItem->GetTitle().GetString();
    PoDoFo::PdfDestination* pDest = pItem->GetDestination( mDoc );
    if ( pDest ) {	// then it's a destination

        PoDoFo::PdfPage* pPage = pDest->GetPage( mDoc );
        if( pPage ) 
            sOutStream << "\tDestination: Page #" << pPage->GetPageNumber();
        else
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

void PdfInfo::OutputOneName( std::ostream& sOutStream, PoDoFo::PdfNamesTree* inTreeObj, 
							 const std::string& inTitle, const std::string& inKey )
{
    sOutStream << "\t" << inTitle << std::endl;
    PoDoFo::PdfDictionary dict;
    inTreeObj->ToDictionary( PoDoFo::PdfName( inKey ), dict );

    const PoDoFo::TKeyMap& keys = dict.GetKeys();
    PoDoFo::TCIKeyMap      it   = keys.begin();

    std::string str;
    while( it != keys.end() )
    {
        (*it).second->ToString( str );
        sOutStream << "\t-> " << (*it).first.GetName().c_str() << "=" << str << std::endl;
        ++it;
    }

    sOutStream << std::endl;
}

void PdfInfo::OutputNames( std::ostream& sOutStream )
{
    PoDoFo::PdfNamesTree*	namesObj = mDoc->GetNamesTree( PoDoFo::ePdfDontCreateObject );
    if ( namesObj ) {
        OutputOneName( sOutStream, namesObj, "Destinations", "Dests" );
        OutputOneName( sOutStream, namesObj, "JavaScripts", "JavaScript" );
        OutputOneName( sOutStream, namesObj, "Embedded Files", "EmbeddedFiles" );
    } else {
        sOutStream << "\t\tNone Found" << std::endl;
    }
}

std::string PdfInfo::GuessFormat()
{
	typedef std::pair<double,double> Format;
	
	PoDoFo::PdfPage*  curPage;
	int	pgCount = mDoc->GetPageCount();
	std::map<  Format , int > sizes;
	std::map<  Format , int >::iterator sIt;
	PoDoFo::PdfRect  rect;
	for ( int pg=0; pg<pgCount; pg++ ) 
	{
		curPage = mDoc->GetPage( pg );
		if( !curPage )
		{
			PODOFO_RAISE_ERROR( PoDoFo::ePdfError_PageNotFound );
		}
		rect = curPage->GetMediaBox();
		Format s( rect.GetWidth() - rect.GetLeft(), rect.GetHeight() - rect.GetBottom());
		sIt = sizes.find(s);
		if(sIt == sizes.end())
			sizes.insert(std::pair<Format,int>(s,1));
		else
			++(sIt->second);
	}
	
	Format format;
	std::stringstream ss;
	if(sizes.size() == 1)
	{
		format = sizes.begin()->first;
		ss << format.first << " x " << format.second << " pts"  ;
	}
	else
	{
		// We’re looking for the most represented format
		int max=0;
		for(sIt = sizes.begin();sIt != sizes.end(); ++sIt)
		{
			if(sIt->second > max)
			{
				max = sIt->second;
				format = sIt->first;
			}
		}
		ss << format.first << " x " << format.second << " pts "<<std::string(sizes.size(), '*');
	}
	
	return ss.str();
}
