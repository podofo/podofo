/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfImage.h"

#ifdef PODOFO_HAVE_TIFF_LIB
extern "C" {
#include <tiffio.h>
}
#endif // PODOFO_HAVE_TIFF_LIB

#include <utfcpp/utf8.h>

#include <podofo/private/FileSystem.h>
#include <podofo/private/PdfFiltersPrivate.h>
#include <podofo/private/ImageUtils.h>

#include <pdfium/core/fxcodec/fax/faxmodule.h>

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfArray.h"
#include "PdfColor.h"
#include "PdfObjectStream.h"
#include "PdfStreamDevice.h"

// TIFF and JPEG headers already included through "PdfFiltersPrivate.h",
// although in opposite order (first JPEG, then TIFF), if available of course

using namespace std;
using namespace PoDoFo;

#ifdef PODOFO_HAVE_PNG_LIB
#include <png.h>
static void pngReadData(png_structp pngPtr, png_bytep data, png_size_t length);
static void loadFromPngContent(PdfImage& image, png_structp png, png_infop info);
static void createPngContext(png_structp& png, png_infop& pnginfo);
#endif // PODOFO_HAVE_PNG_LIB

static void fetchPDFScanLineRGB(unsigned char* dstScanLine,
    unsigned width, const unsigned char* srcScanLine, PdfPixelFormat srcPixelFormat);

PdfImage::PdfImage(PdfDocument& doc, const string_view& prefix)
    : PdfXObject(doc, PdfXObjectType::Image, prefix), m_Width(0), m_Height(0)
{
}

void PdfImage::DecodeTo(charbuff& buffer, PdfPixelFormat format, int rowSize) const
{
    buffer.resize(getBufferSize(format));
    SpanStreamDevice stream(buffer);
    DecodeTo(stream, format, rowSize);
}

void PdfImage::DecodeTo(const bufferspan& buffer, PdfPixelFormat format, int rowSize) const
{
    SpanStreamDevice stream(buffer);
    DecodeTo(stream, format, rowSize);
}

// TODO: Improve performance and format support
void PdfImage::DecodeTo(OutputStream& stream, PdfPixelFormat format, int rowSize) const
{
    auto istream = GetObject().MustGetStream().GetInputStream();
    auto& mediaFilters = istream.GetMediaFilters();
    charbuff imageData;
    ContainerStreamDevice device(imageData);
    istream.CopyTo(device);

    charbuff smaskData;
    charbuff scanLine = initScanLine(format, rowSize, smaskData);

    if (mediaFilters.size() == 0)
    {
        switch (GetColorSpace())
        {
            case PdfColorSpace::DeviceRGB:
                utls::FetchImageRGB(stream, m_Width, m_Height, format, (const unsigned char*)imageData.data(), smaskData, scanLine);
                break;
            case PdfColorSpace::DeviceGray:
                utls::FetchImageGrayScale(stream, m_Width, m_Height, format, (const unsigned char*)imageData.data(), smaskData, scanLine);
                break;
            default:
                PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
        }
    }
    else
    {
        switch (mediaFilters[0])
        {
            case PdfFilterType::DCTDecode:
            {
#ifdef PODOFO_HAVE_JPEG_LIB
                jpeg_decompress_struct ctx;

                // Setup variables for JPEGLib
                ctx.out_color_space = format == PdfPixelFormat::Grayscale ? JCS_GRAYSCALE : JCS_RGB;
                JpegErrorHandler jerr;
                try
                {
                    InitJpegDecompressContext(ctx, jerr);

                    PoDoFo::jpeg_memory_src(&ctx, reinterpret_cast<JOCTET*>(imageData.data()), imageData.size());

                    if (jpeg_read_header(&ctx, TRUE) <= 0)
                        PODOFO_RAISE_ERROR(PdfErrorCode::UnexpectedEOF);

                    jpeg_start_decompress(&ctx);

                    unsigned rowBytes = (unsigned)(ctx.output_width * ctx.output_components);

                    // buffer will be deleted by jpeg_destroy_decompress
                    JSAMPARRAY jScanLine = (*ctx.mem->alloc_sarray)(reinterpret_cast<j_common_ptr>(&ctx), JPOOL_IMAGE, rowBytes, 1);
                    utls::FetchImageJPEG(stream, format, &ctx, jScanLine, smaskData, scanLine);
                }
                catch (...)
                {
                    jpeg_destroy_decompress(&ctx);
                    throw;
                }

                jpeg_destroy_decompress(&ctx);
#else
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Missing jpeg support");
#endif
                break;
            }
            case PdfFilterType::CCITTFaxDecode:
            {
                int k = 0;
                bool endOfLine = false;
                bool encodedByteAlign = false;
                bool blackIs1 = false;
                int columns = 1728;
                int rows = 0;
                auto decodeParms = istream.GetMediaDecodeParms()[0];
                if (decodeParms != nullptr)
                {
                    k = (int)decodeParms->FindKeyAs<int64_t>("K");
                    endOfLine = decodeParms->FindKeyAs<bool>("EndOfLine");
                    encodedByteAlign = decodeParms->FindKeyAs<bool>("EncodedByteAlign");
                    blackIs1 = decodeParms->FindKeyAs<bool>("BlackIs1");
                    columns = (int)decodeParms->FindKeyAs<int64_t>("Columns", 1728);
                    rows = (int)decodeParms->FindKeyAs<int64_t>("Rows");
                }
                auto decoder = fxcodec::FaxModule::CreateDecoder(
                    pdfium::span<const uint8_t>((const uint8_t *)imageData.data(), imageData.size()),
                    (int)m_Width, (int)m_Height, k, endOfLine, encodedByteAlign, blackIs1, columns, rows);

                utls::FetchImageBW(stream, m_Width, m_Height,
                    format, *decoder, smaskData, scanLine);
                break;
            }
            case PdfFilterType::JBIG2Decode:
            case PdfFilterType::JPXDecode:
            default:
                PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);
        }
    }
}

charbuff PdfImage::GetDecodedCopy(PdfPixelFormat format)
{
    charbuff buffer;
    DecodeTo(buffer, format);
    return buffer;
}

PdfImage::PdfImage(PdfObject& obj)
    : PdfXObject(obj, PdfXObjectType::Image)
{
    m_Width = static_cast<unsigned>(this->GetDictionary().MustFindKey("Width").GetNumber());
    m_Height = static_cast<unsigned>(this->GetDictionary().MustFindKey("Height").GetNumber());
}

charbuff PdfImage::initScanLine(PdfPixelFormat format, int rowSize, charbuff& smaskData) const
{
    unsigned defaultRowSize;
    switch (format)
    {
        case PdfPixelFormat::RGBA:
        case PdfPixelFormat::BGRA:
        {
            auto smaskObj = GetObject().GetDictionary().FindKey("SMask");
            if (smaskObj != nullptr)
            {
                unique_ptr<const PdfImage> smask;
                if (PdfXObject::TryCreateFromObject(*smaskObj, smask))
                    smask->GetObject().MustGetStream().CopyTo(smaskData);
            }

            defaultRowSize = 4 * m_Width;
            break;
        }
        case PdfPixelFormat::RGB24:
        case PdfPixelFormat::BGR24:
        {
            defaultRowSize = 4 * ((3 * m_Width + 3) / 4);
            break;
        }
        case PdfPixelFormat::Grayscale:
        {
            defaultRowSize = 4 * ((m_Width + 3) / 4);;
            break;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    if (rowSize < 0)
    {
        return charbuff(defaultRowSize);
    }
    else
    {
        if (rowSize < (int)defaultRowSize)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "The buffer stride is too small");

        return charbuff((size_t)rowSize);
    }
}

PdfColorSpace PdfImage::GetColorSpace() const
{
    auto colorSpace = GetDictionary().FindKey("ColorSpace");
    if (colorSpace == nullptr)
        return PdfColorSpace::Unknown;

    // CHECK-ME: Check if this is correct in the general case
    if (colorSpace->IsArray())
        return PdfColorSpace::Indexed;

    const PdfName* name;
    if (colorSpace->TryGetName(name))
        return PoDoFo::NameToColorSpaceRaw(name->GetString());

    return PdfColorSpace::Unknown;
}

void PdfImage::SetICCProfile(InputStream& stream, unsigned colorComponents, PdfColorSpace alternateColorSpace)
{
    // Check lColorComponents for a valid value
    if (colorComponents != 1 &&
        colorComponents != 3 &&
        colorComponents != 4)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "SetImageICCProfile lColorComponents must be 1,3 or 4!");
    }

    // Create a colorspace object
    auto& iccObject = this->GetDocument().GetObjects().CreateDictionaryObject();
    iccObject.GetDictionary().AddKey("Alternate", PdfName(PoDoFo::ColorSpaceToNameRaw(alternateColorSpace)));
    iccObject.GetDictionary().AddKey("N", static_cast<int64_t>(colorComponents));
    iccObject.GetOrCreateStream().SetData(stream);

    // Add the colorspace to our image
    PdfArray array;
    array.Add(PdfName("ICCBased"));
    array.Add(iccObject.GetIndirectReference());
    this->GetDictionary().AddKey("ColorSpace", array);
}

void PdfImage::SetSoftMask(const PdfImage& softmask)
{
    GetDictionary().AddKeyIndirect("SMask", softmask.GetObject());
}

void PdfImage::SetData(const bufferview& buffer, unsigned width, unsigned height, PdfPixelFormat format, int rowSize)
{
    SpanStreamDevice stream(buffer);
    SetData(stream, width, height, format, rowSize);
}

void PdfImage::SetData(InputStream& stream, unsigned width, unsigned height, PdfPixelFormat format, int rowSize)
{
    m_Width = width;
    m_Height = height;
    PdfColorSpace colorSpace;
    unsigned defaultRowSize;
    unsigned pdfRowSize;
    bool needFetch = false;
    switch (format)
    {
        case PdfPixelFormat::Grayscale:
            colorSpace = PdfColorSpace::DeviceGray;
            defaultRowSize = 4 * ((width + 3) / 4);
            pdfRowSize = width;
            break;
        case PdfPixelFormat::RGB24:
            colorSpace = PdfColorSpace::DeviceRGB;
            defaultRowSize = 4 * ((3 * width + 3) / 4);
            pdfRowSize = 3 * width;
            break;
        case PdfPixelFormat::BGR24:
            colorSpace = PdfColorSpace::DeviceRGB;
            defaultRowSize = 4 * ((3 * width + 3) / 4);
            pdfRowSize = 3 * width;
            needFetch = true;
            break;
        case PdfPixelFormat::RGBA:
        case PdfPixelFormat::BGRA:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Missing transparency support");
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    auto output = GetObject().GetOrCreateStream().GetOutputStream();
    charbuff lineBuffer(rowSize < 0 ? defaultRowSize : (unsigned)rowSize);
    if (needFetch)
    {
        charbuff pdfLineBuffer(pdfRowSize);
        for (unsigned i = 0; i < height; i++)
        {
            stream.Read(lineBuffer.data(), lineBuffer.size());
            fetchPDFScanLineRGB((unsigned char*)pdfLineBuffer.data(), width, (const unsigned char*)lineBuffer.data(), format);
            output.Write(pdfLineBuffer.data(), pdfRowSize);
        }
    }
    else
    {
        for (unsigned i = 0; i < height; i++)
        {
            stream.Read(lineBuffer.data(), lineBuffer.size());
            output.Write(lineBuffer.data(), pdfRowSize);
        }
    }

    auto& dict = GetDictionary();
    dict.AddKey("Width", static_cast<int64_t>(width));
    dict.AddKey("Height", static_cast<int64_t>(height));
    dict.AddKey("BitsPerComponent", static_cast<int64_t>(8));
    dict.AddKey("ColorSpace", PdfName(PoDoFo::ColorSpaceToNameRaw(colorSpace)));
    // Remove possibly existing /Decode array
    dict.RemoveKey("Decode");
}

void PdfImage::SetDataRaw(const bufferview& buffer, const PdfImageInfo& info)
{
    SpanStreamDevice stream(buffer);
    SetDataRaw(stream, info);
}

void PdfImage::SetDataRaw(InputStream& stream, const PdfImageInfo& info)
{
    m_Width = info.Width;
    m_Height = info.Height;

    auto& dict = GetDictionary();
    dict.AddKey("Width", static_cast<int64_t>(info.Width));
    dict.AddKey("Height", static_cast<int64_t>(info.Height));
    dict.AddKey("BitsPerComponent", static_cast<int64_t>(info.BitsPerComponent));
    if (info.Decode.GetSize() == 0)
        dict.RemoveKey("Decode");
    else
        dict.AddKey("Decode", info.Decode);

    if (info.ColorSpaceArray.GetSize() == 0)
    {
        dict.AddKey("ColorSpace", PdfName(PoDoFo::ColorSpaceToNameRaw(info.ColorSpace)));
    }
    else
    {
        PdfArray arr;
        arr.Add(PdfName(PoDoFo::ColorSpaceToNameRaw(info.ColorSpace)));
        arr.insert(arr.begin(), info.ColorSpaceArray.begin(), info.ColorSpaceArray.end());
        dict.AddKey("ColorSpace", info.ColorSpaceArray);
    }

    if (info.Filters.size() == 0)
        GetObject().GetOrCreateStream().SetData(stream, true);
    else
        GetObject().GetOrCreateStream().SetData(stream, info.Filters, true);
}

void PdfImage::Load(const string_view& filepath)
{
    if (filepath.length() > 3)
    {
        auto extension = fs::u8path(filepath).extension().u8string();
        extension = utls::ToLower(extension);

#ifdef PODOFO_HAVE_TIFF_LIB
        if (extension == ".tif" || extension == ".tiff")
        {
            loadFromTiff(filepath);
            return;
        }
#endif

#ifdef PODOFO_HAVE_JPEG_LIB
        if (extension == ".jpg" || extension == ".jpeg")
        {
            loadFromJpeg(filepath);
            return;
        }
#endif

#ifdef PODOFO_HAVE_PNG_LIB
        if (extension == ".png")
        {
            loadFromPng(filepath);
            return;
        }
#endif

    }
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, filepath);
}

void PdfImage::LoadFromBuffer(const bufferview& buffer)
{
    if (buffer.size() <= 4)
        return;

    unsigned char magic[4];
    memcpy(magic, buffer.data(), 4);

#ifdef PODOFO_HAVE_TIFF_LIB
    if ((magic[0] == 0x4D &&
        magic[1] == 0x4D &&
        magic[2] == 0x00 &&
        magic[3] == 0x2A) ||
        (magic[0] == 0x49 &&
            magic[1] == 0x49 &&
            magic[2] == 0x2A &&
            magic[3] == 0x00))
    {
        loadFromTiffData((const unsigned char*)buffer.data(), buffer.size());
        return;
    }
#endif

#ifdef PODOFO_HAVE_JPEG_LIB
    if (magic[0] == 0xFF &&
        magic[1] == 0xD8)
    {
        loadFromJpegData((const unsigned char*)buffer.data(), buffer.size());
        return;
    }
#endif

#ifdef PODOFO_HAVE_PNG_LIB
    if (magic[0] == 0x89 &&
        magic[1] == 0x50 &&
        magic[2] == 0x4E &&
        magic[3] == 0x47)
    {
        loadFromPngData((const unsigned char*)buffer.data(), buffer.size());
        return;
    }
#endif
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "Unknown magic number");
}

void PdfImage::ExportTo(charbuff& buff, PdfExportFormat format, PdfArray args) const
{
    buff.clear();
    switch (format)
    {
        case PdfExportFormat::Png:
            PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
        case PdfExportFormat::Jpeg:
#ifdef PODOFO_HAVE_JPEG_LIB
            exportToJpeg(buff, args);
#else
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Missing jpeg support");
#endif
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

#ifdef PODOFO_HAVE_JPEG_LIB

void PdfImage::loadFromJpeg(const string_view& filename)
{
    FILE* file = utls::fopen(filename, "rb");
    jpeg_decompress_struct ctx;
    JpegErrorHandler jerr;
    try
    {
        InitJpegDecompressContext(ctx, jerr);
        jpeg_stdio_src(&ctx, file);

        PdfImageInfo info;
        loadFromJpegInfo(ctx, info);

        FileStreamDevice input(filename);
        this->SetDataRaw(input, info);
    }
    catch (...)
    {
        jpeg_destroy_decompress(&ctx);
        fclose(file);
        throw;
    }

    jpeg_destroy_decompress(&ctx);
    fclose(file);
}

void PdfImage::exportToJpeg(charbuff& destBuff, const PdfArray& args) const
{
    int jquality = 85;
    double quality;
    if (args.GetSize() >= 1 && args[0].TryGetReal(quality))
    {
        // Assume first argument is jpeg quality in range [0, 1]
        jquality = (int)(std::clamp(quality, 0.0, 1.0) * 100);
    }

    charbuff inputBuff;
    DecodeTo(inputBuff, PdfPixelFormat::RGB24);

    jpeg_compress_struct ctx;
    JpegErrorHandler jerr;

    try
    {
        InitJpegCompressContext(ctx, jerr);

        JpegBufferDestination jdest;
        PoDoFo::SetJpegBufferDestination(ctx, destBuff, jdest);

        ctx.image_width = m_Width;
        ctx.image_height = m_Height;
        ctx.input_components = 3;
        ctx.in_color_space = JCS_RGB;

        jpeg_set_defaults(&ctx);

        jpeg_set_quality(&ctx, jquality, TRUE);
        jpeg_start_compress(&ctx, TRUE);

        unsigned rowsize = 4 * ((m_Width * 3 + 3) / 4);
        JSAMPROW row_pointer[1];
        for (unsigned i = 0; i < m_Height; i++)
        {
            row_pointer[0] = (unsigned char*)(inputBuff.data() + i * rowsize);
            (void)jpeg_write_scanlines(&ctx, row_pointer, 1);
        }

        jpeg_finish_compress(&ctx);
    }
    catch (...)
    {
        jpeg_destroy_compress(&ctx);
        throw;
    }

    jpeg_destroy_compress(&ctx);
}

void PdfImage::loadFromJpegData(const unsigned char* data, size_t len)
{
    jpeg_decompress_struct ctx;
    JpegErrorHandler jerr;

    try
    {
        InitJpegDecompressContext(ctx, jerr);
        jpeg_memory_src(&ctx, data, len);

        PdfImageInfo info;
        loadFromJpegInfo(ctx, info);

        SpanStreamDevice input((const char*)data, len);
        this->SetDataRaw(input, info);
    }
    catch (...)
    {
        jpeg_destroy_decompress(&ctx);
        throw;
    }
    jpeg_destroy_decompress(&ctx);
}

void PdfImage::loadFromJpegInfo(jpeg_decompress_struct& ctx, PdfImageInfo& info)
{
    if (jpeg_read_header(&ctx, TRUE) <= 0)
    {
        jpeg_destroy_decompress(&ctx);
        PODOFO_RAISE_ERROR(PdfErrorCode::UnexpectedEOF);
    }

    jpeg_start_decompress(&ctx);

    info.Width = ctx.output_width;
    info.Height = ctx.output_height;
    info.BitsPerComponent = 8;
    info.Filters.push_back(PdfFilterType::DCTDecode);

    // I am not sure whether this switch is fully correct.
    // it should handle all cases though.
    // Index jpeg files might look strange as jpeglib+
    // returns 1 for them.
    switch (ctx.output_components)
    {
        case 3:
        {
            info.ColorSpace = PdfColorSpace::DeviceRGB;
            break;
        }
        case 4:
        {
            info.ColorSpace = PdfColorSpace::DeviceCMYK;

            // The jpeg-doc ist not specific in this point, but cmyk's seem to be stored
            // in a inverted fashion. Fix by attaching a decode array
            PdfArray decode;
            decode.Add(1.0);
            decode.Add(0.0);
            decode.Add(1.0);
            decode.Add(0.0);
            decode.Add(1.0);
            decode.Add(0.0);
            decode.Add(1.0);
            decode.Add(0.0);

            info.Decode = decode;
            break;
        }
        default:
        {
            info.ColorSpace = PdfColorSpace::DeviceGray;
            break;
        }
    }
}

#endif // PODOFO_HAVE_JPEG_LIB

#ifdef PODOFO_HAVE_TIFF_LIB

static void TIFFErrorWarningHandler(const char*, const char*, va_list)
{
    // Do nothing
}

void PdfImage::loadFromTiffHandle(void* handle)
{
    TIFF* hInTiffHandle = (TIFF*)handle;

    int32_t row, width, height;
    uint16_t samplesPerPixel, bitsPerSample;
    uint16_t* sampleInfo;
    uint16_t extraSamples;
    uint16_t planarConfig, photoMetric, orientation;
    int32_t resolutionUnit;

    TIFFGetField(hInTiffHandle, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(hInTiffHandle, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_PLANARCONFIG, &planarConfig);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_PHOTOMETRIC, &photoMetric);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_EXTRASAMPLES, &extraSamples, &sampleInfo);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_ORIENTATION, &orientation);

    resolutionUnit = 0;
    float resX;
    float resY;
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_XRESOLUTION, &resX);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_YRESOLUTION, &resY);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_RESOLUTIONUNIT, &resolutionUnit);

    int colorChannels = samplesPerPixel - extraSamples;

    int bitsPixel = bitsPerSample * samplesPerPixel;

    // TODO: implement special cases
    if (TIFFIsTiled(hInTiffHandle))
    {
        TIFFClose(hInTiffHandle);
        PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
    }

    if (planarConfig != PLANARCONFIG_CONTIG && colorChannels != 1)
    {
        TIFFClose(hInTiffHandle);
        PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
    }

    if (orientation != ORIENTATION_TOPLEFT)
    {
        TIFFClose(hInTiffHandle);
        PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
    }

    PdfImageInfo info;
    info.Width = width;
    info.Height = height;
    info.BitsPerComponent = (unsigned char)bitsPerSample;
    switch (photoMetric)
    {
        case PHOTOMETRIC_MINISBLACK:
        {
            if (bitsPixel == 1)
            {
                PdfArray decode;
                decode.insert(decode.end(), PdfObject(static_cast<int64_t>(0)));
                decode.insert(decode.end(), PdfObject(static_cast<int64_t>(1)));
                info.Decode = std::move(decode);
            }
            else if (bitsPixel == 8 || bitsPixel == 16)
            {
                info.ColorSpace = PdfColorSpace::DeviceGray;
            }
            else
            {
                TIFFClose(hInTiffHandle);
                PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
            }
            break;
        }
        case PHOTOMETRIC_MINISWHITE:
        {
            if (bitsPixel == 1)
            {
                PdfArray decode;
                decode.insert(decode.end(), PdfObject(static_cast<int64_t>(1)));
                decode.insert(decode.end(), PdfObject(static_cast<int64_t>(0)));
                info.Decode = std::move(decode);
            }
            else if (bitsPixel == 8 || bitsPixel == 16)
            {
                info.ColorSpace = PdfColorSpace::DeviceGray;
            }
            else
            {
                TIFFClose(hInTiffHandle);
                PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
            }
            break;
        }
        case PHOTOMETRIC_RGB:
        {
            if (bitsPixel != 24)
            {
                TIFFClose(hInTiffHandle);
                PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
            }
            info.ColorSpace = PdfColorSpace::DeviceRGB;
            break;
        }
        case PHOTOMETRIC_SEPARATED:
        {
            if (bitsPixel != 32)
            {
                TIFFClose(hInTiffHandle);
                PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
            }
            info.ColorSpace = PdfColorSpace::DeviceCMYK;
            break;
        }
        case PHOTOMETRIC_PALETTE:
        {
            unsigned numColors = (1 << bitsPixel);

            PdfArray decode;
            decode.insert(decode.end(), PdfObject(static_cast<int64_t>(0)));
            decode.insert(decode.end(), PdfObject(static_cast<int64_t>(numColors) - 1));
            info.Decode = std::move(decode);

            uint16_t* rgbRed;
            uint16_t* rgbGreen;
            uint16_t* rgbBlue;
            TIFFGetField(hInTiffHandle, TIFFTAG_COLORMAP, &rgbRed, &rgbGreen, &rgbBlue);

            charbuff data(numColors * 3);

            for (unsigned clr = 0; clr < numColors; clr++)
            {
                data[3 * clr + 0] = rgbRed[clr] / 257;
                data[3 * clr + 1] = rgbGreen[clr] / 257;
                data[3 * clr + 2] = rgbBlue[clr] / 257;
            }

            // Create a colorspace object
            auto& idxObj = this->GetDocument().GetObjects().CreateDictionaryObject();
            idxObj.GetOrCreateStream().SetData(data);

            // Add the colorspace to our image
            PdfArray colorSpace;
            colorSpace.Add(PdfName("Indexed"));
            colorSpace.Add(PdfName("DeviceRGB"));
            colorSpace.Add(static_cast<int64_t>(numColors) - 1);
            colorSpace.Add(idxObj.GetIndirectReference());
            info.ColorSpaceArray = std::move(colorSpace);
            break;
        }

        default:
        {
            TIFFClose(hInTiffHandle);
            PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
            break;
        }
    }

    size_t scanlineSize = TIFFScanlineSize(hInTiffHandle);
    size_t bufferSize = scanlineSize * height;
    charbuff buffer(bufferSize);
    for (row = 0; row < height; row++)
    {
        if (TIFFReadScanline(hInTiffHandle,
            &buffer[row * scanlineSize],
            row) == (-1))
        {
            TIFFClose(hInTiffHandle);
            PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
        }
    }

    SpanStreamDevice input(buffer);
    SetDataRaw(input, info);
}

void PdfImage::loadFromTiff(const string_view& filename)
{
    TIFFSetErrorHandler(TIFFErrorWarningHandler);
    TIFFSetWarningHandler(TIFFErrorWarningHandler);

    if (filename.length() == 0)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

#ifdef _WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    TIFF* hInfile = TIFFOpenW((wchar_t*)filename16.c_str(), "rb");
#else
    TIFF* hInfile = TIFFOpen(filename.data(), "rb");
#endif

    if (hInfile == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::FileNotFound, filename);

    try
    {
        loadFromTiffHandle(hInfile);
    }
    catch (...)
    {
        TIFFClose(hInfile);
        throw;
    }

    TIFFClose(hInfile);
}

struct TiffData
{
    TiffData(const unsigned char* data, tsize_t size) :m_data(data), m_pos(0), m_size(size) {}

    tsize_t read(tdata_t data, tsize_t length)
    {
        tsize_t bytesRead = 0;
        if (length > m_size - static_cast<tsize_t>(m_pos))
        {
            memcpy(data, &m_data[m_pos], m_size - (tsize_t)m_pos);
            bytesRead = m_size - (tsize_t)m_pos;
            m_pos = m_size;
        }
        else
        {
            memcpy(data, &m_data[m_pos], length);
            bytesRead = length;
            m_pos += length;
        }
        return bytesRead;
    }

    toff_t size()
    {
        return m_size;
    }

    toff_t seek(toff_t pos, int whence)
    {
        if (pos == 0xFFFFFFFF) {
            return 0xFFFFFFFF;
        }
        switch (whence)
        {
            case SEEK_SET:
                if (static_cast<tsize_t>(pos) > m_size)
                {
                    m_pos = m_size;
                }
                else
                {
                    m_pos = pos;
                }
                break;
            case SEEK_CUR:
                if (static_cast<tsize_t>(pos + m_pos) > m_size)
                {
                    m_pos = m_size;
                }
                else
                {
                    m_pos += pos;
                }
                break;
            case SEEK_END:
                if (static_cast<tsize_t>(pos) > m_size)
                {
                    m_pos = 0;
                }
                else
                {
                    m_pos = m_size - pos;
                }
                break;
        }
        return m_pos;
    }

private:
    const unsigned char* m_data;
    toff_t m_pos;
    tsize_t m_size;
};
tsize_t tiff_Read(thandle_t st, tdata_t buffer, tsize_t size)
{
    TiffData* data = (TiffData*)st;
    return data->read(buffer, size);
};
tsize_t tiff_Write(thandle_t st, tdata_t buffer, tsize_t size)
{
    (void)st;
    (void)buffer;
    (void)size;
    return 0;
};
int tiff_Close(thandle_t)
{
    return 0;
};
toff_t tiff_Seek(thandle_t st, toff_t pos, int whence)
{
    TiffData* data = (TiffData*)st;
    return data->seek(pos, whence);
};
toff_t tiff_Size(thandle_t st)
{
    TiffData* data = (TiffData*)st;
    return data->size();
};
int tiff_Map(thandle_t, tdata_t*, toff_t*)
{
    return 0;
};
void tiff_Unmap(thandle_t, tdata_t, toff_t)
{
    return;
};
void PdfImage::loadFromTiffData(const unsigned char* data, size_t len)
{
    TIFFSetErrorHandler(TIFFErrorWarningHandler);
    TIFFSetWarningHandler(TIFFErrorWarningHandler);

    if (data == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    TiffData tiffData(data, (tsize_t)len);
    TIFF* hInHandle = TIFFClientOpen("Memory", "r", (thandle_t)&tiffData,
        tiff_Read, tiff_Write, tiff_Seek, tiff_Close, tiff_Size,
        tiff_Map, tiff_Unmap);
    if (hInHandle == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    loadFromTiffHandle(hInHandle);
}

#endif // PODOFO_HAVE_TIFF_LIB

#ifdef PODOFO_HAVE_PNG_LIB

void PdfImage::loadFromPng(const string_view& filename)
{
    FILE* file = utls::fopen(filename, "rb");

    try
    {
        loadFromPngHandle(file);
    }
    catch (...)
    {
        fclose(file);
        throw;
    }

    fclose(file);
}

void PdfImage::loadFromPngHandle(FILE* stream)
{
    png_byte header[8];
    if (fread(header, 1, 8, stream) != 8 ||
        png_sig_cmp(header, 0, 8))
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "The file could not be recognized as a PNG file");
    }

    png_structp png = nullptr;
    png_infop pnginfo = nullptr;
    try
    {
        createPngContext(png, pnginfo);
        png_init_io(png, stream);
        loadFromPngContent(*this, png, pnginfo);
    }
    catch (...)
    {
        png_destroy_read_struct(&png, &pnginfo, (png_infopp)nullptr);
        throw;
    }

    png_destroy_read_struct(&png, &pnginfo, (png_infopp)nullptr);
}

struct PngData
{
    PngData(const unsigned char* data, png_size_t size) :
        m_data(data), m_pos(0), m_size(size) {}

    void read(png_bytep data, png_size_t length)
    {
        if (length > m_size - m_pos)
        {
            memcpy(data, &m_data[m_pos], m_size - m_pos);
            m_pos = m_size;
        }
        else
        {
            memcpy(data, &m_data[m_pos], length);
            m_pos += length;
        }
    }

private:
    const unsigned char* m_data;
    png_size_t m_pos;
    png_size_t m_size;
};

void PdfImage::loadFromPngData(const unsigned char* data, size_t len)
{
    if (data == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    PngData pngData(data, len);
    png_byte header[8];
    pngData.read(header, 8);
    if (png_sig_cmp(header, 0, 8))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "The file could not be recognized as a PNG file");

    png_structp png;
    png_infop pnginfo;
    try
    {
        createPngContext(png, pnginfo);
        png_set_read_fn(png, (png_voidp)&pngData, pngReadData);
        loadFromPngContent(*this, png, pnginfo);
    }
    catch (...)
    {
        png_destroy_read_struct(&png, &pnginfo, (png_infopp)nullptr);
        throw;
    }

    png_destroy_read_struct(&png, &pnginfo, (png_infopp)nullptr);
}

void loadFromPngContent(PdfImage& image, png_structp png, png_infop pnginfo)
{
    png_set_sig_bytes(png, 8);
    png_read_info(png, pnginfo);

    // Begin
    png_uint_32 width;
    png_uint_32 height;
    int depth;
    int color_type;
    int interlace;

    png_get_IHDR(png, pnginfo,
        &width, &height, &depth,
        &color_type, &interlace, NULL, NULL);

    // convert palette/gray image to rgb
    // expand gray bit depth if needed
    if (color_type == PNG_COLOR_TYPE_GRAY)
    {
#if PNG_LIBPNG_VER >= 10209
        png_set_expand_gray_1_2_4_to_8(png);
#else
        png_set_gray_1_2_4_to_8(pPng);
#endif
    }
    else if (color_type != PNG_COLOR_TYPE_PALETTE && depth < 8)
    {
        png_set_packing(png);
    }

    // transform transparency to alpha
    if (color_type != PNG_COLOR_TYPE_PALETTE && png_get_valid(png, pnginfo, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (depth == 16)
        png_set_strip_16(png);

    if (interlace != PNG_INTERLACE_NONE)
        png_set_interlace_handling(png);

    //png_set_filler (pPng, 0xff, PNG_FILLER_AFTER);

    // recheck header after setting EXPAND options
    png_read_update_info(png, pnginfo);
    png_get_IHDR(png, pnginfo,
        &width, &height, &depth,
        &color_type, &interlace, NULL, NULL);
    // End

    // Read the file
    size_t rowLen = png_get_rowbytes(png, pnginfo);
    size_t len = rowLen * height;
    charbuff buffer(len);

    unique_ptr<png_bytep[]> rows(new png_bytep[height]);
    for (unsigned int y = 0; y < height; y++)
    {
        rows[y] = reinterpret_cast<png_bytep>(buffer.data() + y * rowLen);
    }

    png_read_image(png, rows.get());

    png_bytep paletteTrans = nullptr;
    int numTransColors = 0;
    if (color_type & PNG_COLOR_MASK_ALPHA
        || (color_type == PNG_COLOR_TYPE_PALETTE
            && png_get_valid(png, pnginfo, PNG_INFO_tRNS)
            && png_get_tRNS(png, pnginfo, &paletteTrans, &numTransColors, NULL)))
    {
        // Handle alpha channel and create smask
        charbuff smask(width * height);
        png_uint_32 smaskIndex = 0;
        if (color_type == PNG_COLOR_TYPE_PALETTE)
        {
            for (png_uint_32 r = 0; r < height; r++)
            {
                png_bytep row = rows[r];
                for (png_uint_32 c = 0; c < width; c++)
                {
                    png_byte color;
                    switch (depth)
                    {
                        case 8:
                            color = row[c];
                            break;
                        case 4:
                            color = c % 2 ? row[c / 2] >> 4 : row[c / 2] & 0xF;
                            break;
                        case 2:
                            color = (row[c / 4] >> c % 4 * 2) & 3;
                            break;
                        case 1:
                            color = (row[c / 4] >> c % 8) & 1;
                            break;
                        default:
                            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
                    }

                    smask[smaskIndex++] = color < numTransColors ? paletteTrans[color] : 0xFF;
                }
            }
        }
        else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        {
            for (png_uint_32 r = 0; r < height; r++)
            {
                png_bytep row = rows[r];
                for (png_uint_32 c = 0; c < width; c++)
                {
                    memmove(buffer.data() + 3 * smaskIndex, row + 4 * c, 3); // 3 byte for rgb
                    smask[smaskIndex++] = row[c * 4 + 3]; // 4th byte for alpha
                }
            }
            len = 3 * width * height;
        }
        else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        {
            for (png_uint_32 r = 0; r < height; r++)
            {
                png_bytep row = rows[r];
                for (png_uint_32 c = 0; c < width; c++)
                {
                    buffer[smaskIndex] = row[c * 2]; // 1 byte for gray
                    smask[smaskIndex++] = row[c * 2 + 1]; // 2nd byte for alpha
                }
            }
            len = width * height;
        }
        PdfImageInfo smaksInfo;
        smaksInfo.Width = (unsigned)width;
        smaksInfo.Height = (unsigned)height;
        smaksInfo.BitsPerComponent = (unsigned char)depth;
        smaksInfo.ColorSpace = PdfColorSpace::DeviceGray;

        auto smakeImage = image.GetDocument().CreateImage();
        smakeImage->SetDataRaw(smask, smaksInfo);
        image.SetSoftMask(*smakeImage);
    }

    PdfImageInfo info;
    info.Width = (unsigned)width;
    info.Height = (unsigned)height;
    info.BitsPerComponent = (unsigned char)depth;
    // Set color space
    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_color* colors;
        int colorCount;
        png_get_PLTE(png, pnginfo, &colors, &colorCount);

        charbuff data(colorCount * 3);
        for (int i = 0; i < colorCount; i++, colors++)
        {
            data[3 * i + 0] = colors->red;
            data[3 * i + 1] = colors->green;
            data[3 * i + 2] = colors->blue;
        }
        auto& idxObj = image.GetDocument().GetObjects().CreateDictionaryObject();
        idxObj.GetOrCreateStream().SetData(data);

        PdfArray array;
        array.Add(PdfName("DeviceRGB"));
        array.Add(static_cast<int64_t>(colorCount - 1));
        array.Add(idxObj.GetIndirectReference());
        info.ColorSpaceArray = std::move(array);
    }
    else if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
        info.ColorSpace = PdfColorSpace::DeviceGray;
    }
    else
    {
        info.ColorSpace = PdfColorSpace::DeviceRGB;
    }

    // Set the image data and flate compress it
    image.SetDataRaw(buffer, info);
}

void createPngContext(png_structp& png, png_infop& pnginfo)
{
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "png_create_read_struct");

    pnginfo = png_create_info_struct(png);
    if (pnginfo == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "png_create_info_struct");

    if (setjmp(png_jmpbuf(png)))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Error when reading the image");
}

void pngReadData(png_structp pngPtr, png_bytep data, png_size_t length)
{
    PngData* a = (PngData*)png_get_io_ptr(pngPtr);
    a->read(data, length);
}

#endif // PODOFO_HAVE_PNG_LIB

void PdfImage::SetChromaKeyMask(int64_t r, int64_t g, int64_t b, int64_t threshold)
{
    PdfArray array;
    array.Add(r - threshold);
    array.Add(r + threshold);
    array.Add(g - threshold);
    array.Add(g + threshold);
    array.Add(b - threshold);
    array.Add(b + threshold);

    this->GetDictionary().AddKey("Mask", array);
}

void PdfImage::SetInterpolate(bool value)
{
    this->GetDictionary().AddKey("Interpolate", value);
}

PdfRect PdfImage::GetRect() const
{
    return PdfRect(0, 0, m_Width, m_Height);
}

unsigned PdfImage::GetWidth() const
{
    return m_Width;
}

unsigned PdfImage::GetHeight() const
{
    return m_Height;
}

unsigned PdfImage::getBufferSize(PdfPixelFormat format) const
{
    switch (format)
    {
        case PdfPixelFormat::RGBA:
        case PdfPixelFormat::BGRA:
            return 4 * m_Width * m_Height;
        case PdfPixelFormat::RGB24:
        case PdfPixelFormat::BGR24:
            return 4 * ((3 * m_Width + 3) / 4) * m_Height;
        case PdfPixelFormat::Grayscale:
            return 4 * ((m_Width + 3) / 4) * m_Height;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

void fetchPDFScanLineRGB(unsigned char* dstScanLine, unsigned width, const unsigned char* srcScanLine, PdfPixelFormat srcPixelFormat)
{
    switch (srcPixelFormat)
    {
        case PdfPixelFormat::BGRA:
        case PdfPixelFormat::BGR24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 3 + 0] = srcScanLine[i * 3 + 2];
                dstScanLine[i * 3 + 1] = srcScanLine[i * 3 + 1];
                dstScanLine[i * 3 + 2] = srcScanLine[i * 3 + 0];
            }
            break;
        }
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "Unsupported pixel format");
    }
}
