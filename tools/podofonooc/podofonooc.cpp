/**
 * SPDX-FileCopyrightText: (C) 2020 Ivan Romanov <drizt72@zoho.eu>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <iostream>
#include <iterator>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

#ifdef _MSC_VER
#include <io.h>
#include <fcntl.h>
#endif

#include <podofo/podofo.h>

using namespace std;
using namespace PoDoFo;

int main(int argc, char* argv[])
{
    using namespace PoDoFo;
    PdfMemDocument* doc = nullptr;
    int result = 0;

    try
    {
        PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::None);
        if (argc < 3)
        {
            cout << "Usage" << endl;
            cout << "  " << argv[0] << " <in.pdf> <out.pdf> [OC_name]..." << endl;
            return EXIT_FAILURE;
        }

        if (argv[1] == string_view("-"))
        {
            cin >> std::noskipws;
#ifdef _MSC_VER
            _setmode(_fileno(stdin), _O_BINARY); // @TODO: MSVC specific binary setmode -- not sure if other platforms need it
            cin.sync_with_stdio();
#endif
            istream_iterator<char> it(std::cin);
            istream_iterator<char> end;
            string buffer(it, end);
            doc = new PdfMemDocument();
            doc->LoadFromBuffer(buffer);
        }
        else
        {
            doc = new PdfMemDocument();
            doc->Load(argv[1]);
        }

        vector<string> ocToRemove;
        for (int i = 3; i < argc; i++)
        {
            ocToRemove.push_back(string(argv[i]));
        }

        int ocCount = 0;
        PdfObject* ocProperties = doc->GetTrailer().GetDictionary().MustFindKey("Root").GetDictionary().FindKey("OCProperties");

        if (ocProperties)
        {
            auto ocgs = ocProperties->GetDictionary().FindKey("OCGs");
            if (ocgs)
            {
                auto& objects = doc->GetObjects();
                PdfArray ocgsArr = ocgs->GetArray();
                for (PdfArray::iterator it = ocgsArr.begin(); it != ocgsArr.end(); it++)
                {
                    PdfReference ocgRef = (*it).GetReference();
                    if (!objects.GetObject(ocgRef))
                        continue;

                    const string& ocgName = objects.MustGetObject(ocgRef).GetDictionary().MustFindKey("Name").GetString().GetString();

                    if (!ocToRemove.empty() && find(ocToRemove.begin(), ocToRemove.end(), ocgName) == ocToRemove.end())
                        continue;

                    for (auto it = objects.rbegin(); it != objects.rend(); it++)
                    {
                        auto ob = *it;
                        if (ob->IsDictionary())
                        {
                            auto oc = ob->GetDictionary().GetKey("OC");
                            if (oc != nullptr)
                            {
                                PdfReference ocRef = oc->GetReference();
                                const PdfObject* ocgs;
                                if (ocRef == ocgRef || ((ocgs = objects.MustGetObject(ocRef).GetDictionary().GetKey("OCGs")) != nullptr
                                    && ocgs->GetReference() == ocgRef))
                                {
                                    objects.RemoveObject(ob->GetIndirectReference());
                                    ocCount++;
                                }
                            }
                        }
                    }

                    objects.RemoveObject(ocgRef);
                }
            }
        }

        if (ocCount)
        {
            doc->Save(argv[2]);
        }
        else
        {
            cout << "No optional content in this PDF" << endl;
        }
    }
    catch (PdfError& e)
    {
        std::cerr << "Error: An error " << (int)e.GetCode() << " occurred during the process of the pdf file:" << std::endl;
        e.PrintErrorMsg();
        result = (int)e.GetCode();
    }

    if (doc)
        delete doc;

    return result;
}
