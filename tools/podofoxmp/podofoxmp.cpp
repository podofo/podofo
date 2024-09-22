/**
 * SPDX-FileCopyrightText: (C) 2010 Ian Ashley <Ian.Ashley@opentext.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>

#include <iostream>
#include <iterator>
#include <string>
#include <cstdlib>
#include <cstdio>

#ifdef _MSC_VER
#include <io.h>
#include <fcntl.h>
#endif

#include <podofo/podofo.h>

using namespace std;
using namespace PoDoFo;

void Main(const cspan<string_view>& args)
{
    PdfMemDocument* doc = nullptr;
    PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::None);
    if (args.size() != 2 && args.size() != 4)
    {
        cout << "Syntax" << endl;
        cout << "  " << args[0] << " <pdf file> - display the XMP in a file (use \"-\" to specify stdin)" << endl;
        cout << "or" << endl;
        cout << "  " << args[0] << " <src pdf file> <xmp file> <new pdf file> - create a new PDF with the XMP in" << endl;
        exit(-1);
    }

    if (args[1] == "-")
    {
        cin >> noskipws;
#ifdef _MSC_VER
        _setmode(_fileno(stdin), _O_BINARY); // @TODO: MSVC specific binary setmode -- not sure if other platforms need it
        cin.sync_with_stdio();
#endif
        istream_iterator<char> it(cin);
        istream_iterator<char> end;
        string buffer(it, end);
        doc = new PdfMemDocument();
        doc->LoadFromBuffer(buffer);
    }
    else
    {
        doc = new PdfMemDocument();
        doc->Load(args[1]);
    }


    if (args.size() == 2)
    {
        auto metadata = doc->GetCatalog().GetMetadataObject();
        if (metadata == nullptr)
        {
            cout << "No metadata" << endl;
        }
        else
        {
            auto stream = metadata->GetStream();
            if (stream != nullptr)
            {
                charbuff buffer = stream->GetCopy();
                printf("%s", buffer.data());
                printf("\n");
                fflush(stdout);
            }
        }
    }

    if (args.size() == 4)
    {
        char* xmpBuf;
        FILE* fp;

        if ((fp = fopen(args[2].data(), "rb")) == nullptr)
            cout << "Cannot open " << args[2] << endl;
        else
        {
            if (fseek(fp, 0, SEEK_END) == -1)
            {
                fclose(fp);
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to seek to the end of the file");
            }

            long rc = ftell(fp);
            if (rc == -1)
            {
                fclose(fp);
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to read size of the file");
            }

            size_t xmpLen = (size_t)rc;
            xmpBuf = new char[xmpLen];
            if (!xmpBuf)
            {
                fclose(fp);
                PODOFO_RAISE_ERROR(PdfErrorCode::OutOfMemory);
            }

            if (fseek(fp, 0, SEEK_SET) == -1)
            {
                delete[] xmpBuf;
                fclose(fp);

                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to seek to the beginning of the file");
            }

            if (fread(xmpBuf, 1, xmpLen, fp) != xmpLen)
            {
                delete[] xmpBuf;
                fclose(fp);

                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to read whole file into the memory");
            }

            auto metadata = doc->GetCatalog().GetMetadataObject();
            if (metadata != nullptr)
            {
                metadata->GetOrCreateStream().SetData({ xmpBuf, xmpLen });
            }
            else
            {
                metadata = &doc->GetObjects().CreateDictionaryObject("Metadata");
                metadata->GetDictionary().AddKey("Subtype", PdfName("XML"));
                metadata->GetOrCreateStream().SetData({ xmpBuf, xmpLen });
                doc->GetCatalog().GetDictionary().AddKey("Metadata", metadata->GetIndirectReference());
            }

            delete[] xmpBuf;
            doc->Save(args[3]);
        }
    }

    if (doc)
        delete doc;
}
