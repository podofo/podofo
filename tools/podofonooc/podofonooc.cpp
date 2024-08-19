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

void Main(const cspan<string_view>& args)
{
    using namespace PoDoFo;
    PdfMemDocument* doc = nullptr;

    PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::None);
    if (args.size() < 3)
    {
        cout << "Usage" << endl;
        cout << "  " << args[0] << " <in.pdf> <out.pdf> [OC_name]..." << endl;
        exit(-1);
    }

    if (args[1] == "-")
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
        doc->Load(args[1]);
    }

    vector<string> ocToRemove;
    for (unsigned i = 3; i < args.size(); i++)
    {
        ocToRemove.push_back(string(args[i]));
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
            for (PdfArray::iterator it1 = ocgsArr.begin(); it1 != ocgsArr.end(); it1++)
            {
                PdfReference ocgRef = (*it1).GetReference();
                if (!objects.GetObject(ocgRef))
                    continue;

                auto ocgName = objects.MustGetObject(ocgRef).GetDictionary().MustFindKey("Name").GetString().GetString();

                if (!ocToRemove.empty() && find(ocToRemove.begin(), ocToRemove.end(), ocgName) == ocToRemove.end())
                    continue;

                /* FIXME: The following used PdfIndirectObjectList::RemoveObject(), which has been removed
                   It should be fixed by just removing they key from the dictionaries and then rely on garbage collection
                for (auto it2 = objects.rbegin(); it2 != objects.rend(); it2++)
                {
                    auto ob = *it2;
                    if (ob->IsDictionary())
                    {
                        auto oc = ob->GetDictionary().GetKey("OC");
                        if (oc != nullptr)
                        {
                            PdfReference ocRef = oc->GetReference();
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
                */
            }
        }
    }

    if (ocCount)
    {
        doc->Save(args[2]);
    }
    else
    {
        cout << "No optional content in this PDF" << endl;
    }

    if (doc)
        delete doc;
}
