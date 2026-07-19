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
    p->SetInputOutput("mydoc.pdf", "myimposeddoc.pdf");
    p->LoadPlan("in4-32p.plan");
    p->Impose();
    */
    class PdfTranslator
    {
    public:
        PdfTranslator();

        /**
        Set the source document(s) to be imposed and the path of the file where the
        imposed PDF document will be saved.
        Argument input is the path of a PDF file, or the path of a file containing a list
        of PDF file paths. Argument output is the path of the file to be written.
        */
        void SetInputOutput(const std::string_view& input, const std::string_view& output);

        /**
        Load an imposition plan file of form:
        widthOfSheet heightOfSheet
        sourcePage destPage rotation translationX translationY
        ...        ...      ...      ...          ...
        */
        void LoadPlan(const std::string_view& planFile, PoDoFo::Impose::PlanReader loader);

        /**
        When all is prepared, call it to do the job.
        */
        void Impose();

    private:
        Matrix transformMatrix;
        void transform(const Matrix& transform);
        void translate(double dx, double dy);
        void scale(double sx, double sy);
        void rotate(double angleDeg);
        void rotateAndTranslate(double angleDeg, double dx, double dy);

    private:
        std::unique_ptr<PdfMemDocument> m_targetDoc;

        std::string m_outFilePath;

        std::unique_ptr<ImpositionPlan> m_planImposition;

        std::map<int, std::unique_ptr<PdfXObjectForm>> m_xobjects;
        std::map<int, Rect> m_cropRect;
        std::map<int, Rect> m_bleedRect;
        std::map<int, Rect> m_trimRect;
        std::map<int, Rect> m_artRect;

        unsigned m_pageCount;
        double m_sourceWidth;
        double m_sourceHeight;
        double m_destWidth;
        double m_destHeight;
        double m_scaleFactor;
        std::string m_boundingBox;
    };

}

#endif // PDFTRANSLATOR_H
