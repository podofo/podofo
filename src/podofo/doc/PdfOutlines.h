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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_OUTLINE_H_
#define _PDF_OUTLINE_H_

#include "podofo/base/PdfDefines.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfDestination;
class PdfAction;
class PdfObject;
class PdfOutlineItem;
class PdfString;
class PdfVecObjects;

/**
 * The title of an outline item can be displayed
 * in different formating styles since PDF 1.4.
 */
enum EPdfOutlineFormat {
    ePdfOutlineFormat_Default    = 0x00,   /**< Default format */
    ePdfOutlineFormat_Italic     = 0x01,   /**< Italic */
    ePdfOutlineFormat_Bold       = 0x02,   /**< Bold */
    ePdfOutlineFormat_BoldItalic = 0x03,   /**< Bold Italic */

    ePdfOutlineFormat_Unknown    = 0xFF
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
class PODOFO_DOC_API PdfOutlineItem : public PdfElement {
 public:
    virtual ~PdfOutlineItem();

    /** Create a PdfOutlineItem that is a child of this item
     *  \param sTitle title of this item
     *  \param rDest destination of this item
     */
    PdfOutlineItem* CreateChild( const PdfString & sTitle, const PdfDestination & rDest );

    /** Create a PdfOutlineItem that is on the same level and follows the current item.
     *  \param sTitle title of this item
     *  \param rDest destination of this item
     */
    PdfOutlineItem* CreateNext ( const PdfString & sTitle, const PdfDestination & rDest );

    /** Create a PdfOutlineItem that is on the same level and follows the current item.
     *  \param sTitle title of this item
     *  \param rAction action of this item
     */
    PdfOutlineItem* CreateNext ( const PdfString & sTitle, const PdfAction & rAction );

    /** Inserts a new PdfOutlineItem as a child of this outline item.
     *  The former can't be in the same tree as this one, as the tree property
     *  would be broken. If this prerequisite is violated, a PdfError
     *  exception (code ePdfError_OutlineItemAlreadyPresent) is thrown and
     *  nothing is changed.
     *  The item inserted is not copied, i.e. Erase() calls affect the original!
     *  Therefore also shared ownership is in effect, i.e. deletion by where it
     *  comes from damages the data structure it's inserted into.
     *
     *  \param pItem an existing outline item
     */
    void InsertChild( PdfOutlineItem* pItem );

    /** 
     * \returns the previous item or NULL if this is the first on the current level
     */
    inline PdfOutlineItem* Prev() const;

    /** 
     * \returns the next item or NULL if this is the last on the current level
     */
    inline PdfOutlineItem* Next() const;

    /** 
     * \returns the first outline item that is a child of this item
     */
    inline PdfOutlineItem* First() const;

    /** 
     * \returns the last outline item that is a child of this item
     */
    inline PdfOutlineItem* Last() const;

    /**
     * \returns the parent item of this item or NULL if it is
     *          the top level outlines dictionary
     */
    inline PdfOutlineItem* GetParentOutline() const;

    /** Deletes this outline item and all its children from 
     *  the outline hierarchy and removes all objects from
     *  the list of PdfObjects
     *  All pointers to this item will be invalid after this function
     *  call.
     */
    void Erase();

    /** Set the destination of this outline.
     *  \param rDest the destination
     */
    void SetDestination( const PdfDestination & rDest );

    /** Get the destination of this outline.
     *  \param pDoc a PdfDocument owning this annotation.
     *         This is required to resolve names and pages.
     *  \returns the destination, if there is one, or NULL
     */
    PdfDestination* GetDestination( PdfDocument* pDoc );

    /** Set the action of this outline.
     *  \param rAction the action
     */
    void SetAction( const PdfAction & rAction );

    /** Get the action of this outline.
     *  \returns the action, if there is one, or NULL
     */
    PdfAction* GetAction( void );

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
    void SetLast    ( PdfOutlineItem* pItem );
    void SetFirst   ( PdfOutlineItem* pItem );

    void InsertChildInternal( PdfOutlineItem* pItem, bool bCheckParent );

 protected:
    /** Create a new PdfOutlineItem dictionary
     *  \param pParent parent vector of objects
     */
    PdfOutlineItem( PdfVecObjects* pParent );

    /** Create a new PdfOutlineItem from scratch
     *  \param sTitle title of this item
     *  \param rDest destination of this item
     *  \param pParentOutline parent of this outline item 
     *                        in the outline item hierarchie
     *  \param pParent parent vector of objects which is required
     *                 to create new objects
     */
    PdfOutlineItem( const PdfString & sTitle, const PdfDestination & rDest, 
                    PdfOutlineItem* pParentOutline, PdfVecObjects* pParent );

    /** Create a new PdfOutlineItem from scratch
     *  \param sTitle title of this item
     *  \param rAction action of this item
     *  \param pParentOutline parent of this outline item 
     *                        in the outline item hierarchie
     *  \param pParent parent vector of objects which is required
     *                 to create new objects
     */
    PdfOutlineItem( const PdfString & sTitle, const PdfAction & rAction, 
                    PdfOutlineItem* pParentOutline, PdfVecObjects* pParent );

	/** Create a PdfOutlineItem from an existing PdfObject
     *  \param pObject an existing outline item
     *  \param pParentOutline parent of this outline item 
     *                        in the outline item hierarchie
     *  \param pPrevious previous item of this item
     */
    PdfOutlineItem( PdfObject* pObject, PdfOutlineItem* pParentOutline, PdfOutlineItem* pPrevious );

 private:
    PdfOutlineItem*    m_pParentOutline;

    PdfOutlineItem*    m_pPrev;
    PdfOutlineItem*    m_pNext;

    PdfOutlineItem*    m_pFirst;
    PdfOutlineItem*    m_pLast;

    PdfDestination*    m_pDestination;
    PdfAction*		   m_pAction;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfOutlineItem* PdfOutlineItem::GetParentOutline() const
{
    return m_pParentOutline;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfOutlineItem* PdfOutlineItem::First() const
{
    return m_pFirst;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfOutlineItem* PdfOutlineItem::Last() const
{
    return m_pLast;
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


/** The main PDF outlines dictionary.
 *  
 *  Do not create it by yourself but 
 *  use PdfDocument::GetOutlines() instead.
 *
 *  \see PdfDocument
 */
class PODOFO_DOC_API PdfOutlines : public PdfOutlineItem {
 public:
   
    /** Create a new PDF outlines dictionary
     *  \param pParent parent vector of objects
     */
    PdfOutlines( PdfVecObjects* pParent );

    /** Create a PDF outlines object from an existing dictionary
     *  \param pObject an existing outlines dictionary
     */
    PdfOutlines( PdfObject* pObject );

    virtual ~PdfOutlines() { }

    /** Create the root node of the 
     *  outline item tree.
     *
     *  \param sTitle the title of the root node
     */
    PdfOutlineItem* CreateRoot( const PdfString & sTitle );
};

};

#endif // _PDF_OUTLINE_H_
