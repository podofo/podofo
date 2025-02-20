/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_OUTLINE_H
#define PDF_OUTLINE_H

#include "PdfElement.h"
#include "PdfColor.h"

namespace PoDoFo {

class PdfDestination;
class PdfAction;

/**
 * The title of an outline item can be displayed
 * in different formatting styles since PDF 1.4.
 */
enum class PdfOutlineFormat : uint8_t
{
    Default = 0,      ///< Default format
    Italic = 1,       ///< Italic
    Bold = 2,         ///< Bold
    BoldItalic = 3,   ///< Bold Italic
};

/**
 * A PDF outline item has an title and a destination.
 * It is an element in the documents outline which shows
 * its hierarchical structure.
 *
 * \see PdfDocument
 * \see PdfOutlines
 * \see PdfDestination
 */
class PODOFO_API PdfOutlineItem : public PdfDictionaryElement
{
    friend class PdfOutlines;
    friend class PdfDocument;

private:
    /** Create a new PdfOutlineItem dictionary
     * \param doc parent document
     * \param parentOutline parent of this outline item
     *      in the outline item hierarchy
     */
    PdfOutlineItem(PdfDocument& doc, PdfOutlineItem* parentOutline = nullptr);

    /** Create a PdfOutlineItem from an existing PdfObject
     *  \param obj an existing outline item
     *  \param parentOutline parent of this outline item
     *                        in the outline item hierarchy
     *  \param previous previous item of this item
     */
    PdfOutlineItem(PdfObject& obj, PdfOutlineItem* parentOutline, PdfOutlineItem* previous);

    PdfOutlineItem(const PdfOutlineItem&) = delete;

public:
    virtual ~PdfOutlineItem();

    /** Create a PdfOutlineItem that is on the same level and follows the current item.
     *  \param title title of this item
     */
    PdfOutlineItem& CreateChild(const PdfString& title);

    /** Create a PdfOutlineItem that is on the same level and follows the current item.
     *  \param title title of this item
     */
    PdfOutlineItem& CreateNext(const PdfString& title);

    /**
     * \returns the previous item or nullptr if this is the first on the current level
     */
    inline PdfOutlineItem* Prev() const { return m_Prev; }

    /**
     * \returns the next item or nullptr if this is the last on the current level
     */
    inline PdfOutlineItem* Next() const { return m_Next; }

    /**
     * \returns the first outline item that is a child of this item
     */
    inline PdfOutlineItem* First() const { return m_First; }

    /**
     * \returns the last outline item that is a child of this item
     */
    inline PdfOutlineItem* Last() const { return m_Last; }

    /**
     * \returns the parent item of this item or nullptr if it is
     *          the top level outlines dictionary
     */
    inline PdfOutlineItem* GetParentOutline() const { return m_ParentOutline; }

    /** Deletes this outline item and all its children from
     *  the outline hierarchy and removes all objects from
     *  the list of PdfObjects
     *  All pointers to this item will be invalid after this function
     *  call.
     */
    void Erase();

    /** Set the destination of this outline.
     *  \param dest the destination
     */
    void SetDestination(nullable<const PdfDestination&> dest);

    /** Get the destination of this outline.
     *  \returns the destination, if there is one, or nullptr
     */
    nullable<const PdfDestination&> GetDestination() const;
    nullable<PdfDestination&> GetDestination();

    /** Set the action of this outline.
     *  \param action the action
     */
    void SetAction(nullable<const PdfAction&> action);

    /** Get the action of this outline.
     *  \returns the action, if there is one, or nullptr
     */
    nullable<const PdfAction&> GetAction() const;
    nullable<PdfAction&> GetAction();

    /** Set the title of this outline item
     *  \param title the title to use
     */
    void SetTitle(const PdfString& title);

    /** Get the title of this item
     *  \returns the title as PdfString
     */
    const PdfString& GetTitle() const;

    /** Set the text format of the title.
     *  Supported since PDF 1.4.
     *
     *  \param format the formatting options
     *                 for the title
     */
    void SetTextFormat(PdfOutlineFormat format);

    /** Get the text format of the title
     *  \returns the text format of the title
     */
    PdfOutlineFormat GetTextFormat() const;

    /** Set the color of the title of this item.
     *  This property is supported since PDF 1.4.
     *  \param color the RGB color that should be set
     */
    void SetTextColor(const PdfColor& color);

    /** Get the color of the title of this item.
     *  Supported since PDF 1.4.
     *
     *  \see SetTextColor
     */
    PdfColor GetTextColor() const;

private:
    /** Inserts a new PdfOutlineItem as a child of this outline item.
     *  The former can't be in the same tree as this one, as the tree property
     *  would be broken. If this prerequisite is violated, a PdfError
     *  exception (code PdfErrorCode::OutlineItemAlreadyPresent) is thrown and
     *  nothing is changed.
     *  The item inserted is not copied, i.e. Erase() calls affect the original!
     *  Therefore also shared ownership is in effect, i.e. deletion by where it
     *  comes from damages the data structure it's inserted into.
     *
     *  \param item an existing outline item
     */
    void InsertChild(std::unique_ptr<PdfOutlineItem> item);

    nullable<PdfAction&> getAction();
    nullable<PdfDestination&> getDestination();
    void setPrevious(PdfOutlineItem* item);
    void setNext(PdfOutlineItem* item);
    void setLast(PdfOutlineItem* item);
    void setFirst(PdfOutlineItem* item);
    void insertChildInternal(PdfOutlineItem* item, bool checkParent);

private:
    PdfOutlineItem* m_ParentOutline;

    PdfOutlineItem* m_Prev;
    PdfOutlineItem* m_Next;

    PdfOutlineItem* m_First;
    PdfOutlineItem* m_Last;

    nullable<std::unique_ptr<PdfDestination>> m_Destination;
    nullable<std::unique_ptr<PdfAction>> m_Action;
};

/** The main PDF outlines dictionary.
 *
 *  Normally accessible through PdfDocument::GetOutlines()
 *
 *  \see PdfDocument
 */
class PODOFO_API PdfOutlines final : public PdfOutlineItem
{
    friend class PdfDocument;

private:
    /** Create a new PDF outlines dictionary
     */
    PdfOutlines(PdfDocument& doc);

    /** Create a PDF outlines object from an existing dictionary
     *  \param obj an existing outlines dictionary
     */
    PdfOutlines(PdfObject& obj);

public:
    /** Create the root node of the
     *  outline item tree.
     *
     *  \param title the title of the root node
     */
    PdfOutlineItem& CreateRoot(const PdfString& title);
};

};

#endif // PDF_OUTLINE_H
