//
// C++ Implementation: planreader_legacy
//
// Description: 
//
//
// Author: Pierre Marchand <pierremarc@oep-h.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "planreader_legacy.h"
#ifdef PODOFO_HAVE_LUA
#include "planreader_lua.h"
#endif // PODOFO_HAVE_LUA

#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <istream>
#include <ostream>
using std::ostringstream;
using std::map;
using std::vector;
using std::string;
using std::ifstream;
using std::istream;
using std::ostream;
using std::endl;
using std::runtime_error;

#include <iostream> //XXX#define MAX_SOURCE_PAGES 5000
#define MAX_RECORD_SIZE 2048

int PlanReader_Legacy::sortLoop(std::vector<std::string>& memfile, int numline)
{
// 	std::cerr<<"===================================== "<<numline<<std::endl;
	//Debug
// 	for(std::map<std::string, double>::iterator dit(localvars.begin());dit!=localvars.end();++dit)
// 	{
// 		std::cerr<<"R "<<dit->first<<" = "<<dit->second<<std::endl;
// 	}
	//
	std::map<std::string,std::string> storedvars = I->vars;
	int startAt(numline);
	std::string buffer( memfile.at(numline) );
	int blen = buffer.length();
	std::string iterN;
	int a(1);
	char ca(0);
	for(;a<blen;++a)
	{
		ca = buffer.at(a);
		if(ca == '[')
			break;
		else if(ca == 0x20 || ca == 0x9 )
			continue;
		iterN += buffer.at(a);
	}
			
	std::map<std::string, double> increments;
	std::string tvar;
	std::string tinc;
	++a;
	bool varside(true);
	for(;a<blen;++a)
	{
		ca = buffer.at(a);
// 		if(ca == 0x20 || ca == 0x9 )
// 			continue;
		if( (ca == ']') || (ca == ';') ) // time to commit
		{
			if(I->vars.find(tvar) != I->vars.end())
			{
// 				std::cerr<< "I " << tvar <<" = "<< tinc <<std::endl;
				increments.insert(std::pair<std::string, double>( tvar, std::atof(tinc.c_str())));
			}
			tvar.clear();
			tinc.clear();
			if(ca == ';')
				varside = true;
			else
				break;
		}
		else if(ca == '+')
		{
			varside = false;
			continue;
		}
		else
		{
			if(varside)
				tvar += ca;
			else
				tinc += ca;
		}
	}
			
	int endOfloopBlock(numline + 1);
	int openLoop(0);
	for(unsigned int bolb2 = (numline + 1); bolb2 < memfile.size();++bolb2)
	{
// 		std::cerr<<"| "<< memfile.at ( bolb2 ) <<" |"<<std::endl;
		if(memfile.at ( bolb2 ).at( 0 ) == '<')
			++openLoop;
		else if(memfile.at ( bolb2 ).at( 0 ) == '>')
		{
			if(openLoop == 0)
				break;
			else
				--openLoop;
		}
		else	
			endOfloopBlock = bolb2 + 1;		
	}

	int maxIter(PoDoFo::Impose::PageRecord::calc(iterN, I->vars));
	for(int iter(0); iter < maxIter ; ++iter )
	{
		if(iter != 0)
		{
			// we set the vars
			std::map<std::string, double>::iterator vit;
			for(vit = increments.begin(); vit != increments.end() ; ++vit)
			{
				I->vars[vit->first] = PoDoFo::Impose::Util::dToStr( std::atof(I->vars[vit->first].c_str()) + vit->second );
			}
		}
		for(int subi(numline + 1);subi < endOfloopBlock ; ++subi)
		{
// 					std::cerr<< subi <<"/"<< endOfloopBlock <<" - "<<memfile.at(subi) <<std::endl;
			
			if(memfile.at ( subi ).at( 0 ) == '<')
			{
				subi += sortLoop(memfile , subi);
// 				std::cerr<< "||  "  << memfile.at ( subi )  <<std::endl;
			}
			else
			{
				PoDoFo::Impose::PageRecord p;
				p.load ( memfile.at(subi), I->vars ) ;
				if(!p.isValid() || p.sourcePage > I->sourceVars.PageCount)
				{
// 					std::cerr<< "Error p("<<(p.isValid()?"valid":"invalid")<<") "<< p.sourcePage  <<std::endl;
					continue;
				}
// 				maxPageDest = std::max ( maxPageDest, p.destPage );
// 				bool isDup(false);
// 				for(ImpositionPlan::const_iterator ipIt(planImposition.begin());ipIt != planImposition.end(); ++ipIt)
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
				I->push_back ( p );
			}
		}
				
	}
// 	numline = endOfloopBlock;
// 	std::cerr<<"EOL"<<std::endl;
	int retvalue(endOfloopBlock - startAt + 1);
	I->vars = storedvars;
// 	std::cerr<<"------------------------------------- "<<retvalue<<std::endl;
	return retvalue;
}

PlanReader_Legacy::PlanReader_Legacy(const std::string & plan, PoDoFo::Impose::ImpositionPlan *Imp)
	:I(Imp)
{
	ifstream in ( plan.c_str(), ifstream::in );
	if ( !in.good() )
		throw runtime_error ( "Failed to open plan file" );

// 	duplicate = MAX_SOURCE_PAGES;
	std::vector<std::string> memfile;
	do
	{
		std::string buffer;
		if ( !std::getline ( in, buffer ) && ( !in.eof() || in.bad() ) )
		{
			throw runtime_error ( "Failed to read line from plan" );
		}

#ifdef PODOFO_HAVE_LUA
// This was "supposed" to be a legacy file, but if it starts 
// with two dashes, it must be a lua file, so process it accordingly:
        if (buffer.substr(0,2) == "--") {
            in.close();
            PlanReader_Lua(plan, Imp);
            return;
        }
#endif // PODOFO_HAVE_LUA

		if ( buffer.length() < 2 ) // Nothing
			continue;
		
		PoDoFo::Impose::Util::trimmed_str(buffer);
		if(buffer.length() < 2)
			continue;
		else if ( buffer.at ( 0 ) == '#' ) // Comment
			continue;
		else
		{
			memfile.push_back(buffer);
// 			std::cerr<<buffer<<std::endl;
		}
	}
	while(!in.eof());
	/// PROVIDED 
	I->vars[std::string("$PagesCount")] = PoDoFo::Impose::Util::iToStr( I->sourceVars.PageCount );
	I->vars[std::string("$SourceWidth")] = PoDoFo::Impose::Util::dToStr( I->sourceVars.PageWidth );
	I->vars[std::string("$SourceHeight")] = PoDoFo::Impose::Util::dToStr( I->sourceVars.PageHeight );
	/// END OF PROVIDED
	
	for( unsigned int numline = 0; numline < memfile.size() ; ++numline)
	{
		std::string buffer( memfile.at(numline) );
		if ( buffer.at ( 0 ) == '$' ) // Variable
		{
			int sepPos ( buffer.find_first_of ( '=' ) );
			std::string key(buffer.substr ( 0,sepPos ));
			std::string value(buffer.substr ( sepPos + 1 ));
			
			{
				I->vars[key] = value;
			}
		}
		else if( buffer.at ( 0 ) == '<' ) // Loop - experimental
		{
			numline += sortLoop( memfile , numline  );
		}
		else // Record? We hope!
		{
			PoDoFo::Impose::PageRecord p;
			p.load ( buffer, I->vars ) ;
			if(!p.isValid() || p.sourcePage > I->sourceVars.PageCount)
				continue;
// 			maxPageDest = std::max ( maxPageDest, p.destPage );
// 			if ( pagesIndex.find ( p.sourcePage ) != pagesIndex.end() )
// 			{
// 				p.duplicateOf = p.sourcePage;
// 			}
			I->push_back ( p );
		}
		
	}
	
	
	/// REQUIRED
	if ( I->vars.find("$PageWidth") == I->vars.end() )
		throw runtime_error ( "$PageWidth not set" );
	if (I->vars.find("$PageHeight") == I->vars.end() )
		throw runtime_error ( "$PageHeight not set" );
	
	I->setDestWidth( PoDoFo::Impose::PageRecord::calc( I->vars["$PageWidth"] , I->vars) );
	I->setDestHeight( PoDoFo::Impose::PageRecord::calc( I->vars["$PageHeight"] , I->vars));
	/// END OF REQUIRED
	
	/// SUPPORTED
	if ( I->vars.find("$ScaleFactor") != I->vars.end() )
		I->setScale( PoDoFo::Impose::PageRecord::calc( I->vars["$ScaleFactor"] , I->vars));
	/// END OF SUPPORTED
	
	
	
}
