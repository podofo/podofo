/**
 * SPDX-FileCopyrightText: (C) 2007 Pierre Marchand <pierre@moulindetouvois.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "pdftranslator.h"

#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <istream>
#include <ostream>
#include <cstdlib>
#include <iostream>

#include "planreader_legacy.h"

#ifdef PODOFO_HAVE_LUA
#include "planreader_lua.h"
#endif

#define MAX_SOURCE_PAGES 5000
#define MAX_RECORD_SIZE 2048

using namespace std;
using namespace PoDoFo;
using namespace PoDoFo::Impose;

bool PdfTranslator::checkIsPDF(string path)
{
    ifstream in(path.c_str(), ifstream::in);
    if (!in.good())
        throw runtime_error("setSource() failed to open input file");

    const int magicBufferLen = 5;
    char magicBuffer[magicBufferLen];
    in.read(magicBuffer, magicBufferLen);
    string magic(magicBuffer, magicBufferLen);

    in.close();
    if (magic.find("%PDF") < 5)
        return true;
    // 			throw runtime_error("First bytes of the file tend to indicate it is not a PDF file");
    return false;
}

PdfTranslator::PdfTranslator()
{
    cerr << "PdfTranslator::PdfTranslator" << endl;
    sourceDoc = nullptr;
    targetDoc = nullptr;
    planImposition = nullptr;
    duplicate = 0;
    extraSpace = 0;
    scaleFactor = 1.0;
    pageCount = 0;
    sourceWidth = 0.0;
    sourceHeight = 0.0;
    destWidth = 0.0;
    destHeight = 0.0;
}

void PdfTranslator::setSource(const string& source)
{
    int dbg = 0;
    // 			cerr<<"PdfTranslator::setSource "<<source<<endl;
    cerr << ++dbg << endl;
    if (checkIsPDF(source))
    {
        // 		cerr << "Appending "<<source<<" to source" << endl;
        multiSource.push_back(source);
    }
    else
    {
        ifstream in(source.c_str(), ifstream::in);
        if (!in.good())
            throw runtime_error("setSource() failed to open input file");

        char* filenameBuffer = new char[1000];
        do
        {
            if (!in.getline(filenameBuffer, 1000))
                throw runtime_error("failed reading line from input file");

            string ts(filenameBuffer, (size_t)in.gcount());
            if (ts.size() > 4) // at least ".pdf" because just test if ts is empty doesn't work.
            {
                multiSource.push_back(ts);
                cerr << "Appending " << ts << " to source" << endl;
            }
        } while (!in.eof());
        in.close();
        delete[] filenameBuffer;
    }
    cerr << ++dbg << endl;

    if (multiSource.empty())
        throw runtime_error("No recognized source given");

    for (vector<string>::const_iterator ms = multiSource.begin(); ms != multiSource.end(); ms++)
    {
        if (ms == multiSource.begin())
        {
            // 					cerr << "First doc is "<< (*ms).c_str()   << endl;
            try
            {
                sourceDoc = new PdfMemDocument();
                sourceDoc->Load(*ms);
            }
            catch (PdfError& e)
            {
                cerr << "Unable to create Document: " << PdfError::ErrorMessage(e.GetCode()) << endl;
                return;
            }
        }
        else
        {
            PdfMemDocument mdoc;
            mdoc.Load(*ms);
            // 			cerr << "Appending "<< mdoc.GetPageCount() << " page(s) of " << *ms  << endl;
            sourceDoc->GetPages().AppendDocumentPages(mdoc, 0, mdoc.GetPages().GetCount());
        }
    }

    pageCount = sourceDoc->GetPages().GetCount();
    // 	cerr << "Document has "<< pcount << " page(s) " << endl;
    if (pageCount > 0) // only here to avoid possible segfault, but PDF without page is not conform IIRC
    {
        auto& firstPage = sourceDoc->GetPages().GetPageAt(0);

        Rect rect(firstPage.GetMediaBox());
        // keep in mind it’s just a hint since PDF can have different page sizes in a same doc
        sourceWidth = rect.Width - rect.X;
        sourceHeight = rect.Height - rect.Y;
    }
}

void PdfTranslator::addToSource(const string& source)
{
    // 			cerr<<"PdfTranslator::addToSource "<< source<<endl;
    if (!sourceDoc)
        return;

    PdfMemDocument extraDoc;
    extraDoc.Load(source);
    sourceDoc->GetPages().AppendDocumentPages(extraDoc, 0, extraDoc.GetPages().GetCount());
    multiSource.push_back(source);

}

PdfObject* PdfTranslator::migrateResource(PdfObject* obj)
{
    // 			cerr<<"PdfTranslator::migrateResource"<<endl;
    PdfObject* ret = nullptr;

    if (!obj)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "migrateResource called"
            " with nullptr object");

    if (obj->IsDictionary())
    {
        if (obj->GetIndirectReference().IsIndirect())
            ret = &targetDoc->GetObjects().CreateObject(*obj);
        else
            ret = new PdfObject(*obj);

        for (auto& pair : obj->GetDictionary())
        {
            PdfObject* o = &pair.second;
            auto res = setMigrationPending.insert(o);
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
    }
    else if (obj->IsArray())
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
            ret = &targetDoc->GetObjects().CreateObject(narray);
        }
        else
        {
            ret = new PdfObject(narray);
        }
    }
    else if (obj->IsReference())
    {
        if (migrateMap.find(obj->GetReference().ToString()) != migrateMap.end())
        {
            ostringstream oss;
            oss << "Referenced object " << obj->GetReference().ToString()
                << " already migrated." << endl;
            PoDoFo::LogMessage(PdfLogSeverity::Debug, oss.str());

            const PdfObject* const found = migrateMap[obj->GetReference().ToString()];
            return new PdfObject(found->GetIndirectReference());
        }

        PdfObject* to_migrate = sourceDoc->GetObjects().GetObject(obj->GetReference());

        pair<set<PdfObject*>::iterator, bool> res
            = setMigrationPending.insert(to_migrate);
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
    }
    else if (obj->IsName())
    {
        ret = &targetDoc->GetObjects().CreateObject(obj->GetName());
    }
    else if (obj->IsNumber())
    {
        ret = &targetDoc->GetObjects().CreateObject(obj->GetNumber());
    }
    else if (obj->IsNull())
    {
        ret = &targetDoc->GetObjects().CreateDictionaryObject();
    }
    else
    {
        ret = new PdfObject(*obj);//targetDoc->GetObjects().CreateObject(*obj);
    }

    if (obj->GetIndirectReference().IsIndirect())
    {
        migrateMap.insert(pair<string, PdfObject*>(obj->GetIndirectReference().ToString(), ret));
    }

    return ret;
}

PdfObject* PdfTranslator::getInheritedResources(PdfPage& page)
{
    // 			cerr<<"PdfTranslator::getInheritedResources"<<endl;
    PdfObject* res(0);
    // mabri: resources are inherited as whole dict, not at all if the page has the dict
    // mabri: specified in PDF32000_2008.pdf section 7.7.3.4 Inheritance of Page Attributes
    // mabri: and in section 7.8.3 Resource Dictionaries
    PdfObject* sourceRes = page.GetDictionary().FindKeyParent("Resources");
    if (sourceRes)
    {
        res = migrateResource(sourceRes);
    }
    return res;
}

void PdfTranslator::setTarget(const string& target)
{
    // 			cerr<<"PdfTranslator::setTarget "<<target<<endl;
    if (!sourceDoc)
        throw logic_error("setTarget() called before setSource()");

    targetDoc = new PdfMemDocument;
    outFilePath = target;

    for (unsigned i = 0; i < pageCount; i++)
    {
        auto& page = sourceDoc->GetPages().GetPageAt(i);
        charbuff buff;
        BufferStreamDevice outMemStream(buff);


        auto xobj = sourceDoc->CreateXObjectForm(page.GetMediaBox());
        if (page.GetContents() != nullptr)
            page.GetContents()->CopyTo(outMemStream);

        /// Its time to manage other keys of the page dictionary.
        vector<string> pageKeys;
        vector<string>::const_iterator itKey;
        pageKeys.push_back("Group");
        for (itKey = pageKeys.begin(); itKey != pageKeys.end(); itKey++)
        {
            PdfName keyname(*itKey);
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

        resources[i + 1] = getInheritedResources(page);
        xobjects[i + 1] = xobj.get();
        cropRect[i + 1] = page.GetCropBox();
        bleedRect[i + 1] = page.GetBleedBox();
        trimRect[i + 1] = page.GetTrimBox();
        artRect[i + 1] = page.GetArtBox();
    }

    targetDoc->GetMetadata().SetPdfVersion(sourceDoc->GetMetadata().GetPdfVersion());

    auto& sourceMetadata = sourceDoc->GetMetadata();
    auto& targetMetadata = targetDoc->GetMetadata();

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

    // 	PdfObject *scat( sourceDoc->GetCatalog() );
    // 	PdfObject *tcat( targetDoc->GetCatalog() );
    // 	TKeyMap catmap = scat->GetDictionary().GetKeys();
    // 	for ( TCIKeyMap itc = catmap.begin(); itc != catmap.end(); ++itc )
    // 	{
    // 		if(tcat->GetDictionary().GetKey(itc->first) == 0)
    // 		{
    // 			PdfObject *o = itc->second;
    // 			tcat->GetDictionary().AddKey (itc->first , migrateResource( o ) );
    // 		}
    // 	}

    // 	delete sourceDoc;
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
    else {
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

void PdfTranslator::loadPlan(const string& planFile, PlanReader loader)
{
    // 			cerr<< "loadPlan" << planFile<<endl;
    SourceVars sv;
    sv.PageCount = pageCount;
    sv.PageHeight = sourceHeight;
    sv.PageWidth = sourceWidth;
    planImposition = new ImpositionPlan(sv);
    if (loader == PlanReader::Legacy)
    {
        PlanReader_Legacy(planFile, planImposition);
    }
#if defined(PODOFO_HAVE_LUA)
    else if (loader == PlanReader::Lua)
    {
        PlanReader_Lua(planFile, planImposition);
    }
#endif

    if (!planImposition->valid())
        throw runtime_error("Unable to build a valid imposition plan");

    destWidth = planImposition->destWidth();
    destHeight = planImposition->destHeight();
    scaleFactor = planImposition->scale();
    boundingBox = planImposition->boundingBox();
    // 	cerr <<"Plan completed "<< planImposition.size() <<endl;

}

void PdfTranslator::impose()
{
    // 			cerr<<"PdfTranslator::impose"<<endl;
    if (!targetDoc)
        throw invalid_argument("impose() called with empty target");

    //			PdfObject trimbox;
    //			Rect trim ( 0, 0, destWidth, destHeight );
    //			trim.ToVariant ( trimbox );
    map<int, Rect>* bbIndex = nullptr;
    if (boundingBox.size() > 0)
    {
        if (boundingBox.find("crop") != string::npos)
        {
            bbIndex = &cropRect;
        }
        else if (boundingBox.find("bleed") != string::npos)
        {
            bbIndex = &bleedRect;
        }
        else if (boundingBox.find("trim") != string::npos)
        {
            bbIndex = &trimRect;
        }
        else if (boundingBox.find("art") != string::npos)
        {
            bbIndex = &artRect;
        }
    }

    typedef map<int, vector<PageRecord> > groups_t;
    groups_t groups;
    for (unsigned i = 0; i < planImposition->size(); i++)
    {
        groups[(*planImposition)[i].destPage].push_back((*planImposition)[i]);
    }

    unsigned int lastPlate(0);
    groups_t::const_iterator  git = groups.begin();
    const groups_t::const_iterator gitEnd = groups.end();
    while (git != gitEnd)
    {
        PdfPage* newpage = nullptr;
        // Allow "holes" in dest. pages sequence.
        unsigned int curPlate(git->first);
        while (lastPlate != curPlate)
        {
            newpage = &targetDoc->GetPages().CreatePage(Rect(0.0, 0.0, destWidth, destHeight));
            lastPlate++;
        }
        // 		newpage->GetObject()->GetDictionary().AddKey ( PdfName ( "TrimBox" ), trimbox );
        PdfDictionary xdict;

        ostringstream buffer;
        // Scale
        buffer << fixed << scaleFactor << " 0 0 " << scaleFactor << " 0 0 cm\n";

        for (unsigned int i = 0; i < git->second.size(); i++)
        {
            PageRecord curRecord(git->second[i]);
            // 					cerr<<curRecord.sourcePage<< " " << curRecord.destPage<<endl;
            if (curRecord.sourcePage <= pageCount)
            {
                double rot = curRecord.rotate;
                double tx = curRecord.transX;
                double ty = curRecord.transY;
                double sx = curRecord.scaleX;
                double sy = curRecord.scaleY;

                int resourceIndex( /*(curRecord.duplicateOf > 0) ? curRecord.duplicateOf : */curRecord.sourcePage);
                PdfXObjectForm* xo = xobjects[resourceIndex];
                if (nullptr != bbIndex)
                {
                    PdfArray bb;
                    // DominikS: Fix compilation using Visual Studio on Windows
                    // mabri: ML post archive URL is https://sourceforge.net/p/podofo/mailman/message/24609746/
                    // bbIndex->at(resourceIndex).ToVariant( bb );							
                    ((*bbIndex)[resourceIndex]).ToArray(bb);
                    xo->GetDictionary().AddKey("BBox", bb);
                }
                ostringstream op;
                op << "OriginalPage" << resourceIndex;
                xdict.AddKey(PdfName(op.str()), xo->GetObject().GetIndirectReference());

                if (resources[resourceIndex])
                {
                    if (resources[resourceIndex]->IsDictionary())
                    {
                        for (auto& pair : resources[resourceIndex]->GetDictionary())
                        {
                            xo->GetOrCreateResources().GetDictionary().AddKey(pair.first, pair.second);
                        }
                    }
                    else if (resources[resourceIndex]->IsReference())
                    {
                        xo->GetDictionary().AddKey("Resources", *resources[resourceIndex]);
                    }
                    else
                    {
                        cerr << "ERROR Unknown type resource " << resources[resourceIndex]->GetDataTypeString() << endl;
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

        if (!newpage)
            PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

        string bufStr = buffer.str();
        newpage->GetOrCreateContents().CreateStreamForAppending().SetData(bufStr);
        newpage->GetResources().GetDictionary().AddKey(PdfName("XObject"), xdict);
        git++;
    }

    targetDoc->Save(outFilePath);

    // The following is necessary to avoid line 195 being detected as allocation having a memory leak
    // without changing other files than this one (thorough leak prevention shall be applied later).
    for (map<int, PdfObject*>::iterator it = resources.begin(); it != resources.end(); it++)
        delete (*it).second;

    resources.clear();
}
