/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_PAGES_TREE_H
#define PDF_PAGES_TREE_H

#include "PdfDeclarations.h"

#include "PdfElement.h"
#include "PdfArray.h"
#include "PdfPage.h"

namespace PoDoFo {

/** Class for managing the tree of Pages in a PDF document
 *  Don't use this class directly. Use PdfDocument instead.
 *
 *  \see PdfDocument
 */
class PODOFO_API PdfPageCollection final : public PdfDictionaryElement
{
    friend class PdfDocument;
    friend class PdfPage;

public:
    /** Construct a new PdfPageTree
     */
    PdfPageCollection(PdfDocument& doc);

    /** Construct a PdfPageTree from the root /Pages object
     *  \param pagesRoot pointer to page tree dictionary
     */
    PdfPageCollection(PdfObject& pagesRoot);

    /** Close/down destruct a PdfPageTree
     */
    virtual ~PdfPageCollection();

    /** Return the number of pages in document
     *  \returns number of pages
     */
    unsigned GetCount() const;

    /** Return a PdfPage for the specified Page index
     *  The returned page is owned by the pages tree and
     *  deleted along with it.
     *
     *  \param index page index, 0-based
     *  \returns a pointer to the requested page
     */
    PdfPage& GetPageAt(unsigned index);
    const PdfPage& GetPageAt(unsigned index) const;

    /** Return a PdfPage for the specified Page reference.
     *  The returned page is owned by the pages tree and
     *  deleted along with it.
     *
     *  \param ref the reference of the pages object
     *  \returns a pointer to the requested page
     */
    PdfPage& GetPage(const PdfReference& ref);
    const PdfPage& GetPage(const PdfReference& ref) const;

    /** Creates a new page object and inserts it into the internal
     *  page tree.
     *  The returned page is owned by the pages tree and will get deleted along
     *  with it!
     *
     *  \param size a Rect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \returns a pointer to a PdfPage object
     */
    PdfPage& CreatePage(const nullable<Rect>& size = nullptr);
    PdfPage& CreatePage(PdfPageSize pageSize);

    /** Creates a new page object and inserts it at index atIndex.
     *  The returned page is owned by the pages tree and will get deleted along
     *  with it!
     *
     *  \param size a Rect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \param atIndex index where to insert the new page (0-based)
     *  \returns a pointer to a PdfPage object
     */
    PdfPage& CreatePageAt(unsigned atIndex, const nullable<Rect>& size = nullptr);
    PdfPage& CreatePageAt(unsigned atIndex, PdfPageSize pageSize);

    /** Create count new page objects and insert at the index atIndex. This is significantly faster
     *  than calling CreatePageAt repeatedly.
     *
     *  \param size a Rect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \param count number of pages to create
     *  \param atIndex index where to insert the new page (0-based)
     */
    void CreatePagesAt(unsigned atIndex, unsigned count, const nullable<Rect>& size = nullptr);
    void CreatePagesAt(unsigned atIndex, unsigned count, PdfPageSize pageSize);

    /** Appends another PdfDocument to this document.
     *  \param doc the document to append
     */
    void AppendDocumentPages(const PdfDocument& doc);

    /** Copies one or more pages from another PdfMemDocument to this document
     *  \param doc the document to append
     *  \param atIndex the first page number to copy (0-based)
     *  \param pageCount the number of pages to copy
     */
    void AppendDocumentPages(const PdfDocument& doc, unsigned pageIndex, unsigned pageCount);

    /** Inserts existing page from another PdfDocument to this document.
     *  \param atIndex index at which to add the page in this document
     *  \param doc the document to append from
     *  \param pageIndex index of page to append from doc
     */
    void InsertDocumentPageAt(unsigned atIndex, const PdfDocument& doc, unsigned pageIndex);

    /**  Delete the specified page object from the internal pages tree.
     *   It does NOT remove any PdfObjects from memory - just the reference from the tree
     *
     *   \param atIndex the page number (0-based) to be removed
     *
     *   The PdfPage object referring to this page will be deleted by this call!
     *   Empty page nodes will also be deleted.
     *
     *   \see PdfMemDocument::DeletePages
     */
    void RemovePageAt(unsigned atIndex);

    /**  Flatten the document page structure tree
     *
     * This copy pages inheritable attributes and remove intermediate /Pages nodes.
     * This operation is allowed by the PDF specification, see "ISO 32000-2:2020, 7.7.3.2 Page tree nodes"
     */
    void FlattenStructure();

public:
    template <typename TObject, typename TListIterator>
    class Iterator final
    {
        friend class PdfPageCollection;
    public:
        using difference_type = void;
        using value_type = TObject*;
        using pointer = void;
        using reference = void;
        using iterator_category = std::forward_iterator_tag;
    public:
        Iterator() { }
    private:
        Iterator(const TListIterator& iterator) : m_iterator(iterator) { }
    public:
        Iterator(const Iterator&) = default;
        Iterator& operator=(const Iterator&) = default;
        bool operator==(const Iterator& rhs) const
        {
            return m_iterator == rhs.m_iterator;
        }
        bool operator!=(const Iterator& rhs) const
        {
            return m_iterator != rhs.m_iterator;
        }
        Iterator& operator++()
        {
            m_iterator++;
            return *this;
        }
        Iterator operator++(int)
        {
            auto copy = *this;
            m_iterator++;
            return copy;
        }
        value_type operator*()
        {
            return *m_iterator;
        }
        value_type operator->()
        {
            return *m_iterator;
        }
    private:
        TListIterator m_iterator;
    };

    using PageList = std::vector<PdfPage*>;

    using iterator = Iterator<PdfPage, PageList::iterator>;
    using const_iterator = Iterator<const PdfPage, PageList::const_iterator>;

    public:
        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;

private:
    /**
     * Insert page at the given index
     * \remarks Can be used by PdfDocument
     */
    void InsertPageAt(unsigned atIndex, PdfPage& page);

    /**
     * Insert pages at the given index
     * \remarks Can be used by PdfDocument
     */
    void InsertPagesAt(unsigned atIndex, cspan<PdfPage*> pages);

    bool TryMovePageTo(unsigned atIndex, unsigned toIndex);

private:
    void insertPageAt(unsigned atIndex, PdfPage& page);
    void insertPagesAt(unsigned atIndex, cspan<PdfPage*> pages);
    Rect getActualRect(const nullable<Rect>& size);

    PdfPage& getPage(const PdfReference& ref) const;

    void initPages();

    unsigned traversePageTreeNode(PdfObject& obj, unsigned count,
        std::vector<PdfObject*>& parents, std::unordered_set<PdfObject*>& visitedNodes);

    PdfPageCollection(PdfPageCollection&) = delete;
    PdfPageCollection& operator=(PdfPageCollection&) = delete;

private:
    bool m_initialized;
    PageList m_Pages;
    PdfArray* m_kidsArray;
};

};

#endif // PDF_PAGES_TREE_H
