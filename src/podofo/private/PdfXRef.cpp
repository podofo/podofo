/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PdfDeclarationsPrivate.h"
#include "PdfXRef.h"

#include <podofo/auxiliary/OutputDevice.h>
#include "PdfWriter.h"

#include <algorithm>

constexpr unsigned UnavailableObjectGenerationNumber = 65535;

using namespace std;
using namespace PoDoFo;

PdfXRef::PdfXRef(PdfWriter& writer)
    : m_writer(&writer), m_offset(0) { }

PdfXRef::~PdfXRef() { }

void PdfXRef::AddInUseObject(const PdfReference& ref, uint64_t offset)
{
    PODOFO_ASSERT(ref.ObjectNumber() != 0 && offset < (uint64_t)numeric_limits<int64_t>::max());
    addObject(ref, (int64_t)offset);
}

void PdfXRef::AddFreeObject(const PdfReference& ref)
{
    PODOFO_ASSERT(ref.ObjectNumber() != 0 && ref.GenerationNumber() != 0);
    addObject(ref, -1);
}

void PdfXRef::AddUnavailableObject(uint32_t objNum)
{
    PODOFO_ASSERT(objNum != 0);
    addObject(PdfReference(objNum, UnavailableObjectGenerationNumber), -1);
}

void PdfXRef::addObject(const PdfReference& ref, int64_t offset)
{
    // Find the insertion point for the object in the ordered set
    auto it = m_xrefObjects.lower_bound(ref);
    if (it == m_xrefObjects.end() || it->Reference.ObjectNumber() != ref.ObjectNumber())
    {
        // The object is not present, just insert it
        (void)m_xrefObjects.emplace_hint(it, ref, offset);
    }
    else
    {
        // Update the existing object by extracting
        // the node and reinsert it in the set
        auto hintIt = std::next(it);
        auto node = m_xrefObjects.extract(it);
        node.value().Reference = ref;
        node.value().Offset = offset;
        (void)m_xrefObjects.insert(hintIt, std::move(node));
    }
}

void PdfXRef::Write(OutputStreamDevice& device, charbuff& buffer)
{
    XRefSubSectionList sections;
    buildSubSections(sections);

    m_offset = device.GetPosition();
    this->BeginWrite(device, buffer);

    for (unsigned i = 0; i < sections.GetSize(); i++)
    {
        auto& section = sections[i];
        PODOFO_ASSERT(section.GetCount() != 0);

        // when there is only one, then we need to start with 0 and the bogus object...
        this->WriteSubSection(device, section.GetFirst(), section.GetCount(), buffer);

        auto it = section.begin();
        PdfReference ref;
        PdfXRefEntry entry;
        while (section.TryGetXRefEntryIncrement(it, ref, entry))
            WriteXRefEntry(device, ref, entry, buffer);
    }

    endWrite(device, buffer);
}

void PdfXRef::buildSubSections(XRefSubSectionList& sections)
{
    // Check if this an incremental update and we have a valid
    // previous XRef section to refer to
    if (m_writer->IsIncrementalUpdate() && m_writer->GetPrevXRefOffset() > 0)
    {
        // The following effectively adds a free entry for object 0
        // with generation number (65535, meaning it's unavailable).
        // It was present in PoDoFo 0.9.x[1] since they introduced
        // incremental saving/signing. It may be required to
        // workaround opening of files with incremental saves, as
        // Acrobat is sometimes silly if there's no cross-reference
        // section starting with ObjNum 0
        // [1] https://github.com/podofo/podofo/blob/5723d09bbab68340a3a32923d90910c3d6912cdd/src/podofo/base/PdfWriter.cpp#L182
        auto section = &sections.PushSubSection();
        for (auto& obj : m_xrefObjects)
        {
            // Try to add the XRef object to the current section,
            // or append a new one
             if (!section->TryAddObject(obj))
                section = &sections.PushSubSection(obj);
        }
    }
    else
    {
        // Per ISO 32000-2:2020 7.5.4 Cross-reference table "For a PDF file
        // that has never been incrementally updated, the cross-reference
        // section shall contain only one subsection, whose object numbering
        // begins at 0"
        (void)sections.PushSubSection(m_xrefObjects, 0, m_writer->GetObjects().GetLastObjectNumber());
    }
}

uint32_t PdfXRef::GetSize() const
{
    // From the PdfReference: /Size's value is 1 greater than the highest object number used in the file.
    return m_writer->GetObjects().GetLastObjectNumber() + 1;
}

void PdfXRef::BeginWrite(OutputStreamDevice& device, charbuff& buffer)
{
    (void)buffer;
    device.Write("xref\n");
}

void PdfXRef::WriteSubSection(OutputStreamDevice& device, uint32_t first, uint32_t count, charbuff& buffer)
{
#ifndef VERBOSE_DEBUG_DISABLED
    PoDoFo::LogMessage(PdfLogSeverity::Debug, "Writing XRef section: {} {}", first, count);
#endif // DEBUG
    utls::FormatTo(buffer, "{} {}\n", first, count);
    device.Write(buffer);
}

void PdfXRef::WriteXRefEntry(OutputStreamDevice& device, const PdfReference& ref, const PdfXRefEntry& entry, charbuff& buffer)
{
    (void)ref;
    uint64_t variant;
    switch (entry.Type)
    {
        case PdfXRefEntryType::Free:
        {
            variant = entry.ObjectNumber;
            break;
        }
        case PdfXRefEntryType::InUse:
        {
            variant = entry.Offset;
            break;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    utls::FormatTo(buffer, "{:010d} {:05d} {} \n", variant, entry.Generation, XRefEntryTypeToChar(entry.Type));
    device.Write(buffer);
}

void PdfXRef::EndWriteImpl(OutputStreamDevice& device, charbuff& buffer)
{
    PdfObject trailer;

    // if we have a dummy offset we write also a prev entry to the trailer
    m_writer->FillTrailerObject(trailer, GetSize(), false);

    device.Write("trailer\n");

    // NOTE: Do not encrypt the trailer dictionary
    trailer.WriteFinal(device, m_writer->GetWriteFlags(), nullptr, buffer);
}

void PdfXRef::endWrite(OutputStreamDevice& device, charbuff& buffer)
{
    EndWriteImpl(device, buffer);
    utls::FormatTo(buffer, "startxref\n{}\n%%EOF\n", GetOffset() - m_writer->GetMagicOffset());
    device.Write(buffer);
}

bool PdfXRef::ShouldSkipWrite(const PdfReference& ref)
{
    (void)ref;
    // No object to skip in PdfXRef table
    return false;
}

size_t PdfXRef::GetOffset() const
{
    return m_offset;
}

PdfXRef::XRefSubSection::XRefSubSection() :
    m_parent(nullptr), m_Index(numeric_limits<size_t>::max()),
    m_First(numeric_limits<unsigned>::max()), m_Last(0) { }

bool PdfXRef::XRefSubSection::TryAddObject(const XRefObject& obj)
{
    // Check if the added object is the next one after the last one
    if (obj.Reference.ObjectNumber() != m_Last + 1)
        return false;

    // Insert at back, unless it's an unavailable object. Those
    // are handled as fallbacks when iterating the section
    if (obj.Reference.GenerationNumber() != UnavailableObjectGenerationNumber)
        m_Objects.emplace_back(obj.Reference, obj.Offset);

    m_Last++;
    return true;
}

bool PdfXRef::XRefSubSection::TryGetXRefEntryIncrement(iterator& it, PdfReference& ref, PdfXRefEntry& entry) const
{
    if (it.ObjectNum > m_Last)
        return false;

    const XRefObject* obj = nullptr;
    if (it.ObjectIt != m_Objects.end() && (obj = &*it.ObjectIt)->Reference.ObjectNumber() == it.ObjectNum)
    {
        // The current object number lies in the lists,
        // which contains in use and proper free objects
        ref = obj->Reference;
        if (obj->IsInUse())
            entry = PdfXRefEntry::CreateInUse((uint64_t)obj->Offset, obj->Reference.GenerationNumber());
        else
            entry = PdfXRefEntry::CreateFree(m_parent->GetNextFreeXRefObjectNumber(m_Index, it.ObjectNum + 1, std::next(it.ObjectIt)), ref.GenerationNumber());
        it.ObjectIt++;
    }
    else
    {
        // The current object number is unavailable, create a free entry for it
        ref = PdfReference(it.ObjectNum, UnavailableObjectGenerationNumber);
        entry = PdfXRefEntry::CreateFree(m_parent->GetNextFreeXRefObjectNumber(m_Index, it.ObjectNum + 1, it.ObjectIt), UnavailableObjectGenerationNumber);
    }

    it.ObjectNum++;
    return true;
}

PdfXRef::XRefSubSection::iterator PdfXRef::XRefSubSection::begin() const
{
    return iterator(m_First, m_Objects.begin());
}

uint32_t PdfXRef::XRefSubSection::GetCount() const
{
    return m_Last - m_First + 1;
}

PdfXRef::XRefObject::XRefObject(const PdfReference& ref, int64_t offset)
    : Reference(ref), Offset(offset) { }

bool PdfXRef::XRefObject::IsFree() const
{
    return Offset < 0;
}

bool PdfXRef::XRefObject::IsInUse() const
{
    return Offset >= 0;
}

bool PdfXRef::XRefObject::IsUnavailable() const
{
    return Reference.GenerationNumber() == UnavailableObjectGenerationNumber;
}

PdfXRef::XRefSubSection::iterator::iterator(uint32_t objNum, XRefObjectList::const_iterator&& objectIt)
    : ObjectNum(objNum), ObjectIt(std::move(objectIt)) { }

PdfXRef::XRefSubSectionList::XRefSubSectionList() { }

PdfXRef::XRefSubSection& PdfXRef::XRefSubSectionList::PushSubSection()
{
    size_t index = m_Sections.size();
    auto& ret = *m_Sections.emplace_back(new XRefSubSection());
    ret.m_parent = this;
    ret.m_Index = index;
    ret.m_First = 0;
    ret.m_Last = 0;
    return ret;
}

PdfXRef::XRefSubSection& PdfXRef::XRefSubSectionList::PushSubSection(const XRefObject& item)
{
    size_t index = m_Sections.size();
    auto& ret = *m_Sections.emplace_back(new XRefSubSection());
    ret.m_parent = this;
    ret.m_Index = index;
    if (!item.IsUnavailable())
        ret.m_Objects.emplace_back(item.Reference, item.Offset);

    ret.m_First = item.Reference.ObjectNumber();
    ret.m_Last = ret.m_First;
    return ret;
}

PdfXRef::XRefSubSection& PdfXRef::XRefSubSectionList::PushSubSection(const XRefObjectSet& objects, uint32_t firstObjectNum, uint32_t lastObjectNum)
{
    PODOFO_ASSERT(firstObjectNum <= lastObjectNum);
    size_t index = m_Sections.size();
    auto& ret = *m_Sections.emplace_back(new XRefSubSection());
    ret.m_parent = this;
    ret.m_Index = index;
    for (auto& obj : objects)
        (void)ret.m_Objects.emplace_back(obj.Reference, obj.Offset);

    PODOFO_ASSERT(ret.m_Objects.size() == 0 || (ret.m_Objects.front().Reference.ObjectNumber() >= firstObjectNum
        && ret.m_Objects.back().Reference.ObjectNumber() <= lastObjectNum));
    ret.m_First = firstObjectNum;
    ret.m_Last = lastObjectNum;
    return ret;
}

uint32_t PdfXRef::XRefSubSectionList::GetNextFreeXRefObjectNumber(size_t sectionIdx, uint32_t currObjectNum, XRefObjectList::const_iterator itObject) const
{
    auto sectionIt = m_Sections.begin() + sectionIdx;
    auto objects = &(*sectionIt)->GetObjects();
    while (true)
    {
        auto& section = **sectionIt;
        for (; currObjectNum <= section.GetLast(); currObjectNum++)
        {
            // Iterate remaining objects in the section, determining if they lies
            // in the list or they are absent, meaning they are unavailable
            if (itObject != objects->end() && itObject->Reference.ObjectNumber() == currObjectNum)
            {
                if (itObject->IsFree())
                    return currObjectNum;

                // Increment the list iterator and test the next object number
                itObject++;
                continue;
            }

            // If the object is not found in the list, it's unavailable
            // so it's free by definition
            return currObjectNum;
        }

        sectionIt++;
        if (sectionIt == m_Sections.end())
            break;

        objects = &(*sectionIt)->GetObjects();
        currObjectNum = (*sectionIt)->GetFirst();
        itObject = objects->begin();
    }

    return 0;
}
