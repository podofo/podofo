/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>
#include <podofo/private/PdfFilterFactory.h>

using namespace std;
using namespace PoDoFo;

static void testFilter(PdfFilterType filterType, const bufferview& buffer);

static string_view s_testBuffer1 = "Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.";

const char s_testBuffer2[] = {
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x01,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x03,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x02,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x00,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x00,
    0x00, 0x00, 0x00, 0x00, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

TEST_CASE("TestFilters")
{
    for (unsigned i = 0; i <= (unsigned)PdfFilterType::Crypt; i++)
    {
        testFilter(static_cast<PdfFilterType>(i), { s_testBuffer1.data(), s_testBuffer1.length() });
        testFilter(static_cast<PdfFilterType>(i), { s_testBuffer2, std::size(s_testBuffer2) });
    }
}

void testFilter(PdfFilterType filterType, const bufferview& view)
{
    charbuff encoded;
    charbuff decoded;

    unique_ptr<PdfFilter> filter;
    if (!PdfFilterFactory::TryCreate(filterType, filter))
    {
        INFO(utls::Format("!!! Filter {} not implemented.\n", (int)filterType));
        return;
    }

    INFO(utls::Format("Testing Algorithm {}:", (int)filterType));
    INFO("\t-> Testing Encoding");
    try
    {
        filter->EncodeTo(encoded, view);
    }
    catch (PdfError& e)
    {
        if (e == PdfErrorCode::UnsupportedFilter)
        {
            INFO(utls::Format("\t-> Encoding not supported for filter {}.", (unsigned)filterType));
            return;
        }

        throw;
    }

    INFO("\t-> Testing Decoding");
    try
    {
        filter->DecodeTo(decoded, encoded);
    }
    catch (PdfError& e)
    {
        if (e == PdfErrorCode::UnsupportedFilter)
        {
            INFO(utls::Format("\t-> Decoding not supported for filter {}", (int)filterType));
            return;
        }

        throw;
    }

    INFO(utls::Format("\t-> Original Data Length: {}", view.size()));
    INFO(utls::Format("\t-> Encoded  Data Length: {}", encoded.size()));
    INFO(utls::Format("\t-> Decoded  Data Length: {}", decoded.size()));

    REQUIRE(decoded == view);

    INFO("\t-> Test succeeded!");
}
