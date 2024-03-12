/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FILTERS_PRIVATE_H
#define PDF_FILTERS_PRIVATE_H

/**
 * \file PdfFiltersPrivate.h
 *
 * Provides implementations of various PDF stream filters.
 *
 * This is an internal header. It should not be included in podofo.h, and
 * should not be included directly by client applications. These filters should
 * only be accessed through the factory interface in PdfFilters.h .
 */

#include "PdfFilter.h"

#include <zlib.h>

namespace PoDoFo {

class PdfPredictorDecoder;
class OutputStreamDevice;

/** The ascii hex filter.
 */
class PdfHexFilter final : public PdfFilter
{
public:
    PdfHexFilter();

    inline bool CanEncode() const override { return true; }

    void EncodeBlockImpl(const char* buffer, size_t len) override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* buffer, size_t len) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::ASCIIHexDecode; }

private:
    char m_DecodedByte;
    bool m_Low;
};

/** The Ascii85 filter.
 */
class PdfAscii85Filter final : public PdfFilter
{
public:
    PdfAscii85Filter();

    inline bool CanEncode() const override { return true; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* buffer, size_t len) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* buffer, size_t len) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::ASCII85Decode; }

private:
    void EncodeTuple(unsigned tuple, int bytes);
    void WidePut(unsigned tuple, int bytes) const;

private:
    int m_count;
    unsigned m_tuple;
};

/** The Flate filter.
 */
class PdfFlateFilter final : public PdfFilter
{
    static constexpr unsigned BUFFER_SIZE = 4096;

public:
    PdfFlateFilter();

    inline bool CanEncode() const override { return true; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* buffer, size_t len) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary* decodeParms) override;

    void DecodeBlockImpl(const char* buffer, size_t len) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::FlateDecode; }

private:
    void EncodeBlockInternal(const char* buffer, size_t len, int nMode);

private:
    unsigned char m_buffer[BUFFER_SIZE];

    z_stream m_stream;
    std::shared_ptr<PdfPredictorDecoder> m_Predictor;
};

/** The RLE filter.
 */
class PdfRLEFilter final : public PdfFilter
{
public:
    PdfRLEFilter();

    inline bool CanEncode() const override { return false; }

    void EncodeBlockImpl(const char* buffer, size_t len) override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* buffer, size_t len) override;

    inline PdfFilterType GetType() const override { return PdfFilterType::RunLengthDecode; }

private:
    int m_CodeLen;
};

/** The LZW filter.
 */
class PdfLZWFilter final : public PdfFilter
{
    struct TLzwItem
    {
        std::vector<unsigned char> value;
    };

    using TLzwTable = std::vector<TLzwItem>;
    using TILzwTable = TLzwTable::iterator;
    using TCILzwTable = TLzwTable::const_iterator;

public:
    PdfLZWFilter();

    inline bool CanEncode() const override { return false; }

    void EncodeBlockImpl(const char* buffer, size_t len) override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* buffer, size_t len) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::LZWDecode; }

private:
    void InitTable();

private:
    static const unsigned short s_masks[4];
    static const unsigned short s_clear;
    static const unsigned short s_eod;

    TLzwTable m_table;

    unsigned m_mask;
    unsigned m_code_len;
    unsigned char m_character;

    bool m_First;

    std::shared_ptr<PdfPredictorDecoder> m_Predictor;
};

/** The crypt filter.
 */
class PdfCryptFilter final : public PdfFilter
{
public:
    PdfCryptFilter();

    inline bool CanEncode() const override { return false; }

    void EncodeBlockImpl(const char* buffer, size_t len) override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* buffer, size_t len) override;

    inline PdfFilterType GetType() const override { return PdfFilterType::Crypt; }
};

}

#endif // PDF_FILTERS_PRIVATE_H
