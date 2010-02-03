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
#ifndef PDFTRANSLATOR_H
#define PDFTRANSLATOR_H

#include "podofo.h"
#include "impositionplan.h"

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <istream>
#include <string>



// using namespace PoDoFo;

namespace PoDoFo
{
	namespace Impose
	{
		
/**
PdfTranslator create a new PDF file which is the imposed version, following the imposition
plan provided by the user, of the source PDF file.
Pdftranslate does not really create a new PDF doc, it rather works on source doc, getting all page contents
as XObjects and put these XObjects on new pages. At the end, it removes original pages from the doc, but since
PoDoFo keeps them --- just removing from the pages tree ---, if it happens that you have a lot of content
in content stream rather than in resources, you'll get a huge file.
Usage is something like :
p = new PdfTranslator;
p->setSource("mydoc.pdf");
p->setTarget("myimposeddoc.pdf");
p->loadPlan("in4-32p.plan");
p->impose();
p->mailItToMyPrinterShop("job@proprint.com");//Would be great, doesn't it ?
*/
class PdfTranslator
{
	public:
		PdfTranslator();

		~PdfTranslator() { }

		PdfMemDocument *sourceDoc;
		PdfMemDocument *targetDoc;

		/**
		Set the source document(s) to be imposed.
		Argument source is the path of the PDF file, or the path of a file containing a list of paths of PDF files...
		*/
		void setSource ( const std::string & source );

		/**
		Another way to set many files as source document.
		Note that a source must be set before you call addToSource().
		*/
		void addToSource ( const std::string & source );

		/**
		Set the path of the file where the imposed PDF doc will be save.
		*/
		void setTarget ( const std::string & target );

		/**
		Load an imposition plan file of form:
		widthOfSheet heightOfSheet
		sourcePage destPage rotation translationX translationY
		...        ...      ...      ...          ...
		*/
		void loadPlan ( const std::string & planFile , PoDoFo::Impose::PlanReader loader );
		
		/**
		When all is prepared, call it to do the job.
		*/
		void impose();

	private:
		std::string inFilePath;
		std::string outFilePath;

		PdfReference globalResRef;
		
		ImpositionPlan *planImposition;
		
		std::map<int, PdfXObject*> xobjects;
		std::map<int,PdfObject*> resources;
		std::map<int, PdfRect> cropRect;
		std::map<int,PdfRect> bleedRect;
		std::map<int, PdfRect> trimRect;
		std::map<int,PdfRect> artRect;
		std::map<int, PdfDictionary*> pDict;
		std::map<int, int> virtualMap;
// 		int maxPageDest;
		int duplicate;

		bool checkIsPDF ( std::string path );
		PdfObject* getInheritedResources ( PdfPage* page );
		void mergeResKey ( PdfObject *base, PdfName key,  PdfObject *tomerge );
		PdfObject* migrateResource(PdfObject * obj);
		void drawLine ( double x, double y, double xx, double yy, std::ostringstream & a );
		void signature ( double x , double y, int sheet, const std::vector<int> & pages, std::ostringstream & a );
		
		// An attempt to allow nested loops
		// returns new position in records list.
		int sortLoop(std::vector<std::string>& memfile, int numline);

		std::string useFont;
		PdfReference useFontRef;
		double extraSpace;

		std::vector<std::string> multiSource;
		
		std::map<std::string, PdfObject*> migrateMap;
	public:
		int pcount;
		double sourceWidth;
		double sourceHeight;
		double destWidth;
		double destHeight;
		double scaleFactor;
		std::string boundingBox;


};

	};}; // end of namespace
#endif
