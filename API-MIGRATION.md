## 0.10.1 -> 1.0.0
- `PdfTextBox`/`PdChoiceField`: Fixed `Spellchecking` casing to `SpellChecking`
- `PdfDrawTextMultiLineParams`:
  * Inverted semantics of `Clip` and renamed to `SkipClip`
  * Inverted semantics of `SkipSpaces` and renamed to `PreserveTrailingSpaces`
- `PdfVariant`/`PdfObect`: `GetDataTypeString()` now returns `string_view` instead of `const char*`
- `PdfErrorCode`:
  * Renamed `FreeType` -> `FreeTypeError`
  * Renamed `OpenSSL` -> `OpenSSLError`
  * Renamed `InvalidDeviceOperation` -> `IOError`
  * Renamed `NoPdfFile` -> `InvalidPDF`
  * Renamed `NoObject` -> `ObjectNotFound`
  * Renamed `NoTrailer` -> `InvalidTrailer`
  * Renamed `NoEOFToken` -> `InvalidEOFToken`
  * Renamed `XmpMetadata` -> `XmpMetadataError`
  * Renamed Flate -> FlateError
  * Removed unused `NoXRef`, `Date`, `ActionAlreadyPresent`, `MissingEndStream`, `InvalidTrailerSize`,
    `SignatureError`, `NotCompiled`, `InvalidTrailerSize`, `DestinationAlreadyPresent`,
    `OutlineItemAlreadyPresent`, `NotLoadedForUpdate`, `CannotEncryptedForUpdate`,
    `InvalidHexString`, `InvalidStreamLength`, `InvalidXRefType`
- `PdfDocument`:
  * Removed `AttachFile()`, `GetAttachment()`: Use `GetNames().GetNameTree<PdfEmbeddedFiles>()` or similar methods and use that instance
  * Removed `AddNamedDestination()`: Use `GetNames().GetTree<PdfDestinations>()` or similar methods and use that instance
- `PdfName`: Removed `operator<`, `std::hash` overload. Just use new `PdfNameMap`, `PdfNameHashMap`,
  or use `PdfNameInequality`, `PdfNameEquality` and `PdfNameHashing` to create your new data structure
- `PdfNameComparator`: Renamed to `PdfNameInequality`
- `PdfDictionaryMap`: Renamed to `PdfNameMap`
- `PdfResources`: Moved all string resource type functions to the `PdfResourceOperations` interface and
  make all the implementations private. Cast `PdfResources` instances to this `PdfResourceOperations`
  interface if you want to use the now reserved generic functions
- `PdfXObject`, `PdfFont`: Removed `GetIdentifier()`, the identifiers are now generated when inserted to `PdfResources`
- `PdfXObjet:SetMatrix()`: Removed and moved it to `PdfXObjectForm` (specification tells it doesn't belong to other XObject)
- `PdfPage::SetICCProfile()`, `PdfImage::SetICCProfile()`: Removed. Create a
  `PdfColorSpaceFilterICCBased` and set it through `PdfGraphicsStateWrapper::SetNonStrokingColorSpace`
  or `PdfGraphicsStateWrapper::SetStrokingColorSpace`
- `PdfGraphicsStateWrapper`:
  * Renamed `SetFillColor()` -> `SetNonStrokingColor()`
  * Renamed `SetFillColorSpace()` -> `SetNonStrokingColorSpace()`
  * Renamed `SetStrokeColor()` -> `SetStrokingColor()`
  * Renamed `SetStrokeColorSpace()` -> `SetStrokingColorSpace()`
  * Renamed `SetCurrentMatrix()` -> `ConcatenateTransformationMatrix()`
- `PdfPage`: Renamed `MoveAt()` -> `MoveTo()`
- `FileStreamDevice` doesn't inherit `StandardStreamDevice` anymore
- `PdfString`:
  * `GetString()` and `GetRawData()` now returns `std::string_view`
  * `PdfStringState` renamed to `PdfStringCharset`, `PdfString::GetState()`
    renamed to `PdfString::GetCharset()` and added `PdfString::IsStringEvaluated()`
- `PdfName`: `GetString()` and `GetRawData()` now returns `std::string_view`
- `PdfMemDocument`:
  * Renamed `LoadFromDevice()` -> `Load()`
  * FreeObjectMemory: Removed, use PdfObject TryUnload() instead
- `PdfAppearanceState`: Renamed to `PdfAppearanceStream`
- `PdfMetadata`:
  * Removed argument `trySyncXMP` from all functions setting values. Manually call new `TrySyncXMPMetadata` instead
  * Removed `EnsureXMPMetadata`, use `SyncXMPMetadata` instead
- Added `optional/PdfNames.h` and moved all known `PdfName::Key...` names there
- `PdfEncrypt`:
  * `GenerateEncryptionKey` renamed to `EnsureEncryptionInitialized` and takes `PdfEncryptContxt` as an argument
  * `Authenticate`, `EncryptTo`, `DecryptTo`, `CreateEncryptionInputStream`, `CreateEncryptionOutputStream` now take `PdfEncryptContxt` as an argument
- `PdfIndirectObjectList`:
  * `SetStreamFactory` is now private, it's supposed to be used only by private PdfImmediateWriter
  * `ReplaceObject`: Removed, it was added during pdfmm times when there was no better way to rewrite object streams without temporary objects
  * `SetCanReuseObjectNumbers`, `GetCanReuseObjectNumbers`: Removed, the default is true ans it's untested with false. Reusing object numbers
    is a standard PDF feature and it's better to fix bugs in that part (if any) than allowing to mess with internal indirect object numbering
    in the public API
  * `RemoveObject()`, `CreateStream()`, `Attach()` `Detach`, `Clear()`,`BeginAppendStream()`,`EndAppendStream()`, `TryIncrementObjectCount()`:
    Removed from the public API: They have always been for inner use and dangerous to call for the user. For object removal we now rely on garbage collection
  * Renamed `ObjectListComparator` to `PdfObjectInequality` and moved it to PoDoFo namespace
- `PdfExtGState`:
  * Costructor is now private, create it through `PdfDocument::CreateExtGState()`
  * All methods now accept nullable
  * `SetOverprint` -> `SetOverprintEnabled`
  * `SetFillOverprint` -> `SetFillOverprintEnabled`
  * `SetStrokeOverprint` -> `SetStrokeOverprintEnabled`
  * `SetRenderingIntent` uses `PdfRenderingIntent` enum
  * `SetBlendMode` uses `PdfBlendMode` enum
- `PdfStreamedObjectStream`: Removed from public API, it's an internal implementation detail
- `PdfXRefEntry`,`PdfXRefEntries`, `PdfParserObject`, `PdfXRefStreamParserObject`: Removed from public API,
they are internal implementation details
- `PdfParser`: Taken out of the public API, moved `GetMaxObjectCount`/`SetMaxObjectCount` to `PdfCommon`
- `PdfEncrypt`:
  * Renamed `PdfEncryptAlgorithm::AESV3` -> `PdfEncryptAlgorithm::AESV3R5`
  * Removed `SetEnabledEncryptionAlgorithms()`: Disabling algorithms is not supported anymore
  * Removed `CreateFromEncrypt()`: Internal usage only
  * Removed `GetUserPassword()`, `GetOwnerPassword()`: sensitive content, one shouldn't be able to retrieve again after setting;
  * Removed `PdfAESV3Revision`: Internal usage only
  * Removed `PdfEncryptSHABase`: Not needed
  * Removed `PdfEncryptAESBase`, `PdfEncryptRC4Base`: Implementation details, moved to composition instead of multiple inheritance
  * `PdfEncryptRC4`, `PdfEncryptAESV2`, `PdfEncryptAESV3` are now final
  * Removed `PdfEncryptRC4`, `PdfEncryptAESV2`, `PdfEncryptAESV3` public constructors: Implementation details,
    instances are not supposed to be created by API users
  * `PdfEncryptMD5Base`: Removed `GetMD5Binary`, `GetMD5String`: They are not supposed to be part of a PDF library
- `PdfWriter`,`PdfImmediateWriter`: Removed from public API, they were an implementation detail
- `PdfFontManager::GetOrCreateFont(face)`, `PdfFontMetricsFreetype::CreateFromFace(face)`, `PdfFontMetrics::TryGetOrLoadFace(face)`, `PdfFontMetrics::GetOrLoadFace`: removed, exposing methods with `FT_Face` type may be dangerous in a public API because of possible mismatch of FreeType library version used by the API consumer and the version used in the PoDoFo compilation
- `FreeTypeFacePtr`: Removed, it was just used in the implementation
- Inverted parameters in `PdfDifferenceEncoding` constructor
- Moved `PdfCMapEncoding::CreateFromObject` to `PdfEncodingMapFactory::ParseCMapEncoding`
- `PdfNameTree`:
  * Renamed -> `PdfNameTrees`
  * Moved all string tree type functions to the `PdfNameTreeOperations` and make mutable functions private. Cast to that type if you still want to use them
  * `ToDictionary` now takes `std::map<PdfString, PdfObject>` as input
  * `HasValue` -> renamed to `HasKey`
- Renamed enum `PdfColorSpace` -> `PdfColorSpaceType`, `PdfColorSpace` is now a doc element.
- `PdfColor` now it's used just to represent GrayScale, RGB, CMYK colors. Now `PdfColorRaw` is used to supply color components for other color spaces
- `PdfCanvas`: Rename `GetStreamForAppending()` -> `GetOrCreateContentsStream()`
- `PdfContents`: `Reset()` is now parameterless. It was created to replace the stream. To achieve the same one can do GetStreamForAppending()
and use move semantics on the stream
- `PdfContents`: Rename `GetStreamForAppending()` -> `CreateStreamForAppending()`
- `PdfParserObject::HasStreamToParse()` Make it protected virtual in `PdfObject` (it's unreliable to access it publicly)
- Removed `PdfPage::SetPageWidth()`, `PdfPage::SetPageHeight()`. Use `SetRect()`, `SetMediaBox()`, `SetCropBox()`, etc. instead
- Removed `PdfFontMetricsFreetype::FromBuffer()`
- Make `PdfFontMetricsFreetype::FromMetrics()` private (it's really an internal method)
- Removed `PdfFontTrueTypeSubset`: it's an implementation detail and not to be exposed in the public API
- `PdfFilespec`:
    * Removed public constructors, moved construction to `PdfDocument::CreateFilespec()`
    * Moved setters to public methods instead of construction parameters. By default now just the `/UF` entry is set
- `PdfDestination`:
    * Removed public constructors, moved construction to `PdfDocument::CreateDestination()`
- `PdfAction`:
    * Reworked hierarchy, create `PdfActionURI`, `PdfActionJavascript` and so on classes. Moved URI, script accessors
      to respective classes
    * Removed public constructors, moved construction to `PdfDocument::CreateAction()`
- `PdfOutlineItem`, `PdfOutlines`:
    * Removed public constructors, they are now construct privately only
    * Removed `GetTextColorRed()`, `GetTextColorGreen()`, `GetTextColorBlue()`, added `GetTextColor()` that returns `PdfColor`
    * `SetTextColor()` now takes a `PdfColor`
    * Setting/Getting destination now uses `nullable<PdfDestination&>`
    * Setting/Getting action now uses `nullable<PdfAction&>`
    * `InsertChild` is now private only
- `PdfAnnotationActionBase`:
    * Setting/Getting action now uses `nullable<PdfAction&>`
- `PdfAnnotationLink`:
    * Setting/Getting destination now uses `nullable<PdfDestination&>`
- `PdfAnnotationFileAttachment`:
    * Setting/Getting filespec now uses `nullable<PdfFilespec&>`
- `PdfRef`, `PdfXRefStream`: Make the constructor internal, `PdfXRefStream` class final
- `PdfSignature::PrepareForSigning()`: Make it internal
- `PdfACtion` hierarchy: make all hierarchy constructors internals and leave classes final
- `PdfField` hierarchy: make all hierarchy leave classes final
- `PdfDataProvider`, `PdfDataContainer`: Make the classes internal
- `PdfEncodingMap`, `PdfEncodingMapOneByte`, `PdfBuiltInEncoding`, `PdfPredefinedEncoding`, `PdfEncodingMapBase`: make the constructors internal
- `PdfEncodingMap`:
  * Removed `IsBuiltinEncoding`. No replacement, it just told if the font was built-in in Type1 font program. For now it's not expected to be useful
  * `TryGetCodePoints()`, `TryGetNextCodePoints` now takes `CodePointSpan` instead of vector<codepoint>
- `PdfCharCodeMap`: `TryGetCodePoints()` now takes `CodePointSpan` instead of vector<codepoint>
- `PdfEncoding`: `TryScan` now takes `CodePointSpan` instead of vector<codepoint>
- `PdfExtension`: Make the constructor internal and class final
- `PdfFilter`: Make the constructor internal
- `PdfFilterFactory`: Make class internal use only
- `PdfFont`, `PdfFontSimple`, `PdfFontCID`, `PdfFontObject`: Make the constructor internal
- `PdfFontType1`, `PdfFontType3`, `PdfFontTrueType`, `PdfFontCIDTrueType`, `PdfFontCIDType1`: Make the classes final
- `PdfObjectInputStream`, `PdfObjectOutputStream`: Make the classes final
- `PdfObjectStreamParser`: Made the class internal use only
- `PdfWinAnsiEncoding`: Made the class final
- `PdfXObjectPostScript`: Made the class final
- `PdfContents`: Made the constructor internal and the class internal
- `PdfCatalog`: Made the constructor internal
- `PdfEncoding`: Made the class final, maked `ExportToFont()` internal

## 0.10.0 -> 0.10.1
- `PdfParser::TakeEncrypt()` -> `PdfParser::GetEncrypt()` which now returns `std::shared_ptr`. This change was needed to address a vulnerability concern in #70. Although public, This method is considered to be infrastructural and not called often outside of PoDofo;
- `PdfPageTreeCache` was removed. Also this class was infrastructural and probably not used outside of PoDofo.

## 0.9.8 -> 0.10.0

The following is an incomplete list of 0.9.8 -> 0.10.0 API modifications. Feel free to suggest improvements in the ML or in a GitHub issue.

- Removed all `pdf_int*` types and moved to standard `int*_t` types;
- Removed `pdf_long` and all usages converted to either `size_t` and `ssize_t`;
- Renamed `PdfVecObjects` -> `PdfIndirectObjectList`
- Renamed `PdfFontCache` -> `PdfFontManager`
- Renamed `PdfNamesTree` -> `PdfNameTree`
- Renamed `PdfPagesTree` -> `PdfPageCollection`
- Renamed `PdfInputDevice` -> `InputDevice`
- Renamed `PdfOutputDevice` -> `OutputDevice`
- Renamed `PdfInputStream` -> `InputStream`
- Renamed `PdfOutputStream` -> `OutputStream`
- Merged `InputDevice`/`OutputDevice` into `StreamDevice`
- Renamed `PdfDocument::GetNameTree()` -> `GetNames()`
- `PdfDocument::GetPage()`/`PdfDocument::CreatePage()` removed and moved to `PdfPageCollection`
- `PdfDocument::CreateFont()`, `PdfDocument::CreateFontSubset` moved to `PdfFontManager::SearchFonts()`, `PdfFontManager::GetStandard14Font()`, `PdfFontManager::GetOrCreateFont()`, `PdfDocument::GetOrCreateFontFromBuffer()`
- Removed `PdfDocument::CreateDuplicateFontType1()`
- `PdfArray::FindAt()` now returns reference
- `PdfDocument::GetFontCache()` -> `PdfDocument::GetFonts()`
- `PdfPage::GetAnnot()` and annotations methods are removed. Use `PdfPage::GetAnnots()` instead
- `PdfWriteFlags` are now internal use, use `PdfSaveOptions` instead
- `PdfXObject`, is now an abstract class. Refer to `PdfXObjectForm`
- To create a `PdfXObjectForm`, use `PdfDocument::CreateXObjectForm()`
- `PdfAnnotation`, is now an abstract class. Refer to the full new hierarchy (`PdfAnnotationWidget`, `PdfAnnotationLink`, ...)
- Renamed `PdfSignatureField` -> `PdfSignature`
- Renamed `PdfTextField` -> `PdfTextBox`
- Renamed `PdfListField` -> `PdfChoiceField`
- Renamed `PdfImage::GetFilteredCopy()` -> `PdfImage::GetDecodedCopy()`
- `PdfObject::GetIndirectKey()` like methods removed. Use `PdfObject::TryGetDictionary(dict)` and `PdfDictionary` methods instead
- `PdfSignOutputDevice` removed, use `PoDoFo::SignDocument()` instead
- `PdfDate::PdfDate()` now creates an epoch date. Use `PdfDate::LocalNow()` `PdfDate::UtcNow()`.
`PdfDate::PdfDate(str)` is moved to `PdfDate::Parse(str)`
- `PdfFontMetrics::GetStringWidth()` -> `PdfFont::GetStringLength(state)`, `PdfFontMetrics::GetGlyphWidth()` -> `PdfFont::GetCharLength()` with state filled with `FontSize`
- `PdfFont::SetFontSize()` removed. See functions in `PdfFont` that accepts `PdfTextState`. `PdfFont::SetBold()`, `PdfFont::SetItalic()` removed as they were no sense. Font style now is read-only and can be read from `PdfFontMetrics::GetFontStyle()`
- `PdfTable`: Removed as providing formatting features that are too high level for the scope of PoDoFo
- `PdfDocument::Clear()`: Removed, reintroduced in >0.10 as `PdfDocument::Reset()`
- `PdfDocument::InsertExistingPageAt`: Moved and renamed to `PdfPageCollection::InsertDocumentPageAt`
- `PdfDocument::Append`: Moved, renamed and improved to `PdfPageCollection::AppendDocumentPages`
- `PdfPage::GetField()`, `PdfPage::GetFieldCount()`: Removed, [iterate annotations](https://github.com/podofo/podofo/issues/158#issuecomment-2081646748) instead for now.
