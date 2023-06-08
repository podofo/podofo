/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_COLOR_SPACE_FILTER_H
#define PDF_COLOR_SPACE_FILTER_H

#include "PdfIndirectObjectList.h"

namespace PoDoFo {

    /** Output pixel format for a PdfColorSpaceFilter
     */
    enum class PdfColorSpacePixelFormat
    {
        Unknown = 0,
        Grayscale,
        RGB,
        CMYK,
        // TODO:
        // Custom    ///< Used for /DeviceN colorspaces
    };

    /** A class that implements methods to sample colors from a scanline buffer
     */
    class PODOFO_API PdfColorSpace
    {
        friend class PdfColorSpaceUnkown;
        friend class PdfColorSpaceDeviceGray;
        friend class PdfColorSpaceDeviceRGB;
        friend class PdfColorSpaceDeviceCMYK;
        friend class PdfColorSpaceIndexed;

    private:
        PdfColorSpace();
    public:
        virtual ~PdfColorSpace();
        virtual PdfColorSpaceType GetType() const = 0;
        /** True if the code space doesn't perform any non-trivial
         * encoding/filtering. In other words pixels can be sampled
         * by just copying scan lines
         */
        virtual bool IsRawEncoded() const = 0;
        /** Get the output pixel format of this color space
         */
        virtual PdfColorSpacePixelFormat GetPixelFormat() const = 0;
        /** Get the size of the scan line to sample from
         */
        virtual unsigned GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const = 0;
        /** Get the size of the scan line to sample to
         */
        virtual unsigned GetScanLineSize(unsigned width, unsigned bitsPerComponent) const = 0;
        /** Fetch the actual scanline of the exported format from/to the given buffers
         */
        virtual void FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine,
            unsigned width, unsigned bitsPerComponent) const = 0;
        /** Get an export object
         */
        virtual PdfObject GetExportObject(PdfIndirectObjectList& objectsj) const = 0;
    };

    /** Convenience alias for a constant PdfColorSpace shared ptr
     */
    using PdfColorSpacePtr = std::shared_ptr<const PdfColorSpace>;

    class PODOFO_API PdfColorSpaceUnkown final : public PdfColorSpace
    {
        friend class PdfColorSpaceFactory;
    private:
        PdfColorSpaceUnkown();
    public:
        PdfColorSpaceType GetType() const override;
        bool IsRawEncoded() const override;
        PdfColorSpacePixelFormat GetPixelFormat() const override;
        unsigned GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const override;
        unsigned GetScanLineSize(unsigned width, unsigned bitsPerComponent) const override;
        void FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine,
            unsigned width, unsigned bitsPerComponent) const override;
        PdfObject GetExportObject(PdfIndirectObjectList& objects) const override;
    };

    class PODOFO_API PdfColorSpaceDeviceGray final : public PdfColorSpace
    {
        friend class PdfColorSpaceFactory;
    private:
        PdfColorSpaceDeviceGray();
    public:
        PdfColorSpaceType GetType() const override;
        bool IsRawEncoded() const override;
        PdfColorSpacePixelFormat GetPixelFormat() const override;
        unsigned GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const override;
        unsigned GetScanLineSize(unsigned width, unsigned bitsPerComponent) const override;
        void FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine,
            unsigned width, unsigned bitsPerComponent) const override;
        PdfObject GetExportObject(PdfIndirectObjectList& objects) const override;
    };

    class PODOFO_API PdfColorSpaceDeviceRGB final : public PdfColorSpace
    {
        friend class PdfColorSpaceFactory;
    private:
        PdfColorSpaceDeviceRGB();
    public:
        PdfColorSpaceType GetType() const override;
        bool IsRawEncoded() const override;
        PdfColorSpacePixelFormat GetPixelFormat() const override;
        unsigned GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const override;
        unsigned GetScanLineSize(unsigned width, unsigned bitsPerComponent) const override;
        void FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine,
            unsigned width, unsigned bitsPerComponent) const override;
        PdfObject GetExportObject(PdfIndirectObjectList& objects) const override;
    };

    class PODOFO_API PdfColorSpaceDeviceCMYK final : public PdfColorSpace
    {
        friend class PdfColorSpaceFactory;
    private:
        PdfColorSpaceDeviceCMYK();
    public:
        PdfColorSpaceType GetType() const override;
        bool IsRawEncoded() const override;
        PdfColorSpacePixelFormat GetPixelFormat() const override;
        unsigned GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const override;
        unsigned GetScanLineSize(unsigned width, unsigned bitsPerComponent) const override;
        void FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine,
            unsigned width, unsigned bitsPerComponent) const override;
        PdfObject GetExportObject(PdfIndirectObjectList& objects) const override;
    };

    /** Color space as described by ISO 32000-2:2020 "8.6.6.3 Indexed colour spaces"
     */
    class PODOFO_API PdfColorSpaceIndexed final : public PdfColorSpace
    {
    public:
        PdfColorSpaceIndexed(const PdfColorSpacePtr& baseColorSpace, unsigned mapSize, charbuff&& lookup);
    public:
        PdfColorSpaceType GetType() const override;
        bool IsRawEncoded() const override;
        PdfColorSpacePixelFormat GetPixelFormat() const override;
        unsigned GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const override;
        unsigned GetScanLineSize(unsigned width, unsigned bitsPerComponent) const override;
        void FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine,
            unsigned width, unsigned bitsPerComponent) const override;
        PdfObject GetExportObject(PdfIndirectObjectList& objects) const override;
    private:
        PdfColorSpacePtr m_BaseColorSpace;
        unsigned m_MapSize;
        charbuff m_lookup;
    };

    class PODOFO_API PdfColorSpaceFactory final
    {
    public:
        static bool TryCreateFromObject(const PdfObject& obj, PdfColorSpacePtr& colorSpace);

        /** Singleton method which returns a global instance
         *  of Uknown color space
         */
        static PdfColorSpacePtr GetUnkownInstance();

        /** Singleton method which returns a global instance
         *  of /DeviceGray color space
         */
        static PdfColorSpacePtr GetDeviceGrayInstace();

        /** Singleton method which returns a global instance
         *  of /DeviceRGB color space
         */
        static PdfColorSpacePtr GetDeviceRGBInstace();

        /** Singleton method which returns a global instance
         *  of /DeviceCMYK color space
         */
        static PdfColorSpacePtr GetDeviceCMYKInstace();
    };
}

#endif // PDF_COLOR_SPACE_FILTER_H
