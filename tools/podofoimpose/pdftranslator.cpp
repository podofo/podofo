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

bool PdfTranslator::checkIsPDF(const string_view& path)
{
    FileStreamDevice device(path);
    PdfVersion version;
    return PdfParser::TryReadHeader(device, version);
}

PdfTranslator::PdfTranslator()
{
    m_scaleFactor = 1;
    m_pageCount = 0;
    m_sourceWidth = 0;
    m_sourceHeight = 0;
    m_destWidth = 0;
    m_destHeight = 0;
}

void PdfTranslator::SetSource(const string_view& source)
{
    if (checkIsPDF(source))
    {
        m_multiSource.push_back((string)source);
    }
    else
    {
        ifstream in = utls::open_ifstream(source, ifstream::in);
        if (!in.good())
            throw runtime_error("setSource() failed to open input file");

        string filename;
        while (true)
        {
            if (!std::getline(in, filename))
            {
                if (in.eof())
                    break;

                throw runtime_error("failed reading line from input file");
            }

            if (utls::IsStringEmptyOrWhiteSpace(filename))
                continue;

            if (filename.size() > 4) // at least ".pdf" because just test if ts is empty doesn't work.
                m_multiSource.push_back(filename);
        }
        in.close();
    }

    if (m_multiSource.empty())
        throw runtime_error("No recognized source given");

    m_sourceDoc.reset(new PdfMemDocument());
    m_sourceDoc->Load(m_multiSource.front());

    for (unsigned i = 1; i < m_multiSource.size(); i++)
    {
        PdfMemDocument doc;
        doc.Load(m_multiSource[i]);
        m_sourceDoc->GetPages().AppendDocumentPages(doc, 0, doc.GetPages().GetCount());
    }

    m_pageCount = m_sourceDoc->GetPages().GetCount();
    if (m_pageCount > 0) // only here to avoid possible segfault, but PDF without page is not conform IIRC
    {
        auto& firstPage = m_sourceDoc->GetPages().GetPageAt(0);

        Rect rect(firstPage.GetMediaBox());
        // keep in mind it’s just a hint since PDF can have different page sizes in a same doc
        m_sourceWidth = rect.Width - rect.X;
        m_sourceHeight = rect.Height - rect.Y;
    }
}

PdfObject* PdfTranslator::migrateResource(PdfObject* obj)
{
    PdfObject* ret = nullptr;

    if (obj == nullptr)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "migrateResource called"
            " with nullptr object");
    }

    switch (obj->GetDataType())
    {
        case PdfDataType::Dictionary:
        {
            if (obj->GetIndirectReference().IsIndirect())
                ret = &m_targetDoc->GetObjects().CreateObject(*obj);
            else
                ret = new PdfObject(*obj);

            for (auto& pair : obj->GetDictionary())
            {
                PdfObject* o = &pair.second;
                auto res = m_setMigrationPending.insert(o);
                if (!res.second)
                {
                    ostringstream oss;
                    oss << "Cycle detected: Object with ref " << o->GetIndirectReference().ToString()
                        << " is already pending migration to the target.\n";
                    PoDoFo::LogMessage(PdfLogSeverity::Warning, oss.str());
                    continue;
                }
                PdfObject* migrated = migrateResource(o);
                if (migrated != nullptr)
                {
                    ret->GetDictionary().AddKey(pair.first, *migrated);
                    if (!(migrated->GetIndirectReference().IsIndirect()))
                        delete migrated;
                }
            }

            if (obj->HasStream())
            {
                *(ret->GetStream()) = *(obj->GetStream());
            }
            break;
        }
        case PdfDataType::Array:
        {
            PdfArray carray(obj->GetArray());
            PdfArray narray;
            for (unsigned ci = 0; ci < carray.GetSize(); ci++)
            {
                PdfObject* co(migrateResource(&carray[ci]));
                if (co == nullptr)
                    continue;

                narray.Add(*co);
                if (!(co->GetIndirectReference().IsIndirect()))
                {
                    delete co;
                }
            }
            if (obj->GetIndirectReference().IsIndirect())
            {
                ret = &m_targetDoc->GetObjects().CreateObject(narray);
            }
            else
            {
                ret = new PdfObject(narray);
            }

            break;
        }
        case PdfDataType::Reference:
        {
            if (m_migrateMap.find(obj->GetReference().ToString()) != m_migrateMap.end())
            {
                ostringstream oss;
                oss << "Referenced object " << obj->GetReference().ToString()
                    << " already migrated." << endl;
                PoDoFo::LogMessage(PdfLogSeverity::Debug, oss.str());

                const PdfObject* const found = m_migrateMap[obj->GetReference().ToString()];
                return new PdfObject(found->GetIndirectReference());
            }

            PdfObject* to_migrate = m_sourceDoc->GetObjects().GetObject(obj->GetReference());

            pair<set<PdfObject*>::iterator, bool> res
                = m_setMigrationPending.insert(to_migrate);
            if (!res.second)
            {
                ostringstream oss;
                oss << "Cycle detected: Object with ref " << obj->GetReference().ToString()
                    << " is already pending migration to the target.\n";
                PoDoFo::LogMessage(PdfLogSeverity::Warning, oss.str());
                return nullptr; // skip this migration
            }
            PdfObject* o(migrateResource(to_migrate));
            if (nullptr != o)
                ret = new PdfObject(o->GetIndirectReference());
            else
                return nullptr; // avoid going through rest of method
            break;
        }
        case PdfDataType::Name:
        {
            ret = &m_targetDoc->GetObjects().CreateObject(obj->GetName());
            break;
        }
        case PdfDataType::Number:
        {
            ret = &m_targetDoc->GetObjects().CreateObject(obj->GetNumber());
            break;
        }
        case PdfDataType::Null:
        {
            ret = &m_targetDoc->GetObjects().CreateDictionaryObject();
            break;
        }
        default:
        {
            ret = new PdfObject(*obj);
            break;
        }
    }

    if (obj->GetIndirectReference().IsIndirect())
    {
        m_migrateMap.insert(pair<string, PdfObject*>(obj->GetIndirectReference().ToString(), ret));
    }

    return ret;
}

PdfObject* PdfTranslator::getInheritedResources(PdfPage& page)
{
    PdfObject* res(0);
    // mabri: resources are inherited as whole dict, not at all if the page has the dict
    // mabri: specified in PDF32000_2008.pdf section 7.7.3.4 Inheritance of Page Attributes
    // mabri: and in section 7.8.3 Resource Dictionaries
    PdfObject* sourceRes = page.GetDictionary().FindKeyParent("Resources");
    if (sourceRes != nullptr)
        res = migrateResource(sourceRes);

    return res;
}

void PdfTranslator::SetTarget(const string_view& target)
{
    if (m_sourceDoc == nullptr)
        throw logic_error("setTarget() called before setSource()");

    m_targetDoc.reset(new PdfMemDocument);
    m_outFilePath = target;

    for (unsigned i = 0; i < m_pageCount; i++)
    {
        auto& page = m_sourceDoc->GetPages().GetPageAt(i);
        charbuff buff;
        BufferStreamDevice outMemStream(buff);

        auto xobj = m_sourceDoc->CreateXObjectForm(page.GetMediaBox());
        if (page.GetContents() != nullptr)
            page.GetContents()->CopyTo(outMemStream);

        /// Its time to manage other keys of the page dictionary.
        vector<string> pageKeys;
        pageKeys.push_back("Group");
        for (auto& key : pageKeys)
        {
            PdfName keyname(key);
            if (page.GetDictionary().HasKey(keyname))
            {
                PdfObject* migObj = migrateResource(page.GetDictionary().GetKey(keyname));
                if (nullptr == migObj)
                    continue;
                xobj->GetDictionary().AddKey(keyname, *migObj);
            }
        }

        outMemStream.Close();

        xobj->GetObject().GetOrCreateStream().SetData(buff);

        m_resources[i + 1] = getInheritedResources(page);
        m_xobjects[i + 1] = std::move(xobj);
        m_cropRect[i + 1] = page.GetCropBox();
        m_bleedRect[i + 1] = page.GetBleedBox();
        m_trimRect[i + 1] = page.GetTrimBox();
        m_artRect[i + 1] = page.GetArtBox();
    }

    m_targetDoc->GetMetadata().SetPdfVersion(m_sourceDoc->GetMetadata().GetPdfVersion());

    auto& sourceMetadata = m_sourceDoc->GetMetadata();
    auto& targetMetadata = m_targetDoc->GetMetadata();

    if (sourceMetadata.GetAuthor().has_value())
        targetMetadata.SetAuthor(*sourceMetadata.GetAuthor());
    if (sourceMetadata.GetCreator().has_value())
        targetMetadata.SetCreator(*sourceMetadata.GetCreator());
    if (sourceMetadata.GetSubject().has_value())
        targetMetadata.SetSubject(*sourceMetadata.GetSubject());
    if (sourceMetadata.GetTitle().has_value())
        targetMetadata.SetTitle(*sourceMetadata.GetTitle());
    if (sourceMetadata.GetKeywords().size() != 0)
        targetMetadata.SetKeywords(sourceMetadata.GetKeywords());
    if (sourceMetadata.GetTrapped().has_value())
        targetMetadata.SetTrapped(*sourceMetadata.GetTrapped());
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

                if (m_resources[resourceIndex] != nullptr)
                {
                    if (m_resources[resourceIndex]->IsDictionary())
                    {
                        for (auto& resPair : m_resources[resourceIndex]->GetDictionary())
                            xo->GetOrCreateResources().GetDictionary().AddKey(resPair.first, resPair.second);
                    }
                    else if (m_resources[resourceIndex]->IsReference())
                    {
                        xo->GetDictionary().AddKey("Resources", *m_resources[resourceIndex]);
                    }
                    else
                    {
                        cerr << "ERROR Unknown type resource " << m_resources[resourceIndex]->GetDataTypeString() << endl;
                    }
                }
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
    m_resources.clear();
}
