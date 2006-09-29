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

#ifndef _PDF_ANNOTATION_H_
#define _PDF_ANNOTATION_H_

#include "PdfDefines.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfAction;
class PdfName;
class PdfPage;
class PdfRect;
class PdfReference;
class PdfString;
class PdfXObject;

/** The type of the annotation.
 *  PDF supports different annotation types, each of 
 *  them has different keys and propeties.
 *  
 *  Not all annotation types listed here are supported yet.
 *
 *  Please make also sure that the annotation type you use is
 *  supported by the PDF version you are using.
 */
typedef enum EPdfAnnotation {
    ePdfAnnotation_Text = 0,                   // - supported
    ePdfAnnotation_Link,
    ePdfAnnotation_FreeText,       // PDF 1.3  // - supported
    ePdfAnnotation_Line,           // PDF 1.3  // - supported
    ePdfAnnotation_Square,         // PDF 1.3
    ePdfAnnotation_Circle,         // PDF 1.3
    ePdfAnnotation_Polygon,        // PDF 1.5
    ePdfAnnotation_PolyLine,       // PDF 1.5
    ePdfAnnotation_Highlight,      // PDF 1.3
    ePdfAnnotation_Underline,      // PDF 1.3
    ePdfAnnotation_Squiggly,       // PDF 1.4
    ePdfAnnotation_StrikeOut,      // PDF 1.3
    ePdfAnnotation_Stamp,          // PDF 1.3
    ePdfAnnotation_Caret,          // PDF 1.5
    ePdfAnnotation_Ink,            // PDF 1.3
    ePdfAnnotation_Popup,          // PDF 1.3
    ePdfAnnotation_FileAttachement,// PDF 1.3
    ePdfAnnotation_Sound,          // PDF 1.2
    ePdfAnnotation_Movie,          // PDF 1.2
    ePdfAnnotation_Widget,         // PDF 1.2  // - supported
    ePdfAnnotation_Screen,         // PDF 1.5
    ePdfAnnotation_PrinterMark,    // PDF 1.4
    ePdfAnnotation_TrapNet,        // PDF 1.3
    ePdfAnnotation_Watermark,      // PDF 1.6
    ePdfAnnotation_3D,             // PDF 1.6

    ePdfAnnotation_Unknown = 0xff
};

/** Flags that control the appearance of a PdfAnnotation.
 *  You can OR them together and pass it to 
 *  PdfAnnotation::SetFlags.
 */
typedef enum EPdfAnnotationFlags {
    ePdfAnnotationFlags_Invisible    = 0x0001,
    ePdfAnnotationFlags_Hidden       = 0x0002,
    ePdfAnnotationFlags_Print        = 0x0004,
    ePdfAnnotationFlags_NoZoom       = 0x0008,
    ePdfAnnotationFlags_NoRotate     = 0x0010,
    ePdfAnnotationFlags_NoView       = 0x0020,
    ePdfAnnotationFlags_ReadOnly     = 0x0040,
    ePdfAnnotationFlags_Locked       = 0x0080,
    ePdfAnnotationFlags_ToggleNoView = 0x0100,

    ePdfAnnotationFlags_Unknow       = 0xffff
};

/** An annotation to a PdfPage 
 */
class PdfAnnotation : public PdfElement {
 public:
    /** Create a new annotation object
     *
     *  \param pPage the parent page of this annotation
     *  \param eAnnot type of the annotation
     *  \param rRect the rectangle in which the annotation will appear on the page
     *  \param pParent parent of this annotation
     */
    PdfAnnotation( PdfPage* pPage, EPdfAnnotation eAnnot, const PdfRect & rRect, PdfVecObjects* pParent );

    /** Create a PdfAnnotation from an existing PdfObject
     *
     *  \param pPage the parent page of this annotation
     *  \param eAnnot type of the annotation
     *  \param rRect the rectangle in which the annotation will appear on the page
     *  \param pObject an annotation object
     */
    PdfAnnotation( PdfPage* pPage, EPdfAnnotation eAnnot, const PdfRect & rRect, PdfObject* pObject );

    PdfAnnotation( PdfPage* pPage, PdfObject* pObject );

    /** Set an appearance stream for this object
     *  to specify its visual appearance
     *  \param pObject an XObject
     */
    void SetAppearanceStream( PdfXObject* pObject );

    /** Set the flags of this annotation.
     *  \param uiFlags is an unsigned 32bit integer with different 
     *                 EPdfAnnotationFlags OR'ed together.
     */
    void SetFlags( pdf_uint32 uiFlags );

    /** Set the title of this annotation.
     *  \param sTitle title of the annoation as string in PDF format
     */
    void SetTitle( const PdfString & sTitle );

    /** Set the text of this annotation.
     *  Use this if type() == ePdfAnnotation_Text
     *  \param sContents text of the annoation as string in PDF format
     */
    void SetContents( const PdfString & sContents );

    /** Set the destination for Link annotations
     *  \param pPage target of the link
     */
    void SetDestination( const PdfPage* pPage );

    /** Set the destination for Link annotations
     *  \param rReference must be an indirect reference to an page object
     */
    void SetDestination( const PdfReference & rReference );

    /** Set the destination for Link annotations
     *  Perform an action when clicking on this action.
     *  \param pAction a PdfAction
     */
    void SetDestination( const PdfAction* pAction );

    /** Get the type of this annotation
     *  \returns the annotation type
     */
    inline EPdfAnnotation GetType() const;

 private:
    /** Convert an annotation enum to its string representation
     *  which can be written to the PDF file.
     *  \returns the string representation or NULL for unsupported annotation types
     */
    const char* AnnotationKey( EPdfAnnotation eAnnot );

    void AddReferenceToKey( PdfObject* pObject, const PdfName & keyName, const PdfReference & rRef );

    static const long  s_lNumActions;
    static const char* s_names[];

 private:
    EPdfAnnotation m_eAnnotation;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline EPdfAnnotation PdfAnnotation::GetType() const
{
    return m_eAnnotation;
}

};

#endif /* _PDF_ANNOTATION_H_ */
