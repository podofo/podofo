/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_OUTLINE_H
#define PDF_OUTLINE_H

#include "PdfDeclarations.h"

#include "PdfElement.h"

namespace PoDoFo {

class PdfDestination;
class PdfAction;
class PdfObject;
class PdfOutlineItem;
class PdfString;
class PdfIndirectObjectList;

/**
 * The title of an outline item can be displayed
 * in different formating styles since PDF 1.4.
 */
enum class PdfOutlineFormat
{
    Default = 0x00,      ///< Default format
    Italic = 0x01,       ///< Italic
    Bold = 0x02,         ///< Bold
    BoldItalic = 0x03,   ///< Bold Italic
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
public:
    virtual ~PdfOutlineItem();

    /** Create a PdfOutlineItem that is a child of this item
     *  \param title title of this item
     *  \param dest destination of this item
     */
    PdfOutlineItem* CreateChild(const PdfString& title, const std::shared_ptr<PdfDestination>& dest);

    /** Create a PdfOutlineItem that is on the same level and follows the current item.
     *  \param title title of this item
     *  \param dest destination of this item
     */
    PdfOutlineItem* CreateNext(const PdfString& title, const std::shared_ptr<PdfDestination>& dest);

    /** Create a PdfOutlineItem that is on the same level and follows the current item.
     *  \param title title of this item
     *  \param action action of this item
     */
    PdfOutlineItem* CreateNext(const PdfString& title, const std::shared_ptr<PdfAction>& action);

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
    void InsertChild(PdfOutlineItem* item);

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
    void SetDestination(const std::shared_ptr<PdfDestination>& dest);

    /** Get the destination of this outline.
     *  \returns the destination, if there is one, or nullptr
     */
    std::shared_ptr<PdfDestination> GetDestination() const;

    /** Set the action of this outline.
     *  \param action the action
     */
    void SetAction(const std::shared_ptr<PdfAction>& action);

    /** Get the action of this outline.
     *  \returns the action, if there is one, or nullptr
     */
    std::shared_ptr<PdfAction> GetAction() const;

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
     *  \param r red color component
     *  \param g green color component
     *  \param b blue color component
     */
    void SetTextColor(double r, double g, double b);

    /** Get the color of the title of this item.
     *  Supported since PDF 1.4.
     *  \returns the red color component
     *
     *  \see SetTextColor
     */
    double GetTextColorRed() const;

    /** Get the color of the title of this item.
     *  Supported since PDF 1.4.
     *  \returns the red color component
     *
     *  \see SetTextColor
     */
    double GetTextColorBlue() const;

    /** Get the color of the title of this item.
     *  Supported since PDF 1.4.
     *  \returns the red color component
     *
     *  \see SetTextColor
     */
    double GetTextColorGreen() const;

private:
    std::shared_ptr<PdfAction> getAction();
    std::shared_ptr<PdfDestination> getDestination();
    void SetPrevious(PdfOutlineItem* item);
    void SetNext(PdfOutlineItem* item);
    void SetLast(PdfOutlineItem* item);
    void SetFirst(PdfOutlineItem* item);

    void InsertChildInternal(PdfOutlineItem* item, bool checkParent);

protected:
    /** Create a new PdfOutlineItem dictionary
     *  \param parent parent vector of objects
     */
    PdfOutlineItem(PdfDocument& doc);

    /** Create a new PdfOutlineItem from scratch
     *  \param title title of this item
     *  \param dest destination of this item
     *  \param parentOutline parent of this outline item
     *                        in the outline item hierarchie
     *  \param parent parent vector of objects which is required
     *                 to create new objects
     */
    PdfOutlineItem(PdfDocument& doc, const PdfString& title, const std::shared_ptr<PdfDestination>& dest,
        PdfOutlineItem* parentOutline);

    /** Create a new PdfOutlineItem from scratch
     *  \param title title of this item
     *  \param action action of this item
     *  \param parentOutline parent of this outline item
     *                        in the outline item hierarchie
     *  \param parent parent vector of objects which is required
     *                 to create new objects
     */
    PdfOutlineItem(PdfDocument& doc, const PdfString& title, const std::shared_ptr<PdfAction>& action,
        PdfOutlineItem* parentOutline);

    /** Create a PdfOutlineItem from an existing PdfObject
     *  \param obj an existing outline item
     *  \param parentOutline parent of this outline item
     *                        in the outline item hierarchie
     *  \param previous previous item of this item
     */
    PdfOutlineItem(PdfObject& obj, PdfOutlineItem* parentOutline, PdfOutlineItem* previous);

private:
    PdfOutlineItem* m_ParentOutline;

    PdfOutlineItem* m_Prev;
    PdfOutlineItem* m_Next;

    PdfOutlineItem* m_First;
    PdfOutlineItem* m_Last;

    std::shared_ptr<PdfDestination> m_destination;
    std::shared_ptr<PdfAction> m_action;
};

/** The main PDF outlines dictionary.
 *
 *  Do not create it by yourself but
 *  use PdfDocument::GetOutlines() instead.
 *
 *  \see PdfDocument
 */
class PODOFO_API PdfOutlines : public PdfOutlineItem
{
public:
    /** Create a new PDF outlines dictionary
     *  \param parent parent vector of objects
     */
    PdfOutlines(PdfDocument& doc);

    /** Create a PDF outlines object from an existing dictionary
     *  \param obj an existing outlines dictionary
     */
    PdfOutlines(PdfObject& obj);

    /** Create the root node of the
     *  outline item tree.
     *
     *  \param title the title of the root node
     */
    PdfOutlineItem* CreateRoot(const PdfString& title);
};

};

#endif // PDF_OUTLINE_H
