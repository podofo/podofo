/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _PDF_OUTLINE_H_
#define _PDF_OUTLINE_H_

#include "PdfDefines.h"
#include "PdfElement.h"

#include <list>

namespace PoDoFo {

class PdfDestination;
class PdfObject;
class PdfOutlineItem;
class PdfString;
class PdfVecObjects;

typedef std::list<PdfOutlineItem*>       TOutlineItemList;
typedef TOutlineItemList::iterator       TIOutlineItemList;
typedef TOutlineItemList::const_iterator TCIOutlineItemList;

/**
 * The title of an outline item can be displayed
 * in different formating styles since PDF 1.4.
 */
typedef enum EPdfOutlineFormat {
    ePdfOutlineFormat_Default    = 0x00,   /**< Default format */
    ePdfOutlineFormat_Italic     = 0x01,   /**< Italic */
    ePdfOutlineFormat_Bold       = 0x02,   /**< Bold */
    ePdfOutlineFormat_BoldItalic = 0x03,   /**< Bold Italic */

    ePdfOutlineFormat_Unknown    = 0xFF
};

class PdfOutlineItem : public PdfElement {
 public:
    PdfOutlineItem( const PdfString & sTitle, const PdfDestination & rDest, PdfOutlineItem* pParentOutline, PdfVecObjects* pParent );

    PdfOutlineItem( PdfObject* pObject );

    virtual ~PdfOutlineItem();

    PdfOutlineItem* CreateChild( const PdfString & sTitle, const PdfDestination & rDest );
    PdfOutlineItem* CreateNext ( const PdfString & sTitle, const PdfDestination & rDest );

    inline PdfOutlineItem* Prev() const;
    inline PdfOutlineItem* Next() const;

    inline PdfOutlineItem* First() const;
    inline PdfOutlineItem* Last() const;

    inline PdfOutlineItem* ParentOutline() const;

    /** Set the destination of this outline.
     *  \param rDest the destination
     */
    void SetDestination( const PdfDestination & rDest );

    /** Set the title of this outline item
     *  \param sTitle the title to use
     */
    void SetTitle( const PdfString & sTitle );

    /** Get the title of this item
     *  \returns the title as PdfString
     */
    const PdfString & GetTitle() const;

    /** Set the text format of the title.
     *  Supported since PDF 1.4.
     *
     *  \param eFormat the formatting options 
     *                 for the title
     */
    void SetTextFormat( EPdfOutlineFormat eFormat );

    /** Get the text format of the title
     *  \returns the text format of the title
     */
    EPdfOutlineFormat GetTextFormat() const;

    /** Set the color of the title of this item.
     *  This property is supported since PDF 1.4.
     *  \param r red color component
     *  \param g green color component
     *  \param b blue color component
     */
    void SetTextColor( double r, double g, double b );

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
    void SetPrevious( PdfOutlineItem* pItem );
    void SetNext    ( PdfOutlineItem* pItem );
    void AppendChild( PdfOutlineItem* pItem, PdfOutlineItem* pChild );

 protected:
    PdfOutlineItem( PdfVecObjects* pParent );

 private:
    PdfOutlineItem*    m_pParentOutline;

    PdfOutlineItem*    m_pPrev;
    PdfOutlineItem*    m_pNext;

    TOutlineItemList   m_lstItems;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfOutlineItem* PdfOutlineItem::ParentOutline() const
{
    return m_pParentOutline;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfOutlineItem* PdfOutlineItem::First() const
{
    return m_lstItems.size() ? m_lstItems.front() : NULL;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfOutlineItem* PdfOutlineItem::Last() const
{
    return m_lstItems.size() ? m_lstItems.back() : NULL;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfOutlineItem* PdfOutlineItem::Prev() const
{
    return m_pPrev;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfOutlineItem* PdfOutlineItem::Next() const
{
    return m_pNext;
}


////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

class PdfOutlines : public PdfOutlineItem {
 public:
    
    PdfOutlines( PdfVecObjects* pParent );

    PdfOutlines( PdfObject* pObject );

    /** Create the root node of the 
     *  outline item tree.
     *
     *  \param sTitle the title of the root node
     */
    PdfOutlineItem* CreateRoot( const PdfString & sTitle );
};

};

#endif // _PDF_OUTLINE_H_
