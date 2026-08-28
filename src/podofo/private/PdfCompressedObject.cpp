// SPDX-FileCopyrightText: 2026 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include "PdfDeclarationsPrivate.h"
#include "PdfCompressedObject.h"

using namespace std;
using namespace PoDoFo;

PdfCompressedObject::PdfCompressedObject(const PdfReference& indirectReference,
        const shared_ptr<PdfObjectStreamParser>& parser, unsigned index) :
    // Parsed objects by definition are initially not dirty
    PdfObject(PdfVariant(), indirectReference, false),
    m_parser(parser),
    m_index(index),
    m_isRevised(false)
{
    EnableDelayedLoading();
}

bool PdfCompressedObject::TryUnload()
{
    if (!IsDelayedLoadDone() || m_isRevised)
        return false;

    m_Variant = PdfVariant();
    EnableDelayedLoading();
    m_parser->notifyObjectUnloaded();
    return true;
}

void PdfCompressedObject::delayedLoad()
{
    PdfVariant variant;
    if (!m_parser->TryReadObject(GetIndirectReference().ObjectNumber(), m_index, variant))
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidObject,
            "Object {} was not found in the object stream {}",
            GetIndirectReference().ToString(), m_parser->m_streamRef.ToString());
    }

    m_Variant = std::move(variant);
}

void PdfCompressedObject::SetRevised()
{
    // NOTE: Modifications are strictly local to this object. The object
    // stream storing it must not be invalidated, as it still holds the
    // contents of the other objects stored there
    m_isRevised = true;
}
