/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfColorSpace.h"
#include "PdfArray.h"

using namespace std;
using namespace PoDoFo;

PdfColorSpace::PdfColorSpace() { }

PdfColorSpace::~PdfColorSpace() { }

// TODO: pdfjs does some caching of the map based on object reference, we should do it as well
bool PdfColorSpaceFactory::TryCreateFromObject(const PdfObject& obj, PdfColorSpacePtr& colorSpace)
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
        if (!arr->MustFindAt(0).TryGetName(name) || !PoDoFo::TryNameToColorSpaceRaw(*name, type))
            return false;

        switch (type)
        {
            case PdfColorSpaceType::Indexed:
            {
                const PdfObjectStream* stream;
                charbuff lookup;
                int64_t maxIndex;
                PdfColorSpacePtr baseColorSpace;
                unsigned short componentCount;
                if (arr->GetSize() < 4)
                    goto InvalidIndexed; // Invalid array entry count

                if (!TryCreateFromObject(arr->MustFindAt(1), baseColorSpace))
                    goto InvalidIndexed;

                if (!arr->MustFindAt(2).TryGetNumber(maxIndex) && maxIndex < 1)
                    goto InvalidIndexed;

                stream = arr->MustFindAt(3).GetStream();
                if (stream == nullptr)
                    goto InvalidIndexed;

                switch (baseColorSpace->GetPixelFormat())
                {
                    case PdfColorSpacePixelFormat::RGB:
                        componentCount = 3;
                        break;
                    default:
                        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported base color space in /Indexed color space");
                }

                lookup = stream->GetCopy();
                if (lookup.size() < componentCount * ((unsigned)maxIndex + 1))
                    goto InvalidIndexed;        // Table has invalid lookup map size

                colorSpace.reset(new PdfColorSpaceIndexed(baseColorSpace, (unsigned)maxIndex + 1, std::move(lookup)));
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
        if (!obj.TryGetName(name) || !PoDoFo::TryNameToColorSpaceRaw(name->GetString(), type))
            return false;

        switch (type)
        {
            case PdfColorSpaceType::DeviceGray:
            {
                colorSpace = GetDeviceGrayInstace();
                return true;
            }
            case PdfColorSpaceType::DeviceRGB:
            {
                colorSpace = GetDeviceRGBInstace();
                return true;
            }
            case PdfColorSpaceType::DeviceCMYK:
            {
                colorSpace = GetDeviceCMYKInstace();
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

PdfColorSpacePtr PdfColorSpaceFactory::GetUnkownInstance()
{
    static shared_ptr<PdfColorSpaceUnkown> s_unknown(new PdfColorSpaceUnkown());
    return s_unknown;
}

PdfColorSpacePtr PdfColorSpaceFactory::GetDeviceGrayInstace()
{
    static shared_ptr<PdfColorSpaceDeviceGray> s_deviceGray(new PdfColorSpaceDeviceGray());
    return s_deviceGray;
}

PdfColorSpacePtr PdfColorSpaceFactory::GetDeviceRGBInstace()
{
    static shared_ptr<PdfColorSpaceDeviceRGB> s_deviceRGB(new PdfColorSpaceDeviceRGB());
    return s_deviceRGB;
}

PdfColorSpacePtr PdfColorSpaceFactory::GetDeviceCMYKInstace()
{
    static shared_ptr<PdfColorSpaceDeviceCMYK> s_deviceCMYK(new PdfColorSpaceDeviceCMYK());
    return s_deviceCMYK;
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

PdfObject PdfColorSpaceDeviceGray::GetExportObject(PdfIndirectObjectList& objects) const
{
    (void)objects;
    return PdfName("DeviceGray");
}

PdfColorSpaceDeviceRGB::PdfColorSpaceDeviceRGB() { }

PdfColorSpaceType PdfColorSpaceDeviceRGB::GetType() const
{
    return PdfColorSpaceType::DeviceRGB;
}

bool PdfColorSpaceDeviceRGB::IsRawEncoded() const
{
    return true;
}

PdfColorSpacePixelFormat PdfColorSpaceDeviceRGB::GetPixelFormat() const
{
    return PdfColorSpacePixelFormat::RGB;
}

unsigned PdfColorSpaceDeviceRGB::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (3 * width * bitsPerComponent + 8 - 1) / 8;
}

unsigned PdfColorSpaceDeviceRGB::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (3 * width * bitsPerComponent + 8 - 1) / 8;
}

void PdfColorSpaceDeviceRGB::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    std::memcpy(dstScanLine, srcScanLine, 3 * width * bitsPerComponent / 8);
}

PdfObject PdfColorSpaceDeviceRGB::GetExportObject(PdfIndirectObjectList& objects) const
{
    (void)objects;
    return PdfName("DeviceRGB");
}

PdfColorSpaceDeviceCMYK::PdfColorSpaceDeviceCMYK() { }

PdfColorSpaceType PdfColorSpaceDeviceCMYK::GetType() const
{
    return PdfColorSpaceType::DeviceCMYK;
}

bool PdfColorSpaceDeviceCMYK::IsRawEncoded() const
{
    return true;
}

PdfColorSpacePixelFormat PdfColorSpaceDeviceCMYK::GetPixelFormat() const
{
    return PdfColorSpacePixelFormat::CMYK;
}

unsigned PdfColorSpaceDeviceCMYK::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (4 * width * bitsPerComponent + 8 - 1) / 8;
}

unsigned PdfColorSpaceDeviceCMYK::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (4 * width * bitsPerComponent + 8 - 1) / 8;
}

void PdfColorSpaceDeviceCMYK::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    std::memcpy(dstScanLine, srcScanLine, 4 * width * bitsPerComponent / 8);
}

PdfObject PdfColorSpaceDeviceCMYK::GetExportObject(PdfIndirectObjectList& objects) const
{
    (void)objects;
    return PdfName("DeviceCMYK");
}

PdfColorSpaceIndexed::PdfColorSpaceIndexed(const PdfColorSpacePtr& baseColorSpace, unsigned mapSize, charbuff&& lookup)
    : m_BaseColorSpace(baseColorSpace), m_MapSize(mapSize), m_lookup(std::move(lookup))
{
}

PdfColorSpaceType PdfColorSpaceIndexed::GetType() const
{
    return PdfColorSpaceType::Indexed;
}

bool PdfColorSpaceIndexed::IsRawEncoded() const
{
    return false;
}

PdfColorSpacePixelFormat PdfColorSpaceIndexed::GetPixelFormat() const
{
    return m_BaseColorSpace->GetPixelFormat();
}

unsigned PdfColorSpaceIndexed::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    // bitsPerComponent Ignored in /Indexed source scan line size. The "lookup" table
    // always map to color components that are 8 bits size long
    (void)bitsPerComponent;
    return width;
}

unsigned PdfColorSpaceIndexed::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    switch (m_BaseColorSpace->GetPixelFormat())
    {
        case PdfColorSpacePixelFormat::RGB:
            return (3 * width * bitsPerComponent + 8 - 1) / 8;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported base color space in /Indexed color space");
    }
}

void PdfColorSpaceIndexed::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
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

PdfObject PdfColorSpaceIndexed::GetExportObject(PdfIndirectObjectList& objects) const
{
    auto& lookupObj = objects.CreateDictionaryObject();
    lookupObj.GetOrCreateStream().SetData(m_lookup);

    PdfArray arr;
    arr.Add(PdfName("Indexed"));
    arr.Add(m_BaseColorSpace->GetExportObject(objects));
    arr.Add(static_cast<int64_t>(m_MapSize - 1));
    arr.Add(lookupObj.GetIndirectReference());
    return arr;
}

PdfColorSpaceUnkown::PdfColorSpaceUnkown() { }

PdfColorSpaceType PdfColorSpaceUnkown::GetType() const
{
    return PdfColorSpaceType::Unknown;
}

bool PdfColorSpaceUnkown::IsRawEncoded() const
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

PdfColorSpacePixelFormat PdfColorSpaceUnkown::GetPixelFormat() const
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

unsigned PdfColorSpaceUnkown::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

unsigned PdfColorSpaceUnkown::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

void PdfColorSpaceUnkown::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    (void)dstScanLine;
    (void)srcScanLine;
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

PdfObject PdfColorSpaceUnkown::GetExportObject(PdfIndirectObjectList& objects) const
{
    (void)objects;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}
