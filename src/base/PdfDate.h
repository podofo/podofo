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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_DATE_H_
#define _PDF_DATE_H_

#include "PdfDefines.h"
#include "PdfString.h"

#include <ctime>

// a PDF date has a maximum of 26 bytes incuding the terminating \0
#define PDF_DATE_BUFFER_SIZE 26

namespace PoDoFo {

/** This class is a date datatype as specified in the PDF 
 *  reference. You can easily convert from Unix time_t to
 *  the PDF time representation and back. Dates like these
 *  are used for example in the PDF info dictionary for the
 *  creation time and date of the PDF file.
 *
 *  PdfDate objects are immutable.
 *
 *  From the PDF reference:
 *
 *  PDF defines a standard date format, which closely follows 
 *  that of the international standard ASN.1 (Abstract Syntax
 *  Notation One), defined in ISO/IEC 8824 (see the Bibliography). 
 *  A date is a string of the form
 *  (D:YYYYMMDDHHmmSSOHH'mm')
 */
class PODOFO_API PdfDate {
 public:
    /** Create a PdfDate object with the current date and time.
     */
    PdfDate();

    /** Create a PdfDate with a specified date and time
     *  \param t the date and time of this object
     *  
     *  Use IsValid to check wether the time_t could be 
     *  converted to a valid PdfDate object.
     *
     *  \see IsValid()
     */
    PdfDate( const time_t & t );

    /** Create a PdfDate with a specified date and time
     *  \param szDate the date and time of this object 
     *         in PDF format. It has to be a string of 
     *         the format  (D:YYYYMMDDHHmmSSOHH'mm').
     *         Otherwise IsValid will return false.
     *  
     *  Use IsValid to check wether the string could be 
     *  converted to a valid PdfDate object.
     *
     *  \see IsValid()
     */
    PdfDate( const PdfString & sDate );

    /** Delete the PdfDate object
     */
    virtual ~PdfDate();

    /** You can use this function to check wether the date
     *  you passed to the constructor could be converted to
     *  a valid pdf date string or a valid time_t.
     *
     *  \returns true if the PdfDate object is valid
     */
    inline bool IsValid() const;

    /** \returns the date and time of this PdfDate in 
     *  seconds since epoch.
     */
    inline const time_t & GetTime() const;

    /** The value returned by this function can be used in any PdfObject
     *  where a date is needed.
     * 
     *  \param rsString write the date to a PdfString
     */         
    inline void ToString( PdfString & rsString ) const;

 private:
    /** Creates the internal string representation from
     *  a time_t value and writes it to m_szDate.
     */
    void CreateStringRepresentation();

    /** Parse fixed length number from string
     *  \param in string to read number from
     *  \param length of number to read 
     *  \param min minimal value of number
     *  \param max maximal value of number
     *  \param ret parsed number
     */
    bool ParseFixLenNumber(const char *&in, unsigned int length, int min, int max, int &ret);

 private:
    time_t m_time;
    char   m_szDate[PDF_DATE_BUFFER_SIZE];

    bool   m_bValid;
};

const time_t & PdfDate::GetTime() const
{
    return m_time;
}

void PdfDate::ToString( PdfString & rsString ) const
{
    rsString = PdfString(  m_szDate );
}

bool PdfDate::IsValid() const
{
    return m_bValid;
}

};

#endif // _PDF_DATE_H_
