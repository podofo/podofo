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
#include "PdfObject.h"

namespace PoDoFo {

class PdfAction;
class PdfPage;
class PdfRect;
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
class PdfAnnotation : public PdfObject {
 public:
    PdfAnnotation( unsigned int nObjectNo, unsigned int nGenerationNo );

    /** Initalize the PdfAnnotation object
     *  \param pPage the parent page of this annotation
     *  \param eAnnot type of the annotation
     *  \param rRect the rectangle in which the annotation will appear on the page
     *
     *  \see EPdfAnnotation
     */
    PdfError Init( PdfPage* pPage, EPdfAnnotation eAnnot, PdfRect & rRect );

    /** Initalize the PdfAnnotation object
     *
     *  This init function is provided so that you can annotation to PdfObjects which 
     *  are pages you got from a PdfParser but do not have a PdfPage object for.
     *
     *  \param pObject the parent page of this annotation (should be a PdfPage object)
     *  \param eAnnot type of the annoation
     *  \param rRect the rectangle in which the annotation will appear on the page
     *
     *  \see EPdfAnnotation
     */
    PdfError Init( PdfObject* pObject, EPdfAnnotation eAnnot, PdfRect & rRect );

    /** Set an appearance stream for this object
     *  to specify its visual appearance
     *  \param pObject an XObject
     *  \returns ErrOk on success
     */
    PdfError SetAppearanceStream( PdfXObject* pObject );

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
     *  \returns ErrOk on success
     */
    PdfError SetDestination( const PdfPage* pPage );

    /** Set the destination for Link annotations
     *  \param rReference must be an indirect reference to an page object
     *  \returns ErrOk on success
     */
    PdfError SetDestination( const PdfVariant & rReference );

    /** Set the destination for Link annotations
     *  Perform an action when clicking on this action.
     *  \param pAction a PdfAction
     *  \returns ErrOk on success
     */
    PdfError SetDestination( const PdfAction* pAction );

    /** Get the type of this annotation
     *  \returns the annotation type
     */
    inline EPdfAnnotation type() const;

 private:

    /** Convert an annotation enum to its string representation
     *  which can be written to the PDF file.
     *  \returns the string representation or NULL for unsupported annotation types
     */
    const char* AnnotationKey( EPdfAnnotation eAnnot );

    PdfError AddReferenceToKey( PdfObject* pObject, const char* pszKeyName, const char* pszReference );

 private:
    EPdfAnnotation m_eAnnotation;
};

EPdfAnnotation PdfAnnotation::type() const
{
    return m_eAnnotation;
}

};

#endif /* _PDF_ANNOTATION_H_ */
