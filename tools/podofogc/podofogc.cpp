/**
 * SPDX-FileCopyrightText: (C) 2010 Ian Ashley <Ian.Ashley@opentext.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <iostream>
#include <cstdlib>
#include <cstdio>

#include <podofo/podofo.h>

using namespace std;
using namespace PoDoFo;

void Main(const cspan<string_view>& args)
{
    PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::None);

    PdfMemDocument document;

    if (args.size() != 3)
    {
        cerr << "Usage: podofogc <input_filename> <output_filename>\n"
            << "    Performs garbage collection on a PDF file.\n"
            << "    All objects that are not reachable from within\n"
            << "    the trailer are deleted.\n"
            << flush;
        return;
    }

    cerr << "Parsing  " << args[1] << " ... (this might take a while)"
        << flush;

    bool incorrectPw = false;
    string pw;
    do
    {
        try
        {
            document.Load(args[1], pw);
        }
        catch (PdfError& e)
        {
            if (e.GetCode() == PdfErrorCode::InvalidPassword)
            {
                cout << endl << "Password :";
                std::getline(cin, pw);
                cout << endl;

                // try to continue with the new password
                incorrectPw = true;
            }
            else
            {
                throw;
            }
        }
    } while (incorrectPw);

    cerr << " done" << endl;

    cerr << "Writing..." << flush;
    document.Save(args[2]);

    cerr << "Parsed and wrote successfully" << endl;
}
