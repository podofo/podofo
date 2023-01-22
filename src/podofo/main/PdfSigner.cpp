/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfSigner.h"
#include "PdfDictionary.h"
#include "PdfStreamDevice.h"

using namespace std;
using namespace PoDoFo;

constexpr const char* ByteRangeBeacon = "[ 0 1234567890 1234567890 1234567890]";
constexpr size_t BufferSize = 65536;

static size_t readForSignature(StreamDevice& device,
    size_t conentsBeaconOffset, size_t conentsBeaconSize,
    char* buffer, size_t size);
static void adjustByteRange(StreamDevice& device, size_t byteRangeOffset,
    size_t conentsBeaconOffset, size_t conentsBeaconSize, charbuff& buffer);
static void setSignature(StreamDevice& device, const string_view& sigData,
    size_t conentsBeaconOffset, charbuff& buffer);
static void prepareBeaconsData(size_t signatureSize, string& contentsBeacon, string& byteRangeBeacon);

PdfSigner::~PdfSigner() { }

bool PdfSigner::SkipBufferClear() const
{
    return false;
}

string PdfSigner::GetSignatureFilter() const
{
    // Default value
    return "Adobe.PPKLite";
}

void PoDoFo::SignDocument(PdfMemDocument& doc, StreamDevice& device, PdfSigner& signer,
    PdfSignature& signature, PdfSaveOptions opts)
{
    charbuff signatureBuf;
    signer.ComputeSignature(signatureBuf, true);
    size_t beaconSize = signatureBuf.size();
    PdfSignatureBeacons beacons;
    prepareBeaconsData(beaconSize, beacons.ContentsBeacon, beacons.ByteRangeBeacon);
    signature.PrepareForSigning(signer.GetSignatureFilter(), signer.GetSignatureSubFilter(),
        signer.GetSignatureType(), beacons);
    auto& form = doc.GetOrCreateAcroForm();

    // TABLE 8.68 Signature flags: SignaturesExist (1) | AppendOnly (2)
    // NOTE: This enables the signature panel visualization
    form.GetObject().GetDictionary().AddKey("SigFlags", (int64_t)3);

    auto acroForm = doc.GetAcroForm();
    if (acroForm != nullptr)
    {
        // NOTE: Adobe is crazy and if the /NeedAppearances is set to true,
        // it will not show up the signature upon signing. Just
        // remore the key just in case it's present (defaults to false)
        acroForm->GetDictionary().RemoveKey("NeedAppearances");
    }

    doc.SaveUpdate(device, opts);
    device.Flush();

    charbuff buffer;
    adjustByteRange(device, *beacons.ByteRangeOffset, *beacons.ContentsOffset,
        beacons.ContentsBeacon.size(), buffer);
    device.Flush();

    // Read data from the device to prepare the signature
    signer.Reset();
    device.Seek(0);
    size_t readBytes;
    buffer.resize(BufferSize);
    while ((readBytes = readForSignature(device, *beacons.ContentsOffset, beacons.ContentsBeacon.size(),
        buffer.data(), BufferSize)) != 0)
    {
        signer.AppendData({ buffer.data(), readBytes });
    }

    if (!signer.SkipBufferClear())
        signatureBuf.clear();

    signer.ComputeSignature(signatureBuf, false);
    if (signatureBuf.size() > beaconSize)
        throw runtime_error("Actual signature size bigger than beacon size");

    // Ensure the signature will be as big as the
    // beacon size previously cached to fill all
    // available reserved space for the /Contents
    signatureBuf.resize(beaconSize);
    setSignature(device, signatureBuf, *beacons.ContentsOffset, buffer);
    device.Flush();
}

size_t readForSignature(StreamDevice& device, size_t conentsBeaconOffset, size_t conentsBeaconSize,
    char* buffer, size_t bufferSize)
{
    if (device.Eof())
        return 0;

    size_t pos = device.GetPosition();
    size_t readSize = 0;
    // Check if we are before beacon
    if (pos < conentsBeaconOffset)
    {
        readSize = std::min(bufferSize, conentsBeaconOffset - pos);
        if (readSize > 0)
        {
            device.Read(buffer, readSize);
            buffer += readSize;
            bufferSize -= readSize;
            if (bufferSize == 0)
                return readSize;
        }
    }

    // shift at the end of beacon
    if ((pos + readSize) >= conentsBeaconOffset
        && pos < (conentsBeaconOffset + conentsBeaconSize))
    {
        device.Seek(conentsBeaconOffset + conentsBeaconSize);
    }

    // read after beacon
    bufferSize = std::min(bufferSize, device.GetLength() - device.GetPosition());
    if (bufferSize == 0)
        return readSize;

    device.Read(buffer, bufferSize);
    return readSize + bufferSize;
}

void adjustByteRange(StreamDevice& device, size_t byteRangeOffset,
    size_t conentsBeaconOffset, size_t conentsBeaconSize, charbuff& buffer)
{
    // Get final position
    size_t fileEnd = device.GetLength();
    PdfArray arr;
    arr.Add(PdfObject(static_cast<int64_t>(0)));
    arr.Add(PdfObject(static_cast<int64_t>(conentsBeaconOffset)));
    arr.Add(PdfObject(static_cast<int64_t>(conentsBeaconOffset + conentsBeaconSize)));
    arr.Add(PdfObject(static_cast<int64_t>(fileEnd - (conentsBeaconOffset + conentsBeaconSize))));

    device.Seek(byteRangeOffset);
    arr.Write(device, PdfWriteFlags::None, { }, buffer);
}

void setSignature(StreamDevice& device, const string_view& contentsData,
    size_t conentsBeaconOffset, charbuff& buffer)
{
    auto sig = PdfString::FromRaw(contentsData);

    // Position at contents beacon after '<'
    device.Seek(conentsBeaconOffset);
    // Write the beacon data
    sig.Write(device, PdfWriteFlags::None, { }, buffer);
}

void prepareBeaconsData(size_t signatureSize, string& contentsBeacon, string& byteRangeBeacon)
{
    // Just prepare strings with spaces, for easy writing later
    contentsBeacon.resize((signatureSize * 2) + 2, ' '); // Signature bytes will be encoded
                                                         // as an hex string
    byteRangeBeacon.resize(char_traits<char>::length(ByteRangeBeacon), ' ');
}
