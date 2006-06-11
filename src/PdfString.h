/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#ifndef _PDF_STRING_H_
#define _PDF_STRING_H_

#include "PdfDefines.h"

namespace PoDoFo {

class PdfOutputDevice;

/** A string that can be written to a PDF document.
 *  If it contains binary data it is automatically 
 *  converted into a hex string, otherwise a normal PDF 
 *  string is written to the document.
 */
class PdfString {
 public:

    /** Create an empty and invalid string 
     */
    PdfString();

    /** Construct a new PdfString from a 0 terminated
     *  string. 
     *  The input string will be copied.
     *
     *  \param pszString the string to copy
     */
    PdfString( const char* pszString );

    /** Construct a new PdfString from a string. 
     *  The input string will be copied.
     *
     *  \param pszString the string to copy
     *  \param lLen length of the string data to encode
     *  \param bHex if true the data will be 
     *              hex encoded and IsHex() will return true.
     */
    PdfString( const char* pszString, long lLen, bool bHex = false );

    /** Set hex encoded data as the strings data. 
     *  \param pszHex must be hex encoded data.
     *  \param lLen   length of the hex encoded data.
     *                if lLen == -1 then strlen( pszHex ) will
     *                be used as length of the hex data. 
     *                pszHex has to be zero terminated in this case.
     *  \returns ePdfError_ErrOk on success.
     */
    PdfError SetHexData( const char* pszHex, long lLen = -1 );

    /** Copy an existing PdfString 
     *  \param rhs another PdfString to copy
     */
    PdfString( const PdfString & rhs );

    ~PdfString();

    /** The string is valid if no error in the constructor has occurred.
     *  If it is valid it is safe to call all the other member functions.
     *  \returns true if this is a valid initialized PdfString
     */
    inline bool       IsValid() const;

    /** Check if this is a hex string or a text string.
     *  
     *  If true the data returned by String() will be hexencoded
     *  and may contain '\\0' characters. You to decode the hex data
     *  by yourself.
     *
     *  \returns true if this is a hex string.
     *  \see String() \see PdfAlgorith::HexDecode
     */
    inline bool        IsHex () const;

    /** The contents of the strings can be read
     *  by this function.
     *  If IsHex() returns true the returned data is 
     *  hex encoded.
     * 
     *  \returns the strings contents
     *  \see IsHex
     */
    inline const char* String() const;

    /** The length of the string data returned by String()
     *  \returns the length of the string. 
     */
    inline long        Length() const;

    /** The size of the string data returned by String()
     *  \returns the size of the string in bytes. Typically
     *           Length() + 1
     *  \see Length
     */
    inline long        Size() const;
    
    /** Write this PdfString in PDF format to a PdfOutputDevice 
     *  \param pDevice the output device.
     *  \returns ErrOk on success
     */
    PdfError           Write ( PdfOutputDevice* pDevice );

    /** Copy an existing PdfString 
     *  \param rhs another PdfString to copy
     *  \returns this object
     */
    const PdfString & operator=( const PdfString & rhs );

    static const PdfString StringNull;

 private:
    /** allocate m_lLen data for m_pszData
     *  and output a warning if not successfull.
     *  \returns true if successfuly.
     */
    bool Allocate();

    /** Construct a new PdfString from a 0 terminated
     *  string. 
     *  The input string will be copied.
     *  if m_bhex is true the copied data will be hex encoded.
     *
     *  \param pszString the string to copy
     *  \param lLen length of the string data to copy
     *  
     */
    void Init( const char* pszString, long lLen );

 private:
    char* m_pszData;
    long  m_lLen;

    bool  m_bHex;
};

bool PdfString::IsValid() const
{
    return (m_pszData != NULL);
}

bool PdfString::IsHex () const
{
    return m_bHex;
}

const char* PdfString::String() const
{
    return m_pszData;
}

long PdfString::Length() const
{
    return m_lLen-1;
}

long PdfString::Size() const
{
    return m_lLen;
}

};

#endif // _PDF_STRING_H_
