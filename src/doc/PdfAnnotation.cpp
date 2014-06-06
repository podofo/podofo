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

#include "PdfAnnotation.h"

#include "base/PdfDefinesPrivate.h"
#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfDate.h"

#include "PdfAction.h"
#include "PdfFileSpec.h"
#include "PdfPage.h"
#include "base/PdfRect.h"
#include "base/PdfVariant.h"
#include "PdfXObject.h"

namespace PoDoFo {

const long  PdfAnnotation::s_lNumActions = 27;
const char* PdfAnnotation::s_names[] = {
    "Text",                       // - supported
    "Link",
    "FreeText",       // PDF 1.3  // - supported
    "Line",           // PDF 1.3  // - supported
    "Square",         // PDF 1.3
    "Circle",         // PDF 1.3
    "Polygon",        // PDF 1.5
    "PolyLine",       // PDF 1.5
    "Highlight",      // PDF 1.3
    "Underline",      // PDF 1.3
    "Squiggly",       // PDF 1.4
    "StrikeOut",      // PDF 1.3
    "Stamp",          // PDF 1.3
    "Caret",          // PDF 1.5
    "Ink",            // PDF 1.3
    "Popup",          // PDF 1.3
    "FileAttachment", // PDF 1.3
    "Sound",          // PDF 1.2
    "Movie",          // PDF 1.2
    "Widget",         // PDF 1.2  // - supported
    "Screen",         // PDF 1.5
    "PrinterMark",    // PDF 1.4
    "TrapNet",        // PDF 1.3
    "Watermark",      // PDF 1.6
    "3D",             // PDF 1.6
    "RichMedia",      // PDF 1.7 ADBE ExtensionLevel 3 ALX: Petr P. Petrov
    "WebMedia",       // PDF 1.7 IPDF ExtensionLevel 1
    NULL
};

PdfAnnotation::PdfAnnotation( PdfPage* pPage, EPdfAnnotation eAnnot, const PdfRect & rRect, PdfVecObjects* pParent )
    : PdfElement( "Annot", pParent ), m_eAnnotation( eAnnot ), m_pAction( NULL ), m_pFileSpec( NULL ), m_pPage( pPage )
{
    PdfVariant    rect;
    PdfDate       date;
    PdfString     sDate;
    const PdfName name( TypeNameForIndex( eAnnot, s_names, s_lNumActions ) );

    if( !name.GetLength() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    rRect.ToVariant( rect );

    this->GetObject()->GetDictionary().AddKey( PdfName::KeyRect, rect );

    rRect.ToVariant( rect );
    date.ToString( sDate );
    
    this->GetObject()->GetDictionary().AddKey( PdfName::KeySubtype, name );
    this->GetObject()->GetDictionary().AddKey( PdfName::KeyRect, rect );
    this->GetObject()->GetDictionary().AddKey( "P", pPage->GetObject()->Reference() );
    this->GetObject()->GetDictionary().AddKey( "M", sDate );
}

PdfAnnotation::PdfAnnotation( PdfObject* pObject, PdfPage* pPage )
    : PdfElement( "Annot", pObject ), m_eAnnotation( ePdfAnnotation_Unknown ), m_pAction( NULL ), m_pFileSpec( NULL ), m_pPage( pPage )
{
    m_eAnnotation = static_cast<EPdfAnnotation>(TypeNameToIndex( this->GetObject()->GetDictionary().GetKeyAsName( PdfName::KeySubtype ).GetName().c_str(), s_names, s_lNumActions, ePdfAnnotation_Unknown ));
}

PdfAnnotation::~PdfAnnotation()
{
    delete m_pAction;
    delete m_pFileSpec;
}

PdfRect PdfAnnotation::GetRect() const
{
   if( this->GetObject()->GetDictionary().HasKey( PdfName::KeyRect ) )
        return PdfRect( this->GetObject()->GetDictionary().GetKey( PdfName::KeyRect )->GetArray() );

   return PdfRect();
}

void PdfAnnotation::SetAppearanceStream( PdfXObject* pObject )
{
    PdfDictionary dict;
    PdfDictionary internal;

    if( !pObject )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    internal.AddKey( "On", pObject->GetObject()->Reference() );
    internal.AddKey( "Off", pObject->GetObject()->Reference() );

    dict.AddKey( "N", internal );

    this->GetObject()->GetDictionary().AddKey( "AP", dict );
    this->GetObject()->GetDictionary().AddKey( "AS", PdfName("On") );
}

bool PdfAnnotation::HasAppearanceStream() const
{
    return this->GetObject()->GetDictionary().HasKey( "AP" );
}


void PdfAnnotation::SetFlags( pdf_uint32 uiFlags )
{
    this->GetObject()->GetDictionary().AddKey( "F", PdfVariant( static_cast<pdf_int64>(uiFlags) ) );
}

pdf_uint32 PdfAnnotation::GetFlags() const
{
    if( this->GetObject()->GetDictionary().HasKey( "F" ) )
        return static_cast<pdf_uint32>(this->GetObject()->GetDictionary().GetKey( "F" )->GetNumber());

    return static_cast<pdf_uint32>(0);
}

void PdfAnnotation::SetBorderStyle( double dHCorner, double dVCorner, double dWidth )
{
    this->SetBorderStyle( dHCorner, dVCorner, dWidth, PdfArray() );
}

void PdfAnnotation::SetBorderStyle( double dHCorner, double dVCorner, double dWidth, const PdfArray & rStrokeStyle )
{
    // TODO : Support for Border style for PDF Vers > 1.0
    PdfArray aValues;

    aValues.push_back(dHCorner);
    aValues.push_back(dVCorner);
    aValues.push_back(dWidth);
    if( rStrokeStyle.size() )
        aValues.push_back(rStrokeStyle);

    this->GetObject()->GetDictionary().AddKey( "Border", aValues );
}

void PdfAnnotation::SetTitle( const PdfString & sTitle )
{
    this->GetObject()->GetDictionary().AddKey( "T", sTitle );
}

PdfString PdfAnnotation::GetTitle() const
{
    if( this->GetObject()->GetDictionary().HasKey( "T" ) )
        return this->GetObject()->GetDictionary().GetKey( "T" )->GetString();

    return PdfString();
}

void PdfAnnotation::SetContents( const PdfString & sContents )
{
    this->GetObject()->GetDictionary().AddKey( "Contents", sContents );
}

PdfString PdfAnnotation::GetContents() const
{
    if( this->GetObject()->GetDictionary().HasKey( "Contents" ) )
        return this->GetObject()->GetDictionary().GetKey( "Contents" )->GetString();

    return PdfString();
}

void PdfAnnotation::SetDestination( const PdfDestination & rDestination )
{
    rDestination.AddToDictionary( this->GetObject()->GetDictionary() );
}

PdfDestination PdfAnnotation::GetDestination( PdfDocument* pDoc ) const
{
    return PdfDestination( this->GetNonConstObject()->GetDictionary().GetKey( "Dest" ), pDoc );
}

bool PdfAnnotation::HasDestination() const
{
    return this->GetObject()->GetDictionary().HasKey( "Dest" );
}

void PdfAnnotation::SetAction( const PdfAction & rAction )
{
    if( m_pAction )
        delete m_pAction;

    m_pAction = new PdfAction( rAction );
    this->GetObject()->GetDictionary().AddKey( "A", m_pAction->GetObject()->Reference() );
}

PdfAction* PdfAnnotation::GetAction() const
{
    if( !m_pAction && HasAction() )
        const_cast<PdfAnnotation*>(this)->m_pAction = new PdfAction( this->GetObject()->GetIndirectKey( "A" ) );

    return m_pAction;
}

bool PdfAnnotation::HasAction() const
{
    return this->GetObject()->GetDictionary().HasKey( "A" );
}

void PdfAnnotation::SetOpen( bool b )
{
    this->GetObject()->GetDictionary().AddKey( "Open", b );
}

bool PdfAnnotation::GetOpen() const
{
    if( this->GetObject()->GetDictionary().HasKey( "Open" ) )
        return this->GetObject()->GetDictionary().GetKey( "Open" )->GetBool();

    return false;
}

bool PdfAnnotation::HasFileAttachement() const
{
    return this->GetObject()->GetDictionary().HasKey( "FS" );
}

void PdfAnnotation::SetFileAttachement( const PdfFileSpec & rFileSpec )
{
    if( m_pFileSpec )
        delete m_pFileSpec;

    m_pFileSpec = new PdfFileSpec( rFileSpec );
    this->GetObject()->GetDictionary().AddKey( "FS", m_pFileSpec->GetObject()->Reference() );
}

PdfFileSpec* PdfAnnotation::GetFileAttachement() const
{
    if( !m_pFileSpec && HasFileAttachement() )
        const_cast<PdfAnnotation*>(this)->m_pFileSpec = new PdfFileSpec( this->GetObject()->GetIndirectKey( "FS" ) );

    return m_pFileSpec;
}

PdfArray PdfAnnotation::GetQuadPoints() const
{
    if( this->GetObject()->GetDictionary().HasKey( "QuadPoints" ) )
        return PdfArray( this->GetObject()->GetDictionary().GetKey( "QuadPoints" )->GetArray() );

    return PdfArray();
}

void PdfAnnotation::SetQuadPoints( const PdfArray & rQuadPoints )
{
    if ( m_eAnnotation != ePdfAnnotation_Highlight &&
         m_eAnnotation != ePdfAnnotation_Underline &&
	 m_eAnnotation != ePdfAnnotation_Squiggly  &&
	 m_eAnnotation != ePdfAnnotation_StrikeOut )
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Must be a text markup annotation (hilight, underline, squiggly or strikeout) to set quad points" );

    this->GetObject()->GetDictionary().AddKey( "QuadPoints", rQuadPoints );
}

PdfArray PdfAnnotation::GetColor() const
{
    if( this->GetObject()->GetDictionary().HasKey( "C" ) )
        return PdfArray( this->GetObject()->GetDictionary().GetKey( "C" )->GetArray() );
    return PdfArray();
}

void PdfAnnotation::SetColor( double r, double g, double b )
{
    PdfArray c;
    c.push_back( PdfVariant( r ) );
    c.push_back( PdfVariant( g ) );
    c.push_back( PdfVariant( b ) );
    this->GetObject()->GetDictionary().AddKey( "C", c );
}
void PdfAnnotation::SetColor( double C, double M, double Y, double K ) 
{
    PdfArray c;
    c.push_back( PdfVariant( C ) );
    c.push_back( PdfVariant( M ) );
    c.push_back( PdfVariant( Y ) );
    c.push_back( PdfVariant( K ) );
    this->GetObject()->GetDictionary().AddKey( "C", c );
}

void PdfAnnotation::SetColor( double gray ) 
{
    PdfArray c;
    c.push_back( PdfVariant( gray ) );
    this->GetObject()->GetDictionary().AddKey( "C", c );
}

void PdfAnnotation::SetColor() 
{
    PdfArray c;
    this->GetObject()->GetDictionary().AddKey( "C", c );
}

};
