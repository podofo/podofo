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
    sOutStream << "PDF Version: " << PoDoFo::s_szPdfVersionNums[static_cast<int>(mDoc->GetPdfVersion())] << std::endl;
    sOutStream << "Page Count: " << mDoc->GetPageCount() << std::endl;
    sOutStream << std::endl;
    sOutStream << "Fast Web View Enabled: " << (mDoc->IsLinearized() ? "Yes" : "No") << std::endl;
    sOutStream << "Tagged: " << (static_cast<PoDoFo::PdfMemDocument*>(mDoc)->GetStructTreeRoot() != NULL ? "Yes" : "No") << std::endl;
    sOutStream << "Printing Allowed: "  << (mDoc->IsPrintAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "Modification Allowed: "  << (mDoc->IsEditAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "Copy&Paste Allowed: "  << (mDoc->IsCopyAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "Add/Modify Annotations Allowed: "  << (mDoc->IsEditNotesAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "Fill&Sign Allowed: "  << (mDoc->IsFillAndSignAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "Accessibility Allowed: "  << (mDoc->IsAccessibilityAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "Document Assembly Allowed: "  << (mDoc->IsDocAssemblyAllowed() ? "Yes" : "No") << std::endl;
    sOutStream << "High Quality Print Allowed: "  << (mDoc->IsHighPrintAllowed() ? "Yes" : "No") << std::endl;
}

void PdfInfo::OutputInfoDict( std::ostream& sOutStream )
{
    if( !mDoc->GetInfo() )
        sOutStream << "No info dictionary in this PDF file!" << std::endl;
    else
    {
        sOutStream << "\tAuthor: "   << ( mDoc->GetInfo()->GetAuthor().GetString()   ? mDoc->GetInfo()->GetAuthor().GetString()  : "" ) << std::endl;
        sOutStream << "\tCreator: "  << ( mDoc->GetInfo()->GetCreator().GetString()  ? mDoc->GetInfo()->GetCreator().GetString() : "" ) << std::endl;
        sOutStream << "\tSubject: "  << ( mDoc->GetInfo()->GetSubject().GetString()  ? mDoc->GetInfo()->GetSubject().GetString() : "" ) << std::endl;
        sOutStream << "\tTitle: "    << ( mDoc->GetInfo()->GetTitle().GetString()    ? mDoc->GetInfo()->GetTitle().GetString()  : "" ) << std::endl;
        sOutStream << "\tKeywords: " << ( mDoc->GetInfo()->GetKeywords().GetString() ? mDoc->GetInfo()->GetKeywords().GetString()  : "" ) << std::endl;
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
    for ( int pg=0; pg<pgCount; pg++ ) 
    {
        sOutStream << "Page " << pg << ":" << std::endl;
        
        curPage = mDoc->GetPage( pg );
        sOutStream << "->Internal Number:" << curPage->GetPageNumber() << std::endl;
        
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
            sOutStream << "\t\tContents: " << (curAnnot->GetContents().GetString() ? curAnnot->GetContents().GetString() : "") << std::endl;
            sOutStream << "\t\tTitle: "    << (curAnnot->GetTitle().GetString()    ? curAnnot->GetTitle().GetString() : "") << std::endl;
            sOutStream << "\t\tFlags: "    << curAnnot->GetFlags() << std::endl;
            sOutStream << "\t\tRect: "     << str << std::endl;
            sOutStream << "\t\tOpen: "     << (curAnnot->GetOpen() ? "true" : "false" ) << std::endl;

            if( curAnnot->GetType() == PoDoFo::ePdfAnnotation_Link ) 
            {
                sOutStream << "\t\tLink Target: " << curAnnot->GetType() << std::endl;
                if( curAnnot->HasAction() && curAnnot->GetAction()->HasURI() )
                    sOutStream << "\t\tAction URI: " << (curAnnot->GetAction()->GetURI().GetString() ? 
                                                         curAnnot->GetAction()->GetURI().GetString() : "") << std::endl;
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
    PoDoFo::PdfDestination* pDest = pItem->GetDestination();
    if ( pDest ) {	// then it's a destination

        PoDoFo::PdfPage* pPage = pDest->GetPage();
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
