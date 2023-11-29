/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef IMAGE_EXTRACTOR_H
#define IMAGE_EXTRACTOR_H

#include <podofo/podofo.h>

/** This class uses the PoDoFo lib to parse
 *  a PDF file and to write all images it finds
 *  in this PDF document to a given directory.
 */
class ImageExtractor
{
    static constexpr unsigned MAX_PATH = 512;

public:
    ImageExtractor();

    /**
     * \param pnNum pointer to an integer were
     *        the number of processed images can be stored
     *        or null if you do not want this information.
     */
    void Init(const std::string_view& input, const std::string_view& output);

    /**
     * \returns the number of successfully extracted images
     */
    inline unsigned GetNumImagesExtracted() const;

private:
    /** Extracts the image form the given PdfObject
     *  which has to be an XObject with Subtype "Image"
     *  \param obj a handle to a PDF object
     *  \param jpeg if true extract as a jpeg, otherwise create a ppm
     *  \returns ErrOk on success
     */
    void ExtractImage(const PoDoFo::PdfObject& obj, bool jpeg);

    /** This function checks whether a file with the
     *  given filename does exist.
     *  \returns true if the file exists otherwise false
     */
    bool FileExists(const std::string_view& filepath);

private:
    std::string_view m_outputDirectory;
    unsigned m_ImageCount;
    unsigned m_fileCounter;
    char m_buffer[MAX_PATH];
};

inline unsigned ImageExtractor::GetNumImagesExtracted() const
{
    return m_ImageCount;
}

#endif // IMAGE_EXTRACTOR_H
