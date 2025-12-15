/**
 * SPDX-FileCopyrightText: (C) 2007 Pierre Marchand <pierre@moulindetouvois.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "pdftranslator.h"

#include <iostream>
#include <string>

using namespace std;
using namespace PoDoFo;
using namespace PoDoFo::Impose;

namespace
{
    struct ImposeParams
    {
        string executablePath;
        string inFilePath;
        string outFilePath;
        string planFilePath;
        PlanReader planReader = PlanReader::Lua;
    };
}

static void usage(const ImposeParams& params)
{
    cerr << "Usage : " << params.executablePath << " Input Output Plan [Interpreter]" << endl;
    cerr << "***" << endl;
    cerr << "\tInput is a PDF file or a file which contains a list of PDF file paths" << endl << endl;
    cerr << "\tOutput will be a PDF file" << endl << endl;
    cerr << "\tPlan is an imposition plan file" << endl << endl;
    cerr << "\t[Interpreter] Can be \"native\" (default value) or \"lua\"" << endl << endl;
    cerr << "PoDoFo Version: " << PODOFO_VERSION_STRING << endl << endl;
}

static void parseCommandLine(const cspan<string_view>& args, ImposeParams& params)
{
    params.executablePath = args[0];
    if (args.size() < 4)
    {
        usage(params);
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
    ImposeParams params;

    parseCommandLine(args, params);

    cerr << "Source : " << params.inFilePath << endl;
    cerr << "Target : " << params.outFilePath << endl;
    cerr << "Plan   : " << params.planFilePath << endl;

    PdfTranslator translator;
    translator.setSource(params.inFilePath);
    translator.setTarget(params.outFilePath);
    translator.loadPlan(params.planFilePath, params.planReader);
    translator.impose();
}
