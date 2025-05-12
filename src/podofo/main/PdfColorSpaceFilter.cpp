/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfColorSpaceFilter.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfIndirectObjectList.h"
#include "PdfColorSpace.h"

using namespace std;
using namespace PoDoFo;

PdfColorSpaceFilter::PdfColorSpaceFilter() { }

PdfColorSpaceFilter::~PdfColorSpaceFilter() { }

bool PdfColorSpaceFilter::IsRawEncoded() const
{
    return false;
}

bool PdfColorSpaceFilter::IsTrivial() const
{
    return false;
}

PdfColorSpaceDeviceGray::PdfColorSpaceDeviceGray() { }

PdfColorSpaceType PdfColorSpaceDeviceGray::GetType() const
{
    return PdfColorSpaceType::DeviceGray;
}

bool PdfColorSpaceDeviceGray::IsRawEncoded() const
{
    return true;
}

bool PdfColorSpaceDeviceGray::IsTrivial() const
{
    return true;
}

PdfColorSpacePixelFormat PdfColorSpaceDeviceGray::GetPixelFormat() const
{
    return PdfColorSpacePixelFormat::Grayscale;
}

unsigned PdfColorSpaceDeviceGray::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (width * bitsPerComponent + 8 - 1) / 8;
}

unsigned PdfColorSpaceDeviceGray::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (width * bitsPerComponent + 8 - 1) / 8;
}

void PdfColorSpaceDeviceGray::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    std::memcpy(dstScanLine, srcScanLine, width * bitsPerComponent / 8);
}

PdfVariant PdfColorSpaceDeviceGray::GetExportObject(PdfIndirectObjectList& objects) const
{
    (void)objects;
    return "DeviceGray"_n;
}

unsigned char PdfColorSpaceDeviceGray::GetColorComponentCount() const
{
    return 1;
}

PdfColorSpaceFilterDeviceRGB::PdfColorSpaceFilterDeviceRGB() { }

PdfColorSpaceType PdfColorSpaceFilterDeviceRGB::GetType() const
{
    return PdfColorSpaceType::DeviceRGB;
}

bool PdfColorSpaceFilterDeviceRGB::IsRawEncoded() const
{
    return true;
}

bool PdfColorSpaceFilterDeviceRGB::IsTrivial() const
{
    return true;
}

PdfColorSpacePixelFormat PdfColorSpaceFilterDeviceRGB::GetPixelFormat() const
{
    return PdfColorSpacePixelFormat::RGB;
}

unsigned PdfColorSpaceFilterDeviceRGB::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (3 * width * bitsPerComponent + 8 - 1) / 8;
}

unsigned PdfColorSpaceFilterDeviceRGB::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (3 * width * bitsPerComponent + 8 - 1) / 8;
}

void PdfColorSpaceFilterDeviceRGB::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    std::memcpy(dstScanLine, srcScanLine, 3 * width * bitsPerComponent / 8);
}

PdfVariant PdfColorSpaceFilterDeviceRGB::GetExportObject(PdfIndirectObjectList& objects) const
{
    (void)objects;
    return "DeviceRGB"_n;
}

unsigned char PdfColorSpaceFilterDeviceRGB::GetColorComponentCount() const
{
    return 3;
}

PdfColorSpaceFilterDeviceCMYK::PdfColorSpaceFilterDeviceCMYK() { }

PdfColorSpaceType PdfColorSpaceFilterDeviceCMYK::GetType() const
{
    return PdfColorSpaceType::DeviceCMYK;
}

bool PdfColorSpaceFilterDeviceCMYK::IsRawEncoded() const
{
    return true;
}

bool PdfColorSpaceFilterDeviceCMYK::IsTrivial() const
{
    return true;
}

PdfColorSpacePixelFormat PdfColorSpaceFilterDeviceCMYK::GetPixelFormat() const
{
    return PdfColorSpacePixelFormat::CMYK;
}

unsigned PdfColorSpaceFilterDeviceCMYK::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (4 * width * bitsPerComponent + 8 - 1) / 8;
}

unsigned PdfColorSpaceFilterDeviceCMYK::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (4 * width * bitsPerComponent + 8 - 1) / 8;
}

void PdfColorSpaceFilterDeviceCMYK::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    std::memcpy(dstScanLine, srcScanLine, 4 * width * bitsPerComponent / 8);
}

PdfVariant PdfColorSpaceFilterDeviceCMYK::GetExportObject(PdfIndirectObjectList& objects) const
{
    (void)objects;
    return "DeviceCMYK"_n;
}

unsigned char PdfColorSpaceFilterDeviceCMYK::GetColorComponentCount() const
{
    return 4;
}

PdfColorSpaceFilterIndexed::PdfColorSpaceFilterIndexed(PdfColorSpaceInitializer&& baseColorSpace, unsigned mapSize, charbuff lookup)
    : m_MapSize(mapSize), m_lookup(std::move(lookup))
{
    m_BaseColorSpace = baseColorSpace.Take(m_colorSpaceExpVar);
    if (m_BaseColorSpace == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The base color space must be not null");
}

PdfColorSpaceFilterIndexed::PdfColorSpaceFilterIndexed(PdfColorSpaceFilterPtr&& baseColorSpace, unsigned mapSize, charbuff&& lookup)
    : m_BaseColorSpace(std::move(baseColorSpace)), m_MapSize(mapSize), m_lookup(std::move(lookup))
{
}

PdfColorSpaceType PdfColorSpaceFilterIndexed::GetType() const
{
    return PdfColorSpaceType::Indexed;
}

PdfColorSpacePixelFormat PdfColorSpaceFilterIndexed::GetPixelFormat() const
{
    return m_BaseColorSpace->GetPixelFormat();
}

unsigned PdfColorSpaceFilterIndexed::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    // bitsPerComponent Ignored in /Indexed source scan line size. The "lookup" table
    // always map to color components that are 8 bits size long
    (void)bitsPerComponent;
    return width;
}

unsigned PdfColorSpaceFilterIndexed::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    switch (m_BaseColorSpace->GetPixelFormat())
    {
        case PdfColorSpacePixelFormat::RGB:
            return (3 * width * bitsPerComponent + 8 - 1) / 8;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported base color space in /Indexed color space");
    }
}

void PdfColorSpaceFilterIndexed::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    switch (m_BaseColorSpace->GetType())
    {
        case PdfColorSpaceType::DeviceRGB:
        {
            if (bitsPerComponent == 8)
            {
                for (unsigned i = 0; i < width; i++)
                {
                    PODOFO_INVARIANT(srcScanLine[i] < m_MapSize);
                    const unsigned char* mappedColor = (const unsigned char*)(m_lookup.data() + srcScanLine[i] * 3);
                    *(dstScanLine + i * 3 + 0) = mappedColor[0];
                    *(dstScanLine + i * 3 + 1) = mappedColor[1];
                    *(dstScanLine + i * 3 + 2) = mappedColor[2];
                }
            }
            else
            {
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "/BitsPerComponent != 8");
            }
            break;
        }
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported base color space in /Indexed color space");
    }


}

PdfVariant PdfColorSpaceFilterIndexed::GetExportObject(PdfIndirectObjectList& objects) const
{
    auto& lookupObj = objects.CreateDictionaryObject();
    if (m_colorSpaceExpVar.IsNull())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Unsupported serializing null base color space");

    lookupObj.GetOrCreateStream().SetData(m_lookup);

    PdfArray arr;
    arr.Add("Indexed"_n);
    arr.Add(m_colorSpaceExpVar);
    arr.Add(static_cast<int64_t>(m_MapSize - 1));
    arr.Add(lookupObj.GetIndirectReference());
    return arr;
}

unsigned char PdfColorSpaceFilterIndexed::GetColorComponentCount() const
{
    return 1;
}

PdfColorSpaceFilterUnkown::PdfColorSpaceFilterUnkown() { }

PdfColorSpaceType PdfColorSpaceFilterUnkown::GetType() const
{
    return PdfColorSpaceType::Unknown;
}

PdfColorSpacePixelFormat PdfColorSpaceFilterUnkown::GetPixelFormat() const
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

unsigned PdfColorSpaceFilterUnkown::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

unsigned PdfColorSpaceFilterUnkown::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

void PdfColorSpaceFilterUnkown::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    (void)dstScanLine;
    (void)srcScanLine;
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

PdfVariant PdfColorSpaceFilterUnkown::GetExportObject(PdfIndirectObjectList& objects) const
{
    (void)objects;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

unsigned char PdfColorSpaceFilterUnkown::GetColorComponentCount() const
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

PdfColorSpaceFilterSeparation::PdfColorSpaceFilterSeparation(const string_view& name, const PdfColor& alternateColor)
    : m_Name(name), m_AlternateColor(alternateColor)
{
    switch (alternateColor.GetColorSpace())
    {
        case PdfColorSpaceType::DeviceGray:
        case PdfColorSpaceType::DeviceRGB:
        case PdfColorSpaceType::DeviceCMYK:
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::CannotConvertColor, "Unsupported color space for color space separation");
    }
}

unique_ptr<PdfColorSpaceFilterSeparation> PdfColorSpaceFilterSeparation::CreateSeparationNone()
{
    return unique_ptr<PdfColorSpaceFilterSeparation>(new PdfColorSpaceFilterSeparation("None", PdfColor(0, 0, 0, 0)));
}

unique_ptr<PdfColorSpaceFilterSeparation> PdfColorSpaceFilterSeparation::CreateSeparationAll()
{
    return unique_ptr<PdfColorSpaceFilterSeparation>(new PdfColorSpaceFilterSeparation("All", PdfColor(1,1,1,1)));
}

PdfColorSpaceType PdfColorSpaceFilterSeparation::GetType() const
{
    return PdfColorSpaceType::Separation;
}

bool PdfColorSpaceFilterSeparation::IsRawEncoded() const
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

PdfColorSpacePixelFormat PdfColorSpaceFilterSeparation::GetPixelFormat() const
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unsigned PdfColorSpaceFilterSeparation::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unsigned PdfColorSpaceFilterSeparation::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfColorSpaceFilterSeparation::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    (void)dstScanLine;
    (void)srcScanLine;
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

PdfVariant PdfColorSpaceFilterSeparation::GetExportObject(PdfIndirectObjectList& objects) const
{
    // Build color-spaces for separation
    auto& csTintFunc = objects.CreateDictionaryObject();
    
    csTintFunc.GetDictionary().AddKey("BitsPerSample"_n, static_cast<int64_t>(8));
    
    PdfArray decode;
    decode.Add(static_cast<int64_t>(0));
    decode.Add(static_cast<int64_t>(1));
    decode.Add(static_cast<int64_t>(0));
    decode.Add(static_cast<int64_t>(1));
    decode.Add(static_cast<int64_t>(0));
    decode.Add(static_cast<int64_t>(1));
    decode.Add(static_cast<int64_t>(0));
    decode.Add(static_cast<int64_t>(1));
    csTintFunc.GetDictionary().AddKey("Decode"_n, decode);
    
    PdfArray domain;
    domain.Add(static_cast<int64_t>(0));
    domain.Add(static_cast<int64_t>(1));
    csTintFunc.GetDictionary().AddKey("Domain"_n, domain);
    
    PdfArray encode;
    encode.Add(static_cast<int64_t>(0));
    encode.Add(static_cast<int64_t>(1));
    csTintFunc.GetDictionary().AddKey("Encode"_n, encode);
    
    csTintFunc.GetDictionary().AddKey("Filter"_n, "FlateDecode"_n);
    csTintFunc.GetDictionary().AddKey("FunctionType"_n, PdfVariant(static_cast<int64_t>(0)));
    //csTintFunc->GetDictionary().AddKey("FunctionType"_n,
    //                                    PdfVariant( static_cast<int64_t>(EPdfFunctionType::Sampled) ) );
    
    switch (m_AlternateColor.GetColorSpace())
    {
        case PdfColorSpaceType::DeviceGray:
        {
            char data[1 * 2];
            data[0] = 0;
            data[1] = static_cast<char> (m_AlternateColor.GetGrayScale());
    
            PdfArray range;
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            csTintFunc.GetDictionary().AddKey("Range"_n, range);
    
            PdfArray size;
            size.Add(static_cast<int64_t>(2));
            csTintFunc.GetDictionary().AddKey("Size"_n, size);
    
            csTintFunc.GetOrCreateStream().SetData(bufferview(data, 1 * 2));
    
            PdfArray csArr;
            csArr.Add("Separation"_n);
            csArr.Add(PdfName(m_Name));
            csArr.Add("DeviceGray"_n);
            csArr.Add(csTintFunc.GetIndirectReference());
            return csArr;
        }
        case PdfColorSpaceType::DeviceRGB:
        {
            char data[3 * 2];
            data[0] = 0;
            data[1] = 0;
            data[2] = 0;
            data[3] = static_cast<char> (m_AlternateColor.GetRed() * 255);
            data[4] = static_cast<char> (m_AlternateColor.GetGreen() * 255);
            data[5] = static_cast<char> (m_AlternateColor.GetBlue() * 255);
    
            PdfArray range;
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            csTintFunc.GetDictionary().AddKey("Range"_n, range);
    
            PdfArray size;
            size.Add(static_cast<int64_t>(2));
            csTintFunc.GetDictionary().AddKey("Size"_n, size);
    
            csTintFunc.GetOrCreateStream().SetData(bufferview(data, 3 * 2));
    
            PdfArray csArr;
            csArr.Add("Separation"_n);
            csArr.Add(PdfName(m_Name));
            csArr.Add("DeviceRGB"_n);
            csArr.Add(csTintFunc.GetIndirectReference());
            return csArr;
        }
        case PdfColorSpaceType::DeviceCMYK:
        {
            char data[4 * 2];
            data[0] = 0;
            data[1] = 0;
            data[2] = 0;
            data[3] = 0;
            data[4] = static_cast<char> (m_AlternateColor.GetCyan() * 255);
            data[5] = static_cast<char> (m_AlternateColor.GetMagenta() * 255);
            data[6] = static_cast<char> (m_AlternateColor.GetYellow() * 255);
            data[7] = static_cast<char> (m_AlternateColor.GetBlack() * 255);
    
            PdfArray range;
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            csTintFunc.GetDictionary().AddKey("Range"_n, range);
    
            PdfArray size;
            size.Add(static_cast<int64_t>(2));
            csTintFunc.GetDictionary().AddKey("Size"_n, size);
    
            PdfArray csArr;
            csArr.Add("Separation"_n);
            csArr.Add(PdfName(m_Name));
            csArr.Add("DeviceCMYK"_n);
            csArr.Add(csTintFunc.GetIndirectReference());
    
            csTintFunc.GetOrCreateStream().SetData(bufferview(data, 4 * 2)); // set stream as last, so that it will work with PdfStreamedDocument
            return csArr;
        }

        case PdfColorSpaceType::Unknown:
        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
        }
    }
}

unsigned char PdfColorSpaceFilterSeparation::GetColorComponentCount() const
{
    return 1;
}

const PdfColorRaw& PdfColorSpaceFilterSeparation::GetAlternateColor() const
{
    return m_AlternateColor.GetRawColor();
}

const PdfColorSpaceFilter& PdfColorSpaceFilterSeparation::GetColorSpace() const
{
    return *PdfColorSpaceFilterFactory::GetTrivialFilterPtr(m_AlternateColor.GetColorSpace());
}

PdfColorSpaceFilterLab::PdfColorSpaceFilterLab(const array<double, 3>& whitePoint,
        nullable<const array<double, 3>&> blackPoint,
        nullable<const array<double, 4>&> range) :
    m_WhitePoint(whitePoint),
    m_BlackPoint(blackPoint == nullptr ? array<double, 3>{ } : *blackPoint),
    m_Range(range == nullptr ? array<double, 4>{ -100, 100, -100, 100 } : *range)
{
}

PdfColorSpaceType PdfColorSpaceFilterLab::GetType() const
{
    return PdfColorSpaceType::Lab;
}

bool PdfColorSpaceFilterLab::IsRawEncoded() const
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

PdfColorSpacePixelFormat PdfColorSpaceFilterLab::GetPixelFormat() const
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unsigned PdfColorSpaceFilterLab::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unsigned PdfColorSpaceFilterLab::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfColorSpaceFilterLab::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    (void)dstScanLine;
    (void)srcScanLine;
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

PdfVariant PdfColorSpaceFilterLab::GetExportObject(PdfIndirectObjectList& objects) const
{
    auto& labDict = objects.CreateDictionaryObject().GetDictionary();
    PdfArray arr;
    arr.Add(m_WhitePoint[0]);
    arr.Add(m_WhitePoint[1]);
    arr.Add(m_WhitePoint[2]);
    labDict.AddKey("WhitePoint"_n, arr);

    if (m_BlackPoint != array<double, 3>{ })
    {
        arr.Clear();
        arr.Add(m_BlackPoint[0]);
        arr.Add(m_BlackPoint[1]);
        arr.Add(m_BlackPoint[2]);
        labDict.AddKey("BlackPoint"_n, arr);
    }

    if (m_Range != array<double, 4>{ -100, 100, -100, 100 })
    {
        arr.Clear();
        arr.Add(m_Range[0]);
        arr.Add(m_Range[1]);
        arr.Add(m_Range[2]);
        arr.Add(m_Range[3]);
        labDict.AddKey("Range"_n, arr);
    }

    PdfArray labArr;
    labArr.Add("Lab"_n);
    labArr.Add(labDict);
    return labArr;
}

unsigned char PdfColorSpaceFilterLab::GetColorComponentCount() const
{
    return 3;
}

PdfColorSpaceFilterICCBased::PdfColorSpaceFilterICCBased(PdfColorSpaceInitializer&& alternateColorSpace,
        charbuff iccprofile)
    : m_iccprofile(std::move(iccprofile))
{
    m_AlternateColorSpace = alternateColorSpace.Take(m_colorSpaceExpVar);
    if (m_AlternateColorSpace == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The alternate color space must be not null");
}

PdfColorSpaceFilterICCBased::PdfColorSpaceFilterICCBased(PdfColorSpaceFilterPtr&& alternateColorSpace, charbuff&& iccprofile)
    : m_AlternateColorSpace(std::move(alternateColorSpace)), m_iccprofile(std::move(iccprofile))
{
}

PdfColorSpaceType PdfColorSpaceFilterICCBased::GetType() const
{
    return PdfColorSpaceType::ICCBased;
}

PdfColorSpacePixelFormat PdfColorSpaceFilterICCBased::GetPixelFormat() const
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unsigned PdfColorSpaceFilterICCBased::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unsigned PdfColorSpaceFilterICCBased::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfColorSpaceFilterICCBased::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    (void)dstScanLine;
    (void)srcScanLine;
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

PdfVariant PdfColorSpaceFilterICCBased::GetExportObject(PdfIndirectObjectList& objects) const
{
    // Create a colorspace object
    auto& iccObject = objects.CreateDictionaryObject();
    if (m_colorSpaceExpVar.IsNull())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Unsupported serializing null alternate color space");

    iccObject.GetDictionary().AddKey("Alternate"_n, m_colorSpaceExpVar);
    iccObject.GetDictionary().AddKey("N"_n, static_cast<int64_t>(m_AlternateColorSpace->GetColorComponentCount()));
    iccObject.GetOrCreateStream().SetData(m_iccprofile);

    // Add the colorspace to our image
    PdfArray arr;
    arr.Add("ICCBased"_n);
    arr.Add(iccObject.GetIndirectReference());
    return arr;
}

unsigned char PdfColorSpaceFilterICCBased::GetColorComponentCount() const
{
    return m_AlternateColorSpace->GetColorComponentCount();
}

PdfColorSpaceFilterPattern::PdfColorSpaceFilterPattern(PdfColorSpaceInitializer&& underlyingColorSpace)
{
    m_UnderlyingColorSpace = underlyingColorSpace.Take(m_colorSpaceExpVar);
    if (m_UnderlyingColorSpace == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The underlying color space must be not null");
}

PdfColorSpaceFilterPattern::PdfColorSpaceFilterPattern(PdfColorSpaceFilterPtr&& alternateColorSpace)
    : m_UnderlyingColorSpace(alternateColorSpace)
{
    if (m_UnderlyingColorSpace == nullptr)
        m_UnderlyingColorSpace = PdfColorSpaceFilterFactory::GetUnkownInstancePtr();
}

PdfColorSpaceType PdfColorSpaceFilterPattern::GetType() const
{
    return PdfColorSpaceType::Pattern;
}

PdfColorSpacePixelFormat PdfColorSpaceFilterPattern::GetPixelFormat() const
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unsigned PdfColorSpaceFilterPattern::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unsigned PdfColorSpaceFilterPattern::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfColorSpaceFilterPattern::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    (void)dstScanLine;
    (void)srcScanLine;
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

PdfVariant PdfColorSpaceFilterPattern::GetExportObject(PdfIndirectObjectList& objects) const
{
    (void)objects;
    if (m_colorSpaceExpVar.IsNull())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported serializing with null color space export object");

    PdfArray arr;
    arr.Add("Pattern"_n);
    arr.Add(m_colorSpaceExpVar);
    return arr;
}

unsigned char PdfColorSpaceFilterPattern::GetColorComponentCount() const
{
    if (m_UnderlyingColorSpace == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid null underlying pattern color space at this stage");

    return m_UnderlyingColorSpace->GetColorComponentCount();
}

// TODO: pdfjs does some caching of the map based on object reference, we should do it as well
bool PdfColorSpaceFilterFactory::TryCreateFromObject(const PdfObject& obj, PdfColorSpaceFilterPtr& colorSpace)
{
    const PdfArray* arr;
    PdfColorSpaceType type;
    if (obj.TryGetArray(arr))
    {
        if (arr->GetSize() == 0)
        {
            PoDoFo::LogMessage(PdfLogSeverity::Warning, "Invalid color space");
            return false;
        }

        const PdfName* name;
        if (!arr->MustFindAt(0).TryGetName(name) || !PoDoFo::TryConvertTo(*name, type))
            return false;

        switch (type)
        {
            case PdfColorSpaceType::Indexed:
            {
                const PdfObjectStream* stream;
                charbuff lookup;
                int64_t maxIndex;
                PdfColorSpaceFilterPtr baseColorSpace;
                if (arr->GetSize() < 4)
                    goto InvalidIndexed; // Invalid array entry count

                if (!TryCreateFromObject(arr->MustFindAt(1), baseColorSpace))
                    goto InvalidIndexed;

                if (!arr->MustFindAt(2).TryGetNumber(maxIndex) && maxIndex < 1)
                    goto InvalidIndexed;

                stream = arr->MustFindAt(3).GetStream();
                if (stream == nullptr)
                    goto InvalidIndexed;

                lookup = stream->GetCopy();
                if (lookup.size() < baseColorSpace->GetColorComponentCount() * ((unsigned)maxIndex + 1))
                    goto InvalidIndexed;        // Table has invalid lookup map size

                colorSpace.reset(new PdfColorSpaceFilterIndexed(std::move(baseColorSpace), (unsigned)maxIndex + 1, std::move(lookup)));
                return true;

            InvalidIndexed:
                PoDoFo::LogMessage(PdfLogSeverity::Warning, "Invalid /Indexed color space name");
                return false;
            }
            default:
                PoDoFo::LogMessage(PdfLogSeverity::Warning, "Unsupported color space filter {}", name->GetString());
                return false;
        }
    }
    else
    {
        const PdfName* name;
        if (!obj.TryGetName(name) || !PoDoFo::TryConvertTo(name->GetString(), type))
            return false;

        switch (type)
        {
            case PdfColorSpaceType::DeviceGray:
            {
                colorSpace = GetDeviceGrayInstancePtr();
                return true;
            }
            case PdfColorSpaceType::DeviceRGB:
            {
                colorSpace = GetDeviceRGBInstancePtr();
                return true;
            }
            case PdfColorSpaceType::DeviceCMYK:
            {
                colorSpace = GetDeviceCMYKInstancePtr();
                return true;
            }
            default:
            {
                PoDoFo::LogMessage(PdfLogSeverity::Warning, "Unsupported color space filter {}", name->GetString());
                return false;
            }
        }
    }
}

PdfColorSpaceFilterPtr PdfColorSpaceFilterFactory::GetTrivialFilterPtr(PdfColorSpaceType type)
{
    switch (type)
    {
        case PdfColorSpaceType::DeviceRGB:
            return GetDeviceRGBInstancePtr();
        case PdfColorSpaceType::DeviceGray:
            return GetDeviceGrayInstancePtr();
        case PdfColorSpaceType::DeviceCMYK:
            return GetDeviceCMYKInstancePtr();
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::CannotConvertColor, "The given color space type is not trivial");
    }
}

PdfColorSpaceFilterPtr PdfColorSpaceFilterFactory::GetTrivialFilterPtr(PdfColorSpaceType type, PdfName& exportName)
{
    switch (type)
    {
        case PdfColorSpaceType::DeviceRGB:
            exportName = "DeviceRGB";
            return GetDeviceRGBInstancePtr();
        case PdfColorSpaceType::DeviceGray:
            exportName = "DeviceGray";
            return GetDeviceGrayInstancePtr();
        case PdfColorSpaceType::DeviceCMYK:
            exportName = "DeviceCMYK";
            return GetDeviceCMYKInstancePtr();
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::CannotConvertColor, "The given color space type is not trivial");
    }
}

PdfColorSpaceFilterPtr PdfColorSpaceFilterFactory::GetDeviceGrayInstancePtr()
{
    return getDeviceGrayInstancePtr();
}

const PdfColorSpaceFilter& PdfColorSpaceFilterFactory::GetDeviceGrayInstance()
{
    return *getDeviceGrayInstancePtr();
}

PdfColorSpaceFilterPtr PdfColorSpaceFilterFactory::GetDeviceRGBInstancePtr()
{
    return getDeviceRGBInstancePtr();
}

const PdfColorSpaceFilter& PdfColorSpaceFilterFactory::GetDeviceRGBInstance()
{
    return *getDeviceRGBInstancePtr();
}

PdfColorSpaceFilterPtr PdfColorSpaceFilterFactory::GetDeviceCMYKInstancePtr()
{
    return getDeviceCMYKInstancePtr();
}

const PdfColorSpaceFilter& PdfColorSpaceFilterFactory::GetDeviceCMYKInstance()
{
    return *getDeviceCMYKInstancePtr();
}

const PdfColorSpaceFilterPtr& PdfColorSpaceFilterFactory::GetUnkownInstancePtr()
{
    static PdfColorSpaceFilterPtr s_unknown(new PdfColorSpaceFilterUnkown());
    return s_unknown;
}

const PdfColorSpaceFilterPtr& PdfColorSpaceFilterFactory::GetParameterLessPatternInstancePtr()
{
    static PdfColorSpaceFilterPtr s_parameterLessPatternInstance(new PdfColorSpaceFilterPattern(nullptr));
    return s_parameterLessPatternInstance;
}

const PdfColorSpaceFilterPtr& PdfColorSpaceFilterFactory::getDeviceGrayInstancePtr()
{
    static PdfColorSpaceFilterPtr s_deviceGray(new PdfColorSpaceDeviceGray());
    return s_deviceGray;
}

const PdfColorSpaceFilterPtr& PdfColorSpaceFilterFactory::getDeviceRGBInstancePtr()
{
    static PdfColorSpaceFilterPtr s_deviceRGB(new PdfColorSpaceFilterDeviceRGB());
    return s_deviceRGB;
}

const PdfColorSpaceFilterPtr& PdfColorSpaceFilterFactory::getDeviceCMYKInstancePtr()
{
    static PdfColorSpaceFilterPtr s_deviceCMYK(new PdfColorSpaceFilterDeviceCMYK());
    return s_deviceCMYK;
}

PdfColorSpaceInitializer::PdfColorSpaceInitializer()
{
}

PdfColorSpaceInitializer::PdfColorSpaceInitializer(PdfColorSpaceFilterPtr&& filter)
    : m_Filter(std::move(filter))
{
    if (m_Filter == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The input filter must not be nullptr");

    switch (m_Filter->GetType())
    {
        case PdfColorSpaceType::DeviceRGB:
            m_ExpVar = "DeviceRGB"_n;
            break;
        case PdfColorSpaceType::DeviceGray:
            m_ExpVar = "DeviceGray"_n;
            break;
        case PdfColorSpaceType::DeviceCMYK:
            m_ExpVar = "DeviceCMYK"_n;
            break;
        default:
            // Do nothing
            break;
    }
}

PdfColorSpaceInitializer::PdfColorSpaceInitializer(const PdfColorSpace& colorSpace)
    : m_Filter(colorSpace.GetFilterPtr()), m_ExpVar(colorSpace.GetObject().GetIndirectReference())
{
}

PdfColorSpaceInitializer::PdfColorSpaceInitializer(PdfColorSpaceType colorSpace)
{
    switch (colorSpace)
    {
        case PdfColorSpaceType::DeviceRGB:
            m_Filter = PdfColorSpaceFilterFactory::GetDeviceRGBInstancePtr();
            m_ExpVar = "DeviceRGB"_n;
            break;
        case PdfColorSpaceType::DeviceGray:
            m_Filter = PdfColorSpaceFilterFactory::GetDeviceGrayInstancePtr();
            m_ExpVar = "DeviceGray"_n;
            break;
        case PdfColorSpaceType::DeviceCMYK:
            m_Filter = PdfColorSpaceFilterFactory::GetDeviceCMYKInstancePtr();
            m_ExpVar = "DeviceCMYK"_n;
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

PdfVariant PdfColorSpaceInitializer::GetExportObject(PdfIndirectObjectList& objects) const
{
    PODOFO_ASSERT(m_Filter != nullptr);
    if (m_ExpVar.IsNull())
        return m_Filter->GetExportObject(objects);
    else
        return m_ExpVar;
}

bool PdfColorSpaceInitializer::IsNull() const
{
    return m_Filter == nullptr;
}

const PdfColorSpaceFilter& PdfColorSpaceInitializer::GetFilter() const
{
    if (m_Filter == nullptr)
        return *PdfColorSpaceFilterFactory::GetUnkownInstancePtr();
    else
        return *m_Filter;
}

PdfColorSpaceFilterPtr PdfColorSpaceInitializer::Take(PdfVariant& expObj)
{
    expObj = std::move(m_ExpVar);
    return std::move(m_Filter);
}
