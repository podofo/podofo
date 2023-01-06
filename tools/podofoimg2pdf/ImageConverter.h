/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef IMAGECONVERTER_H
#define IMAGECONVERTER_H

#include <string>
#include <vector>

class ImageConverter
{
public:
    ImageConverter();

    inline void SetOutputFilename(const std::string_view& outputPath)
    {
        m_outputPath = outputPath;
    }

    inline void AddImage(const std::string_view& image)
    {
        m_images.push_back(std::string(image));
    }

    inline void SetUseImageSize(bool imageSize)
    {
        m_useImageSize = imageSize;
    }

    void Work();

private:
    std::vector<std::string> m_images;
    std::string m_outputPath;
    bool m_useImageSize;
};

#endif // IMAGECONVERTER_H
