/***************************************************************************
 *   Copyright (C) 2007 by Pierre Marchand   *
 *   pierre@moulindetouvois.com   *
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
#include "pdftranslator.h"
#include "charpainter.h"

#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <istream>
#include <ostream>
using std::ostringstream;
using std::map;
using std::vector;
using std::string;
using std::ifstream;
using std::istream;
using std::ostream;
using std::endl;
using std::runtime_error;

#include <iostream> //XXX

PageRecord::PageRecord ( int s,int d,double r, double tx, double ty )
    : sourcePage ( s ),
    destPage ( d ),
    rotate ( r ),
    transX ( tx ),
    transY ( ty )
{
};

PageRecord::PageRecord()
    : sourcePage(0),
    destPage(0),
    rotate(0),
    transX(0),
    transY(0)
{};

istream & operator>>(istream& s, PageRecord& r)
{
    s >> r.sourcePage >> r.destPage >> r.rotate >> r.transX >> r.transY;
    // TODO: set invalid if stream state bad
    return s;
}

ostream & operator<<(ostream& s, const PageRecord& r)
{
    return s << r.sourcePage << r.destPage << r.rotate << r.transX << r.transY << endl;
}

bool PageRecord::isValid() const
{
    //TODO
    return true;
}

PdfTranslator::PdfTranslator(double sp)
{
    sourceDoc = 0;
    targetDoc = 0;
    extraSpace = sp;
}

void PdfTranslator::setSource ( const std::string & source )
{
	ifstream in ( source.c_str(), ifstream::in );
	if (!in.good())
		throw runtime_error("setSource() failed to open input file");
	
        const int magicBufferLen = 5;
        char magicBuffer[magicBufferLen ];
        in.read(magicBuffer, magicBufferLen );
        std::string magic( magicBuffer , magicBufferLen  );
	
	if(magic.find("%PDF") < 5)//it is a PDF file (I hope)
	{
		in.close();
    		inFilePath = source;
    		sourceDoc = new PdfMemDocument ( inFilePath.c_str() ); // must succeed or throw
	}
	else // it has to be a list of PDF files
	{
		in.seekg(0);
		std::vector<std::string> fileList;
		char *filenameBuffer = new char[1000];
		do
		{
			in.getline (filenameBuffer, 1000 );
			fileList.push_back(std::string(filenameBuffer, in.gcount() ) );
		}
		while ( !in.eof() );
		in.close();
		bool first = true;
		for(std::vector<std::string>::const_iterator ms = fileList.begin(); ms != fileList.end(); ++ms)
		{
			if(first)
			{
				inFilePath = *ms;
				sourceDoc = new PdfMemDocument ( inFilePath.c_str() );
				first = false;
			}
			else
			{
			PdfMemDocument mdoc((*ms).c_str());
			targetDoc->InsertPages( mdoc, 0, mdoc.GetPageCount());
			multiSource.push_back(*ms);
			}
		}
	}
}

void PdfTranslator::addToSource( const std::string & source )
{
	if( !sourceDoc )
		return;
	
	PdfMemDocument extraDoc(source.c_str());
	sourceDoc->InsertPages( extraDoc, 0,  extraDoc.GetPageCount() );
	multiSource.push_back(source);
	
}

void PdfTranslator::setTarget ( const std::string & target )
{
    if ( !sourceDoc )
        throw std::logic_error("setTarget() called before setSource()");

    // DOCUMENT: Setting `targetDoc' to the input path will be confusing when reading the code.
    targetDoc = new PdfMemDocument ( inFilePath.c_str() );
    if(!multiSource.empty())
    {
	    for(std::vector<std::string>::const_iterator ms = multiSource.begin(); ms != multiSource.end(); ++ms)
	    {
		    PdfMemDocument mdoc((*ms).c_str());
		    targetDoc->InsertPages( mdoc, 0, mdoc.GetPageCount());
	    }
    }
    outFilePath  = target;
    pcount = targetDoc->GetPageCount();

    for ( int i = 0; i < pcount ; ++i )
    {
        PdfPage * page = targetDoc->GetPage ( i );
        PdfMemoryOutputStream outMemStream ( 1 );

        PdfXObject *xobj = new PdfXObject ( page->GetMediaBox(), targetDoc );
        page->GetContents()->GetStream()->GetFilteredCopy ( &outMemStream );
        outMemStream.Close();

        PdfMemoryInputStream inStream ( outMemStream.TakeBuffer(),outMemStream.GetLength() );
        xobj->GetContents()->GetStream()->Set ( &inStream );

        // I should get /Ressource dict...

        // Getting resources from Pages node this way is suitable for Scribus PDF, that's it.
        PdfObject *rparent = page->GetObject()->GetDictionary().GetKey ( PdfName ( "Parent" ) );
        if ( rparent )
        {
            PdfReference ref = rparent->GetReference();
            //qDebug() << "has Parent ref";
            PdfVecObjects objs = targetDoc->GetObjects();
            //qDebug() << "get obj Parent";
            PdfObject *parent = objs.GetObject ( ref );
            PdfObject *gres = parent->GetDictionary().GetKey ( "Resources" );
            if(gres)
                globalResRef = gres->GetReference();
        }
        else
        {
            //qDebug() << "This page has no parent ?";
            TKeyMap rkeys = page->GetObject()->GetDictionary().GetKeys();
            TCIKeyMap itk = rkeys.begin();
            while ( itk != rkeys.end() )
            {
                //qDebug() << itk->first.GetName().c_str() ;
                ++itk;
            }
        }

        xobjects[i+1] = xobj;
        trimRect[i+1] = page->GetTrimBox();
        bleedRect[i+1] = page->GetBleedBox();
    }
}

void PdfTranslator::loadPlan ( const std::string & plan )
{
    // headline must be :
    // "target page width" "target page height"
    // pagerecord must be :
    // "source page" "dest page" "rotation" "horiz. translation" "vert. translation"
    // ex.: 1 2 90 500 0 <newline>

    ifstream in ( plan.c_str(), ifstream::in );
    if (!in.good())
        throw runtime_error("Failed to open plan file");

    bool first = true;
    int dup = 40000;
    std::string line;
    do
    {
        if ( first )
        {
            in >> destWidth;
            in >> destHeight;
            if (!in.good())
                throw runtime_error("Bad plan file header");
            first = false;
            continue;
        }

        PageRecord p;
        in >> p;
        if (in.eof())
            break;
        if (!p.isValid())
            throw runtime_error("Bad plan file record");

        maxPageDest = std::max ( maxPageDest, p.destPage );
        if( pagesIndex.find( p.sourcePage ) != pagesIndex.end() )
        {
            //qDebug() << "duplicate "<< p.sourcePage << " in " << dup;

            PdfXObject *xobj = new PdfXObject(targetDoc->GetPage(p.sourcePage - 1)->GetMediaBox(), targetDoc );
            PdfMemoryOutputStream outMemStream ( 1 );
            xobjects[p.sourcePage]->GetContents()->GetStream()->GetFilteredCopy ( &outMemStream );
            outMemStream.Close();
            PdfMemoryInputStream inStream ( outMemStream.TakeBuffer(),outMemStream.GetLength() );
            xobj->GetContents()->GetStream()->Set(&inStream);

            xobjects[dup] = xobj;
            trimRect[dup] = targetDoc->GetPage(p.sourcePage - 1)->GetTrimBox();
            bleedRect[dup] = targetDoc->GetPage(p.sourcePage - 1)->GetBleedBox();
            p.sourcePage = dup;
            ++dup;
        }
        planImposition.push_back ( p );
        pagesIndex[p.sourcePage] = planImposition.size() - 1;
    }
    while ( !in.eof() );
}

//returns the number of processed pages.
int PdfTranslator::pageRange(int plan, int sheet, int pagesInBooklet, int numBooklet)
{
	double pw = sourceDoc->GetPage(0)->GetMediaBox().GetWidth();
	double ph = sourceDoc->GetPage(0)->GetMediaBox().GetHeight();
	if(plan == 4) // For now, it is the only "well known" plan ;-)
	{
		destWidth = pw * 2;
		destHeight = ph * 2;
		int firstpage = (plan * (sheet - 1) ) + 1 + ((numBooklet - 1) * pagesInBooklet);
		int lastpage = (numBooklet * pagesInBooklet) - ((sheet-1) * plan);
		{
			PageRecord p;
			//recto
			p.sourcePage = firstpage;
			p.destPage = sheet * 2 - 1;
			p.rotate = 0;
			p.transX = 1.0 * pw;
			p.transY = 0.0;
			planImposition.push_back(p);
			
			p.sourcePage = firstpage + 3;
			p.destPage = sheet * 2- 1;
			p.rotate = 180.0;
			p.transX = 2.0 * pw;
			p.transY = 2.0 * ph;
			planImposition.push_back(p);
			
			p.sourcePage = lastpage - 3;
			p.destPage = sheet * 2- 1;
			p.rotate = 180.0;
			p.transX = 1.0 * pw;
			p.transY = 2.0 * ph;
			planImposition.push_back(p);
			
			p.sourcePage = lastpage;
			p.destPage = sheet * 2- 1;
			p.rotate = 0.0;
			p.transX = 0.0;
			p.transY = 0.0;
			planImposition.push_back(p);
			
			//verso
			p.sourcePage = firstpage + 1;
			p.destPage = sheet * 2 ;
			p.rotate = 0;
			p.transX = 0.0 ;
			p.transY = 0.0;
			planImposition.push_back(p);
			
			p.sourcePage = firstpage + 2;
			p.destPage = sheet * 2 ;
			p.rotate = 180.0;
			p.transX = 1.0 * pw;
			p.transY = 2.0 * ph;
			planImposition.push_back(p);
			
			p.sourcePage = lastpage - 2;
			p.destPage = sheet * 2;
			p.rotate = 180.0;
			p.transX = 2.0 * pw;
			p.transY = 2.0 * ph;
			planImposition.push_back(p);
			
			p.sourcePage = lastpage - 1;
			p.destPage = sheet * 2 ;
			p.rotate = 0.0;
			p.transX = 1.0 * pw;
			p.transY = 0.0;
			planImposition.push_back(p);
			
			return 8;
		} 
	}

        // Fix a gcc warning,
        // this return should never be reached though
        return 0;
}

void PdfTranslator::computePlan(int wellKnownPlan, int sheetsPerBooklet)
{
	std::cerr << " computePlan(" <<  wellKnownPlan << ", "<<sheetsPerBooklet <<")";
	
// 	if(wellKnownPlan < 2 || sheetsPerBooklet < 1 || !sourceDoc);
// 	{
// 		return;
// 	}
	
	int groupSize = wellKnownPlan * 2;
	int pagesPerBooklet = groupSize * sheetsPerBooklet;
	int bookletCount = sourceDoc->GetPageCount() / pagesPerBooklet;
//### 	have to deal with padding
	int processedPages = 0;
	int numBooklet = 1;
	int nextBooklet = pagesPerBooklet;
	int s = 1;
	int p = 0;
	while(processedPages < sourceDoc->GetPageCount() )
	{
		p = pageRange(wellKnownPlan, s, pagesPerBooklet, numBooklet);
		std::cerr << p << " pages processed";
		++s;
		processedPages += p;
		if(processedPages > nextBooklet)
		{
			nextBooklet += pagesPerBooklet;
			++numBooklet;
		}
	}  
	
	//planImposition is filed, no duplicated pages here.
	for(unsigned int i = 0; i < planImposition.size(); ++i)
	{
            pagesIndex[planImposition[i].sourcePage] = i ;
	}
}

void PdfTranslator::impose()
{
    if ( ! (sourceDoc && targetDoc) )
        throw std::invalid_argument("impose() called with empty source or destination path");

    PdfDictionary globalRes;

    double pw = destWidth + (2.0 * extraSpace) ;
    double ph = destHeight  + (2.0 * extraSpace);

    PdfObject trimbox;
    PdfRect trim(extraSpace, extraSpace, destWidth, destHeight);
    trim.ToVariant(trimbox);

    typedef map<int, vector<int> > groups_t;
    groups_t groups;
    for ( unsigned int i = 0; i < planImposition.size(); ++i )
    {
        groups[planImposition[i].destPage].push_back ( planImposition[i].sourcePage );
    }

    groups_t::const_iterator  git = groups.begin();
    const groups_t::const_iterator gitEnd = groups.end();
    while ( git != gitEnd )
    {
        //qDebug() << "Perform destination page " << git.key() << git.value();
        PdfPage * newpage = targetDoc->CreatePage ( PdfRect ( 0.0, 0.0, pw, ph ) );
        newpage->GetObject()->GetDictionary().AddKey( PdfName("TrimBox"), trimbox);
        PdfDictionary xdict;

        ostringstream buffer;
        vector<int> pages;
        for ( int i = 0; i < (*git).second.size(); ++i )
        {

            int curPage = (*git).second[i]; 
            // 			int rec = curPage;
            // 			if( curPage > 39999)
            // 			{
            // 				curPage =  virtualMap.value(git.value()[i]) ;
            // 			}

            int index = pagesIndex[curPage];
            PdfRect rect = trimRect[curPage];
            PdfRect bleed = bleedRect[curPage];
            //qDebug() << "Perform source page " << curPage << index;

            PdfArray matrix;

            double cosR = cos ( planImposition[index].rotate  *  3.14159 / 180 );
            double sinR = sin ( planImposition[index].rotate  *  3.14159 / 180 );
            double tx = planImposition[index].transX + extraSpace;
            double ty = planImposition[index].transY + extraSpace;

            matrix.insert ( matrix.end(), PdfObject ( cosR ) );
            matrix.insert ( matrix.end(), PdfObject ( sinR ) );
            matrix.insert ( matrix.end(), PdfObject ( -sinR ) );
            matrix.insert ( matrix.end(), PdfObject ( cosR ) );
            matrix.insert ( matrix.end(), PdfObject ( tx ) );
            matrix.insert ( matrix.end(), PdfObject ( ty ) );


            PdfName mname ( "Matrix" );
            PdfXObject *xo = xobjects[curPage];
            ostringstream op;
            op << "OriginalPage" << curPage;
            PdfName xname ( op.str() );
            xdict.AddKey ( xname, xo->GetObjectReference() );
            xo->GetContents()->GetDictionary().AddKey ( mname, PdfObject ( matrix ) );
            xo->GetContents()->GetDictionary().AddKey ( PdfName ( "Resources" ), globalResRef );
            pages.push_back( curPage );

            double top = rect.GetHeight() + rect.GetBottom() + ty ;
            double bottom = rect.GetBottom() + ty;
            double left = rect.GetLeft() + tx;
            double right = rect.GetWidth() + rect.GetLeft() + tx;

            double btop = bleed.GetHeight() + bleed.GetBottom() + ty ;
            double bbottom = bleed.GetBottom() + ty;
            double bleft = bleed.GetLeft() + tx;
            double bright = bleed.GetWidth() + bleed.GetLeft() + tx;


            drawLine(0,top ,bleft, top , buffer);
            drawLine(0, bottom , bleft, bottom, buffer);

            drawLine(pw , top , bright , top, buffer);
            drawLine(pw, bottom , bright ,bottom, buffer);

            drawLine(left,0, left , bbottom,  buffer);
            drawLine(right, 0 ,right, bbottom, buffer);

            drawLine(left , ph ,left ,btop, buffer);
            drawLine(right, ph ,right, btop, buffer);
        }


        signature(extraSpace / 2.0 , extraSpace / 10.0, (*git).first, (*git).second, buffer);
        for ( vector<int>::const_iterator it = pages.begin(); it != pages.end(); ++it)
            buffer << "/OriginalPage" << *it << " Do\n";
        string bufStr = buffer.str();
        newpage->GetContentsForAppending()->GetStream()->Set ( bufStr.data(), bufStr.size() );
        PdfName dname ( "XObject" );
        newpage->GetResources()->GetDictionary().AddKey ( dname, xdict );
        ++git;
    }
    targetDoc->DeletePages ( 0,pcount );
    //qDebug() << outFilePath <<" has " << targetDoc->GetPageCount() << " pages.";
    targetDoc->Write ( outFilePath.c_str() );

}

void PdfTranslator::drawLine(double x, double y, double xx, double yy, ostringstream & s)
{
    //TODO: ensure safe double formatting for PDF
    s << "q\n"
         "0.5 w\n"
         "1 0 0 1 0 0 cm\n"
      << x << ' ' << y << " m\n"
      << xx << ' ' << yy << " l\n"
         "S\n"
         "Q\n";
}

void PdfTranslator::signature(double x, double y, int sheet, const vector< int > & pages, ostringstream & s)
{
    CharPainter painter;
    s << "q\n"
         "1 0 0 1 0 0 cm\n";
    painter.multipaint(s, sheet, extraSpace / 2.0 , x  , y);
    // 	for(int i = 0;i < pages.count(); ++i)
    // 	{
    // 		s << painter.multipaint(pages[i], extraSpace /3.0 , x + extraSpace + (i * extraSpace / 2.0),y );
    // 	}
    // 	
    s << "Q\n";

}


