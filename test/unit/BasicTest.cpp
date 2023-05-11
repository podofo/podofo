/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

/** This class tests the basic integer and other types PoDoFo uses
 *  to make sure they satisfy its requirements for behaviour, size, etc.
 */

TEST_CASE("BasicTypeTest")
{
    REQUIRE(std::numeric_limits<uint64_t>::max() >= 9999999999);
}

TEST_CASE("ErrorFilePath")
{
    try
    {
        PdfObject test;
        test.GetString();
    }
    catch (const PdfError& err)
    {
        auto path = err.GetCallStack().front().GetFilePath();
        REQUIRE(fs::u8path(path) == fs::u8path("main") / "PdfVariant.cpp");
    }
}

TEST_CASE("TestMetadataSet")
{
    PdfMemDocument doc;
    auto& metadata = doc.GetMetadata();
    metadata.SetTitle(PdfString("TestTitle"));
    REQUIRE(metadata.GetTitle()->GetString() == "TestTitle");
    metadata.SetTitle(PdfString("TestTitle2"));
    REQUIRE(metadata.GetTitle()->GetString() == "TestTitle2");
    metadata.SetTitle(nullptr);
    REQUIRE(metadata.GetTitle() == nullptr);
}
