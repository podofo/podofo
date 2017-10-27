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

#ifndef _PDF_EXTGSTATE_H_
#define _PDF_EXTGSTATE_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfName.h"

#include "PdfElement.h"

namespace PoDoFo {

    class PdfObject;
    class PdfPage;
    class PdfWriter;

/** This class wraps the ExtGState object used in the Resource
 *  Dictionary of a Content-supporting element (page, Pattern, etc.)
 *  The main usage is for transparency, but it also support a variety
 *  of prepress features.
 */
class PODOFO_DOC_API PdfExtGState : public PdfElement {
 public:
    /** Create a new PdfExtGState object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param pParent parent vector of objects
     *  
     */
    PdfExtGState( PdfVecObjects* pParent );

    /** Create a new PdfExtGState object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param pParent parent document 
     *  
     */
    PdfExtGState( PdfDocument* pParent );

    virtual ~PdfExtGState();

    /** Returns the identifier of this ExtGState how it is known
     *  in the pages resource dictionary.
     *  \returns PdfName containing the identifier (e.g. /ExtGS13)
     */
    inline const PdfName & GetIdentifier() const;

    /** Sets the opacity value to be used for fill operations
     *  \param opac a floating point value from 0 (transparent) to 1 (opaque)
     */
    void SetFillOpacity( float opac );

    /** Sets the opacity value to be used for stroking operations
     *  \param opac a floating point value from 0 (transparent) to 1 (opaque)
     */
    void SetStrokeOpacity( float opac );

    /** Sets the transparency blend mode
     *  \param blendMode one of the predefined blending modes (see PodofoDefines.h)
     */
    void SetBlendMode( const char* blendMode );

    /** Enables/Disables overprinting for both Fill & Stroke
     *  \param enable enable or disable
     */
    void SetOverprint( bool enable=true );

    /** Enables/Disables overprinting for Fill operations
     *  \param enable enable or disable
     */
    void SetFillOverprint( bool enable=true );

    /** Enables/Disables overprinting for Stroke operations
     *  \param enable enable or disable
     */
    void SetStrokeOverprint( bool enable=true );

    /** Enables/Disables non-zero overprint mode
     *  \param enable enable or disable
     */
    void SetNonZeroOverprint( bool enable=true );

    /** Set the Rendering Intent
     *  \param intent one of the predefined intents (see Podofo.h)
     */
    void SetRenderingIntent( const char* intent );

    /** Set the frequency for halftones
     *  \param frequency screen frequency, measured in halftone cells per inch in device space
     */
    void SetFrequency( double frequency );

 private:
    /** Initialize the object
     */
    void Init( void );

 private: 
    PdfName m_Identifier;
};


const PdfName & PdfExtGState::GetIdentifier() const
{
    return m_Identifier;
}

};

#endif // _PDF_EXTGSTATE_H_

