## 1.0.1 -> 1.1.0
- `PdfSignerCmsParams`: Deprecated `Encryption` field. The encryption is determined
  from the public key in the X.509 certificate

## 0.10.1 -> 1.0.0
- `PdfFontConfigWrapper`: Put `GetFcConfig()` and the constructor with `FcConfig*`
  argument under guard by the `PODOFO_3RDPARTY_INTEROP_ENABLED` macro
- `PdfXMPPacket`: `GetDoc()`, `GetOrCreateDescription()`, `GetDescription()`
  are now guarded by `PODOFO_3RDPARTY_INTEROP_ENABLED` macro
- `PdfContent`: the class is now generally not internally accessible as it was before.
  You can now get content fields with safe getters, or you can directly access them
  with `operator*` and `operator->` overloads, possibly after checking the type,
  errors and warnings with `GetType()`, `GetErrors()` and `GetWarnings()`
- `PdfEncodingMapFactory`:
  * `WinAnsiEncodingInstance()` renamed to `GetWinAnsiEncodingInstancePt()`
  * `MacRomanEncodingInstance()` renamed to `GetPtrMacRomanEncodingInstancePtr()`
  * `MacExpertEncodingInstance()` renamed to `GetMacExpertEncodingInstancePtr()`
  * `TwoBytesHorizontalIdentityEncodingInstance()` renamed to `GetHorizontalIdentityEncodingInstancePtr()`
  * `TwoBytesVerticalIdentityEncodingInstance()` renamed to `GetVerticalIdentityEncodingInstancePtr()`
  * `GetStandard14FontEncodingMap()` renamed to `GetStandard14FontEncodingInstancePtr()`
- `PdfDifferenceEncoding`:
  * Inverted parameters in constructor
  * `NameToCodePoint()`: Renamed to `TryGetCodePointsFromCharName()`, changed the semantics and now it returns a `CodePointSpan` instead of `char32_t`
  * `CodePointToName()`: Removed. It's not so simple to have an inverse map from code points to AGL name: there are multiple AGL lists and in the same AGL list there are ambiguous mappings. You can find a safest alternative in `PdfPredefinedEncodingType::TryGetCharNameFromCodePoint()` but it supports a smaller character set
- `PdfDifferenceList`:
  * Renamed to `PdfDifferenceMap`
  * `TryGetMappedName` now returns a `CodePointSpan` instead of `char32_t` in the overload
  * `AddDifference`: Removed overload with name. Use `PdfDifferenceEncoding::TryGetCodePointsFromCharName()` first if you need a replacement
- `Object<T>`: Renamed to `ObjectAdapter<T>`
- `PdfArray`: `FindAtAs` doesn't take a default value anymore and throws on failed lookup . Use `FindAtAsSafe` instead
- `PdfDictionary`: `GetKeyAs`, `FindKeyAs`, `FindKeyAsParent`. doesn't take a default value anymore and throws on failed lookup . Use safe method versions instead
- `PdfXObjectForm`: Removed `HasRotation()`. Rotation is zero for xobject forms and it still implements privately `TryGetRotationRadians()`
- `PoDofo::TransformRectPage`: Removed `inputIsTransformed` parameter. Now the function accepts only rect in the canonical PDF coordinate system
- `PdfExtension`: Reworked constructor parameters
- `PdfTokenizer`:
  * Moved `IsWhitespace`, `IsDelimiter`, `IsTokenDelimiter`, `IsRegular`, `IsPrintable`  to `<podofo/optional/PdfUtils.h>`.
  It doesn't seems justified to have them as part of the regular public API in PdfTokenizer.
  Also renamed them with `IsChar` suffix.
  * `IsPrintable`: Renamed to `IsCharASCIIPrintable`. That should be the correct semantic of the method
- `PoDoFo::GetPdfOperator()`, `PoDoFo::TryGetPdfOperator()`, `PoDoFo::GetPdfOperatorName()`, `PoDoFo::TryGetPdfOperatorName()`: Make them private, include `<podofo/optional/PdfConvert.h>` for substitutes
- `PoDoFo::GetOperandCount()`, `PoDoFo::TryGetOperandCount()`: Make them private, no substitute provided
- `PdfPageMode`:
  * Removed `DontCare` (which was something like a pointless "ignore")
  * Renamed `UseBookmarks` -> `UseOutlines`: "bookmark" is not really part of PDF terminology
- `PdfPageLayout`:
  * Removed `Ignore` (pointless)
  * Removed `Default`: just use nullptr in `PdfCatalog::SetPageLayout()`
- `GIDMap`: Removed, it was just a infrastructural typedef
- `PdfCIDToGIDMap`: Removed `HasGlyphAccess`, this map is always for accessing font program GIDs
- `PdfGlyphAccess`: `Width` renamed to `ReadMetrics`
- `Matrix2D`: Removed, all methods using it were converted to use `Matrix` instead, which is a full replacement
- `Matrix`: Removed `FromCoefficients()`, just use the now public constructor with coefficients
- `PdfTilingPattern`, `PdfShadingPatter`: Wholly changed API and semantics. See `PdfTilingPatternDefinition` and
  `PdfShadingPatternDefinition`
- `PdfPainter`:
  * `SetTilingPattern`, `SetShadingPattern`, `SetStrokingShadingPattern`, `SetStrokingTilingPattern`:
    Removed, use the `SetStrokingPattern`,`SetShadingPattern`, `SetStrokingUncolouredTilingPattern`,
    `SetNonStrokingUncolouredTilingPattern`
  * Removed setting `PdfPainterFlags` in the constructor and moved to the `SetCanvas(canvas, flags)` method instead
- `PdfFontMetrics`:
  * `GetFontNameSafe()` removed: Just use `GetFontName` instead
  * `GetBaseFontName()`: make it protected, `GeFamilyFontNameSafe()` it's the closest substitute
  * `GetBaseFontNameSafe()`: removed, `GeFamilyFontNameSafe()` it's the closest substitute 
  * `GetBoundingBox()` now returns `Corners`
  * `TryGetImplicitEncoding()`: removed, no substitute supplied. Retrieving a implicit encoding it's more involuted
  * `GetCIDToGIDMap()`: removed, no substitute supplied. CID to GID mappings can be retrieved only from the font
- `PdfFontMatchBehaviorFlags`: `MatchPostScriptName` inverted logic and renamed to `SkipMatchPostScriptName`
- `PdfFontConfigSearchFlags`: `MatchPostScriptName` inverted logic and renamed to `SkipMatchPostScriptName`
- `PdfContentType`:
  * Renamed `EndXObjectForm` -> `EndFormXObject`
  * `DoXObject` is issued for Form XObject only if `PdfContentReaderFlags::SkipFollowFormXObjects` is passed, otherwise `BeginXObjectForm` is issued 
- `PdfContentReaderFlags`: Renamed `DontFollowXObjectForms` -> `SkipFollowFormXObjects`
- `PdfFont`:
  * Renamed `IsCIDKeyed()` -> `IsCIDFont()`, which is less confusing
  * Renamed `AddSubsetGIDs` -> `AddSubsetCIDs`
  * Renamed `TryGetSubstituteFont` -> `TryCreateProxyFont`
  * Removed `GetUsedGIDs`: it was more implementation detail for various embedding operations
- `PdfFontFileType`:
  * Removed `CIDType1`. Just use `Type1` instead
  * Renamed `OpenType` -> `OpenTypeCFF`
- Renamed `PdfFontCIDType0` -> `PdfFontCIDCFF`
- Renamed `PdfFontType::CIDType1` -> `PdfFontType::CIDCFF`
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
- `PdfGraphicsStateWrapper`:
  * Renamed `SetFillColor()` -> `SetNonStrokingColor()`
  * Renamed `SetFillColorSpace()` -> `SetNonStrokingColorSpace()`
  * Renamed `SetStrokeColor()` -> `SetStrokingColor()`
  * Renamed `SetStrokeColorSpace()` -> `SetStrokingColorSpace()`
  * Renamed `SetCurrentMatrix()` -> `ConcatenateTransformationMatrix()`
- `PdfPage`:
  * `GetRectRaw()` now returns `Corners` instead of `Rect`
  * `SetRectRaw()` now takes `Corners` instead of `Rect`
  * Removed `rawrect` parameter from `CreateField()`, use `SetRectRaw` after creation if you need it
  * Removed `rawrect` parameter from `CreateAnnotation()`, use `SetRectRaw` after creation if you need it
  * Renamed `MoveAt()` -> `MoveTo()`
  * Removed `SetPageWidth()`, `SetPageHeight()`. Use `SetRect()`, `SetMediaBox()`, `SetCropBox()`, etc. instead
  * Removed `SetICCProfile()`, `SetICCProfile()`: create a
  `PdfColorSpaceFilterICCBased` and set it through `PdfGraphicsStateWrapper::SetNonStrokingColorSpace`
  * `GetResources()`: Now it returns a reference instead (reflecting in the specification resources is required for pages)
  * `MustGetResources()`: Removed, use the reference returning `GetResources()` instead
  * Renamed `HasRotation()` -> `TryGetRotationRadians()`
  * Removed `GetRotationRaw()` and introduced `TryGetRotationRaw()`
  or `PdfGraphicsStateWrapper::SetStrokingColorSpace`
- `PdfColorSpaceFilter`: Make `GetExportObject` protected (no public substitute provided)
- `FileStreamDevice` doesn't inherit `StandardStreamDevice` anymore
- `PdfString`:
  * `GetString()` and `GetRawData()` now returns `std::string_view`
  * `PdfStringState` renamed to `PdfStringCharset`, `PdfString::GetState()`
    renamed to `PdfString::GetCharset()` and added `PdfString::IsStringEvaluated()`
- `PdfName`: `GetString()` and `GetRawData()` now returns `std::string_view`
- `PdfMemDocument`:
  * Renamed `LoadFromDevice()` -> `Load()`
  * FreeObjectMemory: Removed, use PdfObject TryUnload() instead
  * `AddPdfExtension`, `HasPdfExtension`, `RemovePdfExtension`, `GetPdfExtensions` to `PdfDocument`
  * Renamed `AddPdfExtension` to `PushPdfExtension`
- `PdfAppearanceState`: Renamed to `PdfAppearanceStream`
- `PdfMetadata`:
  * `GetTitle()`, `GetAuthor()`, `GetSubject()`, `GetKeywordsRaw()`, `GetCreator()`, `GetProducer()` now return `nullable<const PdfString&>` instead
  * `GetCreationDate()`, `GetModifyDate()` now return `nullable<const PdfDate&>` instead
  * `GetTrapped()` now returns `nullable<bool>` instead
  * `SetTrapped()` now takes `nullable<bool>` instead
  * `GetTrappedRaw()`: removed, you can still access `PdfInfo::GetTrapped()` for a raw version from /Info
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
  * Costructor is now private, create it through `PdfDocument::CreateExtGState(definition)`
  * All methods removed: Retrieve the `PdfExtGStateDefinition` instance
  * Fill opacity -> `PdfExtGStateDefinition::NonStrokingAlpha`
  * Stroke opacity -> `PdfExtGStateDefinition::StrokingAlpha`
  * FillOverprintEnabled, StrokeOverprintEnabled -> `PdfExtGStateDefinition::OverprintControl`
  * NonZeroOverprintEnabled -> `PdfExtGStateDefinition::NonZeroOverprintMode`
  * `SetFrequency()`: Removed, for now. Needs a more extensive HalfTone dictionary support
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
- Moved `PdfCMapEncoding::CreateFromObject` to `PdfEncodingMapFactory::ParseCMapEncoding`
- `PdfNameTree`:
  * Renamed -> `PdfNameTrees`
  * Moved all string tree type functions to the `PdfNameTreeOperations` and make mutable functions private. Cast to that type if you still want to use them
  * `ToDictionary` now takes `std::map<PdfString, PdfObject>` as input
  * `HasValue` -> renamed to `HasKey`
- Renamed enum `PdfColorSpace` -> `PdfColorSpaceType`, `PdfColorSpace` is now a doc element.
- `PdfColor` now it's used just to represent GrayScale, RGB, CMYK colors. Now `PdfColorRaw` is used to supply color components for other color spaces
- `PdfCanvas`:
  * `GetRectRaw()` now returns `Corners` instead of `Rect`
  * Rename `GetStreamForAppending()` -> `GetOrCreateContentsStream()`
  * Removed `GetFromResources`: just use `GetResources`
  * Renamed `HasRotation()` -> `TryGetRotationRadians()`
- `PdfContents`: `Reset()` is now parameterless. It was created to replace the stream. To achieve the same one can do GetStreamForAppending()
and use move semantics on the stream
- `PdfContents`: Rename `GetStreamForAppending()` -> `CreateStreamForAppending()`
- `PdfParserObject::HasStreamToParse()` Make it protected virtual in `PdfObject` (it's unreliable to access it publicly)
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
- `PdfAnnotation`:
  * `GetRectRaw()` now returns `Corners` instead of `Rect`
  * `SetRectRaw()` now takes `Corners` instead of `Rect`
- `PdfAnnotationActionBase`:
    * Setting/Getting action now uses `nullable<PdfAction&>`
- `PdfAnnotationLink`:
    * Setting/Getting destination now uses `nullable<PdfDestination&>`
- `PdfAnnotationFileAttachment`:
    * Setting/Getting filespec now uses `nullable<PdfFilespec&>`
- `PdfRef`, `PdfXRefStream`: Make the constructor internal, `PdfXRefStream` class final
- `PdfSignature`:
   * Removed `SetAppearanceStream`. Use `GetWidget().SetAppearanceStream()` (plus optional `GetWidget().GetOrCreateAppearanceCharacteristics()`, if you needed that) instead
   * `PrepareForSigning()`: Make it internal
- `PdfACtion` hierarchy: make all hierarchy constructors internals and leave classes final
- `PdfField` hierarchy: make all hierarchy leave classes final
- `PdfDataProvider`, `PdfDataContainer`: Make the classes internal
- `PdfEncodingMapOneByte` renamed to `PdfEncodingMapSimple`
- `PdfEncodingMap`, `PdfEncodingMapSimple`, `PdfBuiltInEncoding`, `PdfPredefinedEncoding`, `PdfEncodingMapBase`: make the constructors internal
- `PdfEncodingMap`:
  * Removed `IsBuiltinEncoding`. No replacement, it just told if the font was built-in in Type1 font program. For now it's not expected to be useful
  * `TryGetCodePoints()`, `TryGetNextCodePoints` now takes `CodePointSpan` instead of vector<codepoint>
  * `TryGetExportObject`: Make it private, no public substitute provided
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
- Renamed `PdfMemDocument::Write()` -> `PdfMemDocument::Save()`
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
