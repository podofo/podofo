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

int main(int argc, char* argv[])
{
    PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::None);

    PdfMemDocument document;

    if (argc != 3)
    {
        cerr << "Usage: podofogc <input_filename> <output_filename>\n"
            << "    Performs garbage collection on a PDF file.\n"
            << "    All objects that are not reachable from within\n"
            << "    the trailer are deleted.\n"
            << flush;
        return 0;
    }

    try
    {
        cerr << "Parsing  " << argv[1] << " ... (this might take a while)"
            << flush;

        bool incorrectPw = false;
        string pw;
        do
        {
            try
            {
                document.Load(argv[1]);
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
                    throw e;
                }
            }
        } while (incorrectPw);

        cerr << " done" << endl;

        cerr << "Writing..." << flush;
        document.Save(argv[2]);
    }
    catch (PdfError& e)
    {
        e.PrintErrorMsg();
        return (int)e.GetCode();
    }

    cerr << "Parsed and wrote successfully" << endl;
    return EXIT_SUCCESS;
}
