/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "PdfDate.h"
#include "PdfDefinesPrivate.h"

#include <string.h>
#include <sstream>

namespace PoDoFo {

PdfDate::PdfDate()
    : m_bValid( false )
{
    m_time = time( &m_time );
    CreateStringRepresentation();
}

PdfDate::PdfDate( const time_t & t )
    : m_bValid( false )
{
    m_time = t;
    CreateStringRepresentation();
}


#if 0
PdfDate::PdfDate( const PdfString & sDate )
    : m_bValid( false )
{

struct tm {
        int     tm_sec;         /* seconds */
        int     tm_min;         /* minutes */
        int     tm_hour;        /* hours */
        int     tm_mday;        /* day of the month */
        int     tm_mon;         /* month */
        int     tm_year;        /* year */
        int     tm_wday;        /* day of the week */
        int     tm_yday;        /* day in the year */
        int     tm_isdst;       /* daylight saving time */
};
}
#endif // 0 

PdfDate::~PdfDate()
{
}

void PdfDate::CreateStringRepresentation()
{
    const int   ZONE_STRING_SIZE = 6;
    const char* INVALIDDATE     = "INVALIDDATE";

    char szZone[ZONE_STRING_SIZE];
    char szDate[PDF_DATE_BUFFER_SIZE];

    struct tm* stm = localtime( &m_time );

#ifdef _WIN32
    // On win32, strftime with %z returns a verbose time zone name
    // like "W. Australia Standard time". We use tzset and timezone
    // instead.
    _tzset();
    snprintf( szZone, ZONE_STRING_SIZE, "%+03d", -_timezone/3600 );
#else
    if( strftime( szZone, ZONE_STRING_SIZE, "%z", stm ) == 0 )
    {
        std::ostringstream ss;
        ss << "Generated invalid date from time_t value " << m_time
           << " (couldn't determine time zone)\n";
        PdfError::DebugMessage( ss.str().c_str() );
        strcpy( m_szDate, INVALIDDATE );
        return;
    }
#endif

    // only the first 3 characters are important for the pdf date representation
    // e.g. +01 instead off +0100
    szZone[3] = '\0';
   
    if( strftime( szDate, PDF_DATE_BUFFER_SIZE, "D:%Y%m%d%H%M%S", stm ) == 0 )
    {
        std::ostringstream ss;
	ss << "Generated invalid date from time_t value " << m_time
           << "\n";
	PdfError::DebugMessage( ss.str().c_str() );
        strcpy( m_szDate, INVALIDDATE );
        return;
    }

    snprintf( m_szDate, PDF_DATE_BUFFER_SIZE, "%s%s'00'", szDate, szZone );
    m_bValid = true;
}
    

};


