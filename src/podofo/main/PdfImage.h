/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_IMAGE_H
#define PDF_IMAGE_H

#include "PdfXObject.h"

#ifdef PODOFO_HAVE_JPEG_LIB
struct jpeg_decompress_struct;
#endif // PODOFO_HAVE_JPEG_LIB

namespace PoDoFo {

class PdfArray;
class PdfDocument;
class InputStream;

struct PdfImageInfo
{
    unsigned Width = 0;
    unsigned Height = 0;
    PdfFilterList Filters;
    PdfColorSpace ColorSpace = PdfColorSpace::Unknown;
    PdfArray ColorSpaceArray;
    unsigned char BitsPerComponent = 0;
    PdfArray Decode;
};

/** A PdfImage object is needed when ever you want to embedd an image
 *  file into a PDF document.
 *  The PdfImage object is embedded once and can be drawn as often
 *  as you want on any page in the document using PdfPainter
 *
 *  \see GetImageReference
 *  \see PdfPainter::DrawImage
 */
class PODOFO_API PdfImage final : public PdfXObject
{
    friend class PdfXObject;
    friend class PdfDocument;

private:
    /** Constuct a new PdfImage object
     *  This is an overloaded constructor.
     *
     *  \param parent parent document
     *  \param prefix optional prefix for XObject-name
     */
    PdfImage(PdfDocument& doc, const std::string_view& prefix);

public:
    void DecodeTo(charbuff& buff, PdfPixelFormat format, int rowSize = -1) const;
    void DecodeTo(const bufferspan& buff, PdfPixelFormat format, int rowSize = -1) const;
    void DecodeTo(OutputStream& stream, PdfPixelFormat format, int rowSize = -1) const;

    charbuff GetDecodedCopy(PdfPixelFormat format);

    /** Get the color space of the image
    *
    *  \returns the color space of the image
    */
    PdfColorSpace GetColorSpace() const;

    /** Set an ICC profile for this image.
     *
     *  \param stream an input stream from which the ICC profiles data can be read
     *  \param colorComponents the number of colorcomponents of the ICC profile
     *  \param alternateColorSpace an alternate colorspace to use if the ICC profile cannot be used
     *
     *  \see SetImageColorSpace to set an colorspace instead of an ICC profile for this image
     */
    void SetICCProfile(InputStream& stream, unsigned colorComponents,
        PdfColorSpace alternateColorSpace = PdfColorSpace::DeviceRGB);

    //PdfColorSpace GetImageColorSpace() const;

    /** Set a softmask for this image.
     *  \param pSoftmask a PdfImage pointer to the image, which is to be set as softmask, must be 8-Bit-Grayscale
     *
     */
    void SetSoftMask(const PdfImage& softmask);

    /** Get the width of the image when drawn in PDF units
     *  \returns the width in PDF units
     */
    unsigned GetWidth() const;

    /** Get the height of the image when drawn in PDF units
     *  \returns the height in PDF units
     */
    unsigned GetHeight() const;

    /** Set the actual image data from a buffer
     *
     *  \param buffer buffer supplying image data
     *  \param width width of the image in pixels
     *  \param height height of the image in pixels
     *  \param format pixel format of the bitmap
     *  \param rowSize length of the row, if negative the default is used
     *
     */
    void SetData(const bufferview& buffer, unsigned width, unsigned height,
        PdfPixelFormat format, int rowSize = -1);

    /** Set the actual image data from an input stream
     *
     *  \param stream stream supplying raw image data
     *  \param width width of the image in pixels
     *  \param height height of the image in pixels
     *  \param format pixel format of the bitmap
     *  \param rowSize length of the row, if negative the default is used
     */
    void SetData(InputStream& stream, unsigned width, unsigned height,
        PdfPixelFormat format, int rowSize = -1);

    /** Set the actual image encoded data from a buffer
     *
     *  \param buffer buffer supplying image data
     *  \param info parameters describing the encoded image data
     */
    void SetDataRaw(const bufferview& buffer, const PdfImageInfo& info);

    /** Set the actual image encoded data from an input stream.
     *
     *  \param stream stream supplying encoded image data
     *  \param info parameters describing the encoded image data
     */
    void SetDataRaw(InputStream& stream, const PdfImageInfo& info);

    /** Load the image data from a file
     */
    void Load(const std::string_view& filepath);

    /** Load the image data from bytes
     */
    void LoadFromBuffer(const bufferview& buffer);

    void ExportTo(charbuff& buff, PdfExportFormat format, PdfArray args = {}) const;

    /** Set an color/chroma-key mask on an image.
     *  The masked color will not be painted, i.e. masked as being transparent.
     *
     *  \param r red RGB value of color that should be masked
     *  \param g green RGB value of color that should be masked
     *  \param b blue RGB value of color that should be masked
     *  \param threshold colors are masked that are in the range [(r-threshold, r+threshold),(g-threshold, g+threshold),(b-threshold, b+threshold)]
     */
    void SetChromaKeyMask(int64_t r, int64_t g, int64_t b, int64_t threshold = 0);

    /**
     * Apply an interpolation to the image if the source resolution
     * is lower than the resolution of the output device.
     * Default is false.
     * \param value whether the image should be interpolated
     */
    void SetInterpolate(bool value);

    PdfRect GetRect() const override;

private:
    /** Construct an image from an existing PdfObject
     *
     *  \param obj a PdfObject that has to be an image
     */
    PdfImage(PdfObject& obj);

    charbuff initScanLine(PdfPixelFormat format, int rowSize, charbuff& smask) const;

    unsigned getBufferSize(PdfPixelFormat format) const;

#ifdef PODOFO_HAVE_JPEG_LIB
    void loadFromJpegInfo(jpeg_decompress_struct& ctx, PdfImageInfo& info);
    void exportToJpeg(charbuff& buff, const PdfArray& args) const;
    /** Load the image data from a JPEG file
     *  \param filename
     */
    void loadFromJpeg(const std::string_view& filename);

    /** Load the image data from JPEG bytes
     *  \param data JPEG bytes
     *  \param len number of bytes
     */
    void loadFromJpegData(const unsigned char* data, size_t len);
#endif // PODOFO_HAVE_JPEG_LIB

#ifdef PODOFO_HAVE_TIFF_LIB
    void loadFromTiffHandle(void* handle);
    /** Load the image data from a TIFF file
     *  \param filename
     */
    void loadFromTiff(const std::string_view& filename);

    /** Load the image data from TIFF bytes
     *  \param data TIFF bytes
     *  \param len number of bytes
     */
    void loadFromTiffData(const unsigned char* data, size_t len);
#endif // PODOFO_HAVE_TIFF_LIB

#ifdef PODOFO_HAVE_PNG_LIB
    void loadFromPngHandle(FILE* stream);
    /** Load the image data from a PNG file
     *  \param filename
     */
    void loadFromPng(const std::string_view& filename);

    /** Load the image data from PNG bytes
     *  \param data PNG bytes
     *  \param len number of bytes
     */
    void loadFromPngData(const unsigned char* data, size_t len);
#endif // PODOFO_HAVE_PNG_LIB

private:
    unsigned m_Width;
    unsigned m_Height;
};

};

#endif // PDF_IMAGE_H
