// SPDX-FileCopyrightText: 2007 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <PdfTest.h>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

using namespace std;
using namespace PoDoFo;

int main(int argc, char* argv[])
{
#if defined(_MSC_VER) && defined(_DEBUG)
    // Report CRT debug assertions (eg. checked STL subscripts) on stderr and
    // abort, instead of showing dialogs that block unattended runs
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif

    // Avoid performing test initialization on Catch2 query for tests,
    // which it does with switches "--list-test-names-only" and "--list-reporters"
    if (argc <= 1 || string_view(argv[1]).find("--list") == string_view::npos)
    {
        PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::Warning);

        // Add a fonts directory for more consistents run
        auto fontPath = TestUtils::GetTestInputPath() / "Fonts";
        if (!fs::exists(fontPath))
        {
            throw runtime_error("Missing Fonts directory. Ensure you have correctly "
                "fetched \"extern/resources\" git submodule");
        }

        PdfCommon::AddFontDirectory(fontPath.u8string());
        PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::Warning);
    }

    return Catch::Session().run(argc, argv);
}
