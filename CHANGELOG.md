## Version 1.1.0-dev
- Added support for ECDSA encryption

## Version 1.0.1
- Fixed #265, #264
- Fixed several issues related to use of `nullable<std::unique_ptr<T>>`
- XMP: Fixed removing extension from extension bag
- XMP: Fix double inserting pdfuaid schema
- Text extraction fixes: improved word spacing heuristic, space trimming in ligatures, text in XObject form with non identity matrix
- Fixed pkg-config for shared built library

## Version 1.0.0

- Added support for Type1, CFF and OpenType CFF font subsetting
- Added support for Type3 font subsetting (no encoding subsetting, yet)
- Implemented full "Adobe Glyph List" specification (https://github.com/adobe-type-tools/agl-specification) for text extraction and glyph selection
- Implemented full Type1, TrueType font glyph selection
- Many fixes in TrueType legacy subsetting
- Added support for PDF/UA preserving when adding annotations/form fields
- Improved PDF/A preserving (eg. when PDF/UA level is set as well in the XMP metadata)
- Added support for automatically rotating imported images drawn on a `PdfPainter`
  when a orientation is detected in the source image codec metadata. Currently supported
  on Tiff images only (Jpeg exif metadata support pending)
- Added high-level signing API, see `PdfSignerCMS` and [`TestSignature1`](https://github.com/podofo/podofo/blob/edbcb16a5b18cb20f1d0da1724639cee13608436/test/unit/SignatureTest.cpp#L37) test case
- Added support for signing encrypted documents
- Added support for preserving encryption among savings
- Removed Libidn dependency, default to AESV3R6 encryption
- Added support for predefined CMap(s) for improved CJK text extraction
- Added much better CMake [integration](https://github.com/podofo/podofo?tab=readme-ov-file#consume-podofo-from-package-managers-with-cmake)
- Added raw rectangle corners retrieval in `PdfAnnotation`, `PdfPage`
- `PdfDocument`: Added `GetFieldsIterator()`
- `PdfPage`: Added `GetFieldsIterator()`
- `PdfSignature`: Added `TryGetPreviousRevision()`
- `PdfCanvas`: Added `CopyContentsTo()`
- `FileStreamDevice` now uses again C stdio for better performance
- `PdfName`:
  * Optimized for struct size and construction from string const literal
  * Added `PdfName operator""_nm(const char*, size_t)`
- `PdfString`:
  * Optimized for struct size
  * Added `std::string&&` constructor
- `PdfVariant`: Optimized for accessing `PdfString`, `PdfName` and `PdfReference`
- Reviewed `PdfFileSpec`, `PdfAction`, `PdfDestination` API and their usage in
`PdfOutlineItem`, `PdfOutlines`, `PdfAnnotationActionBase`, `PdfAnnotationLink`, `PdfAnnotationFileAttachment`
- Reviewed `PdfExtension` API
- Reviewed `PdfNameTree`, renamed to `PdfNameTrees` and added `PdfNameTree` to pick specific trees with typed element
- Reviewed `PdfExtGState`
- Reviewed `PdfTilingPattern`, `PdfShadingPattern`, `PdfFunction`: the API now exposes the full capabilities of the PDF specification
- `PdfEncrypt` is now stateless: added `PdfEncryptContext` as a
   separate state context and used as argument in `PdfEncrypt` methods
- Set `PdfSignature` to have correct `/ByteRange` and `/Contents` after signing with `PoDoFo::SignDocument`
- Added `PdfNames` and moved all known names there from `PdfName`
- `PdfPageCollection`: Methods creating pages now takes `PdfPageSize` or default inferred size from doc
- Fixed `PdfStreamedDocument`, see #88
- Tons of API improvements (see [API-MIGRATION.md](https://github.com/podofo/podofo/blob/master/API-MIGRATION.md))
- Tons of other bug fixes

## Version 0.10.5
- Fix #191, #197, #201, #212, #233, #241, #251, #252, #253
- PdfParser: Fixed stack overflow parsing documents with many XRef stream updates
- PdfFont: Fixed GetBoundingBox() retrival
- PdfFontMetricsObject: Fixed reading /FontBBox
- PdfEncodingFactory: Fixed parsing of limits with /FirstChar equals to /LastChar
- PdfFontMetricsStandard14: Fixed parsing /Widths
- PdfMetadata: Fixed missing init ensure for SetAuthor()
- PdfTokenizer: Fixed character escaping when reading strings
- PdfPageCollection: Fix memory leak in RemovePageAt
- Compilation and linking fixes in various conditions
- PdfFontManager: Fixed GetOrCreateFontFromBuffer stealing memory
- PdfPageCollection: Disable copy/assignment
- PdfPage_TextExtraction: Fix `decodeString` with no font
- Fix eating of non-space chars in SplitTextAsLines
- Fix FreeType segfault race condition
- PdfCheckBox: Fixed IsChecked()
- PdfParser: Uncondtionally try to read XRef stream in all PDFs that doesn't have a cross reference section

## Version 0.10.4
- Fixes #161, #162, #167, #183, merges #157
- `StandardStreamDevice`: Fixed `seek()` in case of `iostream`/`fstream`
- `PdfWriter`: Fixed computing the doc identifier with a wrong buffer
- `PdfPainter`: Fix `SetCurrentMatrix()` to really update CTM
- Fixed compilation in mingw < 12
- `PdfCIDToGIDMap`: Fixed map reading
- `PdfPainter`: Fixed offset on multiline text if text is not left aligned

## Version 0.10.3
- Fixed big performance regression introduced in 0.10, see #108
- Fixed data loss with encrypted documents, see #99
- Fixed compilation with VS2022 >= 17.8
- Fixed compilation using libxml >= 2.12.0

## Version 0.10.2
- Security related bugfixes #76, #89, #96
- Some compilation and test fixes

## Version 0.10.1
- Security bugfixes, #66, #67, #69, #70, #71, #72
- Rewritten `PdfPageCollection` for performance
- `PdfCMapEncoding`: Fix parsing some invalid CMap(s) supported by Acrobat
- `PdfXRefStreamParserObject`: Fixed handling of invalid XRef stream entries
- Support compilation of the library header (not the library itself) with C++20

## Version 0.10.0
- `PdfPage`/`PdfAnnotationCollection`/`PdfAnnotation`: Now functions with
  rect input assume it to be using the canonical coordinate system
  with no rotation
- `PdfImage`: Added support for CYMK jpeg
- `PdfParser`: Cleaned `FindToken2` -> `FindTokenBackward`
- Renamed base source folder -> main
- `PdfPainter`: Revamped API, added full state inspection with current point,
  added added `PdfPainterTextContext` to handle text object operations
  Use it with `PdfPainter::Text` instance member.
  Added `PdfContentStreamOperators` low level interface for PdfPainter
- `PdfFontMetrics`: Added `FilePath`/`FaceIndex` for debugging, when available
- `PdfFont`: Renamed `GetStringLength()` overloads with
  `PdfString` to `GetEncodedStringLength()`
- `PdfFontManager`: Renamed `GetFont()` -> `SearchFont()`
  Re-Added better `GetOrCreateFont()` from file/buffer
- `PdfEncrypt`: Cleaned factory methods
- Added `PdfArray::FindAtAs()`, `PdfArray::FindAtAsSafe()`, `PdfArray::TryFindAtAs()`,
  `PdfArray::GetAtAs()`, `PdfArray::GetAtAsSafe()`, `PdfArray::TryGetAtAs()`
- Added `PdfDictionary::FindKeyAsSafe()` and `PdfDictionary::TryFindKeyAs()`
- `PdfDictionary::AddKeyIndirect`/`PdfArray::AddKeyIndirect` accepts a reference
- `PdfAnnotation`/`PdfField` API review
- `PdfDate`: Introduced `PdfDate::LocalNow()` and `PdfDate::UtcNow()`
  and default constructor is epoch time instead
- Renamed `PdfDocument::GetNameTree()` -> `GetNames()`
- `PdfObject`: Flate compress on write objects that have no filters
- `PdfMemDocument` does collect garbage by default when saving
- `PdfField`/`PdfAnntation`: Fully reworked the hierarchy
  and added proper fields ownership
- Added `PdfField::GetParent()`, `PdfField::GetChildren()`
- `PdfImage`: Cleaned/reviewed/fixed `SetData()`/`SetDataRaw()`
- Renamed `PdfPageTree` -> `PdfPageCollection`
- Added XMP metadata reading/saving. Added `PdfMetadata` class
- Added text extraction API
- Review I/O API: Merged `InputDevice`/`OutputDevice` into `StreamDevice`.
  New hierarchy deriving `StreamDevice`
- Reviewed `PdfObjectStream` API: added streaming operations,
  `GetInputStream()`, `GetOutputStream()`. Renamed
  `GetFilteredCopy()` -> `GetUnwrappedCopy()`/`UnwrapTo()`.
  They only unwrap non media filters (see `PdfImage::DecodeTo`
  for media ones). Added proper copy and move assignment operators
- `PdfImage`: Added `DecodeTo(pixelFormat)`

## Version 0.9.22 (pdfmm)
- Fixed serialization of strings with non ASCII PdfDocEncoding
  characters
- Removed PdfLocaleImbue
- PdfEncrypt: Removed PdfReference state. Added PdfStatefulEncrypt
- Removed use of std::ostringstream. Added efficient outstringstream
- Added PdfMath functionalities (matrix transformations and so on)

## Version 0.9.21 (pdfmm)
- Fixed serialization of UTF-16BE strings
- More lenient PdfDate parsing

## Version 0.9.20 (pdfmm)

- The project is now a C++17 library
- Added move semantics for PdfVariant, PdfObject, PdfArray, PdfDictionary
- Improved XRefStream support, added support in incremental saves
- Many fixes in save incremental object/generation number incrementing
- String backed with UTF-8 storage
- PdfName backed with UTF-8 storage
- Brand new PdfEncoding class with support for both /Encoding and /ToUnicode,
  more complete Unicode support
- Added a PdfDynamicEncoding class that creates a custom CID encoding
  based on actual used glyphs used
- Automatic creation of CIDMap and /ToUnicode
- Added PdfSigner class and SignDocument()
- Added PdfFontType1Encoding, which support Type1 implicit encoding
- Added support for PDF 2.0 UTF-8 strings (untested)
- Added indirect iteration for PdfArray/PdfDictionary (see GetIndirectIterator methods)
- Added PdfDocument::GetPdfALevel()
- Added PDFA preserving writing
- Refactored/Reviewed PdfInputDevice: versions that take buffer
  do not copy it (use istringviewstream)
- Added font replacement facility PdfFont::TryCreateFontSubstitute()
- Added standard14 fonts embedding, with font programs from PDFium
- Reviewed PdfXObject hierarchy, added PdfXObjectForm, PdfXObjectPostScript
- Added PdfTextState and use it to compute string widths in PdfFont
- Improved PdfDocEncoding to expose conversion utf8 conversion facilities
- PdfParser: Support also files with whitespace offset before magic start
- PdfObject auto ownership
- PdfContents: create on demand /Contents. First create a single stream, after array
- Improved IsDirty handling: less dirty bit sets
- Added PdfPostScriptTokenizer that as better general support for PostScript
- PdfDictionary: Review/convert GetKey -> FindKey
- PdfDictionary: Review GetKeyAs methods
- Fixed hundreds of warnings. No warnings left in tested builds
- FontConfigManager: better handling with custom configurations
- Removed PdfMutex. Used std::mutex where necessary
- Datatypte: remove PdfDataType::HexString
- Removed support for old compilers (MSVC6, hpux, borland, turbo...)
- Added better endian swap functions
- Removed auto_ptr usage
- Removed pdf_int/pdf_uint types
- Removed use of pdf_long/long types
- Removed use of ptrdiff_t
- Removed unistring and ugly string conversion code. Moved to utfcpp
- Reviewed PdfObject::GetNumber/PdfObject::GetReal (strict/lenient)
- Remove PdfObject inheritance on PdfVariant
- Reviewed PdfVariant/PdfObject/PdfArray/PdfDictionary equality/disequality operators
- Simplified copyright headers
- Object copy constructor must copy also stream
- PdfElement: GetDocument(), GetObject() refs
- PdfVecObjects: GetParentDocument() -> GetDocument()
- Remove PdfSignOutputDevice::SetSignatureSize(size)
- Moved inline code to .cpp
- Remove comments on overrides
- Cleaned CMakeFiles (removed custom Find<>.cmake)
- NULL -> nullptr
- Use shared_ptr in PdfFontMetrics, PdfEncoding in PdfFont
- Cleaned PdfFontCache (renamed PdfFontManager), removed font functions from PdfDocument
- PdfArray::FindAt() return ref
- Simplified license headers
- Reviewed PdfPageTree and PdfPageTreeCache API
- Reviewed most int vs unsigned indexing
- Remove all hungarian notation
- Sanitize code style
- Removed all const char* and passed to string/string_view
- Renamed PdfFontCache -> PdfFontManager
- Renamed PdfVecObjects -> PdfIndirectObjectList
- Renamed PdfNamesTree -> PdfNameTree
- Renamed PdfPagesTree -> PdfPageTree
- Renamed PdfPagesTreeCache -> PdfPageTreeCache
- PdfObject: removed GetIndirectObject(), MustGetIndirectKey (must use PdfDictionary now)
- Remove PdfMemoryManagement.cpp, Removed podofo_new, podofo_free
- Add chars type for char array storage/buffering which inerits string
- Review pointer vs ref parameters/return types
- Remove PdfRefCountedBuffer, PdfRefCountedInputDevice, PdfRefCountedOutputDevice
- Removed PdfMemoryManagement and all C style malloc/free usage
- Refactored PdfOutputDevice (PdfMemoryOutputDevice/PdfFileOutputDevice/etc.)
- Clean PdfError, remove wchar_t
- Remove printf, snprintf
- Renamed PdfElement-> {PdfDictionaryElement|PdfArrayElement} that
  respectively have GetDictionary(), GetArray()
- Reviwed PdfWriteFlags, added PdfSaveOptions

## Old PoDoFo ChangeLog

Version 0.7

- Fixed PdfPainter::ArcTo
- Fixed crash in FlateDecode and LZWDecode predictor functions
- Fixed writing of unicode PdfStrings with brackets (will be escaped
  now)
- Fixed encoding of bytes in PdfName
- Added many new unit tests
- Added methods to free object memory of objects that are not needed
- anymore (These objects will be re-read from disk if they are
  needed again)
- Fixed a crash when appending PDFs
- Added unicode support on Win32 (wchar_t* constructors and methods
- where apropriate, e.g. file handling)
- Fixed a memory leak in PdfStream
- Small optimizations in various places
- Fixed DCTDecode filter for CMYK images
- PdfReference is now immutable, which allows for various optimizations
- Fixed PdfInputDevice::Read to return now the correct number of bytes
  read
- Fixed a memory leak in PdfImmediateWriter/PdfStreamedDocument
- Fixed several minor parsing issues
- Fixed adding text to existing pages
- Added Lua5.1 plan file support to podofoimpose
- Several fixes in creating XObjects from pages
- Fixed a possible crash in PdfNamesTree
- Added support for setting colors in PdfAnnotations
- Added advanced text drawing support to PdfPainter
- Fixed parsing of Type1 fonts
- Fixed deletion of PdfAnnations (fixes a memory leak)

Version 0.5

- Added support for Embedded Files (annotations & named objects)
- Added support for ExtGStates when drawing
  initially only supports basic transparency
- Fixed reading values from nametrees
- Added support for named destinations
- Fixed a memory leak in PdfDestination::GetPage
- Pages do not know their page number inside of the document
- Fixed reading PdfActions from PDF files
- Moved filter implementations to PdfFiltersPrivate.h
- Added PdfFilter::CanEncode and PdfFilter::CanDecode
- Simpliefid PoDoFos handling of XRef tables

Version 0.4

- PdfImage now supports creating an image stream from a "raw bitmap"
  which can also be optionally Flate compressed
- Added some new Page-related methods to PdfDocument:
    * Append - append one document to another
    * InsertPages - insert a range of pages from one document to another
    * DeletePages - delete a range of pages
- Added new tool podofomerge for merging two PDFs together
- Added methods to get & set a document's PageMode
- Added methods to set a document's FullScreen mode
- Added methods to set all the various ViewerPreferences for a document
- Added methods to set the document's PageLayout
- Added Outline support
  modified podofoinfo to display them, if present
- Added a PdfDestination class
- Added PdfNamesTree class for handling the global named objects
  modified podofoinfo to display them, if present
- PdfPainter can draw bezier curves
- Added XCode project for building on Mac OS X
  fixed up conditionals in font code to enable building on
  MacOSX - but no font loading, YET
- Added support for writing linearized PDF
- Added support for garbage collection of unused objects
- Simplified PdfObject constructor
- Improved annotation support
- Added support to encode names and various name testcases
- Fixed ascent and descent of fonts
- Improved PdfImage API
- Added support for the creation of file identifiers which makes
  PoDoFo created PDF files work in more different PDF
  workflows
- PdfImage optionally takes ownership of buffers
- Fixed a major parser bug: The parser expected all objects in
  object streams to be of type dictionary.

Version 0.3
- TOTAL revamp of PdfObject & PdfVariant to enable clean/consistent object handling;
  Added new PdfDocument object - new high level object for reading & writing a PDF doc;
- TOTAL revamp of the PdfDocument, PdfWriter & PdfParser relationship
  PdfDocument is now hub for both reading and writing a document
  it holds the PdfVecObjects - the others just reference them.
- TOTAL revamp of PdfPainter
    * now uses PDF coordinates - UserUnits from bottom/left
      added PdfPainterMM for mm-based coords
    * supports user-specified float precision
      and writes out floats in an optimal manner
    * supports "appending" mode for drawing on existing documents
- Improved handling of the /Info dict for both reading and writing PDFs;
- Added new test app - podofopdfinfo, which will be used to dump metadata, etc. from a PDF;
- Added PdfError::DebugMessage() as the official way to write out debugging info;
  updated all other debugging msgs to use this;
- Added PdfError::DebugEnabled() to enable/disable display of debug messages;
- Added tracking of file size in PdfParser;
- Minor tweak to Linearization handling - to enable getting the status from a doc;
- Added getting GetPdfVersionAsString() to PdfWriter;
- Added new info/object getting methods to PdfDocument:
    * bool IsLinearized();
    * size_t FileSize();
    * PdfObject* GetStructTreeRoot();
    * PdfObject* GetMetadata();
    * PdfObject* GetOutlines();
    * PdfObject* GetAcroForm().
- Updated pdfinfo & podofopdfinfo to call the new PdfDocument methods;
- Added PdfDictionary and PdfArray classes;
- Added new PdfPagesTree (inside of PdfDocument.cpp) for handling walking a /Pages tree;
- Added new GetPageCount() method to PdfDocument;
- Modifications to PdfPage to attach it to a PdfDocument & construct from a PdfObject;
- Added new Legal and A3 standard page sizes;
- Changed page coordinates to be PDF unit-based instead of 1/1000mm;
- Changed PdfRect to use PDF units and also use bottom instead of top;
- Added ability to go between PdfRect and PdfArray & also get string version of a PdfRect;
- Added support for PdfPage to return all the standard boxes (Media, Crop, etc);
- Added support for fetching inherited values from pages (eg. boxes, rotation, etc.)
- Added more methods to PdfPage:
    * GetRotation();
    * GetNumAnnots().
- Use Exceptions now instead of error codes;
- Removed Init from PdfOutputDevice;
- Removed Init from PdfParser;
- Added LZW Filter support;
- Added PdfElement as base class for PdfAction, PdfAnnotation and
- PdfPage;
- Fixed podofoimageextract, podofotxt2pdf and podofopdfinfo;
- Removed PdfSimpleWriter in favour of PdfDocument;
- Headers are now installed in includedir/podofo/;
- Added a new WatermarkTest
  demonstrates how to read an existing PDF and draw on each page.

Version 0.2

- Improved Documentation;
- Added SetInformation for additional error information to PdfError;
- Fixed the underline color of text;
- Introduced PdfReference class;
- Fixed PdfStream::GetFilteredCopy;
- Improved handling of DecodeParms for filters;
- Fixed PDF files with more than one DecodeParms dictionary
    in one object;
- Added on demand loading of objects to the PdfParser;
- Ported to windows by Leonard Rosenthol;
- On demand loading of objects is now the default;
- Refactored PdfFilter interface so that filters are cached;
- Fixed multiple connected XRef tables through /Prev keys in the trailer;
- Fixed a number of compiler warnings;
- Replaced char*'s with std::strings in a number of classes;
- Added std::ostream support to PdfOutputDevice;
- Improvements to the ImageExtractor tool;
- Refactored PdfVariant so that it is easier to use.

Version 0.1 (11 June 2006)

- Initial release.
