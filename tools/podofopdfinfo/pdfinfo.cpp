/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "pdfinfo.h"
#include <sstream>

using namespace std;
using namespace PoDoFo;

PdfInfoHelper::PdfInfoHelper(const string& filepath)
{
    m_doc = new PdfMemDocument();
    m_doc->Load(filepath);
}

PdfInfoHelper::~PdfInfoHelper()
{
    if (m_doc)
    {
        delete m_doc;
        m_doc = nullptr;
    }
}

void PdfInfoHelper::OutputDocumentInfo(ostream& sOutStream)
{
    sOutStream << "\tPDF Version: " << PoDoFo::GetPdfVersionName(m_doc->GetMetadata().GetPdfVersion()).GetString() << endl;
    sOutStream << "\tPage Count: " << m_doc->GetPages().GetCount() << endl;
    sOutStream << "\tPage Size: " << GuessFormat() << endl;
    sOutStream << endl;
    sOutStream << "\tTagged: " << (m_doc->GetCatalog().GetStructTreeRootObject() != nullptr ? "Yes" : "No") << endl;
    sOutStream << "\tEncrypted: " << (m_doc->GetEncrypt() != nullptr ? "Yes" : "No") << endl;
    sOutStream << "\tPrinting Allowed: " << (m_doc->IsPrintAllowed() ? "Yes" : "No") << endl;
    sOutStream << "\tModification Allowed: " << (m_doc->IsEditAllowed() ? "Yes" : "No") << endl;
    sOutStream << "\tCopy&Paste Allowed: " << (m_doc->IsCopyAllowed() ? "Yes" : "No") << endl;
    sOutStream << "\tAdd/Modify Annotations Allowed: " << (m_doc->IsEditNotesAllowed() ? "Yes" : "No") << endl;
    sOutStream << "\tFill&Sign Allowed: " << (m_doc->IsFillAndSignAllowed() ? "Yes" : "No") << endl;
    sOutStream << "\tAccessibility Allowed: " << (m_doc->IsAccessibilityAllowed() ? "Yes" : "No") << endl;
    sOutStream << "\tDocument Assembly Allowed: " << (m_doc->IsDocAssemblyAllowed() ? "Yes" : "No") << endl;
    sOutStream << "\tHigh Quality Print Allowed: " << (m_doc->IsHighPrintAllowed() ? "Yes" : "No") << endl;
}

void PdfInfoHelper::OutputInfoDict(ostream& outStream)
{
    if (m_doc->GetInfo() == nullptr)
    {
        outStream << "No info dictionary in this PDF file!" << endl;
    }
    else
    {
        auto author = m_doc->GetInfo()->GetAuthor();
        if (author != nullptr)
            outStream << "\tAuthor: " << author->GetString() << endl;

        auto creator = m_doc->GetInfo()->GetCreator();
        if (creator != nullptr)
            outStream << "\tCreator: " << creator->GetString() << endl;

        auto subject = m_doc->GetInfo()->GetSubject();
        if (subject != nullptr)
            outStream << "\tSubject: " << subject->GetString() << endl;

        auto title = m_doc->GetInfo()->GetTitle();
        if (title != nullptr)
            outStream << "\tTitle: " << title->GetString() << endl;

        auto keywords = m_doc->GetInfo()->GetKeywords();
        if (keywords != nullptr)
            outStream << "\tKeywords: " << keywords->GetString() << endl;

        auto trapped = m_doc->GetInfo()->GetTrapped();
        if (trapped != nullptr)
            outStream << "\tTrapped: " << trapped->GetEscapedName() << endl;
    }
}

void PdfInfoHelper::OutputPageInfo(ostream& outstream)
{
    PdfArray arr;
    string str;

    unsigned annotCount;
    unsigned pageCount = m_doc->GetPages().GetCount();
    outstream << "Page Count: " << pageCount << endl;
    for (unsigned pg = 0; pg < pageCount; pg++)
    {
        outstream << "Page " << pg << ":" << endl;

        auto& curPage = m_doc->GetPages().GetPageAt(pg);
        outstream << "->Internal Number:" << curPage.GetPageNumber() << endl;
        outstream << "->Object Number:" << curPage.GetObject().GetIndirectReference().ObjectNumber()
            << " " << curPage.GetObject().GetIndirectReference().GenerationNumber() << " R" << endl;

        curPage.GetMediaBox().ToArray(arr);
        arr.ToString(str);

        annotCount = curPage.GetAnnotations().GetCount();
        outstream << "\tMediaBox: " << str << endl;
        outstream << "\tRotation: " << curPage.GetRotation() << endl;
        outstream << "\t# of Annotations: " << annotCount << endl;

        for (unsigned i = 0; i < annotCount; i++)
        {
            auto& curAnnot = curPage.GetAnnotations().GetAnnotAt(i);

            curAnnot.GetRect().ToArray(arr);
            arr.ToString(str);

            outstream << endl;
            outstream << "\tAnnotation " << i << endl;
            outstream << "\t\tType: " << PoDoFo::ToString(curAnnot.GetType()) << endl;
            auto contents = curAnnot.GetContents();
            if (contents != nullptr)
                outstream << "\t\tContents: " << contents->GetString() << endl;

            auto title = curAnnot.GetTitle();
            if (title != nullptr)
                outstream << "\t\tTitle: " << title->GetString() << endl;

            outstream << "\t\tFlags: " << (int)curAnnot.GetFlags() << endl;
            outstream << "\t\tRect: " << str << endl;
            if (curAnnot.GetType() == PdfAnnotationType::Link)
            {
                auto action = static_cast<PdfAnnotationLink&>(curAnnot).GetAction();
                if (action != nullptr && action->GetType() == PdfActionType::URI)
                {
                    auto uri = static_cast<PdfActionURI&>(*action).GetURI();
                    if (uri != nullptr)
                        outstream << "\t\tAction URI: " << uri->GetString() << endl;
                }
            }
        }
    }
}

void PdfInfoHelper::OutputOutlines(ostream& outstream, PdfOutlineItem* item, int level)
{
    if (item == nullptr)
    {
        auto outlines = m_doc->GetOutlines();
        if (outlines == nullptr || !outlines->First())
        {
            outstream << "\tNone Found" << endl;
            return;
        }
        item = outlines->First();
    }

    for (int i = 0; i < level; i++)
        outstream << "-";

    outstream << ">" << item->GetTitle().GetString();
    auto dest = item->GetDestination();
    if (dest == nullptr)
    {
        // then it's one or more actions
        outstream << "\tAction: " << "???";
    }
    else
    {
        // then it's a destination
        auto page = dest->GetPage();
        if (page == nullptr)
            outstream << "\tDestination: Page #" << "???";
        else
            outstream << "\tDestination: Page #" << page->GetPageNumber();
    }

    outstream << endl;

    if (item->First())
        this->OutputOutlines(outstream, item->First(), level + 1);

    if (item->Next())
        this->OutputOutlines(outstream, item->Next(), level);
}

void PdfInfoHelper::OutputOneName(ostream& outStream, PdfNameTrees& names,
    PdfKnownNameTree treeName, const string_view& title)
{
    outStream << "\t" << title << endl;
    PdfStringMap<PdfObject> dict;
    names.ToDictionary(treeName, dict);

    string str;
    for (auto& pair : dict)
    {
        pair.second.ToString(str);
        outStream << "\t-> " << pair.first.GetString() << "=" << str << endl;
    }

    outStream << endl;
}

void PdfInfoHelper::OutputNames(ostream& outStream)
{
    auto nameTree = m_doc->GetNames();
    if (nameTree == nullptr)
    {
        outStream << "\t\tNone Found" << endl;
    }
    else
    {
        OutputOneName(outStream, *nameTree, PdfKnownNameTree::Dests, "Destinations");
        OutputOneName(outStream, *nameTree, PdfKnownNameTree::JavaScript, "JavaScripts");
        OutputOneName(outStream, *nameTree, PdfKnownNameTree::EmbeddedFiles, "Embedded Files");
    }
}

string PdfInfoHelper::GuessFormat()
{
    using Format = pair<double, double>;

    unsigned pageCount = m_doc->GetPages().GetCount();
    map<Format, int> sizes;
    map<Format, int>::iterator it;
    Rect rect;
    for (unsigned i = 0; i < pageCount; i++)
    {
        auto& currPage = m_doc->GetPages().GetPageAt(i);
        rect = currPage.GetMediaBox();
        Format s(rect.Width - rect.X, rect.Height - rect.Y);
        it = sizes.find(s);
        if (it == sizes.end())
            sizes.insert(pair<Format, int>(s, 1));
        else
            it->second++;
    }

    Format format;
    stringstream ss;
    if (sizes.size() == 1)
    {
        format = sizes.begin()->first;
        ss << format.first << " x " << format.second << " pts";
    }
    else
    {
        // We’re looking for the most represented format
        int max = 0;
        for (it = sizes.begin(); it != sizes.end(); ++it)
        {
            if (it->second > max)
            {
                max = it->second;
                format = it->first;
            }
        }
        ss << format.first << " x " << format.second << " pts " << string(sizes.size(), '*');
    }

    return ss.str();
}
