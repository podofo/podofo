/**
 * SPDX-FileCopyrightText: (C) 2008 Pierre Marchand <pierremarc@oep-h.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "planreader_legacy.h"

#ifdef PODOFO_HAVE_LUA
#include "planreader_lua.h"
#endif // PODOFO_HAVE_LUA

using namespace std;
using namespace PoDoFo::Impose;

#define MAX_RECORD_SIZE 2048

int PlanReader_Legacy::sortLoop(vector<string>& memfile, int numline)
{
    map<string, string> storedvars = m_imp->Vars;
    int startAt = numline;
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

        if (ca == 0x20 || ca == 0x9)
            continue;

        iterN += buffer.at(a);
    }

    map<string, double> increments;
    string tvar;
    string tinc;
    a++;
    bool varside = true;
    for (; a < len; a++)
    {
        ca = buffer.at(a);
        if ((ca == ']') || (ca == ';')) // time to commit
        {
            if (m_imp->Vars.find(tvar) != m_imp->Vars.end())
            {
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
    for (unsigned bolb2 = numline + 1; bolb2 < memfile.size(); bolb2++)
    {
        if (memfile.at(bolb2).at(0) == '<')
            openLoop++;
        else if (memfile.at(bolb2).at(0) == '>')
        {
            if (openLoop == 0)
                break;

            openLoop--;
        }
        else
        {
            endOfloopBlock = bolb2 + 1;
        }
    }

    unsigned maxIter = (unsigned)PageRecord::calc(iterN, m_imp->Vars);
    for (unsigned iter = 0; iter < maxIter; iter++)
    {
        if (iter != 0)
        {
            // we set the vars
            map<string, double>::iterator vit;
            for (vit = increments.begin(); vit != increments.end(); vit++)
            {
                m_imp->Vars[vit->first] = Util::dToStr(std::atof(m_imp->Vars[vit->first].c_str()) + vit->second);
            }
        }
        for (int subi = numline + 1; subi < endOfloopBlock; subi++)
        {
            if (memfile.at(subi).at(0) == '<')
            {
                subi += sortLoop(memfile, subi);
            }
            else
            {
                PageRecord p;
                p.load(memfile.at(subi), m_imp->Vars);
                if (!p.isValid() || p.sourcePage > m_imp->SourceVars.PageCount)
                    continue;

                m_imp->push_back(p);
            }
        }
    }

    int retvalue = endOfloopBlock - startAt + 1;
    m_imp->Vars = storedvars;
    return retvalue;
}

PlanReader_Legacy::PlanReader_Legacy(const string& plan, ImpositionPlan& imp)
    : m_imp(&imp)
{
    ifstream in(plan.c_str(), ifstream::in);
    if (!in.good())
        throw runtime_error("Failed to open plan file");

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
            PlanReader_Lua(plan, imp);
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
            memfile.push_back(buffer);
    } while (!in.eof());
    /// PROVIDED 
    m_imp->Vars[string("$PagesCount")] = Util::uToStr(m_imp->SourceVars.PageCount);
    m_imp->Vars[string("$SourceWidth")] = Util::dToStr(m_imp->SourceVars.PageWidth);
    m_imp->Vars[string("$SourceHeight")] = Util::dToStr(m_imp->SourceVars.PageHeight);
    /// END OF PROVIDED

    for (unsigned numline = 0; numline < memfile.size(); numline++)
    {
        string buffer(memfile.at(numline));
        if (buffer.at(0) == '$') // Variable
        {
            unsigned sepPos = (unsigned)buffer.find_first_of('=');
            string key(buffer.substr(0, sepPos));
            string value(buffer.substr(sepPos + 1));
            m_imp->Vars[key] = value;
        }
        else if (buffer.at(0) == '<') // Loop - experimental
        {
            numline += sortLoop(memfile, numline);
        }
        else // Record? We hope!
        {
            PageRecord p;
            p.load(buffer, m_imp->Vars);
            if (!p.isValid() || p.sourcePage > m_imp->SourceVars.PageCount)
                continue;

            m_imp->push_back(p);
        }
    }

    /// REQUIRED
    if (m_imp->Vars.find("$PageWidth") == m_imp->Vars.end())
        throw runtime_error("$PageWidth not set");
    if (m_imp->Vars.find("$PageHeight") == m_imp->Vars.end())
        throw runtime_error("$PageHeight not set");

    m_imp->setDestWidth(PageRecord::calc(m_imp->Vars["$PageWidth"], m_imp->Vars));
    m_imp->setDestHeight(PageRecord::calc(m_imp->Vars["$PageHeight"], m_imp->Vars));
    /// END OF REQUIRED

    /// SUPPORTED
    if (m_imp->Vars.find("$ScaleFactor") != m_imp->Vars.end())
        m_imp->setScale(PageRecord::calc(m_imp->Vars["$ScaleFactor"], m_imp->Vars));
    if (m_imp->Vars.find("$BoundingBox") != m_imp->Vars.end())
        m_imp->setBoundingBox(m_imp->Vars["$BoundingBox"]);
    /// END OF SUPPORTED
}
