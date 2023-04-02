/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/podofo.h>

#include <cstdlib>
#include <string>
#include <iostream>

#include "colorchanger.h"
#include "dummyconverter.h"
#include "grayscaleconverter.h"
#ifdef PODOFO_HAVE_LUA
#include "luaconverter.h"
#endif //  PODOFO_HAVE_LUA

using namespace std;
using namespace PoDoFo;

static void print_help()
{
    cerr << "Usage: podofocolor [converter] [inputfile] [outpufile]\n";
#ifdef PODOFO_HAVE_LUA
    cerr << "\t[converter] can be one of: dummy|grayscale|lua [planfile]\n";
#else
    cerr << "\t[converter] can be one of: dummy|grayscale\n";
#endif //  PODOFO_HAVE_LUA
    cerr << "\tpodofocolor is a tool to change all colors in a PDF file based on a predefined or Lua description.\n";
    cerr << "\nPoDoFo Version: " << PODOFO_VERSION_STRING << "\n\n";
}

/**
 * @return a converter implementation or NULL if unknown
 */
static IConverter* ConverterForName(const string_view& converterName, const string_view& lua)
{
    IConverter* converter = NULL;
    if (converterName == "dummy")
    {
        converter = new DummyConverter();
    }
    else if (converterName == "grayscale")
    {
        converter = new GrayscaleConverter();
    }
#ifdef PODOFO_HAVE_LUA
    else if (converterName == "lua")
    {
        converter = new LuaConverter(lua);
    }
#else
    (void)lua;
#endif //  PODOFO_HAVE_LUA

    return converter;
}

void Main(const cspan<string_view>& args)
{
    if (!(args.size() == 4 || args.size() == 5))
    {
        print_help();
        exit(-1);
    }

    string_view converterName = args[1];
    string_view input = args[2];
    string_view output = args[3];
    string_view lua;

    if (args.size() == 4 && converterName != "lua")
    {
        input = args[2];
        output = args[3];
    }
#ifdef PODOFO_HAVE_LUA
    else if (args.size() == 5 && converterName == "lua")
    {
        lua = argv[2];
        input = argv[3];
        output = argv[4];
    }
#endif //  PODOFO_HAVE_LUA
    else
    {
        print_help();
        exit(-3);
    }

    IConverter* converter = ConverterForName(converterName, lua);
    if (!converter)
    {
        cerr << "Aborting! Unknown converter: " << converterName << endl;
        print_help();
        exit(-2);
    }

    ColorChanger cc(converter, input, output);
    cc.start();
    delete converter;
}
