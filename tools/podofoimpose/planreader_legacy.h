/**
 * SPDX-FileCopyrightText: (C) 2008 Pierre Marchand <pierremarc@oep-h.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PLANREADER_LEGACY_H
#define PLANREADER_LEGACY_H

#include "impositionplan.h"

#include <string>
#include <vector>

namespace PoDoFo::Impose
{
    class PlanReader_Legacy
    {
    public:
        PlanReader_Legacy(const std::string& plan, ImpositionPlan& Imp);
    private:
        int sortLoop(std::vector<std::string>& memfile, int numline);
    private:
        ImpositionPlan* m_imp;
    };
}

#endif // PLANREADER_LEGACY_H
