/**
 * SPDX-FileCopyrightText: (C) 2007 Pierre Marchand <pierre@moulindetouvois.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include <podofo/private/PdfParser.h>

#include "pdftranslator.h"
#include "planreader_legacy.h"

#ifdef PODOFO_HAVE_LUA
#include "planreader_lua.h"
#endif

#define MAX_SOURCE_PAGES 5000
#define MAX_RECORD_SIZE 2048

using namespace std;
using namespace PoDoFo;
using namespace PoDoFo::Impose;

static bool checkIsPDF(const string_view& path);
static Rect toFormSpace(const Rect& rect, unsigned rotation);

PdfTranslator::PdfTranslator()
{
    m_scaleFactor = 1;
    m_pageCount = 0;
    m_sourceWidth = 0;
    m_sourceHeight = 0;
    m_destWidth = 0;
    m_destHeight = 0;
}

void PdfTranslator::SetInputOutput(const string_view& input, const string_view& output)
{
    vector<string> sources;
    if (checkIsPDF(input))
    {
        sources.emplace_back(input);
    }
    else
    {
        ifstream in = utls::open_ifstream(input, ifstream::in);
        if (!in.good())
            throw runtime_error("SetInputOutput() failed to open input file");

        string filename;
        while (true)
        {
            if (std::getline(in, filename).fail())
            {
                if (in.eof())
                    break;

                throw runtime_error("failed reading line from input file");
            }

            if (utls::IsStringEmptyOrWhiteSpace(filename))
                continue;

            if (filename.size() > 4) // at least ".pdf" because just test if ts is empty doesn't work.
                sources.push_back(filename);
        }
        in.close();
    }

    if (sources.empty())
        throw runtime_error("No recognized source given");

    m_outFilePath = output;
    m_pageCount = 0;

    PdfObjectRelocationMap map;
    for (size_t i1 = 0; i1 < sources.size(); i1++)
    {
        const bool isFirst = (i1 == 0);
        PdfMemDocument tmpDoc;
        PdfMemDocument* workingDoc;

        if (isFirst)
        {
            m_targetDoc.reset(new PdfMemDocument());
            m_targetDoc->Load(sources[i1]);
            m_targetDoc->CollectGarbage();
            workingDoc = m_targetDoc.get();
        }
        else
        {
            tmpDoc.Load(sources[i1]);
            workingDoc = &tmpDoc;
        }

        unsigned pageCount = workingDoc->GetPages().GetCount();

        if (isFirst && pageCount > 0)
        {
            // Keep in mind it's just a hint since PDF can have different page sizes in a same doc
            Rect rect = workingDoc->GetPages().GetPageAt(0).GetMediaBox();
            m_sourceWidth = rect.Width - rect.X;
            m_sourceHeight = rect.Height - rect.Y;
        }

        for (unsigned i2 = 0; i2 < pageCount; i2++)
        {
            auto& page = workingDoc->GetPages().GetPageAt(i2);

            auto xobj = m_targetDoc->CreateXObjectForm(Rect());
            xobj->FillFromPage(page, &map);

            unsigned key = m_pageCount + 1;
            unsigned rot = page.GetRotation();
            m_cropRect[key] = toFormSpace(page.GetCropBox(), rot);
            m_bleedRect[key] = toFormSpace(page.GetBleedBox(), rot);
            m_trimRect[key] = toFormSpace(page.GetTrimBox(), rot);
            m_artRect[key] = toFormSpace(page.GetArtBox(), rot);
            m_xobjects[key] = std::move(xobj);

            m_pageCount++;
        }

        if (isFirst)
        {
            // The original pages of the first doc have been captured as xobjects
            // and are no longer needed in the page tree
            for (unsigned i2 = pageCount; i2 > 0; i2--)
                m_targetDoc->GetPages().RemovePageAt(i2 - 1);
        }

        map.Clear();
    }
}

void PdfTranslator::transform(double a, double b, double c, double d, double e, double f)
{
    if (transformMatrix.empty()) {
        transformMatrix.push_back(a);
        transformMatrix.push_back(b);
        transformMatrix.push_back(c);
        transformMatrix.push_back(d);
        transformMatrix.push_back(e);
        transformMatrix.push_back(f);
    }
    else
    {
        vector<double> m0 = transformMatrix;
        vector<double> m;

        m.push_back(m0.at(0) * a + m0.at(1) * c);
        m.push_back(m0.at(0) * b + m0.at(1) * d);
        m.push_back(m0.at(2) * a + m0.at(3) * c);
        m.push_back(m0.at(2) * b + m0.at(3) * d);

        m.push_back(m0.at(4) * a + m0.at(5) * c + e);
        m.push_back(m0.at(4) * b + m0.at(5) * d + f);

        transformMatrix = m;
    }
}

void PdfTranslator::rotate_and_translate(double theta, double dx, double dy)
{
    double cosR = cos(theta * 3.14159 / 180.0);
    double sinR = sin(theta * 3.14159 / 180.0);
    transform(cosR, sinR, -sinR, cosR, dx, dy);
}

void PdfTranslator::translate(double dx, double dy)
{
    transform(1, 0, 0, 1, dx, dy);
}

void PdfTranslator::scale(double sx, double sy)
{
    transform(sx, 0, 0, sy, 0, 0);
}

void PdfTranslator::rotate(double theta)
{
    double cosR = cos(theta * 3.14159 / 180.0);
    double sinR = sin(theta * 3.14159 / 180.0);
    // Counter-clockwise rotation (default):
    transform(cosR, sinR, -sinR, cosR, 0, 0);
    // Clockwise rotation:
    // transform(cosR, -sinR, sinR, cosR, 0, 0);
}

void PdfTranslator::LoadPlan(const string_view& planFile, PlanReader loader)
{
    SourceVars sv;
    sv.PageCount = m_pageCount;
    sv.PageHeight = m_sourceHeight;
    sv.PageWidth = m_sourceWidth;
    m_planImposition.reset(new ImpositionPlan(sv));
    if (loader == PlanReader::Legacy)
    {
        PlanReader_Legacy(planFile, *m_planImposition);
    }
#if defined(PODOFO_HAVE_LUA)
    else if (loader == PlanReader::Lua)
    {
        PlanReader_Lua(planFile, *m_planImposition);
    }
#endif

    if (!m_planImposition->valid())
        throw runtime_error("Unable to build a valid imposition plan");

    m_destWidth = m_planImposition->destWidth();
    m_destHeight = m_planImposition->destHeight();
    m_scaleFactor = m_planImposition->scale();
    m_boundingBox = m_planImposition->boundingBox();

}

void PdfTranslator::Impose()
{
    if (m_targetDoc == nullptr)
        throw invalid_argument("impose() called with empty target");

    map<int, Rect>* bbIndex = nullptr;
    if (m_boundingBox.size() > 0)
    {
        if (m_boundingBox.find("crop") != string::npos)
            bbIndex = &m_cropRect;
        else if (m_boundingBox.find("bleed") != string::npos)
            bbIndex = &m_bleedRect;
        else if (m_boundingBox.find("trim") != string::npos)
            bbIndex = &m_trimRect;
        else if (m_boundingBox.find("art") != string::npos)
            bbIndex = &m_artRect;
    }

    typedef map<int, vector<PageRecord> > groups_t;
    groups_t groups;
    for (unsigned i = 0; i < m_planImposition->size(); i++)
        groups[(*m_planImposition)[i].destPage].push_back((*m_planImposition)[i]);

    unsigned lastPlate = 0;
    for (auto& pair : groups)
    {
        PdfPage* newpage = nullptr;
        // Allow "holes" in dest. pages sequence.
        unsigned curPlate = pair.first;
        while (lastPlate != curPlate)
        {
            newpage = &m_targetDoc->GetPages().CreatePage(Rect(0.0, 0.0, m_destWidth, m_destHeight));
            lastPlate++;
        }

        PdfDictionary xdict;

        ostringstream buffer;
        // Scale
        buffer << fixed << m_scaleFactor << " 0 0 " << m_scaleFactor << " 0 0 cm\n";

        for (unsigned i = 0; i < pair.second.size(); i++)
        {
            PageRecord curRecord(pair.second[i]);
            if (curRecord.sourcePage <= m_pageCount)
            {
                double rot = curRecord.rotate;
                double tx = curRecord.transX;
                double ty = curRecord.transY;
                double sx = curRecord.scaleX;
                double sy = curRecord.scaleY;

                int resourceIndex = curRecord.sourcePage;
                auto& xo = m_xobjects[resourceIndex];
                if (nullptr != bbIndex)
                {
                    PdfArray bb;
                    ((*bbIndex)[resourceIndex]).ToArray(bb);
                    xo->GetDictionary().AddKey("BBox", bb);
                }
                ostringstream op;
                op << "OriginalPage" << resourceIndex;
                xdict.AddKey(PdfName(op.str()), xo->GetObject().GetIndirectReference());

                // Make sure we start with an empty transformMatrix.
                transformMatrix.clear();
                translate(0, 0);
                // 1. Rotate, 2. Translate, 3. Scale
                if (rot != 0 || tx != 0 || ty != 0) {
                    rotate_and_translate(rot, tx, ty);
                }
                scale(sx, sy);

                // Very primitive but it makes it easy to track down imposition plan into content stream.
                buffer << "q\n";
                buffer << fixed << transformMatrix[0] << " " << transformMatrix[1] << " " << transformMatrix[2] << " " << transformMatrix[3] << " " << transformMatrix[4] << " " << transformMatrix[5] << " cm\n";
                buffer << "/OriginalPage" << resourceIndex << " Do\n";
                buffer << "Q\n";
            }
        }

        if (newpage == nullptr)
            PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

        string bufStr = buffer.str();
        newpage->GetOrCreateContents().CreateStreamForAppending().SetData(bufStr);
        newpage->GetResources().GetDictionary().AddKey(PdfName("XObject"), xdict);
    }

    m_targetDoc->Save(m_outFilePath);
}

bool checkIsPDF(const string_view& path)
{
    FileStreamDevice device(path);
    PdfVersion version;
    return PdfParser::TryReadHeader(device, version);
}

Rect toFormSpace(const Rect& rect, unsigned rotation)
{
    Rect out = rect;
    if (rotation == 90 || rotation == 270)
        std::swap(out.Width, out.Height);
    return out;
}
