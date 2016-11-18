//
// C++ Implementation: impositionplan
//
// Description:
//
//
// Author: Pierre Marchand <pierremarc@oep-h.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "impositionplan.h"


#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <istream>
#include <ostream>
#include <cstdio>

using std::ostringstream;
using std::map;
using std::vector;
using std::string;
using std::ifstream;
using std::istream;
using std::ostream;
using std::endl;
using std::runtime_error;

#ifdef _WIN32
#ifdef max
#undef max
#endif // max
#ifdef min
#undef min
#endif // min
#endif // _WIN32

#include <iostream> //XXX
namespace PoDoFo { namespace Impose {
PageRecord::PageRecord ( int s,int d,double r, double tx, double ty, int du )
		: sourcePage ( s ),
		destPage ( d ),
		rotate ( r ),
		transX ( tx ),
		transY ( ty ),
		duplicateOf( du )
{
};

PageRecord::PageRecord ( )
		: sourcePage ( 0 ),
		destPage ( 0 ),
		rotate ( 0 ),
		transX ( 0 ),
		transY ( 0 ),
		duplicateOf( 0 )
{};

void PageRecord::load ( const std::string& buffer, const std::map<std::string, std::string>& vars )
{
	int blen ( buffer.length() );
	std::vector<std::string> tokens;
	std::string ts;
	for ( int i ( 0 ); i < blen; ++i )
	{
		char ci ( buffer.at ( i ) );
		if ( ci == ' ' )
			continue;
		else if ( ci == ';' )
		{
			tokens.push_back ( ts );
			ts.clear();
			continue;
		}
		ts += ci;
	}

	if ( tokens.size() != 5 )
	{
		sourcePage = destPage = 0; // will return false for isValid()
		std::cerr<<"INVALID_RECORD("<< tokens.size() <<") "<<buffer<<std::endl;
		for ( unsigned int i = 0;i<tokens.size();++i )
			std::cerr<<"\t+ "<<tokens.at ( i ) <<std::endl;
	}

	sourcePage	= static_cast<int>(calc ( tokens.at ( 0 ) , vars));
	destPage	= static_cast<int>(calc ( tokens.at ( 1 ) , vars));
	if ( ( sourcePage < 1 ) || ( destPage < 1 ) )
	{
		sourcePage = destPage = 0;
	}

	rotate	= calc ( tokens.at ( 2 ) , vars);
	transX	= calc ( tokens.at ( 3 ) , vars);
	transY	= calc ( tokens.at ( 4 ) , vars);

	std::cerr<<" "<<sourcePage<<" "<<destPage<<" "<<rotate<<" "<<transX<<" "<<transY <<std::endl;

}

double PageRecord::calc ( const std::string& s , const std::map<std::string, std::string>& vars)
{
// 	std::cerr<< s;
	std::vector<std::string> tokens;
	int tlen ( s.length() );
	std::string ts;
	for ( int i ( 0 ); i < tlen; ++i )
	{
		char ci ( s.at ( i ) );
// 		if ( ci == 0x20 || ci == 0x9 )// skip spaces and horizontal tabs
// 			continue;
		if ( ( ci == '+' )
		        || ( ci == '-' )
		        || ( ci == '*' )
		        || ( ci == '/' )
		        || ( ci == '%' )
			|| ( ci == '|' )
			|| ( ci == '"' )
		        || ( ci == '(' )
		        || ( ci == ')' ) )
		{
			// commit current string
			if ( ts.length() > 0 )
			{
				std::map<std::string, std::string>::const_iterator vit = vars.find ( ts );
				if ( vit != vars.end() )
				{
// 					std::cerr<<"A Found "<<ts<<" "<< vit->second <<std::endl;
					tokens.push_back ( Util::dToStr ( calc ( vit->second, vars ) ) );
				}
				else
				{
// 					std::cerr<<"A Not Found "<<ts<<std::endl;
					tokens.push_back ( ts );
				}
			}
			ts.clear();
			// append operator
			ts += ci;
			tokens.push_back ( ts );
			ts.clear();
		}
		else if ( ci > 32 )
		{
			ts += ci;
		}
// 		else
// 			std::cerr<<"Wrong char : "<< ci <<std::endl;
	}
	if ( ts.length() > 0 )
	{
		std::map<std::string, std::string>::const_iterator vit2 = vars.find ( ts );
		if ( vit2 != vars.end() )
		{
// 			std::cerr<<std::endl<<"Found "<<ts<<std::endl;
			tokens.push_back ( Util::dToStr ( calc ( vit2->second , vars) ) );
		}
		else
		{
// 			if((ts.length() > 0) && (ts[0] == '$'))
// 			{
// 				std::cerr<<std::endl<<"Not Found \"";
// 				for(unsigned int c(0);c < ts.length(); ++c)
// 				{
// 					std::cerr<<ts[c]<<"/";
// 				}
// 				std::cerr<<"\""<<std::endl;
// 				for(std::map<std::string,std::string>::iterator i(PoDoFoImpose::vars.begin());i != PoDoFoImpose::vars.end(); ++i)
// 				{
// // 					std::cerr<<"VA \""<< i->first << "\" => " <<(i->first == ts ? "True" : "False") <<std::endl;
// 					for(unsigned int c(0);c < i->first.length(); ++c)
// 					{
// 						std::cerr<<	i->first[c]<<"/";
// 					}
// 					std::cerr<<std::endl;
// 				}
// 			}
			tokens.push_back ( ts );
		}
	}
	double result ( calc ( tokens ) );
// 	std::cerr<<" = "<<result<<std::endl;
	return result;

}

double PageRecord::calc ( const std::vector<std::string>& t )
{
// 	std::cerr<<"C =";
// 	for(uint i(0);i<t.size();++i)
// 		std::cerr<<" "<< t.at(i) <<" ";
// 	std::cerr<<std::endl;


	if ( t.size() == 0 )
		return 0.0;

	double ret ( 0.0 );

	std::vector<double> values;
	std::vector<std::string> ops;
	ops.push_back ( "+" );

	for ( unsigned int vi = 0; vi < t.size(); ++vi )
	{
		if ( t.at ( vi ) == "(" )
		{
			std::vector<std::string> tokens;
			int cdeep ( 0 );
// 			std::cerr<<"(";
			for ( ++vi ; vi < t.size(); ++vi )
			{
// 				std::cerr<<t.at ( ti );
				if ( t.at ( vi ) == ")" )
				{
					if ( cdeep == 0 )
						break;
					else
					{
						--cdeep;
					}
				}
				else if ( t.at ( vi ) == "(" )
				{
					++cdeep;
				}
// 				std::cerr<<std::endl<<"\t";
				tokens.push_back ( t.at ( vi ) );
			}
// 			std::cerr<<std::endl;
			values.push_back ( calc ( tokens ) );
		}
		else if ( t.at ( vi ) == "+" )
			ops.push_back ( "+" );
		else if ( t.at ( vi ) == "-" )
			ops.push_back ( "-" );
		else if ( t.at ( vi ) == "*" )
			ops.push_back ( "*" );
		else if ( t.at ( vi ) == "/" )
			ops.push_back ( "/" );
		else if ( t.at ( vi ) == "%" )
			ops.push_back ( "%" );
		else if ( t.at ( vi ) == "|" )
			ops.push_back ( "|" );
		else if ( t.at ( vi ) == "\"" )
			ops.push_back ( "\"" );
		else
			values.push_back ( std::atof ( t.at ( vi ).c_str() ) );
	}

	if ( values.size() == 1 )
		ret = 	values.at ( 0 );
	else
	{
		for ( unsigned int vi = 0; vi < ops.size(); ++vi )
		{
// 			std::cerr<<"OP>> \""<<ret<<"\" " << ops.at ( vi )<<" \""<<values.at( vi ) <<"\" = ";
			if ( ops.at ( vi ) == "+" )
				ret += values.at ( vi );
			else if ( ops.at ( vi ) == "-" )
				ret -= values.at ( vi );
			else if ( ops.at ( vi ) == "*" )
				ret *= values.at ( vi );
			/// I know it’s not good (tm)
			/// + http://gcc.gnu.org/ml/gcc/2001-08/msg00853.html
			else if ( ops.at ( vi ) == "/" )
			{
				if ( values.at ( vi ) == 0.0 )
					ret = 0.0;
				else
					ret /= values.at ( vi );
			}
			else if ( ops.at ( vi ) == "%" )
			{
				if ( values.at ( vi ) == 0.0 )
					ret = 0.0;
				else
					ret = static_cast<int> ( ret ) % static_cast<int> ( values.at ( vi ) );
			}
			else if ( ops.at ( vi ) == "|" ) // Stands for max(a,b), easier than true condition, allow to filter division by 0
				ret = std::max ( ret , values.at ( vi ) );
			else if ( ops.at ( vi ) == "\"" ) // Stands for min(a,b)
				ret = std::min ( ret , values.at ( vi ) );

// 			std::cerr<<ret<<std::endl;
		}
	}
// 	std::cerr<<" <"<< values.size() <<"> "<<ret<<std::endl;
	return ret;
}

bool PageRecord::isValid() const
{
	//TODO
	if ( !sourcePage || !destPage )
		return false;
	return true;
}

ImpositionPlan::ImpositionPlan(const SourceVars& sv) :
		sourceVars(sv),
		m_destWidth ( 0.0 ),
		m_destHeight ( 0.0 ),
		m_scale ( 1.0 )
{
}

ImpositionPlan::~ ImpositionPlan()
{
}

bool ImpositionPlan::valid() const
{
	if ( destWidth() <= 0.0 )
		return false;
	else if ( destHeight() <= 0.0 )
		return false;
// 	else if ( scale() <= 0.0 )
// 		return false;
	else if(size() == 0)
		return false;

	return true;

}

void ImpositionPlan::setDestWidth ( double theValue )
{
	m_destWidth = theValue;
}


void ImpositionPlan::setDestHeight ( double theValue )
{
	m_destHeight = theValue;
}


void ImpositionPlan::setScale ( double theValue )
{
	m_scale = theValue;
}

void ImpositionPlan::setBoundingBox( const std::string& theString )
{
	m_boundingBox = theString;
}

};};
