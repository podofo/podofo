/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfOperatorUtils.h"

using namespace std;
using namespace PoDoFo;

PdfOperator PoDoFo::GetPdfOperator(const string_view& opstr)
{
    PdfOperator op;
    if (!TryGetPdfOperator(opstr, op))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidName, "Invalid operator");

    return op;
}

bool PoDoFo::TryGetPdfOperator(const string_view& opstr, PdfOperator& op)
{
    if (opstr == "w")
    {
        op = PdfOperator::w;
        return true;
    }
    else if (opstr == "J")
    {
        op = PdfOperator::J;
        return true;
    }
    else if (opstr == "j")
    {
        op = PdfOperator::j;
        return true;
    }
    else if (opstr == "M")
    {
        op = PdfOperator::M;
        return true;
    }
    else if (opstr == "d")
    {
        op = PdfOperator::d;
        return true;
    }
    else if (opstr == "ri")
    {
        op = PdfOperator::ri;
        return true;
    }
    else if (opstr == "i")
    {
        op = PdfOperator::i;
        return true;
    }
    else if (opstr == "gs")
    {
        op = PdfOperator::gs;
        return true;
    }
    else if (opstr == "q")
    {
        op = PdfOperator::q;
        return true;
    }
    else if (opstr == "Q")
    {
        op = PdfOperator::Q;
        return true;
    }
    else if (opstr == "cm")
    {
        op = PdfOperator::cm;
        return true;
    }
    else if (opstr == "m")
    {
        op = PdfOperator::m;
        return true;
    }
    else if (opstr == "l")
    {
        op = PdfOperator::l;
        return true;
    }
    else if (opstr == "c")
    {
        op = PdfOperator::c;
        return true;
    }
    else if (opstr == "v")
    {
        op = PdfOperator::v;
        return true;
    }
    else if (opstr == "y")
    {
        op = PdfOperator::y;
        return true;
    }
    else if (opstr == "h")
    {
        op = PdfOperator::h;
        return true;
    }
    else if (opstr == "re")
    {
        op = PdfOperator::re;
        return true;
    }
    else if (opstr == "S")
    {
        op = PdfOperator::S;
        return true;
    }
    else if (opstr == "s")
    {
        op = PdfOperator::s;
        return true;
    }
    else if (opstr == "f")
    {
        op = PdfOperator::f;
        return true;
    }
    else if (opstr == "F")
    {
        op = PdfOperator::F;
        return true;
    }
    else if (opstr == "f*")
    {
        op = PdfOperator::f_Star;
        return true;
    }
    else if (opstr == "B")
    {
        op = PdfOperator::B;
        return true;
    }
    else if (opstr == "B*")
    {
        op = PdfOperator::B_Star;
        return true;
    }
    else if (opstr == "b")
    {
        op = PdfOperator::b;
        return true;
    }
    else if (opstr == "b*")
    {
        op = PdfOperator::b_Star;
        return true;
    }
    else if (opstr == "n")
    {
        op = PdfOperator::n;
        return true;
    }
    else if (opstr == "W")
    {
        op = PdfOperator::W;
        return true;
    }
    else if (opstr == "W*")
    {
        op = PdfOperator::W_Star;
        return true;
    }
    else if (opstr == "BT")
    {
        op = PdfOperator::BT;
        return true;
    }
    else if (opstr == "ET")
    {
        op = PdfOperator::ET;
        return true;
    }
    else if (opstr == "Tc")
    {
        op = PdfOperator::Tc;
        return true;
    }
    else if (opstr == "Tw")
    {
        op = PdfOperator::Tw;
        return true;
    }
    else if (opstr == "Tz")
    {
        op = PdfOperator::Tz;
        return true;
    }
    else if (opstr == "TL")
    {
        op = PdfOperator::TL;
        return true;
    }
    else if (opstr == "Tf")
    {
        op = PdfOperator::Tf;
        return true;
    }
    else if (opstr == "Tr")
    {
        op = PdfOperator::Tr;
        return true;
    }
    else if (opstr == "Ts")
    {
        op = PdfOperator::Ts;
        return true;
    }
    else if (opstr == "Td")
    {
        op = PdfOperator::Td;
        return true;
    }
    else if (opstr == "TD")
    {
        op = PdfOperator::TD;
        return true;
    }
    else if (opstr == "Tm")
    {
        op = PdfOperator::Tm;
        return true;
    }
    else if (opstr == "T*")
    {
        op = PdfOperator::T_Star;
        return true;
    }
    else if (opstr == "Tj")
    {
        op = PdfOperator::Tj;
        return true;
    }
    else if (opstr == "TJ")
    {
        op = PdfOperator::TJ;
        return true;
    }
    else if (opstr == "'")
    {
        op = PdfOperator::Quote;
        return true;
    }
    else if (opstr == "\"")
    {
        op = PdfOperator::DoubleQuote;
        return true;
    }
    else if (opstr == "d0")
    {
        op = PdfOperator::d0;
        return true;
    }
    else if (opstr == "d1")
    {
        op = PdfOperator::d1;
        return true;
    }
    else if (opstr == "CS")
    {
        op = PdfOperator::CS;
        return true;
    }
    else if (opstr == "cs")
    {
        op = PdfOperator::cs;
        return true;
    }
    else if (opstr == "SC")
    {
        op = PdfOperator::SC;
        return true;
    }
    else if (opstr == "SCN")
    {
        op = PdfOperator::SCN;
        return true;
    }
    else if (opstr == "sc")
    {
        op = PdfOperator::sc;
        return true;
    }
    else if (opstr == "scn")
    {
        op = PdfOperator::scn;
        return true;
    }
    else if (opstr == "G")
    {
        op = PdfOperator::G;
        return true;
    }
    else if (opstr == "g")
    {
        op = PdfOperator::g;
        return true;
    }
    else if (opstr == "RG")
    {
        op = PdfOperator::RG;
        return true;
    }
    else if (opstr == "rg")
    {
        op = PdfOperator::rg;
        return true;
    }
    else if (opstr == "K")
    {
        op = PdfOperator::K;
        return true;
    }
    else if (opstr == "k")
    {
        op = PdfOperator::k;
        return true;
    }
    else if (opstr == "sh")
    {
        op = PdfOperator::sh;
        return true;
    }
    else if (opstr == "BI")
    {
        op = PdfOperator::BI;
        return true;
    }
    else if (opstr == "ID")
    {
        op = PdfOperator::ID;
        return true;
    }
    else if (opstr == "EI")
    {
        op = PdfOperator::EI;
        return true;
    }
    else if (opstr == "Do")
    {
        op = PdfOperator::Do;
        return true;
    }
    else if (opstr == "MP")
    {
        op = PdfOperator::MP;
        return true;
    }
    else if (opstr == "DP")
    {
        op = PdfOperator::DP;
        return true;
    }
    else if (opstr == "BMC")
    {
        op = PdfOperator::BMC;
        return true;
    }
    else if (opstr == "BDC")
    {
        op = PdfOperator::BDC;
        return true;
    }
    else if (opstr == "EMC")
    {
        op = PdfOperator::EMC;
        return true;
    }
    else if (opstr == "BX")
    {
        op = PdfOperator::BX;
        return true;
    }
    else if (opstr == "EX")
    {
        op = PdfOperator::EX;
        return true;
    }
    else
    {
        op = PdfOperator::Unknown;
        return false;
    }
}

int PoDoFo::GetOperandCount(PdfOperator op)
{
    int count;
    if (!TryGetOperandCount(op, count))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Invalid operator");

    return count;
}

bool PoDoFo::TryGetOperandCount(PdfOperator op, int& count)
{
    switch (op)
    {
        case PdfOperator::w:
            count = 1;
            return true;
        case PdfOperator::J:
            count = 1;
            return true;
        case PdfOperator::j:
            count = 1;
            return true;
        case PdfOperator::M:
            count = 1;
            return true;
        case PdfOperator::d:
            count = 2;
            return true;
        case PdfOperator::ri:
            count = 1;
            return true;
        case PdfOperator::i:
            count = 1;
            return true;
        case PdfOperator::gs:
            count = 1;
            return true;
        case PdfOperator::q:
            count = 0;
            return true;
        case PdfOperator::Q:
            count = 0;
            return true;
        case PdfOperator::cm:
            count = 6;
            return true;
        case PdfOperator::m:
            count = 2;
            return true;
        case PdfOperator::l:
            count = 2;
            return true;
        case PdfOperator::c:
            count = 6;
            return true;
        case PdfOperator::v:
            count = 4;
            return true;
        case PdfOperator::y:
            count = 4;
            return true;
        case PdfOperator::h:
            count = 0;
            return true;
        case PdfOperator::re:
            count = 4;
            return true;
        case PdfOperator::S:
            count = 0;
            return true;
        case PdfOperator::s:
            count = 0;
            return true;
        case PdfOperator::f:
            count = 0;
            return true;
        case PdfOperator::F:
            count = 0;
            return true;
        case PdfOperator::f_Star:
            count = 0;
            return true;
        case PdfOperator::B:
            count = 0;
            return true;
        case PdfOperator::B_Star:
            count = 0;
            return true;
        case PdfOperator::b:
            count = 0;
            return true;
        case PdfOperator::b_Star:
            count = 0;
            return true;
        case PdfOperator::n:
            count = 0;
            return true;
        case PdfOperator::W:
            count = 0;
            return true;
        case PdfOperator::W_Star:
            count = 0;
            return true;
        case PdfOperator::BT:
            count = 0;
            return true;
        case PdfOperator::ET:
            count = 0;
            return true;
        case PdfOperator::Tc:
            count = 1;
            return true;
        case PdfOperator::Tw:
            count = 1;
            return true;
        case PdfOperator::Tz:
            count = 1;
            return true;
        case PdfOperator::TL:
            count = 1;
            return true;
        case PdfOperator::Tf:
            count = 2;
            return true;
        case PdfOperator::Tr:
            count = 1;
            return true;
        case PdfOperator::Ts:
            count = 1;
            return true;
        case PdfOperator::Td:
            count = 2;
            return true;
        case PdfOperator::TD:
            count = 2;
            return true;
        case PdfOperator::Tm:
            count = 6;
            return true;
        case PdfOperator::T_Star:
            count = 0;
            return true;
        case PdfOperator::Tj:
            count = 1;
            return true;
        case PdfOperator::TJ:
            count = 1;
            return true;
        case PdfOperator::Quote:
            count = 1;
            return true;
        case PdfOperator::DoubleQuote:
            count = 3;
            return true;
        case PdfOperator::d0:
            count = 2;
            return true;
        case PdfOperator::d1:
            count = 6;
            return true;
        case PdfOperator::CS:
            count = 1;
            return true;
        case PdfOperator::cs:
            count = 1;
            return true;
        case PdfOperator::SC:
            count = -1;
            return true;
        case PdfOperator::SCN:
            count = -1;
            return true;
        case PdfOperator::sc:
            count = -1;
            return true;
        case PdfOperator::scn:
            count = -1;
            return true;
        case PdfOperator::G:
            count = 1;
            return true;
        case PdfOperator::g:
            count = 1;
            return true;
        case PdfOperator::RG:
            count = 3;
            return true;
        case PdfOperator::rg:
            count = 3;
            return true;
        case PdfOperator::K:
            count = 4;
            return true;
        case PdfOperator::k:
            count = 4;
            return true;
        case PdfOperator::sh:
            count = 1;
            return true;
        case PdfOperator::BI:
            count = 0;
            return true;
        case PdfOperator::ID:
            count = 0;
            return true;
        case PdfOperator::EI:
            count = 0;
            return true;
        case PdfOperator::Do:
            count = 1;
            return true;
        case PdfOperator::MP:
            count = 1;
            return true;
        case PdfOperator::DP:
            count = 2;
            return true;
        case PdfOperator::BMC:
            count = 1;
            return true;
        case PdfOperator::BDC:
            count = 2;
            return true;
        case PdfOperator::EMC:
            count = 0;
            return true;
        case PdfOperator::BX:
            count = 0;
            return true;
        case PdfOperator::EX:
            count = 0;
            return true;
        default:
        case PdfOperator::Unknown:
            count = 0;
            return false;
    }
}

string_view PoDoFo::GetPdfOperatorName(PdfOperator op)
{
    string_view opstr;
    if (!TryGetPdfOperatorName(op, opstr))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Invalid operator");

    return opstr;
}

bool PoDoFo::TryGetPdfOperatorName(PdfOperator op, string_view& opstr)
{
    switch (op)
    {
        case PdfOperator::w:
        {
            opstr = "w";
            return true;
        }
        case PdfOperator::J:
        {
            opstr = "J";
            return true;
        }
        case PdfOperator::j:
        {
            opstr = "j";
            return true;
        }
        case PdfOperator::M:
        {
            opstr = "M";
            return true;
        }
        case PdfOperator::d:
        {
            opstr = "d";
            return true;
        }
        case PdfOperator::ri:
        {
            opstr = "ri";
            return true;
        }
        case PdfOperator::i:
        {
            opstr = "i";
            return true;
        }
        case PdfOperator::gs:
        {
            opstr = "gs";
            return true;
        }
        case PdfOperator::q:
        {
            opstr = "q";
            return true;
        }
        case PdfOperator::Q:
        {
            opstr = "Q";
            return true;
        }
        case PdfOperator::cm:
        {
            opstr = "cm";
            return true;
        }
        case PdfOperator::m:
        {
            opstr = "m";
            return true;
        }
        case PdfOperator::l:
        {
            opstr = "l";
            return true;
        }
        case PdfOperator::c:
        {
            opstr = "c";
            return true;
        }
        case PdfOperator::v:
        {
            opstr = "v";
            return true;
        }
        case PdfOperator::y:
        {
            opstr = "y";
            return true;
        }
        case PdfOperator::h:
        {
            opstr = "h";
            return true;
        }
        case PdfOperator::re:
        {
            opstr = "re";
            return true;
        }
        case PdfOperator::S:
        {
            opstr = "S";
            return true;
        }
        case PdfOperator::s:
        {
            opstr = "s";
            return true;
        }
        case PdfOperator::f:
        {
            opstr = "f";
            return true;
        }
        case PdfOperator::F:
        {
            opstr = "F";
            return true;
        }
        case PdfOperator::f_Star:
        {
            opstr = "f*";
            return true;
        }
        case PdfOperator::B:
        {
            opstr = "B";
            return true;
        }
        case PdfOperator::B_Star:
        {
            opstr = "B*";
            return true;
        }
        case PdfOperator::b:
        {
            opstr = "b";
            return true;
        }
        case PdfOperator::b_Star:
        {
            opstr = "b*";
            return true;
        }
        case PdfOperator::n:
        {
            opstr = "n";
            return true;
        }
        case PdfOperator::W:
        {
            opstr = "W";
            return true;
        }
        case PdfOperator::W_Star:
        {
            opstr = "W*";
            return true;
        }
        case PdfOperator::BT:
        {
            opstr = "BT";
            return true;
        }
        case PdfOperator::ET:
        {
            opstr = "ET";
            return true;
        }
        case PdfOperator::Tc:
        {
            opstr = "Tc";
            return true;
        }
        case PdfOperator::Tw:
        {
            opstr = "Tw";
            return true;
        }
        case PdfOperator::Tz:
        {
            opstr = "Tz";
            return true;
        }
        case PdfOperator::TL:
        {
            opstr = "TL";
            return true;
        }
        case PdfOperator::Tf:
        {
            opstr = "Tf";
            return true;
        }
        case PdfOperator::Tr:
        {
            opstr = "Tr";
            return true;
        }
        case PdfOperator::Ts:
        {
            opstr = "Ts";
            return true;
        }
        case PdfOperator::Td:
        {
            opstr = "Td";
            return true;
        }
        case PdfOperator::TD:
        {
            opstr = "TD";
            return true;
        }
        case PdfOperator::Tm:
        {
            opstr = "Tm";
            return true;
        }
        case PdfOperator::T_Star:
        {
            opstr = "T*";
            return true;
        }
        case PdfOperator::Tj:
        {
            opstr = "Tj";
            return true;
        }
        case PdfOperator::TJ:
        {
            opstr = "TJ";
            return true;
        }
        case PdfOperator::Quote:
        {
            opstr = "'";
            return true;
        }
        case PdfOperator::DoubleQuote:
        {
            opstr = "\"";
            return true;
        }
        case PdfOperator::d0:
        {
            opstr = "d0";
            return true;
        }
        case PdfOperator::d1:
        {
            opstr = "d1";
            return true;
        }
        case PdfOperator::CS:
        {
            opstr = "CS";
            return true;
        }
        case PdfOperator::cs:
        {
            opstr = "cs";
            return true;
        }
        case PdfOperator::SC:
        {
            opstr = "SC";
            return true;
        }
        case PdfOperator::SCN:
        {
            opstr = "SCN";
            return true;
        }
        case PdfOperator::sc:
        {
            opstr = "sc";
            return true;
        }
        case PdfOperator::scn:
        {
            opstr = "scn";
            return true;
        }
        case PdfOperator::G:
        {
            opstr = "G";
            return true;
        }
        case PdfOperator::g:
        {
            opstr = "g";
            return true;
        }
        case PdfOperator::RG:
        {
            opstr = "RG";
            return true;
        }
        case PdfOperator::rg:
        {
            opstr = "rg";
            return true;
        }
        case PdfOperator::K:
        {
            opstr = "K";
            return true;
        }
        case PdfOperator::k:
        {
            opstr = "k";
            return true;
        }
        case PdfOperator::sh:
        {
            opstr = "sh";
            return true;
        }
        case PdfOperator::BI:
        {
            opstr = "BI";
            return true;
        }
        case PdfOperator::ID:
        {
            opstr = "ID";
            return true;
        }
        case PdfOperator::EI:
        {
            opstr = "EI";
            return true;
        }
        case PdfOperator::Do:
        {
            opstr = "Do";
            return true;
        }
        case PdfOperator::MP:
        {
            opstr = "MP";
            return true;
        }
        case PdfOperator::DP:
        {
            opstr = "DP";
            return true;
        }
        case PdfOperator::BMC:
        {
            opstr = "BMC";
            return true;
        }
        case PdfOperator::BDC:
        {
            opstr = "BDC";
            return true;
        }
        case PdfOperator::EMC:
        {
            opstr = "EMC";
            return true;
        }
        case PdfOperator::BX:
        {
            opstr = "BX";
            return true;
        }
        case PdfOperator::EX:
        {
            opstr = "EX";
            return true;
        }
        case PdfOperator::Unknown:
        default:
            opstr = { };
            return true;
    }
}
