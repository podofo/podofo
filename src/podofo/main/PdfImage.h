/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_IMAGE_H
#define PDF_IMAGE_H

#include "PdfXObject.h"

#include "PdfColorSpace.h"

#ifdef PODOFO_HAVE_JPEG_LIB
struct jpeg_decompress_struct;
#endif // PODOFO_HAVE_JPEG_LIB

struct png_struct_def;
struct png_info_def;

namespace PoDoFo {

class PdfDocument;
class InputStream;

enum class PdfImageOrientation : uint8_t
{
    Unknown = 0,
    TopLeft,
    TopRight,
    BottomRight,
    BottomLeft,
    LeftTop,
    RightTop,
    RightBottom,
    LeftBottom
};

struct PODOFO_API PdfImageInfo final
{
    unsigned Width = 0;
    unsigned Height = 0;
    nullable<PdfFilterList> Filters;
    unsigned char BitsPerComponent = 0;
    PdfColorSpaceInitializer ColorSpace;
    std::vector<double> DecodeArray;
    PdfImageOrientation Orientation = PdfImageOrientation::TopLeft;
};

/** Non PDF specific image metadata descriptors fetched from image codecs
 */
struct PODOFO_API PdfImageMetadata final
{
    PdfImageOrientation Orientation = PdfImageOrientation::Unknown;
};

enum class PdfImageLoadFlags
{
    None = 0,
    SkipTransform = 1,  ///< Skip applying orientation transform
};

struct PODOFO_API PdfImageLoadParams final
{
    unsigned ImageIndex = 0;
    PdfImageLoadFlags Flags = PdfImageLoadFlags::None;
};

/** A PdfImage object is needed when ever you want to embed an image
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
    /** Construct a new PdfImage object
     *  This is an overloaded constructor.
     *
     *  \param parent parent document
     */
    PdfImage(PdfDocument& doc);

public:
    void DecodeTo(charbuff& buff, PdfPixelFormat format, int scanLineSize = -1) const;
    void DecodeTo(const bufferspan& buff, PdfPixelFormat format, int scanLineSize = -1) const;
    void DecodeTo(OutputStream& stream, PdfPixelFormat format, int scanLineSize = -1) const;

    charbuff GetDecodedCopy(PdfPixelFormat format);

    /** Set a softmask for this image.
     *  \param pSoftmask a PdfImage pointer to the image, which is to be set as softmask, must be 8-Bit-Grayscale
     *
     */
    void SetSoftMask(const PdfImage& softmask);

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

    /** Load the image data from bytes
     * \param params parameters like index to be fed to multi image/page
     *   formats (eg. TIFF)
     * \returns image non PDF specific image metadata
     */
    PdfImageMetadata Load(const std::string_view& filepath, const PdfImageLoadParams& params = { });

    /** Load the image data from bytes
     * \param params parameters like index to be fed to multi image/page
     *   formats (eg. TIFF)
     * \returns image non PDF specific image metadata 
     */
    PdfImageMetadata LoadFromBuffer(const bufferview& buffer, const PdfImageLoadParams& params = { });

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

    Rect GetRect() const override;

    /** Get the color space of the image
     * \returns the color space of the image
     */
    const PdfColorSpaceFilter& GetColorSpace() const { return *m_ColorSpace; }

    /** Get the width of the image when drawn in PDF units
     *  \returns the width in PDF units
     */
    unsigned GetWidth() const { return m_Width; }

    /** Get the height of the image when drawn in PDF units
     *  \returns the height in PDF units
     */
    unsigned GetHeight() const { return m_Height; }

protected:
    const PdfXObjectForm* GetForm() const override;

private:
    /** Construct an image from an existing PdfObject
     *
     *  \param obj a PdfObject that has to be an image
     */
    PdfImage(PdfObject& obj);

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
    void loadFromTiffHandle(void* handle, const PdfImageLoadParams& params, PdfImageMetadata& metadata);
    /** Load the image data from a TIFF file
     *  \param filename
     */
    void loadFromTiff(const std::string_view& filename, const PdfImageLoadParams& params, PdfImageMetadata& metadata);

    /** Load the image data from TIFF bytes
     *  \param data TIFF bytes
     *  \param len number of bytes
     */
    void loadFromTiffData(const unsigned char* data, size_t len, const PdfImageLoadParams& params, PdfImageMetadata& metadata);
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

    static void loadFromPngContent(PdfImage& image, png_struct_def* png, png_info_def* info);
#endif // PODOFO_HAVE_PNG_LIB

    std::unique_ptr<PdfXObjectForm> getTransformation(PdfImageOrientation orientation);

private:
    PdfColorSpaceFilterPtr m_ColorSpace;
    unsigned m_Width;
    unsigned m_Height;
    unsigned m_BitsPerComponent;
    std::unique_ptr<PdfXObjectForm> m_Transformation;
};

};

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfImageLoadFlags);

#endif // PDF_IMAGE_H
