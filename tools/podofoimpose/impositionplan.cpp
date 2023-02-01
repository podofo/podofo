/**
 * SPDX-FileCopyrightText: (C) 2008 Pierre Marchand <pierremarc@oep-h.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "impositionplan.h"
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <ostream>
#include <cstdio>

using namespace std;
using namespace PoDoFo::Impose;

PageRecord::PageRecord(int s, int d, double r, double tx, double ty, int du, double sx, double sy)
    : sourcePage(s),
    destPage(d),
    rotate(r),
    transX(tx),
    transY(ty),
    scaleX(sx),
    scaleY(sy),
    duplicateOf(du)
{
};

PageRecord::PageRecord()
    : sourcePage(0),
    destPage(0),
    rotate(0),
    transX(0),
    transY(0),
    scaleX(1),
    scaleY(1),
    duplicateOf(0)
{};

void PageRecord::load(const string& buffer, const map<string, string>& vars)
{
    unsigned len = (unsigned)buffer.length();
    vector<string> tokens;
    string ts;
    for (unsigned i = 0; i < len; i++)
    {
        char ci(buffer.at(i));
        if (ci == ' ')
            continue;
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
        for (unsigned int i = 0; i < tokens.size(); ++i)
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
    if (tokens.size() == 7) {
        scaleX = calc(tokens.at(5), vars);
        scaleY = calc(tokens.at(6), vars);
    }
    else {
        scaleX = scaleY = 1.0;
    }

    cerr << " " << sourcePage << " " << destPage << " " << rotate << " " << transX << " " << transY << " " << scaleX << " " << scaleY << endl;

}

double PageRecord::calc(const string& s, const map<string, string>& vars)
{
    // 	cerr<< s;
    vector<string> tokens;
    unsigned tokenCount = (unsigned)s.length();
    string ts;
    for (unsigned i = 0; i < tokenCount; i++)
    {
        char ci(s.at(i));
        // 		if ( ci == 0x20 || ci == 0x9 )// skip spaces and horizontal tabs
        // 			continue;
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
                    // 					cerr<<"A Found "<<ts<<" "<< vit->second <<endl;
                    tokens.push_back(Util::dToStr(calc(vit->second, vars)));
                }
                else
                {
                    // 					cerr<<"A Not Found "<<ts<<endl;
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
        // 		else
        // 			cerr<<"Wrong char : "<< ci <<endl;
    }
    if (ts.length() > 0)
    {
        map<string, string>::const_iterator vit2 = vars.find(ts);
        if (vit2 != vars.end())
        {
            // 			cerr<<endl<<"Found "<<ts<<endl;
            tokens.push_back(Util::dToStr(calc(vit2->second, vars)));
        }
        else
        {
            // 			if((ts.length() > 0) && (ts[0] == '$'))
            // 			{
            // 				cerr<<endl<<"Not Found \"";
            // 				for(unsigned int c(0);c < ts.length(); ++c)
            // 				{
            // 					cerr<<ts[c]<<"/";
            // 				}
            // 				cerr<<"\""<<endl;
            // 				for(map<string,string>::iterator i(PoDoFoImpose::vars.begin());i != PoDoFoImpose::vars.end(); ++i)
            // 				{
            // // 					cerr<<"VA \""<< i->first << "\" => " <<(i->first == ts ? "True" : "False") <<endl;
            // 					for(unsigned int c(0);c < i->first.length(); ++c)
            // 					{
            // 						cerr<<	i->first[c]<<"/";
            // 					}
            // 					cerr<<endl;
            // 				}
            // 			}
            tokens.push_back(ts);
        }
    }
    double result(calc(tokens));
    // 	cerr<<" = "<<result<<endl;
    return result;

}

double PageRecord::calc(const vector<string>& t)
{
    // 	cerr<<"C =";
    // 	for(uint i(0);i<t.size();++i)
    // 		cerr<<" "<< t.at(i) <<" ";
    // 	cerr<<endl;


    if (t.size() == 0)
        return 0.0;

    double ret(0.0);

    vector<double> values;
    vector<string> ops;
    ops.push_back("+");

    for (unsigned int vi = 0; vi < t.size(); ++vi)
    {
        if (t.at(vi) == "(")
        {
            vector<string> tokens;
            int cdeep(0);
            // 			cerr<<"(";
            for (++vi; vi < t.size(); ++vi)
            {
                // 				cerr<<t.at ( ti );
                if (t.at(vi) == ")")
                {
                    if (cdeep == 0)
                        break;
                    else
                    {
                        --cdeep;
                    }
                }
                else if (t.at(vi) == "(")
                {
                    ++cdeep;
                }
                // 				cerr<<endl<<"\t";
                tokens.push_back(t.at(vi));
            }
            // 			cerr<<endl;
            values.push_back(calc(tokens));
        }
        else if (t.at(vi) == "+")
            ops.push_back("+");
        else if (t.at(vi) == "-")
            ops.push_back("-");
        else if (t.at(vi) == "*")
            ops.push_back("*");
        else if (t.at(vi) == "/")
            ops.push_back("/");
        else if (t.at(vi) == "%")
            ops.push_back("%");
        else if (t.at(vi) == "|")
            ops.push_back("|");
        else if (t.at(vi) == "\"")
            ops.push_back("\"");
        else
            values.push_back(std::atof(t.at(vi).c_str()));
    }

    if (values.size() == 1)
        ret = values.at(0);
    else
    {
        for (unsigned int vi = 0; vi < ops.size(); ++vi)
        {
            // 			cerr<<"OP>> \""<<ret<<"\" " << ops.at ( vi )<<" \""<<values.at( vi ) <<"\" = ";
            if (ops.at(vi) == "+")
                ret += values.at(vi);
            else if (ops.at(vi) == "-")
                ret -= values.at(vi);
            else if (ops.at(vi) == "*")
                ret *= values.at(vi);
            /// I know it’s not good (tm)
            /// + http://gcc.gnu.org/ml/gcc/2001-08/msg00853.html
            else if (ops.at(vi) == "/")
            {
                if (values.at(vi) == 0.0)
                    ret = 0.0;
                else
                    ret /= values.at(vi);
            }
            else if (ops.at(vi) == "%")
            {
                if (values.at(vi) == 0.0)
                    ret = 0.0;
                else
                    ret = static_cast<int> (ret) % static_cast<int> (values.at(vi));
            }
            else if (ops.at(vi) == "|") // Stands for max(a,b), easier than true condition, allow to filter division by 0
                ret = std::max(ret, values.at(vi));
            else if (ops.at(vi) == "\"") // Stands for min(a,b)
                ret = std::min(ret, values.at(vi));

            // 			cerr<<ret<<endl;
        }
    }
    // 	cerr<<" <"<< values.size() <<"> "<<ret<<endl;
    return ret;
}

bool PageRecord::isValid() const
{
    //TODO
    if (!sourcePage || !destPage)
        return false;
    return true;
}

ImpositionPlan::ImpositionPlan(const SourceVars& sv) :
    sourceVars(sv),
    m_destWidth(0.0),
    m_destHeight(0.0),
    m_scale(1.0)
{
}

ImpositionPlan::~ImpositionPlan()
{
}

bool ImpositionPlan::valid() const
{
    if (destWidth() <= 0.0)
        return false;
    else if (destHeight() <= 0.0)
        return false;
    // 	else if ( scale() <= 0.0 )
    // 		return false;
    else if (size() == 0)
        return false;

    return true;

}

void ImpositionPlan::setDestWidth(double theValue)
{
    m_destWidth = theValue;
}


void ImpositionPlan::setDestHeight(double theValue)
{
    m_destHeight = theValue;
}


void ImpositionPlan::setScale(double theValue)
{
    m_scale = theValue;
}

void ImpositionPlan::setBoundingBox(const string& theString)
{
    m_boundingBox = theString;
}
