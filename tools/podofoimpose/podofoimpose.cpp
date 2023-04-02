/**
 * SPDX-FileCopyrightText: (C) 2007 Pierre Marchand <pierre@moulindetouvois.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "pdftranslator.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <cstdio>

using namespace std;
using namespace PoDoFo;
using namespace PoDoFo::Impose;

struct _params
{
    string executablePath;
    string inFilePath;
    string outFilePath;
    string planFilePath;
    PlanReader planReader;
} params;

void usage()
{
    cerr << "Usage : " << params.executablePath << " Input Output Plan [Interpreter]" << endl;
    cerr << "***" << endl;
    cerr << "\tInput is a PDF file or a file which contains a list of PDF file paths" << endl << endl;
    cerr << "\tOutput will be a PDF file" << endl << endl;
    cerr << "\tPlan is an imposition plan file" << endl << endl;
    cerr << "\t[Interpreter] Can be \"native\" (default value) or \"lua\"" << endl << endl;
    cerr << "PoDoFo Version: " << PODOFO_VERSION_STRING << endl << endl;
}

void parseCommandLine(const cspan<string_view>& args)
{
    params.executablePath = args[0];

    if (args.size() < 4)
    {
        usage();
        exit(-1);
    }

    params.inFilePath = args[1];
    params.outFilePath = args[2];
    params.planFilePath = args[3];
    params.planReader = PlanReader::Legacy;
    if (args.size() >= 5)
    {
        string native("native");
        string lua("lua");
        string interpreter(args[4]);

        if (!interpreter.compare(native))
            params.planReader = PlanReader::Legacy;
        else if (!interpreter.compare(lua))
            params.planReader = PlanReader::Lua;
    }
}

/**
 * Return values:
 *
 * 0 : success
 * 1 : bad command line arguments
 */
void Main(const cspan<string_view>& args)
{
#if 0
    PdfError::EnableDebug(false);
    PdfError::EnableLogging(false);
#endif
    parseCommandLine(args);

    cerr << "Source : " << params.inFilePath << endl;
    cerr << "Target : " << params.outFilePath << endl;
    cerr << "Plan   : " << params.planFilePath << endl;


    PdfTranslator* translator = new  PdfTranslator;

    translator->setSource(params.inFilePath);
    translator->setTarget(params.outFilePath);
    translator->loadPlan(params.planFilePath, params.planReader);

    translator->impose();
}
