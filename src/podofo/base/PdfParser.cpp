/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfParser.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfMemoryObjectStream.h"
#include "PdfObjectStreamParser.h"
#include "PdfOutputDevice.h"
#include "PdfObjectStream.h"
#include "PdfVariant.h"
#include "PdfXRefStreamParserObject.h"

#include <algorithm>

constexpr unsigned PDF_VERSION_LENGHT = 3;
constexpr unsigned PDF_MAGIC_LENGHT = 8;
constexpr unsigned PDF_XREF_ENTRY_SIZE = 20;
constexpr unsigned PDF_XREF_BUF = 512;
constexpr unsigned MAX_XREF_SESSION_COUNT = 512;

using namespace std;
using namespace PoDoFo;

static bool CheckEOL(char e1, char e2);
static bool CheckXRefEntryType(char c);
static bool ReadMagicWord(char ch, unsigned& cursoridx);

static unsigned s_MaxObjectCount = (1U << 23) - 1;

PdfParser::PdfParser(PdfIndirectObjectList& objects) :
    m_buffer(std::make_shared<charbuff>(PdfTokenizer::BufferSize)),
    m_tokenizer(m_buffer, true),
    m_Objects(&objects),
    m_StrictParsing(false)
{
    this->Reset();
}

PdfParser::~PdfParser()
{
    Reset();
}

void PdfParser::Reset()
{
    m_PdfVersion = PdfVersionDefault;
    m_LoadOnDemand = false;

    m_magicOffset = 0;
    m_HasXRefStream = false;
    m_XRefOffset = 0;
    m_XRefLinearizedOffset = 0;
    m_LastEOFOffset = 0;

    m_Trailer = nullptr;
    m_entries.Clear();

    m_Encrypt = nullptr;

    m_IgnoreBrokenObjects = true;
    m_IncrementalUpdateCount = 0;
}

void PdfParser::Parse(InputStreamDevice& device, bool loadOnDemand)
{
    Reset();

    m_LoadOnDemand = loadOnDemand;

    try
    {
        if (!IsPdfFile(device))
            PODOFO_RAISE_ERROR(PdfErrorCode::NoPdfFile);

        ReadDocumentStructure(device);
        ReadObjects(device);
    }
    catch (PdfError& e)
    {
        if (e.GetCode() == PdfErrorCode::InvalidPassword)
        {
            // Do not clean up, expect user to call ParseFile again
            throw e;
        }

        // If this is being called from a constructor then the
        // destructor will not be called.
        // Clean up here  
        Reset();
        PODOFO_PUSH_FRAME_INFO(e, "Unable to load objects from file");
        throw e;
    }
}

void PdfParser::ReadDocumentStructure(InputStreamDevice& device)
{
    // position at the end of the file to search the xref table.
    device.Seek(0, SeekDirection::End);
    m_FileSize = device.GetPosition();

    // Validate the eof marker and when not in strict mode accept garbage after it
    try
    {
        CheckEOFMarker(device);
    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME_INFO(e, "EOF marker could not be found");
        throw e;
    }

    try
    {
        FindXRef(device, &m_XRefOffset);
    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME_INFO(e, "Unable to find startxref entry in file");
        throw e;
    }

    try
    {
        // We begin read the first XRef content, without
        // trying to read first the trailer alone as done
        // previously. This is caused by the fact that
        // the trailer of the last incremental update
        // can't be find along the way close to the "startxref"
        // line in case of linearized PDFs. See ISO 32000-1:2008
        // "F.3.11 Main Cross-Reference and Trailer"
        // https://stackoverflow.com/a/70564329/213871
        ReadXRefContents(device, m_XRefOffset);
    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME_INFO(e, "Unable to load xref entries");
        throw e;
    }

    int64_t entriesCount;
    if (m_Trailer != nullptr && m_Trailer->IsDictionary()
        && (entriesCount = m_Trailer->GetDictionary().FindKeyAs<int64_t>(PdfName::KeySize, -1)) >= 0
        && m_entries.GetSize() > (unsigned)entriesCount)
    {
        // Total number of xref entries to read is greater than the /Size
        // specified in the trailer if any. That's an error unless we're
        // trying to recover from a missing /Size entry.
        PoDoFo::LogMessage(PdfLogSeverity::Warning,
            "There are more objects {} in this XRef "
            "table than specified in the size key of the trailer directory ({})!",
            m_entries.GetSize(), entriesCount);
    }
}

bool PdfParser::IsPdfFile(InputStreamDevice& device)
{
    unsigned i = 0;
    device.Seek(0, SeekDirection::Begin);
    while (true)
    {
        char ch;
        if (!device.Read(ch))
            return false;

        if (ReadMagicWord(ch, i))
            break;
    }

    char versionStr[PDF_VERSION_LENGHT];
    bool eof;
    if (device.Read(versionStr, PDF_VERSION_LENGHT, eof) != PDF_VERSION_LENGHT)
        return false;

    m_magicOffset = device.GetPosition() - PDF_MAGIC_LENGHT;
    // try to determine the excact PDF version of the file
    m_PdfVersion = PoDoFo::GetPdfVersion(string_view(versionStr, std::size(versionStr)));
    if (m_PdfVersion == PdfVersion::Unknown)
        return false;

    return true;
}

void PdfParser::MergeTrailer(const PdfObject& trailer)
{
    PODOFO_ASSERT(m_Trailer != nullptr);

    // Only update keys, if not already present
    auto obj = trailer.GetDictionary().GetKey(PdfName::KeySize);
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey(PdfName::KeySize))
        m_Trailer->GetDictionary().AddKey(PdfName::KeySize, *obj);

    obj = trailer.GetDictionary().GetKey("Root");
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey("Root"))
        m_Trailer->GetDictionary().AddKey("Root", *obj);

    obj = trailer.GetDictionary().GetKey("Encrypt");
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey("Encrypt"))
        m_Trailer->GetDictionary().AddKey("Encrypt", *obj);

    obj = trailer.GetDictionary().GetKey("Info");
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey("Info"))
        m_Trailer->GetDictionary().AddKey("Info", *obj);

    obj = trailer.GetDictionary().GetKey("ID");
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey("ID"))
        m_Trailer->GetDictionary().AddKey("ID", *obj);
}

void PdfParser::ReadNextTrailer(InputStreamDevice& device)
{
    utls::RecursionGuard guard;
    if (m_tokenizer.IsNextToken(device, "trailer"))
    {
        // Ignore the encryption in the trailer as the trailer may not be encrypted
        auto trailer = new PdfParserObject(m_Objects->GetDocument(), device);
        trailer->SetIsTrailer(true);

        unique_ptr<PdfParserObject> trailerTemp;
        if (m_Trailer == nullptr)
        {
            m_Trailer.reset(trailer);
        }
        else
        {
            trailerTemp.reset(trailer);
            // now merge the information of this trailer with the main documents trailer
            MergeTrailer(*trailer);
        }

        if (trailer->GetDictionary().HasKey("XRefStm"))
        {
            // Whenever we read a XRefStm key, 
            // we know that the file was updated.
            if (!trailer->GetDictionary().HasKey("Prev"))
                m_IncrementalUpdateCount++;

            try
            {
                ReadXRefStreamContents(device, static_cast<size_t>(trailer->GetDictionary().FindKeyAs<int64_t>("XRefStm", 0)), false);
            }
            catch (PdfError& e)
            {
                PODOFO_PUSH_FRAME_INFO(e, "Unable to load /XRefStm xref stream");
                throw e;
            }
        }

        auto prevObj = trailer->GetDictionary().FindKey("Prev");
        int64_t offset;
        if (prevObj != nullptr
            && prevObj->TryGetNumber(offset))
        {
            if (offset > 0)
            {
                // Whenever we read a Prev key, 
                // we know that the file was updated.
                m_IncrementalUpdateCount++;

                try
                {
                    if (m_visitedXRefOffsets.find((size_t)offset) == m_visitedXRefOffsets.end())
                        ReadXRefContents(device, (size_t)offset);
                    else
                        PoDoFo::LogMessage(PdfLogSeverity::Warning, "XRef contents at offset {} requested twice, skipping the second read",
                            static_cast<int64_t>(offset));
                }
                catch (PdfError& e)
                {
                    PODOFO_PUSH_FRAME_INFO(e, "Unable to load /Prev xref entries");
                    throw e;
                }
            }
            else
            {
                PoDoFo::LogMessage(PdfLogSeverity::Warning, "XRef offset {} is invalid, skipping the read", offset);
            }
        }
    }
    else // Else belongs to IsNextToken("trailer") and not to HasKey("Prev")
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::NoTrailer);
    }
}

void PdfParser::FindXRef(InputStreamDevice& device, size_t* xRefOffset)
{
    // ISO32000-1:2008, 7.5.5 File Trailer "Conforming readers should read a PDF file from its end"
    FindTokenBackward(device, "startxref", PDF_XREF_BUF);
    if (!m_tokenizer.IsNextToken(device, "startxref"))
    {
        // Could be non-standard startref
        if (!m_StrictParsing)
        {
            FindTokenBackward(device, "startref", PDF_XREF_BUF);
            if (!m_tokenizer.IsNextToken(device, "startref"))
                PODOFO_RAISE_ERROR(PdfErrorCode::NoXRef);
        }
        else
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::NoXRef);
        }
    }

    // Support also files with whitespace offset before magic start
    *xRefOffset = (size_t)m_tokenizer.ReadNextNumber(device) + m_magicOffset;
}

void PdfParser::ReadXRefContents(InputStreamDevice& device, size_t offset, bool positionAtEnd)
{
    utls::RecursionGuard guard;

    int64_t firstObject = 0;
    int64_t objectCount = 0;

    if (m_visitedXRefOffsets.find(offset) != m_visitedXRefOffsets.end())
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef,
            "Cycle in xref structure. Offset {} already visited", offset);
    }
    else
    {
        m_visitedXRefOffsets.insert(offset);
    }

    size_t currPosition = device.GetPosition();
    device.Seek(0, SeekDirection::End);
    size_t fileSize = device.GetPosition();
    device.Seek(currPosition, SeekDirection::Begin);

    if (offset > fileSize)
    {
        // Invalid "startxref"
         // ignore returned value and get offset from the device
        FindXRef(device, &offset);
        offset = device.GetPosition();
        // TODO: hard coded value "4"
        m_buffer->resize(PDF_XREF_BUF * 4);
        FindToken2(device, "xref", PDF_XREF_BUF * 4, offset);
        m_buffer->resize(PDF_XREF_BUF);
        offset = device.GetPosition();
        m_XRefOffset = offset;
    }
    else
    {
        device.Seek(offset);
    }

    string_view token;
    if (!m_tokenizer.TryReadNextToken(device, token))
        PODOFO_RAISE_ERROR(PdfErrorCode::NoXRef);

    if (token != "xref")
    {
        // Found linearized 1.3-pdf's with trailer-info in xref-stream
        if (m_PdfVersion < PdfVersion::V1_3)
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::NoXRef);
        }
        else
        {
            m_HasXRefStream = true;
            ReadXRefStreamContents(device, offset, positionAtEnd);
            return;
        }
    }

    // read all xref subsections
    for (unsigned xrefSectionCount = 0; ; xrefSectionCount++)
    {
        if (xrefSectionCount == MAX_XREF_SESSION_COUNT)
            PODOFO_RAISE_ERROR(PdfErrorCode::NoEOFToken);

        try
        {
            // something like PeekNextToken()
            PdfTokenType tokenType;
            string_view readToken;
            bool gotToken = m_tokenizer.TryReadNextToken(device, readToken, tokenType);
            if (gotToken)
            {
                m_tokenizer.EnqueueToken(readToken, tokenType);
                if (readToken == "trailer")
                    break;
            }

            firstObject = m_tokenizer.ReadNextNumber(device);
            objectCount = m_tokenizer.ReadNextNumber(device);

#ifdef PODOFO_VERBOSE_DEBUG
            PoDoFo::LogMessage(PdfLogSeverity::Debug, "Reading numbers: {} {}", firstObject, objectCount);
#endif // PODOFO_VERBOSE_DEBUG

            if (positionAtEnd)
                device.Seek(static_cast<ssize_t>(objectCount * PDF_XREF_ENTRY_SIZE), SeekDirection::Current);
            else
                ReadXRefSubsection(device, firstObject, objectCount);
        }
        catch (PdfError& e)
        {
            if (e == PdfErrorCode::NoNumber || e == PdfErrorCode::InvalidXRef || e == PdfErrorCode::UnexpectedEOF)
            {
                break;
            }
            else
            {
                PODOFO_PUSH_FRAME(e);
                throw e;
            }
        }
    }

    try
    {
        ReadNextTrailer(device);
    }
    catch (PdfError& e)
    {
        if (e != PdfErrorCode::NoTrailer)
        {
            PODOFO_PUSH_FRAME(e);
            throw e;
        }
    }
}

bool CheckEOL(char e1, char e2)
{
    // From pdf reference, page 94:
    // If the file's end-of-line marker is a single character (either a carriage return or a line feed),
    // it is preceded by a single space; if the marker is 2 characters (both a carriage return and a line feed),
    // it is not preceded by a space.            
    return ((e1 == '\r' && e2 == '\n') || (e1 == '\n' && e2 == '\r') || (e1 == ' ' && (e2 == '\r' || e2 == '\n')));
}

bool CheckXRefEntryType(char c)
{
    return c == 'n' || c == 'f';
}

void PdfParser::ReadXRefSubsection(InputStreamDevice& device, int64_t& firstObject, int64_t& objectCount)
{
#ifdef PODOFO_VERBOSE_DEBUG
    PoDoFo::LogMessage(PdfLogSeverity::Debug, "Reading XRef Section: {} {} Objects", firstObject, objectCount);
#endif // PODOFO_VERBOSE_DEBUG 

    if (firstObject < 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "ReadXRefSubsection: first object is negative");
    if (objectCount < 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "ReadXRefSubsection: object count is negative");

    m_entries.Enlarge((uint64_t)(firstObject + objectCount));

    // consume all whitespaces
    char ch;
    while (device.Peek(ch) && m_tokenizer.IsWhitespace(ch))
        (void)device.ReadChar();

    unsigned index = 0;
    char* buffer = m_buffer->data();
    while (index < objectCount)
    {
        device.Read(buffer, PDF_XREF_ENTRY_SIZE);

        char empty1;
        char empty2;
        buffer[PDF_XREF_ENTRY_SIZE] = '\0';

        unsigned objIndex = static_cast<unsigned>(firstObject + index);

        auto& entry = m_entries[objIndex];
        if (objIndex < m_entries.GetSize() && !entry.Parsed)
        {
            uint64_t variant = 0;
            uint32_t generation = 0;
            char chType = 0;

            // XRefEntry is defined in PDF spec section 7.5.4 Cross-Reference Table as
            // nnnnnnnnnn ggggg n eol
            // nnnnnnnnnn is 10-digit offset number with max value 9999999999 (bigger than 2**32 = 4GB)
            // ggggg is a 5-digit generation number with max value 99999 (smaller than 2**17)
            // eol is a 2-character end-of-line sequence
            int read = sscanf(buffer, "%10" SCNu64 " %5" SCNu32 " %c%c%c",
                &variant, &generation, &chType, &empty1, &empty2);

            if (!CheckXRefEntryType(chType))
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef, "Invalid used keyword, must be eiter 'n' or 'f'");

            XRefEntryType type = XRefEntryTypeFromChar(chType);

            if (read != 5 || !CheckEOL(empty1, empty2))
            {
                // part of XrefEntry is missing, or i/o error
                PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRef);
            }

            switch (type)
            {
                case XRefEntryType::Free:
                {
                    // The variant is the number of the next free object
                    entry.ObjectNumber = variant;
                    break;
                }
                case XRefEntryType::InUse:
                {
                    // Support also files with whitespace offset before magic start
                    variant += (uint64_t)m_magicOffset;
                    if (variant > PTRDIFF_MAX)
                    {
                        // max size is PTRDIFF_MAX, so throw error if llOffset too big
                        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
                    }

                    entry.Offset = variant;
                    break;
                }
                default:
                {
                    // This flow should have beeb alredy been cathed earlier
                    PODOFO_ASSERT(false);
                }
            }

            entry.Generation = generation;
            entry.Type = type;
            entry.Parsed = true;
        }

        index++;
    }

    if (index != (unsigned)objectCount)
    {
        PoDoFo::LogMessage(PdfLogSeverity::Warning, "Count of readobject is {}. Expected {}", index, objectCount);
        PODOFO_RAISE_ERROR(PdfErrorCode::NoXRef);
    }
}

void PdfParser::ReadXRefStreamContents(InputStreamDevice& device, size_t offset, bool readOnlyTrailer)
{
    utls::RecursionGuard guard;

    device.Seek(offset);
    auto xrefObjTrailer = new PdfXRefStreamParserObject(m_Objects->GetDocument(), device, m_entries);
    try
    {
        xrefObjTrailer->ParseStream();
    }
    catch (PdfError& ex)
    {
        PODOFO_PUSH_FRAME_INFO(ex, "The trailer was found in the file, but contains errors");
        delete xrefObjTrailer;
        throw ex;
    }

    unique_ptr<PdfXRefStreamParserObject> xrefObjectTemp;
    if (m_Trailer == nullptr)
    {
        m_Trailer.reset(xrefObjTrailer);
    }
    else
    {
        xrefObjectTemp.reset(xrefObjTrailer);
        MergeTrailer(*xrefObjTrailer);
    }

    if (readOnlyTrailer)
        return;

    xrefObjTrailer->ReadXRefTable();

    // Check for a previous XRefStm or xref table
    size_t previousOffset;
    if (xrefObjTrailer->TryGetPreviousOffset(previousOffset) && previousOffset != offset)
    {
        try
        {
            m_IncrementalUpdateCount++;

            // PDFs that have been through multiple PDF tools may have a mix of xref tables (ISO 32000-1 7.5.4) 
            // and XRefStm streams (ISO 32000-1 7.5.8.1) and in the Prev chain, 
            // so call ReadXRefContents (which deals with both) instead of ReadXRefStreamContents 
            ReadXRefContents(device, previousOffset, readOnlyTrailer);
        }
        catch (PdfError& e)
        {
            // Be forgiving, the error happens when an entry in XRef
            // stream points to a wrong place (offset) in the PDF file.
            if (e != PdfErrorCode::NoNumber)
            {
                PODOFO_PUSH_FRAME(e);
                throw e;
            }
        }
    }
}

void PdfParser::ReadObjects(InputStreamDevice& device)
{
    PODOFO_ASSERT(m_Trailer != nullptr);
    // Check for encryption and make sure that the encryption object
    // is loaded before all other objects
    PdfObject* encrypt = m_Trailer->GetDictionary().GetKey("Encrypt");
    if (encrypt != nullptr && !encrypt->IsNull())
    {
#ifdef PODOFO_VERBOSE_DEBUG
        PoDoFo::LogMessage(PdfLogSeverity::Debug, "The PDF file is encrypted");
#endif // PODOFO_VERBOSE_DEBUG

        if (encrypt->IsReference())
        {
            unsigned i = encrypt->GetReference().ObjectNumber();
            if (i <= 0 || i >= m_entries.GetSize())
            {
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict,
                    "Encryption dictionary references a nonexistent object {} {} R",
                    encrypt->GetReference().ObjectNumber(), encrypt->GetReference().GenerationNumber());
            }

            // The encryption dictionary is not encrypted
            unique_ptr<PdfParserObject> obj(new PdfParserObject(device, (ssize_t)m_entries[i].Offset));
            try
            {
                obj->Parse();
                // NOTE: Never add the encryption dictionary to m_Objects
                // we create a new one, if we need it for writing
                m_entries[i].Parsed = false;
                m_Encrypt = PdfEncrypt::CreateFromObject(*obj);
            }
            catch (PdfError& e)
            {
                PODOFO_PUSH_FRAME_INFO(e, "Error while loading object {} {} R",
                    obj->GetIndirectReference().ObjectNumber(),
                    obj->GetIndirectReference().GenerationNumber());
                throw e;

            }
        }
        else if (encrypt->IsDictionary())
        {
            m_Encrypt = PdfEncrypt::CreateFromObject(*encrypt);
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict,
                "The encryption entry in the trailer is neither an object nor a reference");
        }

        // Generate encryption keys
        bool isAuthenticated = m_Encrypt->Authenticate(m_password, this->GetDocumentId());
        if (!isAuthenticated)
        {
            // authentication failed so we need a password from the user.
            // The user can set the password using PdfParser::SetPassword
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidPassword, "A password is required to read this PDF file");
        }
    }

    ReadObjectsInternal(device);
}

void PdfParser::ReadObjectsInternal(InputStreamDevice& device)
{
    // Read objects
    vector<unsigned> compressedIndices;
    map<int64_t, vector<int64_t>> compressedObjects;
    for (unsigned i = 0; i < m_entries.GetSize(); i++)
    {
        auto& entry = m_entries[i];
#ifdef PODOFO_VERBOSE_DEBUG
        cerr << "ReadObjectsInteral\t" << i << " "
            << (entry.Parsed ? "parsed" : "unparsed") << " "
            << entry.Offset << " "
            << entry.Generation << endl;
#endif
        if (entry.Parsed)
        {
            switch (entry.Type)
            {
                case XRefEntryType::InUse:
                {
                    if (entry.Offset > 0)
                    {
                        PdfReference reference(i, (uint16_t)entry.Generation);
                        unique_ptr<PdfParserObject> obj(new PdfParserObject(m_Objects->GetDocument(), reference, device, (ssize_t)entry.Offset));
                        try
                        {
                            obj->SetEncrypt(m_Encrypt.get());
                            if (m_Encrypt != nullptr && obj->IsDictionary())
                            {
                                auto typeObj = obj->GetDictionary().GetKey(PdfName::KeyType);
                                if (typeObj != nullptr && typeObj->IsName() && typeObj->GetName() == "XRef")
                                {
                                    // XRef is never encrypted
                                    obj.reset(new PdfParserObject(m_Objects->GetDocument(), reference, device, (ssize_t)entry.Offset));
                                    if (m_LoadOnDemand)
                                        obj->DelayedLoad();
                                }
                            }

                            m_Objects->PushObject(obj.release());
                        }
                        catch (PdfError& e)
                        {
                            if (m_IgnoreBrokenObjects)
                            {
                                PoDoFo::LogMessage(PdfLogSeverity::Error, "Error while loading object {} {} R, Offset={}, Index={}",
                                    obj->GetIndirectReference().ObjectNumber(),
                                    obj->GetIndirectReference().GenerationNumber(),
                                    entry.Offset, i);
                                m_Objects->SafeAddFreeObject(reference);
                            }
                            else
                            {
                                PODOFO_PUSH_FRAME_INFO(e, "Error while loading object {} {} R, Offset={}, Index={}",
                                    obj->GetIndirectReference().ObjectNumber(),
                                    obj->GetIndirectReference().GenerationNumber(),
                                    entry.Offset, i);
                                throw e;
                            }
                        }
                    }
                    else if (entry.Generation == 0)
                    {
                        PODOFO_ASSERT(entry.Offset == 0);
                        // There are broken PDFs which add objects with 'n' 
                        // and 0 offset and 0 generation number
                        // to the xref table instead of using free objects
                        // treating them as free objects
                        if (m_StrictParsing)
                        {
                            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef,
                                "Found object with 0 offset which should be 'f' instead of 'n'");
                        }
                        else
                        {
                            PoDoFo::LogMessage(PdfLogSeverity::Warning,
                                "Treating object {} 0 R as a free object", i);
                            m_Objects->AddFreeObject(PdfReference(i, 1));
                        }
                    }
                    break;
                }
                case XRefEntryType::Free:
                {
                    // NOTE: We don't need entry.ObjectNumber, which is supposed to be
                    // the entry of the next free object
                    if (i != 0)
                        m_Objects->SafeAddFreeObject(PdfReference(i, entry.Generation));

                    break;
                }
                case XRefEntryType::Compressed:
                    compressedObjects[entry.ObjectNumber].push_back(i);
                    break;
                default:
                    PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);

            }
        }
        else if (i != 0) // Unparsed
        {
            m_Objects->AddFreeObject(PdfReference(i, 1));
        }
        // the linked free list in the xref section is not always correct in pdf's
        // (especially Illustrator) but Acrobat still accepts them. I've seen XRefs 
        // where some object-numbers are alltogether missing and multiple XRefs where 
        // the link list is broken.
        // Because PdfIndirectObjectList relies on a unbroken range, fill the free list more
        // robustly from all places which are either free or unparsed
    }

    // all normal objects including object streams are available now,
    // we can parse the object streams safely now.
    //
    // Note that even if demand loading is enabled we still currently read all
    // objects from the stream into memory then free the stream.
    //
    for (auto& pair : compressedObjects)
    {
#ifndef VERBOSE_DEBUG_DISABLED
        if (m_LoadOnDemand)
            cerr << "Demand loading on, but can't demand-load from object stream." << endl;
#endif
        ReadCompressedObjectFromStream((uint32_t)pair.first, pair.second);
        m_Objects->AddObjectStream((uint32_t)pair.first);
    }

    if (!m_LoadOnDemand)
    {
        // Force loading of streams. We can't do this during the initial
        // run that populates m_Objects because a stream might have a /Length
        // key that references an object we haven't yet read. So we must do it here
        // in a second pass, or (if demand loading is enabled) defer it for later.
        for (auto objToLoad : *m_Objects)
        {
            auto obj = dynamic_cast<PdfParserObject*>(objToLoad);
            obj->ParseStream();
        }
    }

    UpdateDocumentVersion();
}

void PdfParser::ReadCompressedObjectFromStream(uint32_t objNo, const cspan<int64_t>& objectList)
{
    // generation number of object streams is always 0
    auto streamObj = dynamic_cast<PdfParserObject*>(m_Objects->GetObject(PdfReference(objNo, 0)));
    if (streamObj == nullptr)
    {
        if (m_IgnoreBrokenObjects)
        {
            PoDoFo::LogMessage(PdfLogSeverity::Error, "Loading of object {} 0 R failed!", objNo);
            return;
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NoObject, "Loading of object {} 0 R failed!", objNo);
        }
    }

    PdfObjectStreamParser parserObject(*streamObj, *m_Objects, m_buffer);
    parserObject.Parse(objectList);
}

void PdfParser::FindTokenBackward(InputStreamDevice& device, const char* token, size_t range)
{
    // Offset read position to the EOF marker if it is not the last thing in the file
    device.Seek(-(ssize_t)m_LastEOFOffset, SeekDirection::End);

    char* buffer = m_buffer->data();
    size_t currpos = device.GetPosition();
    size_t searchSize = std::min(currpos, range);
    device.Seek(-(ssize_t)searchSize, SeekDirection::Current);
    device.Read(buffer, searchSize);
    buffer[searchSize] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    ssize_t i; // Do not use an unsigned variable here
    size_t tokenLen = char_traits<char>::length(token);
    for (i = searchSize - tokenLen; i >= 0; i--)
    {
        if (std::strncmp(buffer + i, token, tokenLen) == 0)
            break;
    }

    if (i == 0)
        PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);

    // Ooffset read position to the EOF marker if it is not the last thing in the file
    device.Seek(-((ssize_t)searchSize - i) - m_LastEOFOffset, SeekDirection::End);
}

// CHECK-ME: This function is fishy or bad named
void PdfParser::FindToken2(InputStreamDevice& device, const char* token, const size_t range, size_t searchEnd)
{
    device.Seek((ssize_t)searchEnd, SeekDirection::Begin);

    char* buffer = m_buffer->data();
    size_t currpos = device.GetPosition();
    size_t searchSize = std::min(currpos, range);
    device.Seek(-(ssize_t)searchSize, SeekDirection::Current);
    device.Read(buffer, searchSize);
    buffer[searchSize] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    ssize_t i; // Do not use an unsigned variable here
    size_t tokenLen = char_traits<char>::length(token);
    for (i = searchSize - tokenLen; i >= 0; i--)
    {
        if (std::strncmp(buffer + i, token, tokenLen) == 0)
            break;
    }

    if (i == 0)
        PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);

    device.Seek((ssize_t)(searchEnd - (searchSize - i)), SeekDirection::Begin);
}

const PdfString& PdfParser::GetDocumentId()
{
    if (!m_Trailer->GetDictionary().HasKey("ID"))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "No document ID found in trailer");

    return m_Trailer->GetDictionary().GetKey("ID")->GetArray()[0].GetString();
}

void PdfParser::UpdateDocumentVersion()
{
    if (m_Trailer->IsDictionary() && m_Trailer->GetDictionary().HasKey("Root"))
    {
        auto catalog = m_Trailer->GetDictionary().FindKey("Root");
        if (catalog != nullptr
            && catalog->IsDictionary()
            && catalog->GetDictionary().HasKey("Version"))
        {
            auto& versionObj = catalog->GetDictionary().MustGetKey("Version");
            if (versionObj.IsName())
            {
                auto version = PoDoFo::GetPdfVersion(versionObj.GetName().GetString());
                if (version != PdfVersion::Unknown)
                {
                    PoDoFo::LogMessage(PdfLogSeverity::Information,
                        "Updating version from {} to {}",
                        PoDoFo::GetPdfVersionName(m_PdfVersion),
                        PoDoFo::GetPdfVersionName(version));
                    m_PdfVersion = version;
                }
            }
            else if (IsStrictParsing())
            {
                // Version must be of type name, according to PDF Specification
                PODOFO_RAISE_ERROR(PdfErrorCode::InvalidName);
            }
        }
    }
}

void PdfParser::CheckEOFMarker(InputStreamDevice& device)
{
    // Check for the existence of the EOF marker
    m_LastEOFOffset = 0;
    const char* EOFToken = "%%EOF";
    constexpr size_t EOFTokenLen = 5;
    char buff[EOFTokenLen + 1];

    device.Seek(-static_cast<ssize_t>(EOFTokenLen), SeekDirection::End);
    if (IsStrictParsing())
    {
        // For strict mode EOF marker must be at the very end of the file
        device.Read(buff, EOFTokenLen);
        if (std::strncmp(buff, EOFToken, EOFTokenLen) != 0)
            PODOFO_RAISE_ERROR(PdfErrorCode::NoEOFToken);
    }
    else
    {
        // Search for the Marker from the end of the file
        ssize_t currentPos = (ssize_t)device.GetPosition();

        bool found = false;
        while (true)
        {
            device.Read(buff, EOFTokenLen);
            if (std::strncmp(buff, EOFToken, EOFTokenLen) == 0)
            {
                found = true;
                break;
            }

            currentPos--;
            if (currentPos < 0)
                break;

            device.Seek(currentPos, SeekDirection::Begin);
        }

        // Try and deal with garbage by offsetting the buffer reads in PdfParser from now on
        if (found)
            m_LastEOFOffset = (m_FileSize - (device.GetPosition() - 1)) + EOFTokenLen;
        else
            PODOFO_RAISE_ERROR(PdfErrorCode::NoEOFToken);
    }
}

const PdfObject& PdfParser::GetTrailer() const
{
    if (m_Trailer == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::NoObject);

    return *m_Trailer;
}

bool PdfParser::IsEncrypted() const
{
    return m_Encrypt != nullptr;
}

unique_ptr<PdfEncrypt> PdfParser::TakeEncrypt()
{
    return std::move(m_Encrypt);
}

unsigned PdfParser::GetMaxObjectCount()
{
    return s_MaxObjectCount;
}

void PdfParser::SetMaxObjectCount(unsigned maxObjectCount)
{
    s_MaxObjectCount = maxObjectCount;
}

// Read magic word keeping cursor
bool ReadMagicWord(char ch, unsigned& cursoridx)
{
    bool readchar;
    switch (cursoridx)
    {
        case 0:
            readchar = ch == '%';
            break;
        case 1:
            readchar = ch == 'P';
            break;
        case 2:
            readchar = ch == 'D';
            break;
        case 3:
            readchar = ch == 'F';
            break;
        case 4:
            readchar = ch == '-';
            if (readchar)
                return true;

            break;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unexpected flow");
    }

    if (readchar)
    {
        // Advance cursor
        cursoridx++;
    }
    else
    {
        // Reset cursor
        cursoridx = 0;
    }

    return false;
}
