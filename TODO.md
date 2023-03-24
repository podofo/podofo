### 0.11
- PdfFontManager: Add font hash to cache descriptor
- Add high-level signing API
- Add special SetAppearance for PdfSignature respecting
  "Digital Signature Appearances" document specification

### 1.0
- Restore manuals
- Review all page import functions to check correct working/improve
  the code
- PdfCanvas: Add CopyTo facilities, see PdfContents
- Review PdfNameTree/PdfFileSpec
- Check accessibility of PdfEncrypt.h classes, check AESV3 naming
- PdfFilterFactory: Move CreateFilterList somewhere else (PdfFilter), make it private
- Rename NameToColorSpaceRaw/ColorSpaceToNameRaw to something more consistent?
- More enum <-> strings functions and make them public
- Make PdfObjectStream not flate filter by default in PdfMemDocument?
- PdfElement: Optimize, keep dictionary/array pointer. Add GetObjectPtr()
- PdfPageCollection: Fix caching (it's very slow at the moment)
- PdfPageCollection: Add iteration on PdfPage*. See PdfAnnotationCollection
- PdfPageCollection::CreatePage() with PdfPageSize or default inferred from doc
- PdfPage: Add GetFields() iteration
- PdfDocument: Add GetAnnoationFields()/GetAllFields() iteration
- Fix PdfFontMetrics handling of symbol encoding
- Fix/complete handling of text extraction in rotated pages
- Check PdfWriter should really update doc trailer when saving.
  Now the new trailer is written but the doc still has the old one
- PdfImage: cache PdfColorSpace
- Check PdfSignature to have correct /ByteRange and /Contents
values in the dictionary after signing with SignDocument
- Evaluate move more utf8::next to utf8::unchecked::next
- Add PdfString(string&&) and PdfName(string&&) constructors that
either assume UTF-8 and/or checks for used codepoints
- Added PdfResources::GetResource with enum type
- Add a PdfRect-like class PdfCorners that avoid coordinates normalization
  by default
- Check PdfStreamedDocument working
- Check/Review doxygen doc
- PdfToggleButton: Add proper IsChecked/ExportValue handling
- Review PdfPage::SetICCProfile()
- Review PdfPageCollection::AppendDocumentPages(),
  PdfPageCollection::InsertDocumentPageAt(), PdfPage::MoveAt()
  Add proper text/graphics state stack check/handling
- PdfWriter: Check if SetEncrypt() should accept mutable reference instead
- Add a "on rails" incremental update/sign facilities, so it's more
  clear that either the same file has to be locked and then updated,
  or a buffer is copied from the source file
- Reintroduce other non-unit tests, possibly migrating them into unit ones
- PdfResources: Improve API
- Do more overflow checks using Chromium numerics, which is now
  bundled. See comments in utls::DoesMultiplicationOverflow()
- PdfParser: Handle all pdfs in
  https://www.mail-archive.com/podofo-users@lists.sourceforge.net/msg04801.html

### After 1.0
- PdfParser: Handle invalid startxref by rebuilding the index,
  similarly to what pdf.js does
- high-level signing API: Add PAdES B-B support
- Add text shaping with Harfbuzz https://github.com/harfbuzz/harfbuzz
- Add fail safe sign/update mechanism, meaning the stream gets trimmed
  to initial length if there's a crash. Not so easy, especially since
  we are now using STL streams and it's not easy to trim files
  without access to native handle and low level I/O operations
- Added version of PdfFont::TryGetSubstituteFont for rendering
  (metrics of loaded font override metrics found on /FontFile)
  - Added method to retrieve shared_ptr from PdfObject, PdfFont (and
  maybe others) to possibly outlive document destruction
- PdfDifferenceEncoding: Rework Adobe Glyph List handling and moving it to private folder
- Option to unfold Unicode ligatures to separate codepoints during encoded -> utf8 conversion
- Option to convert Unicode ligatures <-> separate codepoints when drawing strings/converting to encoded
- Optimize charbuff to not initialize memory, keeping std::string compatibility
- Add backtrace: https://github.com/boostorg/stacktrace

### Ideas:
- Consider converting protected PdfFontMetrics::GetFaceHandle() to return just FT_Face,
and reference the face with FT_Reference_Face
- PdfFontManager: Consider also statically caching the queries and filepaths.
  Maybe we could also weakly (weak shared pointer) cache metrics instead of fonts
