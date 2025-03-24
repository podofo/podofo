/**
 * SPDX-FileCopyrightText: (C) 2025 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: MIT-0
 */

#include <PdfTest.h>

#include <podofo/private/SASLprep.h>

using namespace std;
using namespace PoDoFo;

// Ported from https://github.com/reklatsmasters/saslprep/blob/master/test/index.js
TEST_CASE("TestSALSprep")
{
    string prepd;
    INFO("Should work with latin letters");
    sprep::TrySASLprep("user", prepd);
    REQUIRE(prepd == "user");

    INFO("Case should be preserved");
    sprep::TrySASLprep("USER", prepd);
    REQUIRE(prepd == "USER");

    INFO("Should remove 'mapped to nothing' characters");
    REQUIRE(sprep::TrySASLprep("I\u00ADX", prepd));
    REQUIRE(prepd == "IX");

    INFO("Should replace 'non-ASCII space characters' with space");
    REQUIRE(sprep::TrySASLprep("a\u00A0b", prepd));
    REQUIRE(prepd == "a\u0020b");

    INFO("Should normalize as NFKC");
    REQUIRE(sprep::TrySASLprep("\u00AA", prepd));
    REQUIRE(prepd == "a");
    REQUIRE(sprep::TrySASLprep("\u2168", prepd));
    REQUIRE(prepd == "IX");

    INFO("Should throw with prohibited characters");
    INFO("C.2.1 ASCII control characters");
    REQUIRE(!sprep::TrySASLprep("a\u007Fb", prepd));

    INFO("C.2.2 Non-ASCII control characters");
    REQUIRE(!sprep::TrySASLprep("a\u06DDb", prepd));

    INFO("C.3 Private use");
    REQUIRE(!sprep::TrySASLprep("a\uE000b", prepd));

    INFO("C.4 Non-character code points");
    REQUIRE(!sprep::TrySASLprep("a\U0001fffeb", prepd));

    INFO("C.6 Inappropriate for plain text");
    REQUIRE(!sprep::TrySASLprep("a\uFFF9b", prepd));

    INFO("C.7 Inappropriate for canonical representation");
    REQUIRE(!sprep::TrySASLprep("a\u2FF0b", prepd));

    INFO("C.8 Change display properties or are deprecated");
    REQUIRE(!sprep::TrySASLprep("a\u200Eb", prepd));

    INFO("C.9 Tagging characters");
    REQUIRE(!sprep::TrySASLprep("a\U000e0001b", prepd));

    INFO("Should not containt RandALCat and LCat bidi");
    REQUIRE(!sprep::TrySASLprep("a\u06DD\u00AAb", prepd));

    INFO("RandALCat should be first and last");
    REQUIRE(sprep::TrySASLprep("\u0627\u0031\u0628", prepd));
    REQUIRE(!sprep::TrySASLprep("\u0627\u0031", prepd));

    INFO("Should not handle unassigned code points");
    REQUIRE(!sprep::TrySASLprep("a\u0487", prepd));
}
