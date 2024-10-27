### 1.0

- Optimize passing shared_ptr
- Settle https://github.com/podofo/podofo/issues/165
- PdfPage: Add a method to retrieve rotation in radians
- Settle TryGetSubstituteFont
- Restore PdfShadingPattern, PdfTilingPattern
- Check PdfExtension API
- Check performances of PdfContentStreamReader
- Evaluate make private PdfFontManager::EmbedFonts
- Make PdfDocument constructor private
- PdfOperatorUtils: Move to aux/PdfConver.h most functions
- PdfTokenizer: Evaluate making private IsWhitespace, IsDelimiter,
  IsTokenDelimiter, IsRegular, IsPrintable


### After 1.0
- PdfVariant/PdfObject: Evaluate adding a TryGetStringLenient(string_view& str)
  that catches both PdfString/PdfName
- Add remaining PdfNameTree(s) (also stub)
- Add remaining PdfContentStreamOperators
- According to 5014.CIDFont_Spec page 71-72-73 begincidchar, begincidrange,
  beginbfchar, beginbfrange have a limit of 100 entries. Fix it where relevant
- noexcept nullable<T> methods
- nullable<T&>: Accept T* as well?. Disable nullable<T*>
- Check/Review doxygen doc
- If the doc is updated, then should not allow to set an encryption
- If the doc is updated and has an encryption, it should not allow to remove it
  in a following update
- Evaluate make PdfObjectStream not flate filter by default in PdfMemDocument?
- Evaluate move more utf8::next to utf8::unchecked::next
- Add a PdfRect-like class PdfCorners that avoid coordinates normalization
  by default
- PdfToggleButton: Add proper IsChecked/ExportValue handling
- Add version of PdfFont::TryGetSubstituteFont for rendering
  (metrics/widths of loaded font override metrics found on /FontFile)
- Add a fallback to search font on the system for text extraction purposes,
  see #123
- PdfParser: Handle all pdfs in
  https://www.mail-archive.com/podofo-users@lists.sourceforge.net/msg04801.html
- Check PdfWriter should really update doc trailer when saving.
  Now the new trailer is written but the doc still has the old one
- PdfMemDocument: Check the DeviceStream is not empty before doing an incremental update/signing operation
- PdfMemDocument: Prevent Save() operation after signing operation
- PdfMemDocument: Evaluate release the device after all objects have been loaded (eg. after a full Save())
- PdfParserObject: Evaluate release the device after loading
- Review all page import functions to check correct working/improve the code
- PdfElement: Optimize, keep dictionary/array pointer. Evaluate Add shared_ptr PdfElement::GetObjectPtr() 
- Check what do with tools/restore manuals
- Fix/complete handling of text extraction in rotated pages (??? Done?)
- Add method to retrieve shared_ptr from PdfObject, PdfFont (and
  maybe others) to possibly outlive document destruction
- Do more overflow checks using Chromium numerics, which is now
  bundled. See comments in utls::DoesMultiplicationOverflow()
- PdfFontManager: Add font hash to cache descriptor
- Add special SetAppearance for PdfSignature respecting
  "Digital Signature Appearances" document specification
- PdfParser: Handle invalid startxref by rebuilding the index,
  similarly to what pdf.js does
- Review PdfPageCollection::AppendDocumentPages(),
  PdfPageCollection::InsertDocumentPageAt() code
- Add text shaping with Harfbuzz https://github.com/harfbuzz/harfbuzz
- Add fail safe sign/update mechanism, meaning the stream gets trimmed
  to initial length if there's a crash. Not so easy, especially since
  we are now using STL streams and it's not easy to trim files
  without access to native handle and low level I/O operations
- PdfDifferenceEncoding: Rework Adobe Glyph List handling and moving it to private folder
- Option to unfold Unicode ligatures to separate codepoints during encoded -> utf8 conversion
- Option to convert Unicode ligatures <-> separate codepoints when drawing strings/converting to encoded
- Optimize charbuff to not initialize memory, keeping std::string compatibility,
  see https://en.cppreference.com/w/cpp/string/basic_string/resize_and_overwrite
- Add backtrace: https://github.com/boostorg/stacktrace

### Ideas:
- PdfFontManager: Consider also statically caching the queries and filepaths.
  Maybe we could also weakly (weak shared pointer) cache metrics instead of fonts
- PdfName: Evaluate unescape lazily, or offer a way to debug/inspect the unescaped sequence a posteriori
