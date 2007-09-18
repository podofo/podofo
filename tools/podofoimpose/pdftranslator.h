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

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <istream>

class QByteArray ;

using namespace PoDoFo;

/**
  @author Pierre Marchand <pierre@moulindetouvois.com>
  */
struct PageRecord
{
    int sourcePage;
    int xobjIndex;
    int destPage;
    double rotate;
    double transX;
    double transY;
    PageRecord ( int s,int d,double r, double tx, double ty );
    PageRecord();
    bool isValid() const;
    friend std::istream & operator >> (std::istream&, PageRecord&);
    friend std::ostream & operator << (std::ostream&, const PageRecord&);
};

class PdfTranslator
{
    public:
        PdfTranslator(double sp);

        ~PdfTranslator() { }

        PdfMemDocument *sourceDoc;
        PdfMemDocument *targetDoc;

        void setSource ( const std::string & source );
        void setTarget ( const std::string & target );
        void loadPlan ( const std::string & plan );
        void impose();

    private:
        std::string inFilePath;
        std::string outFilePath;
        int pcount;

        PdfReference globalResRef;

        std::vector<PageRecord> planImposition;
        std::map<int, int> pagesIndex;
        std::map<int, PdfXObject*> xobjects;
        std::map<int, PdfRect> trimRect;
        std::map<int,PdfRect> bleedRect;
        std::map<int, PdfDictionary*> pDict;
        std::map<int, int> virtualMap;
        double destWidth;
        double destHeight;
        int maxPageDest;

        void drawLine(double x, double y, double xx, double yy, std::ostringstream & a );
        void signature(double x , double y, int sheet, const std::vector<int> & pages, std::ostringstream & a );

        std::string useFont;
        PdfReference useFontRef;
        double extraSpace;
};

#endif
