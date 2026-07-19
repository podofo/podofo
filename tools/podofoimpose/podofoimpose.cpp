/**
 * SPDX-FileCopyrightText: (C) 2007 Pierre Marchand <pierre@moulindetouvois.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "pdftranslator.h"

#include <iostream>
#include <string>

#include <tclap/CmdLine.h>

using namespace std;
using namespace PoDoFo;
using namespace PoDoFo::Impose;

namespace
{
    struct ImposeParams
    {
        string inFilePath;
        string outFilePath;
        string planFilePath;
        PlanReader planReader = PlanReader::Legacy;
    };
}

static void parseCommandLine(const cspan<string_view>& args, ImposeParams& params)
{
    vector<string> cliArgs(args.begin(), args.end());

    TCLAP::CmdLine cmd("Impose a PDF file following an imposition plan", ' ', PODOFO_VERSION_STRING);

    TCLAP::UnlabeledValueArg<string> inputArg("Input", "A PDF file or a file which contains a list of PDF file paths",
        true, "", "input", cmd);
    TCLAP::UnlabeledValueArg<string> outputArg("Output", "The output PDF file", true, "", "output", cmd);
    TCLAP::UnlabeledValueArg<string> planArg("Plan", "An imposition plan file", true, "", "plan", cmd);

    vector<string> interpreters{ "native", "lua" };
    TCLAP::ValuesConstraint<string> interpreterConstraint(interpreters);
    TCLAP::UnlabeledValueArg<string> interpreterArg("Interpreter", "The imposition plan interpreter to use",
        false, "native", &interpreterConstraint, cmd);

    cmd.parse(cliArgs);

    params.inFilePath = inputArg.getValue();
    params.outFilePath = outputArg.getValue();
    params.planFilePath = planArg.getValue();
    params.planReader = interpreterArg.getValue() == "lua" ? PlanReader::Lua : PlanReader::Legacy;
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
    translator.SetInputOutput(params.inFilePath, params.outFilePath);
    translator.LoadPlan(params.planFilePath, params.planReader);
    translator.Impose();
}
