/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ImageConverter.h"

#include <podofo/podofo.h>

using namespace std;
using namespace PoDoFo;

ImageConverter::ImageConverter()
    : m_useImageSize(false)
{
}

void ImageConverter::Work()
{
    PdfMemDocument document;

    Rect size = PdfPage::CreateStandardPageSize(PdfPageSize::A4, false);
    PdfPainter painter;
    double scaleX = 1.0;
    double scaleY = 1.0;
    double scale = 1.0;

    for (auto& file : m_images)
    {
        auto image = document.CreateImage();
        image->Load(file);

        if (m_useImageSize)
            size = Rect(0.0, 0.0, image->GetWidth(), image->GetHeight());

        auto& page = document.GetPages().CreatePage(size);
        scaleX = size.Width / image->GetWidth();
        scaleY = size.Height / image->GetHeight();
        scale = std::min(scaleX, scaleY);

        painter.SetCanvas(page);
        if (scale < 1.0)
        {
            painter.DrawImage(*image, 0.0, 0.0, scale, scale);
        }
        else
        {
            // Center Image
            double x = (size.Width - image->GetWidth()) / 2.0;
            double y = (size.Height - image->GetHeight()) / 2.0;
            painter.DrawImage(*image, x, y);
        }

        painter.FinishDrawing();
    }

    document.Save(m_outputPath.c_str());
}
