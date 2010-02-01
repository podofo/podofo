/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "ImageConverter.h"

#include <podofo.h>

ImageConverter::ImageConverter() 
    : m_bUseImageSize( false )
{
}

ImageConverter::~ImageConverter() 
{
}

void ImageConverter::Work() 
{
    PoDoFo::PdfMemDocument document;

    std::vector<std::string>::const_iterator it = m_vecImages.begin();
    PoDoFo::PdfRect size = PoDoFo::PdfPage::CreateStandardPageSize( PoDoFo::ePdfPageSize_A4, false );
    PoDoFo::PdfPainter painter;
    double dScaleX = 1.0;
    double dScaleY = 1.0;
    double dScale  = 1.0;

    while( it != m_vecImages.end() ) 
    {
        PoDoFo::PdfPage* pPage;
        PoDoFo::PdfImage image( &document );
        image.LoadFromFile( (*it).c_str() );

        if( m_bUseImageSize ) 
        {
            size = PoDoFo::PdfRect( 0.0, 0.0, image.GetWidth(), image.GetHeight() );
        }

        pPage = document.CreatePage( size );
        dScaleX = size.GetWidth() / image.GetWidth();
        dScaleY = size.GetHeight() / image.GetHeight();
        dScale  = PoDoFo::PDF_MIN( dScaleX, dScaleY );

        painter.SetPage( pPage );
        if( dScale < 1.0 ) 
        {
            painter.DrawImage( 0.0, 0.0, &image, dScale, dScale );
        }
        else
        {
            // Center Image
            double dX = (size.GetWidth() - image.GetWidth())/2.0;
            double dY = (size.GetHeight() - image.GetHeight())/2.0;
            painter.DrawImage( dX, dY, &image );
        }

        painter.FinishPage();

        ++it;
    }

    document.Write( m_sOutputFilename.c_str() );
}

