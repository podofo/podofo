### 1.2.0

- Evaluate restoring API backward compatibility with 1.1.3
- Evaluate avoiding matrix rotation caching in PdfPainter as done in 1.1.x

### After 1.1
- PdfTokenizer: Evaluate making sure data type is set also in case of errors, in DetermineDataType/tryReadDataType. This would help leaving the object operable with the right type even in case of parsing errors. This is done already for dictionaries/arrays, see ReadDictionary/ReadArray
- PdfPredictorDecoder: Support BitsPerComponent != 8
- Cleanup PdfTreeNode
- PdfDifferenceEncoding: Subset the exported /Differences array. The glyph data of Type 3 fonts is already subset
- Optimize compilation SALSprep
- Optimize small allocations in tryGetCodePointsFromCharNameLigatures(),
  tryGetCodePointsFromUnicodeHexLigatures() in PdfDifferenceEncoding.cpp
- PdfEncoding: Evaluate adding move semantics
- PdfVariant/PdfObject: Evaluate adding a TryGetStringLenient(string_view& str)
  that catches both PdfString/PdfName
- Add remaining PdfNameTree(s) (also stub)
- PdfContentStreamOperators: Add the missing F, T*, ', ", d0, d1, BI, ID, EI operators
- According to 5014.CIDFont_Spec page 71-72-73 begincidchar, begincidrange,
  beginbfchar, beginbfrange have a limit of 100 entries. Fix it where relevant
- noexcept nullable<T> methods
- Check/Review doxygen doc
- Evaluate make PdfObjectStream not flate filter by default in PdfMemDocument?
- Evaluate move more utf8::next to utf8::unchecked::next
- PdfToggleButton: Add proper IsChecked/ExportValue handling
- Add version of PdfFont::TryCreateSubstituteFont for rendering
  (metrics/widths of loaded font override metrics found on /FontFile)
- Add a fallback to search font on the system for text extraction purposes,
  see https://github.com/podofo/podofo/issues/123
- Check PdfWriter should really update doc trailer when saving.
  Now the new trailer is written but the doc still has the old one
- PdfMemDocument: Check the DeviceStream is not empty before doing an incremental update/signing operation
- PdfMemDocument: Prevent Save() operation after signing operation
- PdfMemDocument: Evaluate release the device after all objects have been loaded (eg. after a full Save())
- PdfParserObject: Evaluate release the device after loading. NOTE: TryUnload() needs it to reload the object
- PdfElement: Optimize, keep dictionary/array pointer. Evaluate Add shared_ptr PdfElement::GetObjectPtr() 
- Decide what to do with the manuals in man/: they are not installed, as no CMakeLists adds
  the folder, and the tools are unsupported
- Add method to retrieve shared_ptr from PdfObject, PdfFont (and
  maybe others) to possibly outlive document destruction
- Do more overflow checks using Chromium numerics, which is now
  bundled. See comments in utls::DoesMultiplicationOverflow()
- PdfFontManager: Add font hash to cache descriptor
- Add special SetAppearance for PdfSignature respecting
  "Digital Signature Appearances" document specification
- Add text shaping with Harfbuzz https://github.com/harfbuzz/harfbuzz
- Add fail safe sign/update mechanism, meaning the stream gets trimmed
  to initial length if there's a crash. FileStreamDevice now uses low
  level I/O and OutputStreamDevice::Truncate() is available
- Option to unfold Unicode ligatures to separate codepoints during encoded -> utf8 conversion
- Option to convert Unicode ligatures <-> separate codepoints when drawing strings/converting to encoded
- Optimize charbuff to not initialize memory, keeping std::string compatibility,
  see https://en.cppreference.com/w/cpp/string/basic_string/resize_and_overwrite
- Add backtrace: https://github.com/boostorg/stacktrace

### Ideas:
- PdfFontManager: Consider also statically caching the queries and filepaths.
  Maybe we could also weakly (weak shared pointer) cache metrics instead of fonts
- PdfName: Evaluate unescape lazily, or offer a way to debug/inspect the unescaped sequence a posteriori