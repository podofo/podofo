/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfContents.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include <podofo/auxiliary/StreamDevice.h>

#include "PdfDocument.h"
#include "PdfPage.h"

using namespace PoDoFo;

PdfContents::PdfContents(PdfPage &parent, PdfObject &obj)
    : m_parent(&parent), m_object(&obj)
{
}

PdfContents::PdfContents(PdfPage &parent) 
    : m_parent(&parent), m_object(&parent.GetDocument().GetObjects().CreateArrayObject())
{
    reset();
}

void PdfContents::Reset()
{
    m_object = &m_parent->GetDocument().GetObjects().CreateArrayObject();
    reset();
}

charbuff PdfContents::GetCopy() const
{
    charbuff ret;
    CopyTo(ret);
    return ret;
}

void PdfContents::CopyTo(charbuff& buffer) const
{
    buffer.clear();
    BufferStreamDevice stream(buffer);
    CopyTo(stream);
}

void PdfContents::CopyTo(OutputStream& stream) const
{
    const PdfArray* arr;
    if (m_object->TryGetArray(arr))
    {
        copyTo(stream, *arr);
    }
    else if (m_object->IsDictionary())
    {
        auto objStream = m_object->GetStream();
        if (objStream != nullptr)
            objStream->CopyTo(stream);
    }
    else
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);
    }
}

PdfObjectStream & PdfContents::CreateStreamForAppending(PdfStreamAppendFlags flags)
{
    PdfArray *arr;
    if (m_object->IsArray())
    {
        arr = &m_object->GetArray();
    }
    else if (m_object->IsDictionary())
    {
        // Create a /Contents array and put the current stream into it
        auto& newObjArray = m_parent->GetDocument().GetObjects().CreateArrayObject();
        m_parent->GetDictionary().AddKeyIndirect("Contents"_n, newObjArray);
        arr = &newObjArray.GetArray();
        arr->AddIndirect(*m_object);
        m_object = &newObjArray;
    }
    else
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);
    }

    if (arr->GetSize() != 0 && (flags & PdfStreamAppendFlags::NoSaveRestorePrior) == PdfStreamAppendFlags::None)
    {
        // Record all content and readd into a new stream that
        // substitute all the previous streams
        charbuff buffer;
        BufferStreamDevice device(buffer);
        copyTo(device, *arr);

        if (buffer.size() != 0)
        {
            arr->Clear();
            auto& newobj = m_parent->GetDocument().GetObjects().CreateDictionaryObject();
            arr->AddIndirect(newobj);
            auto &stream = newobj.GetOrCreateStream();
            auto output = stream.GetOutputStream();
            output.Write("q\n");
            output.Write(buffer);
            // TODO: Avoid adding useless \n prior Q
            output.Write("\nQ");
        }
    }

    // Create a new stream, add it to the array, return it
    auto& newStm = m_parent->GetDocument().GetObjects().CreateDictionaryObject();
    if ((flags & PdfStreamAppendFlags::Prepend) == PdfStreamAppendFlags::Prepend)
        arr->insert(arr->begin(), newStm.GetIndirectReference());
    else
        arr->Add(newStm.GetIndirectReference());
    return newStm.GetOrCreateStream();
}

void PdfContents::copyTo(OutputStream& stream, const PdfArray& arr) const
{
    for (unsigned i = 0; i < arr.GetSize(); i++)
    {
        const PdfObjectStream* objStream;
        auto streamObj = arr.FindAt(i);
        if (streamObj != nullptr && (objStream = streamObj->GetStream()) != nullptr)
            objStream->CopyTo(stream);
    }
}

void PdfContents::reset()
{
    m_parent->GetDictionary().AddKeyIndirect("Contents"_n, *m_object);
}
