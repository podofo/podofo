//
// C++ Interface: impositionplan
//
// Description:
//
//
// Author: Pierre Marchand <pierremarc@oep-h.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef IMPOSITIONPLAN_H
#define IMPOSITIONPLAN_H

#include "podofo.h"

#include <map>
#include <string>
#include <vector>
#include <cstdio>

namespace PoDoFo
{
	namespace Impose
	{
		enum PlanReader
		{
			Legacy = 0,
			Lua = 1
		};
		
		struct SourceVars
		{
			double PageCount;
			double PageWidth;
			double PageHeight;
		};
		


class Util
{
	public:
		static void trimmed_str ( std::string& s )
		{
			std::string::iterator si ( s.begin() );
			while ( si != s.end() )
			{
				if ( ( *si ) == 0x20 )
					si = s.erase ( si );
				else if ( ( *si ) == 0x9 )
					si = s.erase ( si );
				else
					++si;
			}

		}

		static std::string dToStr ( double d )
		{
			char buffer [126];
			sprintf ( buffer, "%.5f", d );
			std::string ret ( buffer );
			return ret;

		}
		static std::string iToStr ( int i )
		{
			char buffer [126];
			sprintf ( buffer, "%d", i );
			std::string ret ( buffer );
			return ret;
		}
};


/**
  @author Pierre Marchand <pierre@moulindetouvois.com>
 */
class PageRecord
{
	public:
		PageRecord ( int s,int d,double r, double tx, double ty , int du = 0 );
		PageRecord( );
		~PageRecord() {};
		int sourcePage;
		int xobjIndex;
		int destPage;
		double rotate;
		double transX;
		double transY;
		int duplicateOf;
		bool isValid() const;

		/// needed by legacy loader - should be removed soon
		static double calc ( const std::string& s , const std::map<std::string, std::string>& vars );
		static double calc ( const std::vector<std::string>& t );
		void load ( const std::string& s, const std::map<std::string, std::string>& vars );
};

class ImpositionPlan : public std::vector<PageRecord>
{
	public:


		ImpositionPlan(const SourceVars& sv);
		~ImpositionPlan();
		
		// legacy
		std::map<std::string, std::string> vars;
		
		const SourceVars sourceVars;

	private:
		double m_destWidth;
		double m_destHeight;
		double m_scale;
                std::string m_boundingBox;
	public:
		bool valid() const;

		void setDestWidth ( double theValue );
		double destWidth() const{return m_destWidth;}

		void setDestHeight ( double theValue );
		double destHeight() const{return m_destHeight;}
		
		void setScale ( double theValue );
		double scale() const{return m_scale;}

		void setBoundingBox( const std::string& theString );
                std::string boundingBox()const{return m_boundingBox;}
};

};}; // end of namespace

#endif



