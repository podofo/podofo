/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include <podofo/private/XMPUtils.h>
#include "PdfDocument.h"

#include <algorithm>
#include <deque>

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfImmediateWriter.h"
#include "PdfObjectStream.h"
#include "PdfIndirectObjectList.h"
#include "PdfAcroForm.h"
#include "PdfDestination.h"
#include "PdfFileSpec.h"
#include "PdfFontMetrics.h"
#include "PdfInfo.h"
#include "PdfNameTree.h"
#include "PdfOutlines.h"
#include "PdfPage.h"
#include "PdfPageCollection.h"
#include "PdfXObjectForm.h"
#include "PdfImage.h"

using namespace std;
using namespace PoDoFo;

PdfDocument::PdfDocument(bool empty) :
    m_Objects(*this),
    m_Metadata(*this),
    m_FontManager(*this)
{
    if (!empty)
    {
        m_TrailerObj.reset(new PdfObject()); // The trailer is NO part of the vector of objects
        m_TrailerObj->SetDocument(this);
        auto& catalog = m_Objects.CreateDictionaryObject("Catalog");
        m_Trailer.reset(new PdfTrailer(*m_TrailerObj));

        m_Catalog.reset(new PdfCatalog(catalog));
        m_TrailerObj->GetDictionary().AddKeyIndirect("Root", catalog);

        auto& info = m_Objects.CreateDictionaryObject();
        m_Info.reset(new PdfInfo(info,
            PdfInfoInitial::WriteProducer | PdfInfoInitial::WriteCreationTime));
        m_TrailerObj->GetDictionary().AddKeyIndirect("Info", info);

        Init();
    }
}

PdfDocument::PdfDocument(const PdfDocument& doc) :
    m_Objects(*this, doc.m_Objects),
    m_Metadata(*this),
    m_FontManager(*this)
{
    SetTrailer(std::make_unique<PdfObject>(doc.GetTrailer().GetObject()));
    Init();
}

PdfDocument::~PdfDocument()
{
    // Do nothing, all members will autoclear
}

void PdfDocument::Clear() 
{
    m_FontManager.Clear();
    m_Catalog = nullptr;
    m_Info = nullptr;
    m_Pages = nullptr;
    m_AcroForm = nullptr;
    m_Outlines = nullptr;
    m_NameTree = nullptr;
    m_Objects.Clear();
    m_Objects.SetCanReuseObjectNumbers(true);
}

void PdfDocument::Init()
{
    auto pagesRootObj = m_Catalog->GetDictionary().FindKey("Pages");
    if (pagesRootObj == nullptr)
    {
        m_Pages.reset(new PdfPageCollection(*this));
        m_Catalog->GetDictionary().AddKey("Pages", m_Pages->GetObject().GetIndirectReference());
    }
    else
    {
        m_Pages.reset(new PdfPageCollection(*pagesRootObj));
    }

    auto& catalogDict = m_Catalog->GetDictionary();
    auto namesObj = catalogDict.FindKey("Names");
    if (namesObj != nullptr)
        m_NameTree.reset(new PdfNameTree(*namesObj));

    auto outlinesObj = catalogDict.FindKey("Outlines");
    if (outlinesObj != nullptr)
        m_Outlines.reset(new PdfOutlines(*outlinesObj));

    auto acroformObj = catalogDict.FindKey("AcroForm");
    if (acroformObj != nullptr)
        m_AcroForm.reset(new PdfAcroForm(*acroformObj));
}

void PdfDocument::AppendDocumentPages(const PdfDocument& doc)
{
    append(doc, true);
}

void PdfDocument::append(const PdfDocument& doc, bool appendAll)
{
    unsigned difference = static_cast<unsigned>(m_Objects.GetSize() + m_Objects.GetFreeObjects().size());

    // Because GetNextObject uses m_ObjectCount instead 
    // of m_Objects.GetSize() + m_Objects.GetFreeObjects().size() + 1
    // make sure the free objects are already present before appending to
    // prevent overlapping obj-numbers

    // create all free objects again, to have a clean free object list
    for (auto& ref : doc.GetObjects().GetFreeObjects())
        m_Objects.AddFreeObject(PdfReference(ref.ObjectNumber() + difference, ref.GenerationNumber()));

    // append all objects first and fix their references
    for (auto& obj : doc.GetObjects())
    {
        PdfReference ref(static_cast<uint32_t>(obj->GetIndirectReference().ObjectNumber() + difference), obj->GetIndirectReference().GenerationNumber());
        auto newObj = new PdfObject(PdfDictionary());
        newObj->setDirty();
        newObj->SetIndirectReference(ref);
        m_Objects.PushObject(newObj);
        *newObj = *obj;

        PoDoFo::LogMessage(PdfLogSeverity::Information, "Fixing references in {} {} R by {}",
            newObj->GetIndirectReference().ObjectNumber(), newObj->GetIndirectReference().GenerationNumber(), difference);
        fixObjectReferences(*newObj, difference);
    }

    if (appendAll)
    {
        const PdfName inheritableAttributes[] = {
            PdfName("Resources"),
            PdfName("MediaBox"),
            PdfName("CropBox"),
            PdfName("Rotate"),
            PdfName::KeyNull
        };

        // append all pages now to our page tree
        for (unsigned i = 0; i < doc.GetPages().GetCount(); i++)
        {
            auto& page = doc.GetPages().GetPageAt(i);
            auto& obj = m_Objects.MustGetObject(PdfReference(page.GetObject().GetIndirectReference().ObjectNumber()
                + difference, page.GetObject().GetIndirectReference().GenerationNumber()));
            if (obj.IsDictionary() && obj.GetDictionary().HasKey("Parent"))
                obj.GetDictionary().RemoveKey("Parent");

            // Deal with inherited attributes
            auto inherited = inheritableAttributes;
            while (!inherited->IsNull())
            {
                auto attribute = page.GetDictionary().FindKeyParent(*inherited);
                if (attribute != nullptr)
                {
                    PdfObject attributeCopy(*attribute);
                    fixObjectReferences(attributeCopy, difference);
                    obj.GetDictionary().AddKey(*inherited, attributeCopy);
                }

                inherited++;
            }

            m_Pages->InsertPageAt(m_Pages->GetCount(), obj);
        }

        // append all outlines
        PdfOutlineItem* root = this->GetOutlines();
        PdfOutlines* appendRoot = const_cast<PdfDocument&>(doc).GetOutlines();
        if (appendRoot && appendRoot->First())
        {
            // only append outlines if appended document has outlines
            while (root && root->Next())
                root = root->Next();

            PdfReference ref(appendRoot->First()->GetObject().GetIndirectReference().ObjectNumber()
                + difference, appendRoot->First()->GetObject().GetIndirectReference().GenerationNumber());
            root->InsertChild(new PdfOutlines(m_Objects.MustGetObject(ref)));
        }
    }

    // TODO: merge name trees
    // ToDictionary -> then iteratate over all keys and add them to the new one
}

void PdfDocument::InsertDocumentPageAt(unsigned atIndex, const PdfDocument& doc, unsigned pageIndex)
{
    // copy of PdfDocument::Append, only restricts which page to add
    unsigned difference = static_cast<unsigned>(m_Objects.GetSize() + m_Objects.GetFreeObjects().size());


    // Because GetNextObject uses m_ObjectCount instead 
    // of m_Objects.GetSize() + m_Objects.GetFreeObjects().size() + 1
    // make sure the free objects are already present before appending to
    // prevent overlapping obj-numbers

    // create all free objects again, to have a clean free object list
    for (auto& freeObj : GetObjects().GetFreeObjects())
    {
        m_Objects.AddFreeObject(PdfReference(freeObj.ObjectNumber() + difference, freeObj.GenerationNumber()));
    }

    // append all objects first and fix their references
    for (auto& obj : GetObjects())
    {
        PdfReference ref(static_cast<uint32_t>(obj->GetIndirectReference().ObjectNumber() + difference), obj->GetIndirectReference().GenerationNumber());
        auto newObj = new PdfObject(PdfDictionary());
        newObj->setDirty();
        newObj->SetIndirectReference(ref);
        m_Objects.PushObject(newObj);
        *newObj = *obj;

        PoDoFo::LogMessage(PdfLogSeverity::Information, "Fixing references in {} {} R by {}",
            newObj->GetIndirectReference().ObjectNumber(), newObj->GetIndirectReference().GenerationNumber(), difference);
        fixObjectReferences(*newObj, difference);
    }

    const PdfName inheritableAttributes[] = {
        PdfName("Resources"),
        PdfName("MediaBox"),
        PdfName("CropBox"),
        PdfName("Rotate"),
        PdfName::KeyNull
    };

    // append all pages now to our page tree
    for (unsigned i = 0; i < doc.GetPages().GetCount(); i++)
    {
        if (i != pageIndex)
            continue;

        auto& page = doc.GetPages().GetPageAt(i);
        auto& obj = m_Objects.MustGetObject(PdfReference(page.GetObject().GetIndirectReference().ObjectNumber()
            + difference, page.GetObject().GetIndirectReference().GenerationNumber()));
        if (obj.IsDictionary() && obj.GetDictionary().HasKey("Parent"))
            obj.GetDictionary().RemoveKey("Parent");

        // Deal with inherited attributes
        const PdfName* inherited = inheritableAttributes;
        while (!inherited->IsNull())
        {
            auto attribute = page.GetDictionary().FindKeyParent(*inherited);
            if (attribute != nullptr)
            {
                PdfObject attributeCopy(*attribute);
                fixObjectReferences(attributeCopy, difference);
                obj.GetDictionary().AddKey(*inherited, attributeCopy);
            }

            inherited++;
        }

        m_Pages->InsertPageAt(atIndex, obj);
    }

    // append all outlines
    PdfOutlineItem* root = this->GetOutlines();
    PdfOutlines* appendRoot = const_cast<PdfDocument&>(doc).GetOutlines();
    if (appendRoot != nullptr && appendRoot->First())
    {
        // only append outlines if appended document has outlines
        while (root != nullptr && root->Next())
            root = root->Next();

        PdfReference ref(appendRoot->First()->GetObject().GetIndirectReference().ObjectNumber()
            + difference, appendRoot->First()->GetObject().GetIndirectReference().GenerationNumber());
        root->InsertChild(new PdfOutlines(m_Objects.MustGetObject(ref)));
    }

    // TODO: merge name trees
    // ToDictionary -> then iteratate over all keys and add them to the new one
}

void PdfDocument::AppendDocumentPages(const PdfDocument& doc, unsigned pageIndex, unsigned pageCount)
{
    /*
      This function works a bit different than one might expect.
      Rather than copying one page at a time - we copy the ENTIRE document
      and then delete the pages we aren't interested in.

      We do this because
      1) SIGNIFICANTLY simplifies the process
      2) Guarantees that shared objects aren't copied multiple times
      3) offers MUCH faster performance for the common cases

      HOWEVER: because PoDoFo doesn't currently do any sort of "object garbage collection" during
      a Write() - we will end up with larger documents, since the data from unused pages
      will also be in there.
    */

    // calculate preliminary "left" and "right" page ranges to delete
    // then offset them based on where the pages were inserted
    // NOTE: some of this will change if/when we support insertion at locations
    //       OTHER than the end of the document!
    unsigned leftStartPage = 0;
    unsigned leftCount = pageIndex;
    unsigned rightStartPage = pageIndex + pageCount;
    unsigned rightCount = doc.GetPages().GetCount() - rightStartPage;
    unsigned pageOffset = this->GetPages().GetCount();

    leftStartPage += pageOffset;
    rightStartPage += pageOffset;

    // append in the whole document
    this->AppendDocumentPages(doc);

    // delete
    if (rightCount > 0)
        this->deletePages(rightStartPage, rightCount);
    if (leftCount > 0)
        this->deletePages(leftStartPage, leftCount);
}

void PdfDocument::deletePages(unsigned atIndex, unsigned pageCount)
{
    for (unsigned i = 0; i < pageCount; i++)
        this->GetPages().RemovePageAt(atIndex);
}

PdfInfo& PdfDocument::GetOrCreateInfo()
{
    if (m_Info == nullptr)
    {
        auto info = &m_Objects.CreateDictionaryObject();
        m_Info.reset(new PdfInfo(*info));
        m_TrailerObj->GetDictionary().AddKeyIndirect("Info", *info);
    }

    return *m_Info;
}

PdfRect PdfDocument::FillXObjectFromPage(PdfXObjectForm& xobj, const PdfPage& page, bool useTrimBox)
{
    unsigned difference = 0;
    auto& sourceDoc = page.GetDocument();
    if (this != &sourceDoc)
    {
        difference = static_cast<unsigned>(m_Objects.GetSize() + m_Objects.GetFreeObjects().size());
        append(sourceDoc, false);
    }

    // TODO: remove unused objects: page, ...

    auto& pageObj = m_Objects.MustGetObject(PdfReference(page.GetObject().GetIndirectReference().ObjectNumber()
        + difference, page.GetObject().GetIndirectReference().GenerationNumber()));
    PdfRect box = page.GetMediaBox();

    // intersect with crop-box
    box.Intersect(page.GetCropBox());

    // intersect with trim-box according to parameter
    if (useTrimBox)
        box.Intersect(page.GetTrimBox());

    // link resources from external doc to x-object
    if (pageObj.IsDictionary() && pageObj.GetDictionary().HasKey("Resources"))
        xobj.GetObject().GetDictionary().AddKey("Resources", *pageObj.GetDictionary().GetKey("Resources"));

    // copy top-level content from external doc to x-object
    if (pageObj.IsDictionary() && pageObj.GetDictionary().HasKey("Contents"))
    {
        // get direct pointer to contents
        auto& contents = pageObj.GetDictionary().MustFindKey("Contents");
        if (contents.IsArray())
        {
            // copy array as one stream to xobject
            PdfArray arr = contents.GetArray();

            auto& xobjStream = xobj.GetObject().GetOrCreateStream();
            auto output = xobjStream.GetOutputStream({ PdfFilterType::FlateDecode });

            for (auto& child : arr)
            {
                if (child.IsReference())
                {
                    // TODO: not very efficient !!
                    auto obj = GetObjects().GetObject(child.GetReference());

                    while (obj != nullptr)
                    {
                        if (obj->IsReference())    // Recursively look for the stream
                        {
                            obj = GetObjects().GetObject(obj->GetReference());
                        }
                        else if (obj->HasStream())
                        {
                            PdfObjectStream& contStream = obj->GetOrCreateStream();

                            charbuff contStreamBuffer;
                            contStream.CopyTo(contStreamBuffer);
                            output.Write(contStreamBuffer);
                            break;
                        }
                        else
                        {
                            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidStream);
                            break;
                        }
                    }
                }
                else
                {
                    string str;
                    child.ToString(str);
                    output.Write(str);
                    output.Write(" ");
                }
            }
        }
        else if (contents.HasStream())
        {
            // copy stream to xobject
            auto& contentsStream = contents.GetOrCreateStream();
            auto contentsInput = contentsStream.GetInputStream();

            auto& xobjStream = xobj.GetObject().GetOrCreateStream();
            auto output = xobjStream.GetOutputStream({ PdfFilterType::FlateDecode });
            contentsInput.CopyTo(output);
        }
        else
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
        }
    }

    return box;
}

void PdfDocument::fixObjectReferences(PdfObject& obj, int difference)
{
    if (obj.IsDictionary())
    {
        for (auto& pair : obj.GetDictionary())
        {
            if (pair.second.IsReference())
            {
                pair.second = PdfObject(PdfReference(pair.second.GetReference().ObjectNumber() + difference,
                    pair.second.GetReference().GenerationNumber()));
            }
            else if (pair.second.IsDictionary() ||
                pair.second.IsArray())
            {
                fixObjectReferences(pair.second, difference);
            }
        }
    }
    else if (obj.IsArray())
    {
        for (auto& child : obj.GetArray())
        {
            if (child.IsReference())
            {
                child = PdfObject(PdfReference(child.GetReference().ObjectNumber() + difference,
                    child.GetReference().GenerationNumber()));
            }
            else if (child.IsDictionary() || child.IsArray())
            {
                fixObjectReferences(child, difference);
            }
        }
    }
    else if (obj.IsReference())
    {
        obj = PdfObject(PdfReference(obj.GetReference().ObjectNumber() + difference,
            obj.GetReference().GenerationNumber()));
    }
}

void PdfDocument::CollectGarbage()
{
    m_Objects.CollectGarbage();
}

PdfOutlines& PdfDocument::GetOrCreateOutlines()
{
    if (m_Outlines != nullptr)
        return *m_Outlines.get();

    m_Outlines.reset(new PdfOutlines(*this));
    m_Catalog->GetDictionary().AddKey("Outlines", m_Outlines->GetObject().GetIndirectReference());
    return *m_Outlines.get();
}

PdfNameTree& PdfDocument::GetOrCreateNameTree()
{
    if (m_NameTree != nullptr)
        return *m_NameTree;

    PdfNameTree tmpTree(*this);
    auto obj = &tmpTree.GetObject();
    m_Catalog->GetDictionary().AddKey("Names", obj->GetIndirectReference());
    m_NameTree.reset(new PdfNameTree(*obj));
    return *m_NameTree;
}

PdfAcroForm& PdfDocument::GetOrCreateAcroForm(PdfAcroFormDefaulAppearance defaultAppearance)
{
    if (m_AcroForm != nullptr)
        return *m_AcroForm.get();

    m_AcroForm.reset(new PdfAcroForm(*this, defaultAppearance));
    m_Catalog->GetDictionary().AddKey("AcroForm", m_AcroForm->GetObject().GetIndirectReference());
    return *m_AcroForm.get();
}

void PdfDocument::AddNamedDestination(const PdfDestination& dest, const PdfString& name)
{
    auto& names = GetOrCreateNameTree();
    names.AddValue("Dests", name, dest.GetObject().GetIndirectReference());
}

void PdfDocument::AttachFile(const PdfFileSpec& fileSpec)
{
    auto& names = GetOrCreateNameTree();
    names.AddValue("EmbeddedFiles", fileSpec.GetFilename(false), fileSpec.GetObject().GetIndirectReference());
}
    
PdfFileSpec* PdfDocument::GetAttachment(const PdfString& name)
{
    if (m_NameTree == nullptr)
        return nullptr;

    auto obj = m_NameTree->GetValue("EmbeddedFiles", name);
    if (obj == nullptr)
        return nullptr;

    return new PdfFileSpec(*obj);
}

void PdfDocument::SetTrailer(unique_ptr<PdfObject> obj)
{
    if (obj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_TrailerObj = std::move(obj);
    m_TrailerObj->SetDocument(this);
    m_Trailer.reset(new PdfTrailer(*m_TrailerObj));

    auto catalog = m_TrailerObj->GetDictionary().FindKey("Root");
    if (catalog == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NoObject, "Catalog object not found!");

    m_Catalog.reset(new PdfCatalog(*catalog));

    auto info = m_TrailerObj->GetDictionary().FindKey("Info");
    if (info != nullptr)
        m_Info.reset(new PdfInfo(*info));
}

bool PdfDocument::IsEncrypted() const
{
    return GetEncrypt() != nullptr;
}

bool PdfDocument::IsPrintAllowed() const
{
    return GetEncrypt() == nullptr ? true : GetEncrypt()->IsPrintAllowed();
}

bool PdfDocument::IsEditAllowed() const
{
    return GetEncrypt() == nullptr ? true : GetEncrypt()->IsEditAllowed();
}

bool PdfDocument::IsCopyAllowed() const
{
    return GetEncrypt() == nullptr ? true : GetEncrypt()->IsCopyAllowed();
}

bool PdfDocument::IsEditNotesAllowed() const
{
    return GetEncrypt() == nullptr ? true : GetEncrypt()->IsEditNotesAllowed();
}

bool PdfDocument::IsFillAndSignAllowed() const
{
    return GetEncrypt() == nullptr ? true : GetEncrypt()->IsFillAndSignAllowed();
}

bool PdfDocument::IsAccessibilityAllowed() const
{
    return GetEncrypt() == nullptr ? true : GetEncrypt()->IsAccessibilityAllowed();
}

bool PdfDocument::IsDocAssemblyAllowed() const
{
    return GetEncrypt() == nullptr ? true : GetEncrypt()->IsDocAssemblyAllowed();
}

bool PdfDocument::IsHighPrintAllowed() const
{
    return GetEncrypt() == nullptr ? true : GetEncrypt()->IsHighPrintAllowed();
}

unique_ptr<PdfImage> PdfDocument::CreateImage(const string_view& prefix)
{
    return unique_ptr<PdfImage>(new PdfImage(*this, prefix));
}

unique_ptr<PdfXObjectForm> PdfDocument::CreateXObjectForm(const PdfRect& rect, const string_view& prefix)
{
    return unique_ptr<PdfXObjectForm>(new PdfXObjectForm(*this, rect, prefix));
}
