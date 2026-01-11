/**
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

/** Test that PDF permissions are handled correctly.
 */

TEST_CASE("UserHasLimitedPermissions")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("owner_user.pdf"), PdfLoadOptions::None, "user");

    REQUIRE(!doc.IsCopyAllowed());
    REQUIRE(!doc.IsPrintAllowed());
    REQUIRE(!doc.IsEditAllowed());
}

TEST_CASE("OwnerHasFullPermissions")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("owner_user.pdf"), PdfLoadOptions::None, "owner");

    REQUIRE(doc.IsCopyAllowed());
    REQUIRE(doc.IsPrintAllowed());
    REQUIRE(doc.IsEditAllowed());
}
