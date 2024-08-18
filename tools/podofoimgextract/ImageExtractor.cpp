/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "ImageExtractor.h"

#include <sys/stat.h>
#include <cstdlib>
#include <cstdio>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

using namespace std;
using namespace PoDoFo;

ImageExtractor::ImageExtractor()
    : m_ImageCount(0), m_fileCounter(0), m_buffer{}
{
}

void ImageExtractor::Init(const string_view& input, const string_view& output)
{
    PdfMemDocument document;
    document.Load(input);

    m_outputDirectory = output;

    for (auto obj : document.GetObjects())
    {
        if (obj->IsDictionary())
        {
            PdfObject* typeObj = obj->GetDictionary().GetKey("Type"_n);
            PdfObject* subtypeObj = obj->GetDictionary().GetKey("Subtype"_n);
            if ((typeObj && typeObj->IsName() && (typeObj->GetName() == "XObject")) ||
                (subtypeObj && subtypeObj->IsName() && (subtypeObj->GetName() == "Image")))
            {
                auto filter = obj->GetDictionary().GetKey("Filter"_n);
                if (filter != nullptr && filter->IsArray() && filter->GetArray().GetSize() == 1 &&
                    filter->GetArray()[0].IsName() && (filter->GetArray()[0].GetName() == "DCTDecode"))
                    filter = &filter->GetArray()[0];

                if (filter && filter->IsName() && (filter->GetName() == "DCTDecode"))
                {
                    // The only filter is JPEG -> create a JPEG file
                    ExtractImage(*obj, true);
                }
                else
                {
                    ExtractImage(*obj, false);
                }
            }
        }
    }
}

void ImageExtractor::ExtractImage(const PdfObject& obj, bool jpeg)
{
    FILE* file = nullptr;
    const char* extension = jpeg ? "jpg" : "ppm";

    // Do not overwrite existing files:
    do
    {
        snprintf(m_buffer, MAX_PATH, "%s/pdfimage_%04i.%s", m_outputDirectory.data(), m_fileCounter++, extension);
    }
    while (FileExists(m_buffer));

    file = fopen(m_buffer, "wb");
    if (file == nullptr)
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);
    }

    printf("-> Writing image object %s to the file: %s\n", obj.GetIndirectReference().ToString().data(), m_buffer);

    if (jpeg)
    {
        auto& memprovider = dynamic_cast<const PdfMemoryObjectStream&>(obj.GetStream()->GetProvider());
        auto& buffer = memprovider.GetBuffer();
        fwrite(buffer.data(), buffer.size(), sizeof(char), file);
    }
    else
    {
        //long lBitsPerComponent = pObject->GetDictionary().GetKey( PdfName("BitsPerComponent" ) )->GetNumber();
        // TODO: Handle colorspaces

        // Create a ppm image
        const char* ppmHeader = "P6\n# Image extracted by PoDoFo\n%u %u\n%li\n";

        fprintf(file, ppmHeader,
            (unsigned)obj.GetDictionary().MustFindKey("Width").GetNumber(),
            (unsigned)obj.GetDictionary().MustFindKey("Height").GetNumber(),
            255);

        auto buffer = obj.GetStream()->GetCopy();
        fwrite(buffer.data(), buffer.size(), sizeof(char), file);
    }

    fclose(file);
    m_ImageCount++;
}

bool ImageExtractor::FileExists(const string_view& filepath)
{
    bool result = true;

    // if there is an error, it's probably because the file doesn't yet exist
    struct stat	stBuf;
    if (stat(filepath.data(), &stBuf) == -1)
        result = false;

    return result;
}
