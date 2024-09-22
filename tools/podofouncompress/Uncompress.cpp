/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Uncompress.h"

#include <cstdio>

using namespace std;
using namespace PoDoFo;

UnCompress::UnCompress()
    : m_document(nullptr)
{
}

UnCompress::~UnCompress()
{
    delete m_document;
}

void UnCompress::Init(const string_view& input, const string_view& output)
{
    if (m_document)
        delete m_document;

    m_document = new PdfMemDocument();
    m_document->Load(input);

    this->UncompressObjects();

    m_document->Save(output, PdfSaveOptions::Clean
        | PdfSaveOptions::NoMetadataUpdate | PdfSaveOptions::NoFlateCompress);
}

void UnCompress::UncompressObjects()
{
    auto& objects = m_document->GetObjects();

    for (auto obj : objects)
    {
        printf("Reading %i %i R\n", obj->GetIndirectReference().ObjectNumber(), obj->GetIndirectReference().GenerationNumber());
        auto stream = obj->GetStream();
        if (stream == nullptr)
            continue;

        {
            try
            {
                printf("-> Uncompressing object %i %i\n",
                    obj->GetIndirectReference().ObjectNumber(), obj->GetIndirectReference().GenerationNumber());

                printf("-> Original Length: %zu\n", stream->GetLength());
                try
                {
                    stream->Unwrap();
                }
                catch (PdfError& e)
                {
                    if (e.GetCode() == PdfErrorCode::FlateError)
                    {
                        // Ignore ZLib errors
                        fprintf(stderr, "WARNING: ZLib error ignored for this object.\n");
                    }
                    else
                        throw;
                }
                printf("-> Uncompressed Length: %zu\n", stream->GetLength());
            }
            catch (PdfError& e)
            {
                e.PrintErrorMsg();
                if (e.GetCode() != PdfErrorCode::UnsupportedFilter)
                    throw;
            }
        }
    }
}
