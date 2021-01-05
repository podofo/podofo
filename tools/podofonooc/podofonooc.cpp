/***************************************************************************
 *   Copyright (C) 2020  Ivan Romanov <drizt72@zoho.eu                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110, USA                *
 ***************************************************************************/

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

#include <podofo.h>

using namespace std;

int main (int argc, char * argv[])
{
    using namespace PoDoFo;
    PoDoFo::PdfMemDocument * doc = NULL;
    int result = 0;

    try {
        PoDoFo::PdfError::EnableDebug(false);
        if (argc < 3)
        {
            cout << "Usage" << endl;
            cout << "  " << argv[0] << " <in.pdf> <out.pdf> [OC_name]..." << endl;
            return EXIT_FAILURE;
        }

        if ( string("-") == argv[1] )
        {
            cin >> std::noskipws;
#ifdef _MSC_VER
            _setmode(_fileno(stdin), _O_BINARY); // @TODO: MSVC specific binary setmode -- not sure if other platforms need it
            cin.sync_with_stdio();
#endif
            istream_iterator<char> it(std::cin);
            istream_iterator<char> end;
            string buffer(it, end);
            doc = new PoDoFo::PdfMemDocument();
            doc->LoadFromBuffer( buffer.c_str(), static_cast<long>(buffer.size()) );
        }
        else
        {
            doc = new PoDoFo::PdfMemDocument(argv[1]);
        }

        vector<string> ocToRemove;

        for (int i = 3; i < argc; i++)
        {
            ocToRemove.push_back(string(argv[i]));
        }

        int ocCount = 0;
        PdfObject * ocProperties = doc->GetTrailer()->MustGetIndirectKey("Root")->GetIndirectKey("OCProperties");

        if (ocProperties) {
            PdfObject * ocgs = ocProperties->GetIndirectKey("OCGs");

            if (ocgs)
            {
                PdfVecObjects & docObjects = doc->GetObjects();
                PdfArray ocgsArr = ocgs->GetArray();
                for (PdfArray::iterator it = ocgsArr.begin(); it != ocgsArr.end(); it++)
                {
                    PdfReference ocgRef = (*it).GetReference();
                    if (!docObjects.GetObject(ocgRef))
                        continue;

                    const string &ocgName = docObjects.GetObject(ocgRef)->MustGetIndirectKey("Name")->GetString().GetStringUtf8();

                    if (!ocToRemove.empty() && find(ocToRemove.begin(), ocToRemove.end(), ocgName) == ocToRemove.end())
                    {
                        continue;
                    }

                    for (int i = docObjects.GetSize() - 1; i >= 0; i--)
                    {
                        PdfObject * ob = docObjects[i];

                        if (ob->IsDictionary())
                        {
                            PdfObject * oc = ob->GetDictionary().GetKey("OC");
                            if (oc) {
                                PdfReference ocRef = oc->GetReference();
                                if (ocRef == ocgRef || (docObjects.GetObject(ocRef)->GetIndirectKey("OCGs") && docObjects.GetObject(ocRef)->GetDictionary().GetKey("OCGs")->GetReference() == ocgRef))
                                {
                                    docObjects.RemoveObject(ob->Reference());
                                    ocCount++;
                                }
                            }
                        }
                    }

                    docObjects.RemoveObject(ocgRef);
                }
            }
        }

        if (ocCount)
        {
            doc->Write(argv[2]);
        }
        else
        {
            cout << "No optional content in this PDF" << endl;
        }
    } catch( PdfError & e ) {
        std::cerr << "Error: An error " << e.GetError() << " occurred during the process of the pdf file:" << std::endl;
        e.PrintErrorMsg();

        result = e.GetError();
    }

    if( doc )
        delete doc;

    return result;
}
