// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "faxmodule.h"

#include <stdint.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>

#include <pdfium/pdfium_shim.h>
#include <pdfium/core/fxcodec/scanlinedecoder.h>

namespace fxcodec {

namespace {

const uint8_t OneLeadPos[256] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// Limit of image dimension. Use the same limit as the JBIG2 codecs.
constexpr int kFaxMaxImageDimension = 65535;

constexpr int kFaxBpc = 1;
constexpr int kFaxComps = 1;

int FindBit(pdfium::span<const uint8_t> data_buf,
            int max_pos,
            int start_pos,
            bool bit) {
  DCHECK(start_pos >= 0);
  if (start_pos >= max_pos)
    return max_pos;

  const uint8_t bit_xor = bit ? 0x00 : 0xff;
  int bit_offset = start_pos % 8;
  if (bit_offset) {
    const int byte_pos = start_pos / 8;
    uint8_t data = (data_buf[byte_pos] ^ bit_xor) & (0xff >> bit_offset);
    if (data)
      return byte_pos * 8 + OneLeadPos[data];

    start_pos += 7;
  }

  const int max_byte = (max_pos + 7) / 8;
  int byte_pos = start_pos / 8;

  // Try reading in bigger chunks in case there are long runs to be skipped.
  static constexpr int kBulkReadSize = 8;
  if (max_byte >= kBulkReadSize && byte_pos < max_byte - kBulkReadSize) {
    static constexpr uint8_t skip_block_0[kBulkReadSize] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static constexpr uint8_t skip_block_1[kBulkReadSize] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    const uint8_t* skip_block = bit ? skip_block_0 : skip_block_1;
    while (byte_pos < max_byte - kBulkReadSize &&
           memcmp(data_buf.subspan(byte_pos).data(), skip_block,
                  kBulkReadSize) == 0) {
      byte_pos += kBulkReadSize;
    }
  }

  while (byte_pos < max_byte) {
    uint8_t data = data_buf[byte_pos] ^ bit_xor;
    if (data)
      return std::min(byte_pos * 8 + OneLeadPos[data], max_pos);

    ++byte_pos;
  }
  return max_pos;
}

void FaxG4FindB1B2(pdfium::span<const uint8_t> ref_buf,
                   int columns,
                   int a0,
                   bool a0color,
                   int* b1,
                   int* b2) {
  bool first_bit = a0 < 0 || (ref_buf[a0 / 8] & (1 << (7 - a0 % 8))) != 0;
  *b1 = FindBit(ref_buf, columns, a0 + 1, !first_bit);
  if (*b1 >= columns) {
    *b1 = *b2 = columns;
    return;
  }
  if (first_bit == !a0color) {
    *b1 = FindBit(ref_buf, columns, *b1 + 1, first_bit);
    first_bit = !first_bit;
  }
  if (*b1 >= columns) {
    *b1 = *b2 = columns;
    return;
  }
  *b2 = FindBit(ref_buf, columns, *b1 + 1, first_bit);
}

void FaxFillBits(uint8_t* dest_buf, int columns, int startpos, int endpos) {
  startpos = std::max(startpos, 0);
  endpos = pdfium::clamp(endpos, 0, columns);
  if (startpos >= endpos)
    return;

  int first_byte = startpos / 8;
  int last_byte = (endpos - 1) / 8;
  if (first_byte == last_byte) {
    for (int i = startpos % 8; i <= (endpos - 1) % 8; ++i)
      dest_buf[first_byte] -= 1 << (7 - i);
    return;
  }

  for (int i = startpos % 8; i < 8; ++i)
    dest_buf[first_byte] -= 1 << (7 - i);
  for (int i = 0; i <= (endpos - 1) % 8; ++i)
    dest_buf[last_byte] -= 1 << (7 - i);

  if (last_byte > first_byte + 1)
    memset(dest_buf + first_byte + 1, 0, last_byte - first_byte - 1);
}

inline bool NextBit(const uint8_t* src_buf, int* bitpos) {
  int pos = (*bitpos)++;
  return !!(src_buf[pos / 8] & (1 << (7 - pos % 8)));
}

const uint8_t kFaxBlackRunIns[] = {
    0,          2,          0x02,       3,          0,          0x03,
    2,          0,          2,          0x02,       1,          0,
    0x03,       4,          0,          2,          0x02,       6,
    0,          0x03,       5,          0,          1,          0x03,
    7,          0,          2,          0x04,       9,          0,
    0x05,       8,          0,          3,          0x04,       10,
    0,          0x05,       11,         0,          0x07,       12,
    0,          2,          0x04,       13,         0,          0x07,
    14,         0,          1,          0x18,       15,         0,
    5,          0x08,       18,         0,          0x0f,       64,
    0,          0x17,       16,         0,          0x18,       17,
    0,          0x37,       0,          0,          10,         0x08,
    0x00,       0x07,       0x0c,       0x40,       0x07,       0x0d,
    0x80,       0x07,       0x17,       24,         0,          0x18,
    25,         0,          0x28,       23,         0,          0x37,
    22,         0,          0x67,       19,         0,          0x68,
    20,         0,          0x6c,       21,         0,          54,
    0x12,       1984 % 256, 1984 / 256, 0x13,       2048 % 256, 2048 / 256,
    0x14,       2112 % 256, 2112 / 256, 0x15,       2176 % 256, 2176 / 256,
    0x16,       2240 % 256, 2240 / 256, 0x17,       2304 % 256, 2304 / 256,
    0x1c,       2368 % 256, 2368 / 256, 0x1d,       2432 % 256, 2432 / 256,
    0x1e,       2496 % 256, 2496 / 256, 0x1f,       2560 % 256, 2560 / 256,
    0x24,       52,         0,          0x27,       55,         0,
    0x28,       56,         0,          0x2b,       59,         0,
    0x2c,       60,         0,          0x33,       320 % 256,  320 / 256,
    0x34,       384 % 256,  384 / 256,  0x35,       448 % 256,  448 / 256,
    0x37,       53,         0,          0x38,       54,         0,
    0x52,       50,         0,          0x53,       51,         0,
    0x54,       44,         0,          0x55,       45,         0,
    0x56,       46,         0,          0x57,       47,         0,
    0x58,       57,         0,          0x59,       58,         0,
    0x5a,       61,         0,          0x5b,       256 % 256,  256 / 256,
    0x64,       48,         0,          0x65,       49,         0,
    0x66,       62,         0,          0x67,       63,         0,
    0x68,       30,         0,          0x69,       31,         0,
    0x6a,       32,         0,          0x6b,       33,         0,
    0x6c,       40,         0,          0x6d,       41,         0,
    0xc8,       128,        0,          0xc9,       192,        0,
    0xca,       26,         0,          0xcb,       27,         0,
    0xcc,       28,         0,          0xcd,       29,         0,
    0xd2,       34,         0,          0xd3,       35,         0,
    0xd4,       36,         0,          0xd5,       37,         0,
    0xd6,       38,         0,          0xd7,       39,         0,
    0xda,       42,         0,          0xdb,       43,         0,
    20,         0x4a,       640 % 256,  640 / 256,  0x4b,       704 % 256,
    704 / 256,  0x4c,       768 % 256,  768 / 256,  0x4d,       832 % 256,
    832 / 256,  0x52,       1280 % 256, 1280 / 256, 0x53,       1344 % 256,
    1344 / 256, 0x54,       1408 % 256, 1408 / 256, 0x55,       1472 % 256,
    1472 / 256, 0x5a,       1536 % 256, 1536 / 256, 0x5b,       1600 % 256,
    1600 / 256, 0x64,       1664 % 256, 1664 / 256, 0x65,       1728 % 256,
    1728 / 256, 0x6c,       512 % 256,  512 / 256,  0x6d,       576 % 256,
    576 / 256,  0x72,       896 % 256,  896 / 256,  0x73,       960 % 256,
    960 / 256,  0x74,       1024 % 256, 1024 / 256, 0x75,       1088 % 256,
    1088 / 256, 0x76,       1152 % 256, 1152 / 256, 0x77,       1216 % 256,
    1216 / 256, 0xff};

const uint8_t kFaxWhiteRunIns[] = {
    0,          0,          0,          6,          0x07,       2,
    0,          0x08,       3,          0,          0x0B,       4,
    0,          0x0C,       5,          0,          0x0E,       6,
    0,          0x0F,       7,          0,          6,          0x07,
    10,         0,          0x08,       11,         0,          0x12,
    128,        0,          0x13,       8,          0,          0x14,
    9,          0,          0x1b,       64,         0,          9,
    0x03,       13,         0,          0x07,       1,          0,
    0x08,       12,         0,          0x17,       192,        0,
    0x18,       1664 % 256, 1664 / 256, 0x2a,       16,         0,
    0x2B,       17,         0,          0x34,       14,         0,
    0x35,       15,         0,          12,         0x03,       22,
    0,          0x04,       23,         0,          0x08,       20,
    0,          0x0c,       19,         0,          0x13,       26,
    0,          0x17,       21,         0,          0x18,       28,
    0,          0x24,       27,         0,          0x27,       18,
    0,          0x28,       24,         0,          0x2B,       25,
    0,          0x37,       256 % 256,  256 / 256,  42,         0x02,
    29,         0,          0x03,       30,         0,          0x04,
    45,         0,          0x05,       46,         0,          0x0a,
    47,         0,          0x0b,       48,         0,          0x12,
    33,         0,          0x13,       34,         0,          0x14,
    35,         0,          0x15,       36,         0,          0x16,
    37,         0,          0x17,       38,         0,          0x1a,
    31,         0,          0x1b,       32,         0,          0x24,
    53,         0,          0x25,       54,         0,          0x28,
    39,         0,          0x29,       40,         0,          0x2a,
    41,         0,          0x2b,       42,         0,          0x2c,
    43,         0,          0x2d,       44,         0,          0x32,
    61,         0,          0x33,       62,         0,          0x34,
    63,         0,          0x35,       0,          0,          0x36,
    320 % 256,  320 / 256,  0x37,       384 % 256,  384 / 256,  0x4a,
    59,         0,          0x4b,       60,         0,          0x52,
    49,         0,          0x53,       50,         0,          0x54,
    51,         0,          0x55,       52,         0,          0x58,
    55,         0,          0x59,       56,         0,          0x5a,
    57,         0,          0x5b,       58,         0,          0x64,
    448 % 256,  448 / 256,  0x65,       512 % 256,  512 / 256,  0x67,
    640 % 256,  640 / 256,  0x68,       576 % 256,  576 / 256,  16,
    0x98,       1472 % 256, 1472 / 256, 0x99,       1536 % 256, 1536 / 256,
    0x9a,       1600 % 256, 1600 / 256, 0x9b,       1728 % 256, 1728 / 256,
    0xcc,       704 % 256,  704 / 256,  0xcd,       768 % 256,  768 / 256,
    0xd2,       832 % 256,  832 / 256,  0xd3,       896 % 256,  896 / 256,
    0xd4,       960 % 256,  960 / 256,  0xd5,       1024 % 256, 1024 / 256,
    0xd6,       1088 % 256, 1088 / 256, 0xd7,       1152 % 256, 1152 / 256,
    0xd8,       1216 % 256, 1216 / 256, 0xd9,       1280 % 256, 1280 / 256,
    0xda,       1344 % 256, 1344 / 256, 0xdb,       1408 % 256, 1408 / 256,
    0,          3,          0x08,       1792 % 256, 1792 / 256, 0x0c,
    1856 % 256, 1856 / 256, 0x0d,       1920 % 256, 1920 / 256, 10,
    0x12,       1984 % 256, 1984 / 256, 0x13,       2048 % 256, 2048 / 256,
    0x14,       2112 % 256, 2112 / 256, 0x15,       2176 % 256, 2176 / 256,
    0x16,       2240 % 256, 2240 / 256, 0x17,       2304 % 256, 2304 / 256,
    0x1c,       2368 % 256, 2368 / 256, 0x1d,       2432 % 256, 2432 / 256,
    0x1e,       2496 % 256, 2496 / 256, 0x1f,       2560 % 256, 2560 / 256,
    0xff,
};

int FaxGetRun(pdfium::span<const uint8_t> ins_array,
              const uint8_t* src_buf,
              int* bitpos,
              int bitsize) {
  uint32_t code = 0;
  int ins_off = 0;
  while (true) {
    uint8_t ins = ins_array[ins_off++];
    if (ins == 0xff)
      return -1;

    if (*bitpos >= bitsize)
      return -1;

    code <<= 1;
    if (src_buf[*bitpos / 8] & (1 << (7 - *bitpos % 8)))
      ++code;

    ++(*bitpos);
    int next_off = ins_off + ins * 3;
    for (; ins_off < next_off; ins_off += 3) {
      if (ins_array[ins_off] == code)
        return ins_array[ins_off + 1] + ins_array[ins_off + 2] * 256;
    }
  }
}

void FaxG4GetRow(const uint8_t* src_buf,
                 int bitsize,
                 int* bitpos,
                 uint8_t* dest_buf,
                 pdfium::span<const uint8_t> ref_buf,
                 int columns) {
  int a0 = -1;
  bool a0color = true;
  while (true) {
    if (*bitpos >= bitsize)
      return;

    int a1;
    int a2;
    int b1;
    int b2;
    FaxG4FindB1B2(ref_buf, columns, a0, a0color, &b1, &b2);

    int v_delta = 0;
    if (!NextBit(src_buf, bitpos)) {
      if (*bitpos >= bitsize)
        return;

      bool bit1 = NextBit(src_buf, bitpos);
      if (*bitpos >= bitsize)
        return;

      bool bit2 = NextBit(src_buf, bitpos);
      if (bit1) {
        v_delta = bit2 ? 1 : -1;
      } else if (bit2) {
        int run_len1 = 0;
        while (true) {
          int run = FaxGetRun(a0color ? pdfium::make_span(kFaxWhiteRunIns)
                                      : pdfium::make_span(kFaxBlackRunIns),
                              src_buf, bitpos, bitsize);
          run_len1 += run;
          if (run < 64)
            break;
        }
        if (a0 < 0)
          ++run_len1;
        if (run_len1 < 0)
          return;

        a1 = a0 + run_len1;
        if (!a0color)
          FaxFillBits(dest_buf, columns, a0, a1);

        int run_len2 = 0;
        while (true) {
          int run = FaxGetRun(a0color ? pdfium::make_span(kFaxBlackRunIns)
                                      : pdfium::make_span(kFaxWhiteRunIns),
                              src_buf, bitpos, bitsize);
          run_len2 += run;
          if (run < 64)
            break;
        }
        if (run_len2 < 0)
          return;
        a2 = a1 + run_len2;
        if (a0color)
          FaxFillBits(dest_buf, columns, a1, a2);

        a0 = a2;
        if (a0 < columns)
          continue;

        return;
      } else {
        if (*bitpos >= bitsize)
          return;

        if (NextBit(src_buf, bitpos)) {
          if (!a0color)
            FaxFillBits(dest_buf, columns, a0, b2);

          if (b2 >= columns)
            return;

          a0 = b2;
          continue;
        }

        if (*bitpos >= bitsize)
          return;

        bool next_bit1 = NextBit(src_buf, bitpos);
        if (*bitpos >= bitsize)
          return;

        bool next_bit2 = NextBit(src_buf, bitpos);
        if (next_bit1) {
          v_delta = next_bit2 ? 2 : -2;
        } else if (next_bit2) {
          if (*bitpos >= bitsize)
            return;

          v_delta = NextBit(src_buf, bitpos) ? 3 : -3;
        } else {
          if (*bitpos >= bitsize)
            return;

          if (NextBit(src_buf, bitpos)) {
            *bitpos += 3;
            continue;
          }
          *bitpos += 5;
          return;
        }
      }
    }
    a1 = b1 + v_delta;
    if (!a0color)
      FaxFillBits(dest_buf, columns, a0, a1);

    if (a1 >= columns)
      return;

    // The position of picture element must be monotonic increasing.
    if (a0 >= a1)
      return;

    a0 = a1;
    a0color = !a0color;
  }
}

void FaxSkipEOL(const uint8_t* src_buf, int bitsize, int* bitpos) {
  int startbit = *bitpos;
  while (*bitpos < bitsize) {
    if (!NextBit(src_buf, bitpos))
      continue;
    if (*bitpos - startbit <= 11)
      *bitpos = startbit;
    return;
  }
}

void FaxGet1DLine(const uint8_t* src_buf,
                  int bitsize,
                  int* bitpos,
                  uint8_t* dest_buf,
                  int columns) {
  bool color = true;
  int startpos = 0;
  while (true) {
    if (*bitpos >= bitsize)
      return;

    int run_len = 0;
    while (true) {
      int run = FaxGetRun(color ? pdfium::make_span(kFaxWhiteRunIns)
                                : pdfium::make_span(kFaxBlackRunIns),
                          src_buf, bitpos, bitsize);
      if (run < 0) {
        while (*bitpos < bitsize) {
          if (NextBit(src_buf, bitpos))
            return;
        }
        return;
      }
      run_len += run;
      if (run < 64)
        break;
    }
    if (!color)
      FaxFillBits(dest_buf, columns, startpos, startpos + run_len);

    startpos += run_len;
    if (startpos >= columns)
      break;

    color = !color;
  }
}

class FaxDecoder final : public ScanlineDecoder {
 public:
  FaxDecoder(pdfium::span<const uint8_t> src_span,
             int width,
             int height,
             int K,
             bool EndOfLine,
             bool EncodedByteAlign,
             bool BlackIs1);
  ~FaxDecoder() override;

  // ScanlineDecoder:
  bool Rewind() override;
  pdfium::span<uint8_t> GetNextLine() override;
  uint32_t GetSrcOffset() override;

 private:
  void InvertBuffer();

  const int m_Encoding;
  int m_bitpos = 0;
  bool m_bByteAlign = false;
  const bool m_bEndOfLine;
  const bool m_bBlack;
  const pdfium::span<const uint8_t> m_SrcSpan;
  DataVector<uint8_t> m_ScanlineBuf;
  DataVector<uint8_t> m_RefBuf;
};

FaxDecoder::FaxDecoder(pdfium::span<const uint8_t> src_span,
                       int width,
                       int height,
                       int K,
                       bool EndOfLine,
                       bool EncodedByteAlign,
                       bool BlackIs1)
    : ScanlineDecoder(width,
                      height,
                      width,
                      height,
                      kFaxComps,
                      kFaxBpc,
                      fxge::CalculatePitch32OrDie(kFaxBpc, width)),
      m_Encoding(K),
      m_bByteAlign(EncodedByteAlign),
      m_bEndOfLine(EndOfLine),
      m_bBlack(BlackIs1),
      m_SrcSpan(src_span),
      m_ScanlineBuf(m_Pitch),
      m_RefBuf(m_Pitch) {}

FaxDecoder::~FaxDecoder() {
  // Span in superclass can't outlive our buffer.
  m_pLastScanline = pdfium::span<uint8_t>();
}

bool FaxDecoder::Rewind() {
  memset(m_RefBuf.data(), 0xff, m_RefBuf.size());
  m_bitpos = 0;
  return true;
}

pdfium::span<uint8_t> FaxDecoder::GetNextLine() {
  int bitsize = pdfium::base::checked_cast<int>(m_SrcSpan.size() * 8);
  FaxSkipEOL(m_SrcSpan.data(), bitsize, &m_bitpos);
  if (m_bitpos >= bitsize)
    return pdfium::span<uint8_t>();

  memset(m_ScanlineBuf.data(), 0xff, m_ScanlineBuf.size());
  if (m_Encoding < 0) {
    FaxG4GetRow(m_SrcSpan.data(), bitsize, &m_bitpos, m_ScanlineBuf.data(),
                m_RefBuf, m_OrigWidth);
    m_RefBuf = m_ScanlineBuf;
  } else if (m_Encoding == 0) {
    FaxGet1DLine(m_SrcSpan.data(), bitsize, &m_bitpos, m_ScanlineBuf.data(),
                 m_OrigWidth);
  } else {
    if (NextBit(m_SrcSpan.data(), &m_bitpos)) {
      FaxGet1DLine(m_SrcSpan.data(), bitsize, &m_bitpos, m_ScanlineBuf.data(),
                   m_OrigWidth);
    } else {
      FaxG4GetRow(m_SrcSpan.data(), bitsize, &m_bitpos, m_ScanlineBuf.data(),
                  m_RefBuf, m_OrigWidth);
    }
    m_RefBuf = m_ScanlineBuf;
  }
  if (m_bEndOfLine)
    FaxSkipEOL(m_SrcSpan.data(), bitsize, &m_bitpos);

  if (m_bByteAlign && m_bitpos < bitsize) {
    int bitpos0 = m_bitpos;
    int bitpos1 = FxAlignToBoundary<8>(m_bitpos);
    while (m_bByteAlign && bitpos0 < bitpos1) {
      int bit = m_SrcSpan[bitpos0 / 8] & (1 << (7 - bitpos0 % 8));
      if (bit != 0)
        m_bByteAlign = false;
      else
        ++bitpos0;
    }
    if (m_bByteAlign)
      m_bitpos = bitpos1;
  }
  if (m_bBlack)
    InvertBuffer();
  return m_ScanlineBuf;
}

uint32_t FaxDecoder::GetSrcOffset() {
  return pdfium::base::checked_cast<uint32_t>(
      std::min<size_t>((m_bitpos + 7) / 8, m_SrcSpan.size()));
}

void FaxDecoder::InvertBuffer() {
  DCHECK_EQ(m_Pitch, m_ScanlineBuf.size());
  DCHECK_EQ(m_Pitch % 4, 0);
  uint32_t* data = reinterpret_cast<uint32_t*>(m_ScanlineBuf.data());
  for (size_t i = 0; i < m_ScanlineBuf.size() / 4; ++i)
    data[i] = ~data[i];
}

}  // namespace

// static
std::unique_ptr<ScanlineDecoder> FaxModule::CreateDecoder(
    pdfium::span<const uint8_t> src_span,
    int width,
    int height,
    int K,
    bool EndOfLine,
    bool EncodedByteAlign,
    bool BlackIs1,
    int Columns,
    int Rows) {
  int actual_width = Columns ? Columns : width;
  int actual_height = Rows ? Rows : height;

  // Reject invalid values.
  if (actual_width <= 0 || actual_height <= 0)
    return nullptr;

  // Reject unreasonable large input.
  if (actual_width > kFaxMaxImageDimension ||
      actual_height > kFaxMaxImageDimension) {
    return nullptr;
  }

  return std::make_unique<FaxDecoder>(src_span, actual_width, actual_height, K,
                                      EndOfLine, EncodedByteAlign, BlackIs1);
}

// static
int FaxModule::FaxG4Decode(const uint8_t* src_buf,
                           uint32_t src_size,
                           int starting_bitpos,
                           int width,
                           int height,
                           int pitch,
                           uint8_t* dest_buf) {
  DCHECK(pitch != 0);

  DataVector<uint8_t> ref_buf(pitch, 0xff);
  int bitpos = starting_bitpos;
  for (int iRow = 0; iRow < height; ++iRow) {
    uint8_t* line_buf = dest_buf + iRow * pitch;
    memset(line_buf, 0xff, pitch);
    FaxG4GetRow(src_buf, src_size << 3, &bitpos, line_buf, ref_buf, width);
    memcpy(ref_buf.data(), line_buf, pitch);
  }
  return bitpos;
}

}  // namespace fxcodec
