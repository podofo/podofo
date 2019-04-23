/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "PdfFunction.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfStream.h"

namespace PoDoFo {

PdfFunction::PdfFunction( EPdfFunctionType eType, const PdfArray & rDomain, PdfVecObjects* pParent )
    : PdfElement( NULL, pParent )
{
    Init( eType, rDomain );
}

PdfFunction::PdfFunction( EPdfFunctionType eType, const PdfArray & rDomain, PdfDocument* pParent )
    : PdfElement( NULL, pParent )
{
    Init( eType, rDomain );
}

PdfFunction::~PdfFunction()
{

}

void PdfFunction::Init( EPdfFunctionType eType, const PdfArray & rDomain )
{
    this->GetObject()->GetDictionary().AddKey( PdfName("FunctionType"), static_cast<pdf_int64>(eType) );
    this->GetObject()->GetDictionary().AddKey( PdfName("Domain"), rDomain );

}

/////////////////////////////////////////////////////////////////////////////
PdfSampledFunction::PdfSampledFunction( const PdfArray & rDomain,  const PdfArray & rRange, const PdfFunction::Sample & rlstSamples, PdfVecObjects* pParent )
    : PdfFunction( ePdfFunctionType_Sampled, rDomain, pParent )
{
	Init( rDomain, rRange, rlstSamples );
}

PdfSampledFunction::PdfSampledFunction( const PdfArray & rDomain,  const PdfArray & rRange, const PdfFunction::Sample & rlstSamples, PdfDocument* pParent )
    : PdfFunction( ePdfFunctionType_Sampled, rDomain, pParent )
{
	Init( rDomain, rRange, rlstSamples );
}

void PdfSampledFunction::Init( const PdfArray & rDomain,  const PdfArray & rRange, const PdfFunction::Sample & rlstSamples )
{
	PdfArray Size;
	for( unsigned i = 0; i < rDomain.GetSize() / 2; i++ )
		Size.push_back( PdfObject( static_cast<pdf_int64>(rDomain.GetSize() / 2L )) );

    this->GetObject()->GetDictionary().AddKey( PdfName("Domain"), rDomain );
    this->GetObject()->GetDictionary().AddKey( PdfName("Range"), rRange );
    this->GetObject()->GetDictionary().AddKey( PdfName("Size"), Size );
    this->GetObject()->GetDictionary().AddKey( PdfName("Order"), PdfObject( static_cast<pdf_int64>(PODOFO_LL_LITERAL(1)) ) );
    this->GetObject()->GetDictionary().AddKey( PdfName("BitsPerSample"), PdfObject( static_cast<pdf_int64>(PODOFO_LL_LITERAL(8)) ) );

    this->GetObject()->GetStream()->BeginAppend();
    PdfFunction::Sample::const_iterator it = rlstSamples.begin();
    while( it != rlstSamples.end() )
    {
        this->GetObject()->GetStream()->Append( & ( *it ), 1 );
        ++it;
    }
    this->GetObject()->GetStream()->EndAppend();
}

/////////////////////////////////////////////////////////////////////////////
PdfExponentialFunction::PdfExponentialFunction( const PdfArray & rDomain, const PdfArray & rC0, const PdfArray & rC1, double dExponent, PdfVecObjects* pParent )
    : PdfFunction( ePdfFunctionType_Exponential, rDomain, pParent )
{
    Init( rC0, rC1, dExponent );
}

PdfExponentialFunction::PdfExponentialFunction( const PdfArray & rDomain, const PdfArray & rC0, const PdfArray & rC1, double dExponent, PdfDocument* pParent )
    : PdfFunction( ePdfFunctionType_Exponential, rDomain, pParent )
{
    Init( rC0, rC1, dExponent );
}

void PdfExponentialFunction::Init( const PdfArray & rC0, const PdfArray & rC1, double dExponent )
{
    this->GetObject()->GetDictionary().AddKey( PdfName("C0"), rC0 );
    this->GetObject()->GetDictionary().AddKey( PdfName("C1"), rC1 );
    this->GetObject()->GetDictionary().AddKey( PdfName("N"), dExponent );
}

/////////////////////////////////////////////////////////////////////////////

PdfStitchingFunction::PdfStitchingFunction( const PdfFunction::List & rlstFunctions, const PdfArray & rDomain, const PdfArray & rBounds, const PdfArray & rEncode, PdfVecObjects* pParent )
    : PdfFunction( ePdfFunctionType_Stitching, rDomain, pParent )
{
    Init( rlstFunctions, rBounds, rEncode );
}

PdfStitchingFunction::PdfStitchingFunction( const PdfFunction::List & rlstFunctions, const PdfArray & rDomain, const PdfArray & rBounds, const PdfArray & rEncode, PdfDocument* pParent )
    : PdfFunction( ePdfFunctionType_Stitching, rDomain, pParent )
{
    Init( rlstFunctions, rBounds, rEncode );
}

void PdfStitchingFunction::Init( const PdfFunction::List & rlstFunctions, const PdfArray & rBounds, const PdfArray & rEncode )
{
    PdfArray                          functions;
    PdfFunction::List::const_iterator it = rlstFunctions.begin();

    functions.reserve( rlstFunctions.size() );

    while( it != rlstFunctions.end() )
    {
        functions.push_back( (*it).GetObject()->Reference() );
        ++it;
    }
    
    this->GetObject()->GetDictionary().AddKey( PdfName("Functions"), functions );
    this->GetObject()->GetDictionary().AddKey( PdfName("Bounds"), rBounds );
    this->GetObject()->GetDictionary().AddKey( PdfName("Encode"), rEncode );
}

};
