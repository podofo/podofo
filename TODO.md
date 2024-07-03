### 1.0

#### API Review
- PdfDocument: Review AttachFile, GetAttachment, AddNamedDestination
- PdfMemDocument: Consider removing SetEncrypt(encrypt)
- PdfWriter: Check if SetEncrypt() should accept mutable reference instead
- Evaluate hide/protect/remove PdfIndirectObjectList: RemoveObject, RemoveObject, ReplaceObject
- Evaluate make private PdfIndirectObjectList:SetCanReuseObjectNumbers
- PdfExtGState accessibility
- PdfField: Evaluate make a virtual getValueObject()
- Evaluate removing PdfObject::Null and PdfVariant::Null and introduce nullptr_t constructor overloads
- PdfCanvas: Add CopyTo facilities, see PdfContents
- PdfPageCollection::CreatePage() with PdfPageSize or default inferred from doc
- PdfPage: Add GetFields() iteration
- PdfDocument: Add GetAnnotationFields()/GetAllFields() iteration
- Review PdfPage::SetICCProfile(), PdfImage::SetICCProfile()
- PdfErrorCode: Check all values
- Review PdfPageCollection::AppendDocumentPages(),
  PdfPageCollection::InsertDocumentPageAt(), PdfPage::MoveAt() (???)
#### Features
- Evaluate adding PdfString(string&&) and PdfName(string&&) constructors that
either assume UTF-8 and/or checks for used codepoints

### After 1.0
- Check/Review doxygen doc
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
- Implement full text extraction, including search in predefined
  CMap(s) as described in Pdf Reference and here https://stackoverflow.com/a/26910569/213871
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
- Add text shaping with Harfbuzz https://github.com/harfbuzz/harfbuzz
- Add fail safe sign/update mechanism, meaning the stream gets trimmed
  to initial length if there's a crash. Not so easy, especially since
  we are now using STL streams and it's not easy to trim files
  without access to native handle and low level I/O operations
- PdfDifferenceEncoding: Rework Adobe Glyph List handling and moving it to private folder
- Option to unfold Unicode ligatures to separate codepoints during encoded -> utf8 conversion
- Option to convert Unicode ligatures <-> separate codepoints when drawing strings/converting to encoded
- Optimize charbuff to not initialize memory, keeping std::string compatibility
- Add backtrace: https://github.com/boostorg/stacktrace

### Ideas:
- PdfFontManager: Consider also statically caching the queries and filepaths.
  Maybe we could also weakly (weak shared pointer) cache metrics instead of fonts
