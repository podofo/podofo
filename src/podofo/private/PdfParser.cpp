/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PdfDeclarationsPrivate.h"
#include "PdfParser.h"

#include <algorithm>
#include <numerics/checked_math.h>

#include <podofo/auxiliary/OutputDevice.h>
#include <podofo/auxiliary/InputDevice.h>

#include <podofo/main/PdfArray.h>
#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfEncrypt.h>
#include <podofo/main/PdfMemoryObjectStream.h>
#include "PdfXRefStreamParserObject.h"
#include "PdfObjectStreamParser.h"

constexpr unsigned PDF_VERSION_LENGHT = 3;
constexpr unsigned PDF_MAGIC_LENGHT = 8;
constexpr unsigned PDF_XREF_ENTRY_SIZE = 20;
constexpr unsigned PDF_XREF_BUF = 512;
constexpr unsigned MAX_XREF_SESSION_COUNT = 512;

using namespace std;
using namespace PoDoFo;
using namespace chromium::base;

static bool CheckEOL(char e1, char e2);
static bool CheckXRefEntryType(char c);
static bool ReadMagicWord(char ch, unsigned& cursoridx);

PdfParser::PdfParser(PdfIndirectObjectList& objects) :
    m_buffer(std::make_shared<charbuff>(PdfTokenizer::BufferSize)),
    m_tokenizer(m_buffer),
    m_Objects(&objects),
    m_StrictParsing(false)
{
    this->reset();
}

void PdfParser::reset()
{
    m_PdfVersion = PdfVersionDefault;
    m_LoadOnDemand = false;

    m_magicOffset = 0;
    m_HasXRefStream = false;
    m_XRefOffset = 0;
    m_lastEOFOffset = 0;

    m_Trailer = nullptr;
    m_entries.Clear();

    m_Encrypt = nullptr;

    m_IgnoreBrokenObjects = true;
    m_IncrementalUpdateCount = 0;
}

void PdfParser::Parse(InputStreamDevice& device, bool loadOnDemand)
{
    reset();

    m_LoadOnDemand = loadOnDemand;

    try
    {
        if (!IsPdfFile(device))
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidPDF);

        ReadDocumentStructure(device);
        ReadObjects(device);
    }
    catch (PdfError& e)
    {
        if (e.GetCode() == PdfErrorCode::InvalidPassword)
        {
            // Do not clean up, expect user to call ParseFile again
            throw;
        }

        // If this is being called from a constructor then the
        // destructor will not be called.
        // Clean up here  
        reset();
        PODOFO_PUSH_FRAME_INFO(e, "Unable to load objects from file");
        throw;
    }
}

void PdfParser::ReadDocumentStructure(InputStreamDevice& device, ssize_t eofSearchOffset, bool skipFollowPrevious)
{
    // Position at the end of the file, or the given
    // offset, to search the xref table.
    if (eofSearchOffset < 0)
        device.Seek(0, SeekDirection::End);
    else
        device.Seek(eofSearchOffset, SeekDirection::Begin);

    m_FileSize = device.GetPosition();

    // Validate the eof marker and when not in strict mode accept garbage after it
    try
    {
        checkEOFMarker(device);
    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME_INFO(e, "EOF marker could not be found");
        throw;
    }

    try
    {
        findXRef(device, m_XRefOffset);
    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME_INFO(e, "Unable to find startxref entry in file");
        throw;
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
        ReadXRefContents(device, m_XRefOffset, skipFollowPrevious);
    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME_INFO(e, "Unable to load xref entries");
        throw;
    }

    int64_t entriesCount;
    if (m_Trailer != nullptr && m_Trailer->IsDictionary()
        && (entriesCount = m_Trailer->GetDictionary().FindKeyAsSafe<int64_t>("Size", -1)) >= 0
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
    // try to determine the exact PDF version of the file
    m_PdfVersion = PoDoFo::GetPdfVersion(string_view(versionStr, std::size(versionStr)));
    if (m_PdfVersion == PdfVersion::Unknown)
        return false;

    return true;
}

void PdfParser::mergeTrailer(const PdfObject& trailer)
{
    PODOFO_ASSERT(m_Trailer != nullptr);

    // Only update keys, if not already present
    auto obj = trailer.GetDictionary().GetKey("Size");
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey("Size"))
        m_Trailer->GetDictionary().AddKey("Size"_n, *obj);

    obj = trailer.GetDictionary().GetKey("Root");
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey("Root"))
        m_Trailer->GetDictionary().AddKey("Root"_n, *obj);

    obj = trailer.GetDictionary().GetKey("Encrypt");
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey("Encrypt"))
        m_Trailer->GetDictionary().AddKey("Encrypt"_n, *obj);

    obj = trailer.GetDictionary().GetKey("Info");
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey("Info"))
        m_Trailer->GetDictionary().AddKey("Info"_n, *obj);

    obj = trailer.GetDictionary().GetKey("ID");
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey("ID"))
        m_Trailer->GetDictionary().AddKey("ID"_n, *obj);
}

void PdfParser::readNextTrailer(InputStreamDevice& device, bool skipFollowPrevious)
{
    utls::RecursionGuard guard;
    string_view token;
    if (!m_tokenizer.TryReadNextToken(device, token) || token != "trailer")
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidTrailer);

    // Ignore the encryption in the trailer as the trailer may not be encrypted
    auto trailer = new PdfParserObject(m_Objects->GetDocument(), device, -1);
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
        mergeTrailer(*trailer);
    }

    int64_t xrefStmOffset;
    if (trailer->GetDictionary().TryFindKeyAs<int64_t>("XRefStm", xrefStmOffset))
    {
        // The trailer is hybrid-reference file's trailer with a
        // separate XRef stream: just read it
        try
        {
            ReadXRefStreamContents(device, static_cast<size_t>(xrefStmOffset), skipFollowPrevious);
        }
        catch (PdfError& e)
        {
            PODOFO_PUSH_FRAME_INFO(e, "Unable to load /XRefStm xref stream");
            throw;
        }
    }

    auto prevObj = trailer->GetDictionary().FindKey("Prev");
    int64_t offset;
    if (prevObj != nullptr && prevObj->TryGetNumber(offset))
    {
        if (offset > 0)
        {
            // Whenever we read a Prev key, 
            // we know that the file was updated.
            m_IncrementalUpdateCount++;

            if (!skipFollowPrevious)
            {
                if (m_visitedXRefOffsets.find((size_t)offset) == m_visitedXRefOffsets.end())
                    ReadXRefContents(device, (size_t)offset, false);
                else
                    PoDoFo::LogMessage(PdfLogSeverity::Warning, "XRef contents at offset {} requested twice, skipping the second read",
                        static_cast<int64_t>(offset));
            }
        }
        else
        {
            PoDoFo::LogMessage(PdfLogSeverity::Warning, "XRef offset {} is invalid, skipping the read", offset);
        }
    }
}

void PdfParser::findXRef(InputStreamDevice& device, size_t& xRefOffset)
{
    // ISO32000-1:2008, 7.5.5 File Trailer "Conforming readers should read a PDF file from its end"
    findTokenBackward(device, "startxref", PDF_XREF_BUF, m_lastEOFOffset);

    string_view token;
    if (!m_tokenizer.TryReadNextToken(device, token) || token != "startxref")
    {
        // Could be non-standard startref
        if (!m_StrictParsing)
        {
            findTokenBackward(device, "startref", PDF_XREF_BUF, m_lastEOFOffset);
            if (!m_tokenizer.TryReadNextToken(device, token) || token != "startref")
                PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRef);
        }
        else
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRef);
        }
    }

    // Support also files with whitespace offset before magic start
    xRefOffset = (size_t)m_tokenizer.ReadNextNumber(device) + m_magicOffset;
}

void PdfParser::ReadXRefContents(InputStreamDevice& device, size_t offset, bool skipFollowPrevious)
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
        findXRef(device, offset);
        offset = device.GetPosition();
        // TODO: hard coded value "4"
        m_buffer->resize(PDF_XREF_BUF * 4);
        findTokenBackward(device, "xref", PDF_XREF_BUF * 4, offset);
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
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRef);

    if (token != "xref")
    {
        ReadXRefStreamContents(device, offset, skipFollowPrevious);
        m_HasXRefStream = true;
        return;
    }

    // read all xref subsections
    for (unsigned xrefSectionCount = 0; ; xrefSectionCount++)
    {
        if (xrefSectionCount == MAX_XREF_SESSION_COUNT)
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEOFToken);

        try
        {
            if (!m_tokenizer.TryPeekNextToken(device, token))
                PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRef);

            if (token == "trailer")
                break;

            firstObject = m_tokenizer.ReadNextNumber(device);
            objectCount = m_tokenizer.ReadNextNumber(device);

#ifdef PODOFO_VERBOSE_DEBUG
            PoDoFo::LogMessage(PdfLogSeverity::Debug, "Reading numbers: {} {}", firstObject, objectCount);
#endif // PODOFO_VERBOSE_DEBUG

            ReadXRefSubsection(device, firstObject, objectCount);
        }
        catch (PdfError& e)
        {
            if (e == PdfErrorCode::InvalidNumber || e == PdfErrorCode::InvalidXRef || e == PdfErrorCode::UnexpectedEOF)
            {
                break;
            }
            else
            {
                PODOFO_PUSH_FRAME(e);
                throw;
            }
        }
    }

    readNextTrailer(device, skipFollowPrevious);
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
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef, "ReadXRefSubsection: First object is negative");
    if (objectCount < 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef, "ReadXRefSubsection: Object count is negative");

    CheckedNumeric first((uint64_t)firstObject);
    CheckedNumeric count((uint64_t)objectCount);
    unsigned newSize;
    if (!(first + count).AssignIfValid(&newSize))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "ReadXRefSubsection: Object count has reached maximum allowed size");

    m_entries.Enlarge(newSize);

    // consume all whitespaces
    char ch;
    while (device.Peek(ch) && PoDoFo::IsCharWhitespace(ch))
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
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef, "Invalid used keyword, must be either 'n' or 'f'");

            PdfXRefEntryType type = XRefEntryTypeFromChar(chType);

            if (read != 5 || !CheckEOL(empty1, empty2))
            {
                // part of XrefEntry is missing, or i/o error
                PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRef);
            }

            switch (type)
            {
                case PdfXRefEntryType::Free:
                {
                    // The variant is the number of the next free object
                    entry.ObjectNumber = variant;
                    break;
                }
                case PdfXRefEntryType::InUse:
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
                    // This flow should have beeb already been cathed earlier
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
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRef);
    }
}

void PdfParser::ReadXRefStreamContents(InputStreamDevice& device, size_t offset, bool skipFollowPrevious)
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
        mergeTrailer(*xrefObjTrailer);
    }

    xrefObjTrailer->ReadXRefTable();

    // Check for a previous XRefStm or xref table
    size_t previousOffset;
    if (xrefObjTrailer->TryGetPreviousOffset(previousOffset) && previousOffset != offset)
    {
        m_IncrementalUpdateCount++;

        if (!skipFollowPrevious)
        {
            // PDFs that have been through multiple PDF tools may have a mix of xref tables (ISO 32000-1 7.5.4) 
            // and XRefStm streams (ISO 32000-1 7.5.8.1) and in the Prev chain, 
            // so call ReadXRefContents (which deals with both) instead of ReadXRefStreamContents 
            ReadXRefContents(device, previousOffset, false);
        }
    }
}

void PdfParser::ReadObjects(InputStreamDevice& device)
{
    if (m_Trailer == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidTrailer);

    // Check for encryption and make sure that the encryption object
    // is loaded before all other objects
    auto encryptObj = m_Trailer->GetDictionary().GetKey("Encrypt");
    if (encryptObj != nullptr && !encryptObj->IsNull())
    {
#ifdef PODOFO_VERBOSE_DEBUG
        PoDoFo::LogMessage(PdfLogSeverity::Debug, "The PDF file is encrypted");
#endif // PODOFO_VERBOSE_DEBUG

        shared_ptr<PdfEncrypt> encrypt;
        PdfReference encryptRef;
        if (encryptObj->TryGetReference(encryptRef))
        {
            unsigned i = encryptRef.ObjectNumber();
            if (i <= 0 || i >= m_entries.GetSize())
            {
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict,
                    "Encryption dictionary references a nonexistent object {} {} R",
                    encryptObj->GetReference().ObjectNumber(), encryptObj->GetReference().GenerationNumber());
            }

            // The encryption dictionary is not encrypted
            unique_ptr<PdfParserObject> obj(new PdfParserObject(device, encryptRef, (ssize_t)m_entries[i].Offset));
            try
            {
                obj->Parse();
                // NOTE: Never add the encryption dictionary to m_Objects
                // we create a new one, if we need it for writing
                m_entries[i].Parsed = false;
                encrypt = PdfEncrypt::CreateFromObject(*obj);
            }
            catch (PdfError& e)
            {
                PODOFO_PUSH_FRAME_INFO(e, "Error while loading object {} {} R",
                    obj->GetIndirectReference().ObjectNumber(),
                    obj->GetIndirectReference().GenerationNumber());
                throw;

            }
        }
        else if (encryptObj->IsDictionary())
        {
            encrypt = PdfEncrypt::CreateFromObject(*encryptObj);
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict,
                "The encryption entry in the trailer is neither an object nor a reference");
        }

        m_Encrypt.reset(new PdfEncryptSession(std::move(encrypt)));

        // Generate encryption keys
        m_Encrypt->GetEncrypt().Authenticate(m_Password, this->getDocumentId(), m_Encrypt->GetContext());
        if (m_Encrypt->GetContext().GetAuthResult() == PdfAuthResult::Failed)
        {
            // authentication failed so we need a password from the user.
            // The user can set the password using PdfParser::SetPassword
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidPassword, "A password is required to read this PDF file");
        }
    }

    readObjectsInternal(device);
}

void PdfParser::readObjectsInternal(InputStreamDevice& device)
{
    // Read objects
    vector<unsigned> compressedIndices;
    map<int64_t, vector<int64_t>> compressedObjects;
    unique_ptr<PdfParserObject> obj;
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
                case PdfXRefEntryType::InUse:
                {
                    if (entry.Offset > 0)
                    {
                        PdfReference reference(i, (uint16_t)entry.Generation);
                        obj.reset(new PdfParserObject(m_Objects->GetDocument(), reference, device, (ssize_t)entry.Offset));
                        try
                        {
                            if (m_Encrypt != nullptr)
                            {
                                obj->SetEncrypt(m_Encrypt);
                                PdfDictionary* objDict;
                                if (obj->TryGetDictionary(objDict))
                                {
                                    auto typeObj = objDict->GetKey("Type");
                                    if (typeObj != nullptr && typeObj->IsName() && typeObj->GetName() == "XRef")
                                    {
                                        // XRef is never encrypted
                                        obj.reset(new PdfParserObject(m_Objects->GetDocument(), reference, device, (ssize_t)entry.Offset));
                                        if (m_LoadOnDemand)
                                            obj->DelayedLoad();
                                    }
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
                                throw;
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
                case PdfXRefEntryType::Free:
                {
                    // NOTE: We don't need entry.ObjectNumber, which is supposed to be
                    // the entry of the next free object
                    if (i != 0)
                        m_Objects->SafeAddFreeObject(PdfReference(i, (uint16_t)entry.Generation));

                    break;
                }
                case PdfXRefEntryType::Compressed:
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
        // where some object-numbers are altogether missing and multiple XRefs where 
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
        readCompressedObjectFromStream((uint32_t)pair.first, pair.second);
        m_Objects->AddCompressedObjectStream((uint32_t)pair.first);
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

    updateDocumentVersion();
}

void PdfParser::readCompressedObjectFromStream(uint32_t objNo, const cspan<int64_t>& objectList)
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
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidObject, "Loading of object {} 0 R failed!", objNo);
        }
    }

    PdfObjectStreamParser parserObject(*streamObj, *m_Objects, m_buffer);
    parserObject.Parse(objectList);
}

void PdfParser::findTokenBackward(InputStreamDevice& device, const char* token, size_t range, size_t searchEnd)
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

const PdfString& PdfParser::getDocumentId()
{
    const PdfArray* idArr;
    if (!m_Trailer->GetDictionary().TryFindKeyAs("ID", idArr))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "No document ID found in trailer");

    return (*idArr)[0].GetString();
}

void PdfParser::updateDocumentVersion()
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
                        PoDoFo::GetPdfVersionName(m_PdfVersion).GetString(),
                        PoDoFo::GetPdfVersionName(version).GetString());
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

void PdfParser::checkEOFMarker(InputStreamDevice& device)
{
    // Check for the existence of the EOF marker
    m_lastEOFOffset = 0;
    const char* EOFToken = "%%EOF";
    constexpr size_t EOFTokenLen = 5;
    char buff[EOFTokenLen + 1];

    device.Seek(-static_cast<ssize_t>(EOFTokenLen), SeekDirection::End);
    if (IsStrictParsing())
    {
        // For strict mode EOF marker must be at the very end of the file
        device.Read(buff, EOFTokenLen);
        if (std::strncmp(buff, EOFToken, EOFTokenLen) != 0)
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEOFToken);
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
            m_lastEOFOffset = device.GetPosition() - EOFTokenLen;
        else
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEOFToken);
    }
}

const PdfObject& PdfParser::GetTrailer() const
{
    if (m_Trailer == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    return *m_Trailer;
}

unique_ptr<PdfObject> PdfParser::TakeTrailer()
{
    if (m_Trailer == nullptr)
        return nullptr;

    // We create a new object using move semantics. This may loose XRef
    // stream information stored in PdfXRefStreamParserObject, but we
    // don't want to preserve it
    auto ret = unique_ptr<PdfObject>(new PdfObject(std::move(*m_Trailer)));
    m_Trailer = nullptr;
    return ret;
}

bool PdfParser::TryGetPreviousRevisionOffset(InputStreamDevice& input, size_t currOffset, size_t& eofOffset)
{
    eofOffset = numeric_limits<size_t>::max();

    // NOTE: We partially parse the document, just reading
    // the xref entries of the current revision, and not
    // following previous incremental updates
    PdfIndirectObjectList objects;
    PdfParser parser(objects);
    parser.ReadDocumentStructure(input, (ssize_t)currOffset, true);
    if (parser.GetIncrementalUpdatesCount() == 0)
        return false;

    // We Iterate parsed entries and find the one with
    // the lower offset, which will be deemed the EOF
    // offset of the previous revision
    auto& entries = parser.m_entries;
    bool foundValidEntry = false;
    for (unsigned i = 0; i < entries.GetSize(); i++)
    {
        auto& entry = entries[i];
        if (entry.Parsed && entry.Type == PdfXRefEntryType::InUse
            && entry.Offset < eofOffset)
        {
            eofOffset = (size_t)entry.Offset;
            foundValidEntry = true;
        }
    }

    return foundValidEntry;
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
