/**
 * SPDX-FileCopyrightText: (C) 2025 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <string>
#include <string_view>

namespace sprep
{
    /** Process the input string with the "SALSprep" profile of
     * RFC 3454 ("stringprep"), with NFKC normalization enabled
     * and unassigned code points disallowed
     */
    bool TrySASLprep(const std::string_view& str, std::string& prepd);
}
