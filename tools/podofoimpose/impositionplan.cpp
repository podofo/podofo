/**
 * SPDX-FileCopyrightText: (C) 2008 Pierre Marchand <pierremarc@oep-h.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "impositionplan.h"

#include <iostream>

using namespace std;
using namespace PoDoFo::Impose;

PageRecord::PageRecord(int s, int d, double r, double tx, double ty, int du, double sx, double sy) :
    sourcePage(s),
    destPage(d),
    rotate(r),
    transX(tx),
    transY(ty),
    scaleX(sx),
    scaleY(sy),
    duplicateOf(du)
{
}

PageRecord::PageRecord() :
    sourcePage(0),
    destPage(0),
    rotate(0),
    transX(0),
    transY(0),
    scaleX(1),
    scaleY(1),
    duplicateOf(0)
{
}

void PageRecord::load(const string& buffer, const map<string, string>& vars)
{
    unsigned len = (unsigned)buffer.length();
    vector<string> tokens;
    string ts;
    for (unsigned i = 0; i < len; i++)
    {
        char ci = buffer.at(i);
        if (ci == ' ')
        {
            continue;
        }
        else if (ci == ';')
        {
            tokens.push_back(ts);
            ts.clear();
            continue;
        }
        ts += ci;
    }

    if (tokens.size() != 5 && tokens.size() != 7)
    {
        sourcePage = destPage = 0; // will return false for isValid()
        cerr << "INVALID_RECORD(" << tokens.size() << ") " << buffer << endl;
        for (unsigned i = 0; i < tokens.size(); i++)
            cerr << "\t+ " << tokens.at(i) << endl;
    }

    sourcePage = static_cast<int>(calc(tokens.at(0), vars));
    destPage = static_cast<int>(calc(tokens.at(1), vars));
    if ((sourcePage < 1) || (destPage < 1))
    {
        sourcePage = destPage = 0;
    }

    rotate = calc(tokens.at(2), vars);
    transX = calc(tokens.at(3), vars);
    transY = calc(tokens.at(4), vars);
    if (tokens.size() == 7)
    {
        scaleX = calc(tokens.at(5), vars);
        scaleY = calc(tokens.at(6), vars);
    }
    else
    {
        scaleX = scaleY = 1.0;
    }

    cerr << " " << sourcePage << " " << destPage << " " << rotate << " " << transX << " " << transY << " " << scaleX << " " << scaleY << endl;

}

double PageRecord::calc(const string& s, const map<string, string>& vars)
{
    vector<string> tokens;
    unsigned tokenCount = (unsigned)s.length();
    string ts;
    for (unsigned i = 0; i < tokenCount; i++)
    {
        char ci = s[i];
        if ((ci == '+')
            || (ci == '-')
            || (ci == '*')
            || (ci == '/')
            || (ci == '%')
            || (ci == '|')
            || (ci == '"')
            || (ci == '(')
            || (ci == ')'))
        {
            // commit current string
            if (ts.length() > 0)
            {
                map<string, string>::const_iterator vit = vars.find(ts);
                if (vit != vars.end())
                {
                    tokens.push_back(Util::dToStr(calc(vit->second, vars)));
                }
                else
                {
                    tokens.push_back(ts);
                }
            }
            ts.clear();
            // append operator
            ts += ci;
            tokens.push_back(ts);
            ts.clear();
        }
        else if (ci > 32)
        {
            ts += ci;
        }
    }
    if (ts.length() > 0)
    {
        map<string, string>::const_iterator vit2 = vars.find(ts);
        if (vit2 != vars.end())
        {
            tokens.push_back(Util::dToStr(calc(vit2->second, vars)));
        }
        else
        {
            tokens.push_back(ts);
        }
    }

    return calc(tokens);
}

double PageRecord::calc(const vector<string>& tokens)
{
    if (tokens.size() == 0)
        return 0;

    double ret = 0;

    vector<double> values;
    vector<string> ops;
    ops.push_back("+");

    for (unsigned vi = 0; vi < tokens.size(); vi++)
    {
        auto& t1 = tokens[vi];
        if (t1 == "(")
        {
            vector<string> tokens2;
            int cdeep = 0;
            for (vi++; vi < tokens.size(); vi++)
            {
                auto& t2 = tokens[vi];
                if (t2 == ")")
                {
                    if (cdeep == 0)
                        break;

                    cdeep--;
                }
                else if (t2 == "(")
                {
                    cdeep++;
                }

                tokens2.push_back(t2);
            }

            values.push_back(calc(tokens2));
        }
        else if (t1 == "+")
            ops.push_back("+");
        else if (t1 == "-")
            ops.push_back("-");
        else if (t1 == "*")
            ops.push_back("*");
        else if (t1 == "/")
            ops.push_back("/");
        else if (t1 == "%")
            ops.push_back("%");
        else if (t1 == "|")
            ops.push_back("|");
        else if (t1 == "\"")
            ops.push_back("\"");
        else
            values.push_back(std::atof(t1.c_str()));
    }

    if (values.size() == 1)
    {
        ret = values.at(0);
    }
    else
    {
        for (unsigned vi = 0; vi < ops.size(); vi++)
        {
            auto& op = ops[vi];
            auto& value = values.at(vi);
            if (op == "+")
            {
                ret += value;
            }
            else if (op == "-")
            {
                ret -= value;
            }
            else if (op == "*")
            {
                ret *= value;
            }
            /// I know it’s not good (tm)
            /// + http://gcc.gnu.org/ml/gcc/2001-08/msg00853.html
            else if (op == "/")
            {
                if (value == 0)
                    ret = 0;
                else
                    ret /= value;
            }
            else if (op == "%")
            {
                if (value == 0)
                    ret = 0;
                else
                    ret = static_cast<int> (ret) % static_cast<int> (value);
            }
            else if (op == "|") // Stands for max(a,b), easier than true condition, allow to filter division by 0
            {
                ret = std::max(ret, value);
            }
            else if (op == "\"") // Stands for min(a,b)
            {
                ret = std::min(ret, value);
            }
        }
    }

    return ret;
}

bool PageRecord::isValid() const
{
    //TODO
    if (!sourcePage || !destPage)
        return false;
    return true;
}

ImpositionPlan::ImpositionPlan(const Impose::SourceVars& sv) :
    SourceVars(sv),
    m_destWidth(0.0),
    m_destHeight(0.0),
    m_scale(1.0)
{
}

bool ImpositionPlan::valid() const
{
    if (destWidth() <= 0.0)
        return false;
    else if (destHeight() <= 0.0)
        return false;
    else if (size() == 0)
        return false;

    return true;
}

void ImpositionPlan::setDestWidth(double value)
{
    m_destWidth = value;
}

void ImpositionPlan::setDestHeight(double value)
{
    m_destHeight = value;
}

void ImpositionPlan::setScale(double value)
{
    m_scale = value;
}

void ImpositionPlan::setBoundingBox(const string& value)
{
    m_boundingBox = value;
}
