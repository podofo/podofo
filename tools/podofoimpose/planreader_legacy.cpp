/**
 * SPDX-FileCopyrightText: (C) 2008 Pierre Marchand <pierremarc@oep-h.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "planreader_legacy.h"

#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <istream>
#include <iostream> //XXX#define MAX_SOURCE_PAGES 5000
#include <ostream>

#ifdef PODOFO_HAVE_LUA
#include "planreader_lua.h"
#endif // PODOFO_HAVE_LUA

using namespace std;
using namespace PoDoFo::Impose;

#define MAX_RECORD_SIZE 2048

int PlanReader_Legacy::sortLoop(vector<string>& memfile, int numline)
{
    // 	cerr<<"===================================== "<<numline<<endl;
        //Debug
    // 	for(map<string, double>::iterator dit(localvars.begin());dit!=localvars.end(); dit++)
    // 	{
    // 		cerr<<"R "<<dit->first<<" = "<<dit->second<<endl;
    // 	}
        //
    map<string, string> storedvars = I->vars;
    int startAt(numline);
    string buffer(memfile.at(numline));
    unsigned len = (unsigned)buffer.length();
    string iterN;
    unsigned a = 1;
    char ca = 0;
    for (; a < len; a++)
    {
        ca = buffer.at(a);
        if (ca == '[')
            break;
        else if (ca == 0x20 || ca == 0x9)
            continue;
        iterN += buffer.at(a);
    }

    map<string, double> increments;
    string tvar;
    string tinc;
    a++;
    bool varside(true);
    for (; a < len; a++)
    {
        ca = buffer.at(a);
        // 		if(ca == 0x20 || ca == 0x9 )
        // 			continue;
        if ((ca == ']') || (ca == ';')) // time to commit
        {
            if (I->vars.find(tvar) != I->vars.end())
            {
                // 				cerr<< "I " << tvar <<" = "<< tinc <<endl;
                increments.insert(pair<string, double>(tvar, std::atof(tinc.c_str())));
            }
            tvar.clear();
            tinc.clear();
            if (ca == ';')
                varside = true;
            else
                break;
        }
        else if (ca == '+')
        {
            varside = false;
            continue;
        }
        else
        {
            if (varside)
                tvar += ca;
            else
                tinc += ca;
        }
    }

    int endOfloopBlock = numline + 1;
    int openLoop = 0;
    for (unsigned int bolb2 = (numline + 1); bolb2 < memfile.size(); bolb2++)
    {
        // 		cerr<<"| "<< memfile.at ( bolb2 ) <<" |"<<endl;
        if (memfile.at(bolb2).at(0) == '<')
            openLoop++;
        else if (memfile.at(bolb2).at(0) == '>')
        {
            if (openLoop == 0)
                break;
            else
                openLoop--;
        }
        else
            endOfloopBlock = bolb2 + 1;
    }

    unsigned maxIter = (unsigned)PageRecord::calc(iterN, I->vars);
    for (unsigned iter = 0; iter < maxIter; iter++)
    {
        if (iter != 0)
        {
            // we set the vars
            map<string, double>::iterator vit;
            for (vit = increments.begin(); vit != increments.end(); vit++)
            {
                I->vars[vit->first] = Util::dToStr(std::atof(I->vars[vit->first].c_str()) + vit->second);
            }
        }
        for (int subi(numline + 1); subi < endOfloopBlock; subi++)
        {
            // 					cerr<< subi <<"/"<< endOfloopBlock <<" - "<<memfile.at(subi) <<endl;

            if (memfile.at(subi).at(0) == '<')
            {
                subi += sortLoop(memfile, subi);
                // 				cerr<< "||  "  << memfile.at ( subi )  <<endl;
            }
            else
            {
                PageRecord p;
                p.load(memfile.at(subi), I->vars);
                if (!p.isValid() || p.sourcePage > I->sourceVars.PageCount)
                {
                    // 					cerr<< "Error p("<<(p.isValid()?"valid":"invalid")<<") "<< p.sourcePage  <<endl;
                    continue;
                }
                // 				maxPageDest = std::max ( maxPageDest, p.destPage );
                // 				bool isDup(false);
                // 				for(ImpositionPlan::const_iterator ipIt(planImposition.begin());ipIt != planImposition.end(); ipIt++)
                // 				{
                // 					if(ipIt->sourcePage == p.sourcePage)
                // 					{
                // 						isDup = true;
                // 						break;
                // 					}
                // 				}
                // 				if ( isDup )
                // 				{
                // 					p.duplicateOf = p.sourcePage;
                // 				}
                I->push_back(p);
            }
        }

    }
    // 	numline = endOfloopBlock;
    // 	cerr<<"EOL"<<endl;
    int retvalue(endOfloopBlock - startAt + 1);
    I->vars = storedvars;
    // 	cerr<<"------------------------------------- "<<retvalue<<endl;
    return retvalue;
}

PlanReader_Legacy::PlanReader_Legacy(const string& plan, ImpositionPlan* Imp)
    :I(Imp)
{
    ifstream in(plan.c_str(), ifstream::in);
    if (!in.good())
        throw runtime_error("Failed to open plan file");

    // 	duplicate = MAX_SOURCE_PAGES;
    vector<string> memfile;
    do
    {
        string buffer;
        if (!std::getline(in, buffer) && (!in.eof() || in.bad()))
        {
            throw runtime_error("Failed to read line from plan");
        }

#ifdef PODOFO_HAVE_LUA
        // This was "supposed" to be a legacy file, but if it starts 
        // with two dashes, it must be a lua file, so process it accordingly:
        if (buffer.substr(0, 2) == "--") {
            in.close();
            PlanReader_Lua(plan, Imp);
            return;
        }
#endif // PODOFO_HAVE_LUA

        if (buffer.length() < 2) // Nothing
            continue;

        Util::trimmed_str(buffer);
        if (buffer.length() < 2)
            continue;
        else if (buffer.at(0) == '#') // Comment
            continue;
        else
        {
            memfile.push_back(buffer);
            // 			cerr<<buffer<<endl;
        }
    } while (!in.eof());
    /// PROVIDED 
    I->vars[string("$PagesCount")] = Util::uToStr(I->sourceVars.PageCount);
    I->vars[string("$SourceWidth")] = Util::dToStr(I->sourceVars.PageWidth);
    I->vars[string("$SourceHeight")] = Util::dToStr(I->sourceVars.PageHeight);
    /// END OF PROVIDED

    for (unsigned int numline = 0; numline < memfile.size(); numline++)
    {
        string buffer(memfile.at(numline));
        if (buffer.at(0) == '$') // Variable
        {
            unsigned sepPos = (unsigned)buffer.find_first_of('=');
            string key(buffer.substr(0, sepPos));
            string value(buffer.substr(sepPos + 1));

            {
                I->vars[key] = value;
            }
        }
        else if (buffer.at(0) == '<') // Loop - experimental
        {
            numline += sortLoop(memfile, numline);
        }
        else // Record? We hope!
        {
            PageRecord p;
            p.load(buffer, I->vars);
            if (!p.isValid() || p.sourcePage > I->sourceVars.PageCount)
                continue;
            // 			maxPageDest = std::max ( maxPageDest, p.destPage );
            // 			if ( pagesIndex.find ( p.sourcePage ) != pagesIndex.end() )
            // 			{
            // 				p.duplicateOf = p.sourcePage;
            // 			}
            I->push_back(p);
        }

    }

    /// REQUIRED
    if (I->vars.find("$PageWidth") == I->vars.end())
        throw runtime_error("$PageWidth not set");
    if (I->vars.find("$PageHeight") == I->vars.end())
        throw runtime_error("$PageHeight not set");

    I->setDestWidth(PageRecord::calc(I->vars["$PageWidth"], I->vars));
    I->setDestHeight(PageRecord::calc(I->vars["$PageHeight"], I->vars));
    /// END OF REQUIRED

    /// SUPPORTED
    if (I->vars.find("$ScaleFactor") != I->vars.end())
        I->setScale(PageRecord::calc(I->vars["$ScaleFactor"], I->vars));
    if (I->vars.find("$BoundingBox") != I->vars.end())
        I->setBoundingBox(I->vars["$BoundingBox"]);
    /// END OF SUPPORTED
}
