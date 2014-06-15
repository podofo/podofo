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

#include "PdfCanvas.h"

#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfColor.h"
#include "PdfStream.h"
#include "PdfDefinesPrivate.h"

namespace PoDoFo {

PdfArray PdfCanvas::s_procset;

const PdfArray & PdfCanvas::GetProcSet()
{
    if( s_procset.empty() ) 
    {
        s_procset.push_back( PdfName( "PDF" ) );
        s_procset.push_back( PdfName( "Text" ) );
        s_procset.push_back( PdfName( "ImageB" ) );
        s_procset.push_back( PdfName( "ImageC" ) );
        s_procset.push_back( PdfName( "ImageI" ) );
    }

    return s_procset;
}

void PdfCanvas::AddColorResource( const PdfColor & rColor )
{
    PdfObject* pResource = GetResources();
    
    if( !pResource )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

	switch( rColor.GetColorSpace() )
	{
		case ePdfColorSpace_Separation:
		{
			std::string csPrefix( "ColorSpace" );
			std::string csName = rColor.GetName();
			std::string temp( csPrefix + csName );
            
			if ( 
				! pResource->GetDictionary().HasKey( "ColorSpace" )	||
				! pResource->GetDictionary().GetKey( "ColorSpace" )->GetDictionary().HasKey( csPrefix + csName )
                )
			{
				// Build color-spaces for separation
                PdfObject* csp = rColor.BuildColorSpace( GetContents()->GetOwner() );
 
                AddResource( csPrefix + csName, csp->Reference(), PdfName("ColorSpace") );
			}
		}
		break;

		case ePdfColorSpace_CieLab:
		{
			if ( 
				! pResource->GetDictionary().HasKey( "ColorSpace" )	||
				! pResource->GetDictionary().GetKey( "ColorSpace" )->GetDictionary().HasKey( "ColorSpaceLab" )
			   )
			{
				// Build color-spaces for CIE-lab
                PdfObject* csp = rColor.BuildColorSpace( GetContents()->GetOwner() );

				AddResource( "ColorSpaceCieLab", csp->Reference(), PdfName("ColorSpace") );
			}
		}
		break;

        case ePdfColorSpace_DeviceGray:
        case ePdfColorSpace_DeviceRGB:
        case ePdfColorSpace_DeviceCMYK:
	case ePdfColorSpace_Indexed:
            // No colorspace needed
        case ePdfColorSpace_Unknown:
		default:
		break;
	}
}

void PdfCanvas::AddResource( const PdfName & rIdentifier, const PdfReference & rRef, const PdfName & rName )
{
    if( !rName.GetLength() || !rIdentifier.GetLength() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    PdfObject* pResource = this->GetResources();
    
    if( !pResource )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( !pResource->GetDictionary().HasKey( rName ) )
    {
        pResource->GetDictionary().AddKey( rName, PdfDictionary() );
    }

    // Peter Petrov: 18 December 2008. Bug fix
	if (ePdfDataType_Reference == pResource->GetDictionary().GetKey( rName )->GetDataType())
    {
        PdfObject *directObject = pResource->GetOwner()->GetObject(pResource->GetDictionary().GetKey( rName )->GetReference());

        if (0 == directObject)
        {
            PODOFO_RAISE_ERROR( ePdfError_NoObject );
        }

        if( !directObject->GetDictionary().HasKey( rIdentifier ) )
            directObject->GetDictionary().AddKey( rIdentifier, rRef );
    }else
    {

        if( !pResource->GetDictionary().GetKey( rName )->GetDictionary().HasKey( rIdentifier ) )
            pResource->GetDictionary().GetKey( rName )->GetDictionary().AddKey( rIdentifier, rRef );
    }
}

};

