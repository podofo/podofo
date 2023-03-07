/**
 * SPDX-FileCopyrightText: (C) 2007 Pierre Marchand <pierre@moulindetouvois.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PDFTRANSLATOR_H
#define PDFTRANSLATOR_H

#include <podofo/podofo.h>
#include "impositionplan.h"

#include <string>
#include <map>
#include <set>
#include <vector>
#include <sstream>
#include <istream>
#include <string>

namespace PoDoFo::Impose
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

        PdfMemDocument* sourceDoc;
        PdfMemDocument* targetDoc;

        /**
        Set the source document(s) to be imposed.
        Argument source is the path of the PDF file, or the path of a file containing a list of paths of PDF files...
        */
        void setSource(const std::string& source);

        /**
        Another way to set many files as source document.
        Note that a source must be set before you call addToSource().
        */
        void addToSource(const std::string& source);

        /**
        Set the path of the file where the imposed PDF doc will be save.
        */
        void setTarget(const std::string& target);

        /**
        Load an imposition plan file of form:
        widthOfSheet heightOfSheet
        sourcePage destPage rotation translationX translationY
        ...        ...      ...      ...          ...
        */
        void loadPlan(const std::string& planFile, PoDoFo::Impose::PlanReader loader);

        /**
        When all is prepared, call it to do the job.
        */
        void impose();

    private:
        std::string inFilePath;
        std::string outFilePath;

        PdfReference globalResRef;

        ImpositionPlan* planImposition;

        std::map<int, PdfXObjectForm*> xobjects;
        std::map<int, PdfObject*> resources;
        std::map<int, Rect> cropRect;
        std::map<int, Rect> bleedRect;
        std::map<int, Rect> trimRect;
        std::map<int, Rect> artRect;
        std::map<int, PdfDictionary*> pDict;
        std::map<int, int> virtualMap;
        // 		int maxPageDest;
        int duplicate;

        bool checkIsPDF(std::string path);
        PdfObject* getInheritedResources(PdfPage& page);
        void mergeResKey(PdfObject* base, PdfName key, PdfObject* tomerge);
        PdfObject* migrateResource(PdfObject* obj);
        void drawLine(double x, double y, double xx, double yy, std::ostringstream& a);
        void signature(double x, double y, int sheet, const std::vector<int>& pages, std::ostringstream& a);

        // An attempt to allow nested loops
        // returns new position in records list.
        int sortLoop(std::vector<std::string>& memfile, int numline);

        std::string useFont;
        PdfReference useFontRef;
        double extraSpace;

        std::vector<std::string> multiSource;

        std::map<std::string, PdfObject*> migrateMap;
        std::set<PdfObject*> setMigrationPending;

        std::vector<double> transformMatrix;
        void transform(double a, double b, double c, double d, double e, double f);
        void translate(double dx, double dy);
        void scale(double sx, double sy);
        void rotate(double theta);
        void rotate_and_translate(double theta, double dx, double dy);
    public:
        unsigned pageCount;
        double sourceWidth;
        double sourceHeight;
        double destWidth;
        double destHeight;
        double scaleFactor;
        std::string boundingBox;
    };

}

#endif // PDFTRANSLATOR_H
