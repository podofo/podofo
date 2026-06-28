// SPDX-FileCopyrightText: 2006 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include <podofo/private/XMPUtils.h>
#include "PdfDocument.h"

#include "PdfExtGState.h"
#include "PdfDestination.h"
#include "PdfFileSpec.h"

using namespace std;
using namespace PoDoFo;

static PdfObject& copyPageObject(PdfIndirectObjectList& dest, const PdfObject& srcPageObj,
    unordered_map<PdfReference, PdfObject*>& mappedObjects);
static void copyReferencedObjects(PdfObject& obj, const PdfIndirectObjectList& src,
    PdfIndirectObjectList& dest, unordered_map<PdfReference, PdfObject*>& mappedObjects);
static PdfObject& copyIndirectObject(PdfIndirectObjectList& dest,
    const PdfObject& srcObj, unordered_map<PdfReference, PdfObject*>& mappedObjects);

PdfDocument::PdfDocument(bool empty) :
    m_Objects(*this),
    m_Metadata(*this),
    m_FontManager(*this),
    m_InfoLazyLoaded(false),
    m_OutlinesLazyLoaded(false)
{
    if (!empty)
        resetPrivate();
}

PdfDocument::PdfDocument(const PdfDocument& doc) :
    m_Objects(*this, doc.m_Objects),
    m_Metadata(*this),
    m_FontManager(*this),
    m_InfoLazyLoaded(false),
    m_OutlinesLazyLoaded(false)
{
    SetTrailer(std::make_unique<PdfObject>(doc.GetTrailer().GetObject()));
    Init();
}

PdfDocument::~PdfDocument()
{
    // NOTE: Members will autoclear
}

void PdfDocument::Reset()
{
    Clear();
    resetPrivate();
    reset();
}

void PdfDocument::reset()
{
    // Do nothing, to be overridden
}

void PdfDocument::Clear()
{
    m_FontManager.Clear();
    m_Metadata.Invalidate();
    m_TrailerObj = nullptr;
    m_Trailer = nullptr;
    m_Catalog = nullptr;
    m_Info = nullptr;
    m_Pages = nullptr;
    m_AcroForm = nullptr;
    m_Outlines = nullptr;
    m_NameTrees = nullptr;
    m_InfoLazyLoaded = false;
    m_OutlinesLazyLoaded = false;
    m_Objects.Clear();
    clear();
}

void PdfDocument::clear()
{
    // Do nothing, to be overridden
}

void PdfDocument::Init()
{
    auto pagesRootObj = m_Catalog->GetDictionary().FindKey("Pages");
    if (pagesRootObj == nullptr)
    {
        m_Pages.reset(new PdfPageCollection(*this));
        m_Catalog->GetDictionary().AddKey("Pages"_n, m_Pages->GetObject().GetIndirectReference());
    }
    else
    {
        m_Pages.reset(new PdfPageCollection(*pagesRootObj));
    }

    auto& catalogDict = m_Catalog->GetDictionary();
    auto namesObj = catalogDict.FindKey("Names");
    if (namesObj != nullptr)
        m_NameTrees.reset(new PdfNameTrees(*namesObj));

    auto acroformObj = catalogDict.FindKey("AcroForm");
    if (acroformObj != nullptr)
        m_AcroForm.reset(new PdfAcroForm(*acroformObj));
}

void PdfDocument::AppendDocumentPages(const PdfDocument& doc)
{
    unordered_map<PdfReference, PdfObject*> mappedObjects;

    // Copy all pages
    for (unsigned i = 0; i < doc.GetPages().GetCount(); i++)
    {
        auto& newObj = copyPageObject(m_Objects, doc.GetPages().GetPageAt(i).GetObject(), mappedObjects);
        m_Pages->InsertPageAt(m_Pages->GetCount(), unique_ptr<PdfPage>(new PdfPage(newObj)));
    }

    // Copy outlines
    const PdfOutlineItem* srcOutlines = doc.GetOutlines();
    if (srcOutlines != nullptr && srcOutlines->First() != nullptr)
    {
        PdfOutlineItem& destRoot = this->GetOrCreateOutlines();
        appendOutlineItems(destRoot, srcOutlines->First(),
            doc.GetObjects(), mappedObjects);
    }

    // TODO: merge name trees
}

void PdfDocument::InsertDocumentPageAt(unsigned atIndex, const PdfDocument& doc, unsigned pageIndex)
{
    unordered_map<PdfReference, PdfObject*> mappedObjects;
    auto& newObj = copyPageObject(m_Objects, doc.GetPages().GetPageAt(pageIndex).GetObject(), mappedObjects);
    m_Pages->InsertPageAt(atIndex, unique_ptr<PdfPage>(new PdfPage(newObj)));

    // TODO: merge name trees
}

void PdfDocument::AppendDocumentPages(const PdfDocument& doc, unsigned pageIndex, unsigned pageCount)
{
    unordered_map<PdfReference, PdfObject*> mappedObjects;
    for (unsigned i = 0; i < pageCount; i++)
    {
        auto& newObj = copyPageObject(m_Objects, doc.GetPages().GetPageAt(pageIndex + i).GetObject(), mappedObjects);
        m_Pages->InsertPageAt(m_Pages->GetCount(), unique_ptr<PdfPage>(new PdfPage(newObj)));
    }

    // TODO: merge name trees
}

void PdfDocument::appendOutlineItems(PdfOutlineItem& destParent, const PdfOutlineItem* srcItem,
    const PdfIndirectObjectList& srcObjects, unordered_map<PdfReference, PdfObject*>& mappedObjects)
{
    while (srcItem != nullptr)
    {
        // Copy the outline item object (its dictionary with Title, Dest, A, C, F, SE, Count keys)
        auto& newObj = copyIndirectObject(m_Objects, srcItem->GetObject(), mappedObjects);

        // Remove navigation keys - they will be rebuilt by the tree structure
        newObj.GetDictionary().RemoveKey("First");
        newObj.GetDictionary().RemoveKey("Last");
        newObj.GetDictionary().RemoveKey("Next");
        newObj.GetDictionary().RemoveKey("Prev");
        newObj.GetDictionary().RemoveKey("Parent");
        newObj.GetDictionary().RemoveKey("Count");

        // Remap references within the item (e.g. Dest array entries, action objects)
        copyReferencedObjects(newObj, srcObjects, m_Objects, mappedObjects);

        // Construct without parent so InsertChild's same-tree check passes,
        // then set the parent relationship after insertion
        unique_ptr<PdfOutlineItem> destItem(new PdfOutlineItem(newObj, nullptr, nullptr));
        auto* destItemPtr = destItem.get();
        destParent.InsertChild(std::move(destItem));
        destItemPtr->m_ParentOutline = &destParent;
        newObj.GetDictionary().AddKey("Parent"_n, destParent.GetObject().GetIndirectReference());

        // Recurse into children
        if (srcItem->First() != nullptr)
            appendOutlineItems(*destItemPtr, srcItem->First(), srcObjects, mappedObjects);

        srcItem = srcItem->Next();
    }
}

void PdfDocument::deletePages(unsigned atIndex, unsigned pageCount)
{
    for (unsigned i = 0; i < pageCount; i++)
        this->GetPages().RemovePageAt(atIndex);
}

void PdfDocument::resetPrivate()
{
    m_TrailerObj.reset(new PdfObject()); // The trailer is NO part of the vector of objects
    m_TrailerObj->SetDocument(this);
    auto& catalog = m_Objects.CreateDictionaryObject("Catalog"_n);
    m_Trailer.reset(new PdfTrailer(*m_TrailerObj));

    m_Catalog.reset(new PdfCatalog(catalog));
    m_TrailerObj->GetDictionary().AddKeyIndirect("Root"_n, catalog);

    auto& info = m_Objects.CreateDictionaryObject();
    m_Info.reset(new PdfInfo(info,
        PdfInfoInitial::WriteProducer | PdfInfoInitial::WriteCreationTime));
    m_TrailerObj->GetDictionary().AddKeyIndirect("Info"_n, info);
    m_InfoLazyLoaded = true;

    Init();
}

void PdfDocument::lazyLoadOutlines()
{
    if (m_OutlinesLazyLoaded)
        return;

    PODOFO_INVARIANT(m_Catalog != nullptr);
    auto outlinesObj = m_Catalog->GetDictionary().FindKey("Outlines");
    if (outlinesObj != nullptr)
        m_Outlines = unique_ptr<PdfOutlines>(new PdfOutlines(*outlinesObj));

    m_OutlinesLazyLoaded = true;
}

void PdfDocument::lazyLoadInfo()
{
    if (m_InfoLazyLoaded)
        return;

    PODOFO_INVARIANT(m_Catalog != nullptr);
    auto infoObj = m_TrailerObj->GetDictionary().FindKey("Info");
    if (infoObj != nullptr)
    {
        try
        {
            m_Info = unique_ptr<PdfInfo>(new PdfInfo(*infoObj));
        }
        catch (PdfError& ex)
        {
            PoDoFo::LogMessage(PdfLogSeverity::Warning,
                "Failed to load /Info dictionary: {}", ex.what());
        }
    }

    m_InfoLazyLoaded = true;
}

const PdfInfo* PdfDocument::GetInfo() const
{
    const_cast<PdfDocument&>(*this).lazyLoadInfo();
    return m_Info.get();
}

PdfInfo& PdfDocument::GetOrCreateInfo()
{
    lazyLoadInfo();
    if (m_Info == nullptr)
    {
        auto info = &m_Objects.CreateDictionaryObject();
        m_Info.reset(new PdfInfo(*info));
        m_TrailerObj->GetDictionary().AddKeyIndirect("Info"_n, *info);
    }

    return *m_Info;
}

void PdfDocument::createAction(PdfActionType type, unique_ptr<PdfAction>& action)
{
    action = PdfAction::Create(*this, type);
}

Rect PdfDocument::FillXObjectFromPage(PdfXObjectForm& xobj, const PdfPage& page, bool useTrimBox)
{
    Rect box = page.GetMediaBox();

    // Intersect with crop-box
    box.Intersect(page.GetCropBox());

    // Intersect with trim-box according to parameter
    if (useTrimBox)
        box.Intersect(page.GetTrimBox());

    auto& sourceDoc = page.GetDocument();
    bool isSameDoc = (this == &sourceDoc);

    if (isSameDoc)
    {
        // Same document: just reference existing objects directly
        auto* resourcesObj = page.GetDictionary().FindKeyParent("Resources");
        if (resourcesObj != nullptr)
            xobj.GetDictionary().AddKey("Resources"_n, *resourcesObj);

        // Without /Group the viewer resets to default compositing, dropping the
        // page's isolated/knockout flags and color space (ISO 32000-2:2020 11.6.6 "Transparency group XObjects")
        auto* groupObj = page.GetDictionary().FindKey("Group");
        if (groupObj != nullptr)
            xobj.GetDictionary().AddKey("Group"_n, *groupObj);
    }
    else
    {
        // Cross-document: selectively copy referenced objects
        unordered_map<PdfReference, PdfObject*> mappedObjects;

        auto* resourcesObj = page.GetDictionary().FindKeyParent("Resources");
        if (resourcesObj != nullptr)
        {
            PdfObject resourcesCopy(*resourcesObj);
            copyReferencedObjects(resourcesCopy, sourceDoc.GetObjects(), m_Objects, mappedObjects);
            xobj.GetDictionary().AddKey("Resources"_n, std::move(resourcesCopy));
        }

        // Without /Group the viewer resets to default compositing, dropping the
        // page's isolated/knockout flags and color space (ISO 32000-2:2020 11.6.6 "Transparency group XObjects")
        auto* groupObj = page.GetDictionary().FindKey("Group");
        if (groupObj != nullptr)
        {
            PdfObject groupCopy(*groupObj);
            copyReferencedObjects(groupCopy, sourceDoc.GetObjects(), m_Objects, mappedObjects);
            xobj.GetDictionary().AddKey("Group"_n, std::move(groupCopy));
        }
    }

    // Copy the page content stream(s) into the xobject stream
    auto& xobjStream = xobj.GetObject().GetOrCreateStream();
    auto output = xobjStream.GetOutputStream({ PdfFilterType::FlateDecode });
    page.CopyContentsTo(output);
    return box;
}

void PdfDocument::CollectGarbage(PdfGarbageCollectionFlags flags)
{
    m_Objects.CollectGarbage(flags);
}

PdfOutlines& PdfDocument::GetOrCreateOutlines()
{
    lazyLoadOutlines();
    if (m_Outlines != nullptr)
        return *m_Outlines;

    m_Outlines = unique_ptr<PdfOutlines>(new PdfOutlines(*this));
    m_Catalog->GetDictionary().AddKey("Outlines"_n, m_Outlines->GetObject().GetIndirectReference());
    return *m_Outlines;
}

PdfNameTrees& PdfDocument::GetOrCreateNames()
{
    if (m_NameTrees != nullptr)
        return *m_NameTrees;

    PdfNameTrees tmpTree(*this);
    auto obj = &tmpTree.GetObject();
    m_Catalog->GetDictionary().AddKey("Names"_n, obj->GetIndirectReference());
    m_NameTrees.reset(new PdfNameTrees(*obj));
    return *m_NameTrees;
}

PdfAcroForm& PdfDocument::GetOrCreateAcroForm(PdfAcroFormDefaulAppearance defaultAppearance)
{
    if (m_AcroForm != nullptr)
        return *m_AcroForm.get();

    m_AcroForm.reset(new PdfAcroForm(*this, defaultAppearance));
    m_Catalog->GetDictionary().AddKey("AcroForm"_n, m_AcroForm->GetObject().GetIndirectReference());
    return *m_AcroForm.get();
}

void PdfDocument::SetTrailer(unique_ptr<PdfObject> obj)
{
    PODOFO_ASSERT(!(m_InfoLazyLoaded || m_OutlinesLazyLoaded));
    if (obj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_TrailerObj = std::move(obj);
    m_TrailerObj->SetDocument(this);
    m_Trailer.reset(new PdfTrailer(*m_TrailerObj));

    auto catalog = m_TrailerObj->GetDictionary().FindKey("Root");
    if (catalog == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ObjectNotFound, "Catalog object not found!");

    m_Catalog.reset(new PdfCatalog(*catalog));
}

bool PdfDocument::IsEncrypted() const
{
    return GetEncrypt() != nullptr;
}

bool PdfDocument::IsPrintAllowed() const
{
    return GetEncrypt() == nullptr || GetEncrypt()->IsPrintAllowed() || HasOwnerPermissions();
}

bool PdfDocument::IsEditAllowed() const
{
    return GetEncrypt() == nullptr || GetEncrypt()->IsEditAllowed() || HasOwnerPermissions();
}

bool PdfDocument::IsCopyAllowed() const
{
    return GetEncrypt() == nullptr || GetEncrypt()->IsCopyAllowed() || HasOwnerPermissions();
}

bool PdfDocument::IsEditNotesAllowed() const
{
    return GetEncrypt() == nullptr || GetEncrypt()->IsEditNotesAllowed() || HasOwnerPermissions();
}

bool PdfDocument::IsFillAndSignAllowed() const
{
    return GetEncrypt() == nullptr || GetEncrypt()->IsFillAndSignAllowed() || HasOwnerPermissions();
}

bool PdfDocument::IsAccessibilityAllowed() const
{
    return GetEncrypt() == nullptr || GetEncrypt()->IsAccessibilityAllowed() || HasOwnerPermissions();
}

bool PdfDocument::IsDocAssemblyAllowed() const
{
    return GetEncrypt() == nullptr || GetEncrypt()->IsDocAssemblyAllowed() || HasOwnerPermissions();
}

bool PdfDocument::IsHighPrintAllowed() const
{
    return GetEncrypt() == nullptr || GetEncrypt()->IsHighPrintAllowed() || HasOwnerPermissions();
}

void PdfDocument::PushPdfExtension(const PdfExtension& extension)
{
    PdfDictionary* extensionDict;
    if (!this->GetCatalog().GetDictionary().TryFindKeyAs("Extensions", extensionDict))
    {
        auto& obj = GetObjects().CreateDictionaryObject();
        this->GetCatalog().GetDictionary().AddKeyIndirect("Extensions"_n, obj);
        extensionDict = &obj.GetDictionary();
    }

    auto& newExtension = GetObjects().CreateDictionaryObject();
    auto& newExtensionDict = newExtension.GetDictionary();
    auto version = extension.GetBaseVersion();
    if (version == PdfVersion::Unknown)
        version = GetPdfVersion();

    newExtensionDict.AddKey("BaseVersion"_n, PoDoFo::GetPdfVersionName(version));
    newExtensionDict.AddKey("ExtensionLevel"_n, extension.GetLevel());
    if (extension.GetUrl() != nullptr)
        newExtensionDict.AddKey("URL"_n, *extension.GetUrl());
    if (extension.GetExtensionRevision() != nullptr)
        newExtensionDict.AddKey("ExtensionRevision"_n, *extension.GetExtensionRevision());

    extensionDict->AddKeyIndirect(extension.GetNamespace(), newExtension);
}

bool PdfDocument::HasPdfExtension(const string_view& ns, int64_t level) const
{
    const PdfDictionary* dict;
    int64_t num;
    if (this->GetCatalog().GetDictionary().TryFindKeyAs("Extensions", dict)
        && dict->TryFindKeyAs(ns, dict)
        && dict->TryFindKeyAs("ExtensionLevel", num)
        && num == level)
    {
        return true;
    }

    return false;
}

vector<PdfExtension> PdfDocument::GetPdfExtensions() const
{
    vector<PdfExtension> ret;
    const PdfDictionary* dict;
    if (!this->GetCatalog().GetDictionary().TryFindKeyAs("Extensions", dict))
        return ret;

    // Loop through all declared extensions
    const PdfName* name;
    int64_t num;
    PdfVersion version;
    const PdfString* url;
    const PdfString* extensionRevision;
    for (auto& pair : dict->GetIndirectIterator())
    {
        if (!pair.second->TryGetDictionary(dict)
            || !dict->TryFindKeyAs("ExtensionLevel", num)
            || !dict->TryFindKeyAs("BaseVersion", name)
            || (version = PoDoFo::GetPdfVersion(name->GetString())) == PdfVersion::Unknown)
        {
            continue;
        }

        (void)dict->TryFindKeyAs("URL", url);
        (void)dict->TryFindKeyAs("ExtensionRevision", extensionRevision);

        ret.push_back(PdfExtension(pair.first, num, version,
            url == nullptr ? nullable<const PdfString&>{ } : *url,
            extensionRevision == nullptr ? nullable<const PdfString&>{ } : *extensionRevision));
    }

    return ret;
}

void PdfDocument::RemovePdfExtension(const string_view& ns, int64_t level)
{
    PdfDictionary* dict;
    int64_t num;
    if (!this->GetCatalog().GetDictionary().TryFindKeyAs("Extensions", dict)
        || !dict->TryFindKeyAs(ns, dict)
        || !dict->TryFindKeyAs("ExtensionLevel", num)
        || num != level)
    {
        return;
    }

    dict->RemoveKey(ns);
}

PdfAcroForm& PdfDocument::MustGetAcroForm()
{
    if (m_AcroForm == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "AcroForm is not present");

    return *m_AcroForm;
}

const PdfAcroForm& PdfDocument::MustGetAcroForm() const
{
    if (m_AcroForm == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "AcroForm is not present");

    return *m_AcroForm;
}

PdfNameTrees& PdfDocument::MustGetNames()
{
    if (m_NameTrees == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Names are not present");

    return *m_NameTrees;
}

const PdfNameTrees& PdfDocument::MustGetNames() const
{
    if (m_NameTrees == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Names are not present");

    return *m_NameTrees;
}

PdfOutlines* PdfDocument::GetOutlines()
{
    lazyLoadOutlines();
    return m_Outlines.get();
}

const PdfOutlines* PdfDocument::GetOutlines() const
{
    const_cast<PdfDocument&>(*this).lazyLoadOutlines();
    return m_Outlines.get();
}

PdfOutlines& PdfDocument::MustGetOutlines()
{
    lazyLoadOutlines();
    if (m_Outlines == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Outlines are not present");

    return *m_Outlines;
}

const PdfOutlines& PdfDocument::MustGetOutlines() const
{
    const_cast<PdfDocument&>(*this).lazyLoadOutlines();
    if (m_Outlines == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Outlines are not present");

    return *m_Outlines;
}

PdfDocumentFieldIterable PdfDocument::GetFieldsIterator()
{
    return PdfDocumentFieldIterable(*this);
}

PdfDocumentConstFieldIterable PdfDocument::GetFieldsIterator() const
{
    return PdfDocumentConstFieldIterable(const_cast<PdfDocument&>(*this));
}

unique_ptr<PdfImage> PdfDocument::CreateImage()
{
    return unique_ptr<PdfImage>(new PdfImage(*this));
}

unique_ptr<PdfXObjectForm> PdfDocument::CreateXObjectForm(const Rect& rect)
{
    return unique_ptr<PdfXObjectForm>(new PdfXObjectForm(*this, rect));
}

unique_ptr<PdfDestination> PdfDocument::CreateDestination()
{
    return unique_ptr<PdfDestination>(new PdfDestination(*this));
}

unique_ptr<PdfColorSpace> PdfDocument::CreateColorSpace(PdfColorSpaceFilterPtr filter)
{
    if (filter->IsTrivial())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Only non trivial color spaces can be constructed through document");

    return unique_ptr<PdfColorSpace>(new PdfColorSpace(*this, std::move(filter)));
}

unique_ptr<PdfFunction> PdfDocument::CreateFunction(PdfFunctionDefinitionPtr definition)
{
    return unique_ptr<PdfFunction>(new PdfFunction(*this, std::move(definition)));
}

unique_ptr<PdfUncolouredTilingPattern> PdfDocument::CreateTilingPattern(shared_ptr<PdfUncolouredTilingPatternDefinition> definition)
{
    return unique_ptr<PdfUncolouredTilingPattern>(new PdfUncolouredTilingPattern(*this, std::move(definition)));
}

unique_ptr<PdfColouredTilingPattern> PdfDocument::CreateTilingPattern(shared_ptr<PdfColouredTilingPatternDefinition> definition)
{
    return unique_ptr<PdfColouredTilingPattern>(new PdfColouredTilingPattern(*this, std::move(definition)));
}

unique_ptr<PdfShadingPattern> PdfDocument::CreateShadingPattern(PdfShadingPatternDefinitionPtr definition)
{
    return unique_ptr<PdfShadingPattern>(new PdfShadingPattern(*this, std::move(definition)));
}

unique_ptr<PdfShadingDictionary> PdfDocument::CreateShadingDictionary(PdfShadingDefinitionPtr definition)
{
    return unique_ptr<PdfShadingDictionary>(new PdfShadingDictionary(*this, std::move(definition)));
}

unique_ptr<PdfExtGState> PdfDocument::CreateExtGState(PdfExtGStateDefinitionPtr definition)
{
    return unique_ptr<PdfExtGState>(new PdfExtGState(*this, std::move(definition)));
}

unique_ptr<PdfAction> PdfDocument::CreateAction(PdfActionType type)
{
    return PdfAction::Create(*this, type);
}

unique_ptr<PdfFileSpec> PdfDocument::CreateFileSpec()
{
    return unique_ptr<PdfFileSpec>(new PdfFileSpec(*this));
}

PdfObject& copyPageObject(PdfIndirectObjectList& dest, const PdfObject& srcPageObj,
    unordered_map<PdfReference, PdfObject*>& mappedObjects)
{
    auto& srcObjects = srcPageObj.MustGetDocument().GetObjects();

    // Create the new page object as a copy of the source page
    auto& newPageObj = copyIndirectObject(dest, srcPageObj, mappedObjects);

    // Remove the Parent key - it will be set by the destination page tree
    newPageObj.GetDictionary().RemoveKey("Parent");

    // Resolve inheritable attributes from the source page tree and
    // copy them directly into the new page object
    const PdfName inheritableAttributes[] = {
        "Resources"_n,
        "MediaBox"_n,
        "CropBox"_n,
        "Rotate"_n,
    };

    for (auto& attribName : inheritableAttributes)
    {
        // Skip if the page already has this attribute directly
        if (newPageObj.GetDictionary().HasKey(attribName))
            continue;

        // Look up the attribute in the source page's parent chain
        auto attrib = srcPageObj.GetDictionary().FindKeyParent(attribName);
        if (attrib != nullptr)
            newPageObj.GetDictionary().AddKey(attribName, *attrib);
    }

    // Recursively copy all indirect objects referenced by the page,
    // including the inherited attributes added above
    copyReferencedObjects(newPageObj, srcObjects, dest, mappedObjects);

    return newPageObj;
}

// Recursively walk an object tree, copying any referenced indirect objects
// into dest, and remapping references in-place.
static void copyReferencedObjects(PdfObject& obj, const PdfIndirectObjectList& src,
    PdfIndirectObjectList& dest, unordered_map<PdfReference, PdfObject*>& mappedObjects)
{
    PdfDictionary* dict;
    PdfArray* arr;
    PdfReference srcRef;
    if (obj.TryGetDictionary(dict))
    {
        for (auto& pair : *dict)
        {
            if (pair.second.TryGetReference(srcRef))
            {
                auto it = mappedObjects.find(srcRef);
                if (it != mappedObjects.end())
                {
                    pair.second = PdfObject(it->second->GetIndirectReference());
                }
                else
                {
                    auto* srcChild = src.GetObject(srcRef);
                    if (srcChild != nullptr)
                    {
                        auto& copied = copyIndirectObject(dest, *srcChild, mappedObjects);
                        pair.second = PdfObject(copied.GetIndirectReference());
                        copyReferencedObjects(copied, src, dest, mappedObjects);
                    }
                }
            }
            else if (pair.second.IsDictionary() || pair.second.IsArray())
            {
                copyReferencedObjects(pair.second, src, dest, mappedObjects);
            }
        }
    }
    else if (obj.TryGetArray(arr))
    {
        for (auto& child : *arr)
        {
            if (child.TryGetReference(srcRef))
            {
                auto it = mappedObjects.find(srcRef);
                if (it != mappedObjects.end())
                {
                    child = PdfObject(it->second->GetIndirectReference());
                }
                else
                {
                    auto* srcChild = src.GetObject(srcRef);
                    if (srcChild != nullptr)
                    {
                        auto& copied = copyIndirectObject(dest, *srcChild, mappedObjects);
                        child = PdfObject(copied.GetIndirectReference());
                        copyReferencedObjects(copied, src, dest, mappedObjects);
                    }
                }
            }
            else if (child.IsDictionary() || child.IsArray())
            {
                copyReferencedObjects(child, src, dest, mappedObjects);
            }
        }
    }
}


// Copy an indirect object from a source document into dest, registering it in mappedObjects.
// If the object was already copied, returns the existing mapped object.
static PdfObject& copyIndirectObject(PdfIndirectObjectList& dest,
    const PdfObject& srcObj, unordered_map<PdfReference, PdfObject*>& mappedObjects)
{
    auto srcRef = srcObj.GetIndirectReference();
    auto found = mappedObjects.find(srcRef);
    if (found != mappedObjects.end())
        return *found->second;

    // Create the new indirect object as a copy of the source
    auto& newObj = dest.CreateObject(srcObj);
    mappedObjects[srcRef] = &newObj;
    return newObj;
}
