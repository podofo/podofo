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

#ifndef _PDF_INFO_H_
#define _PDF_INFO_H_

#include "PdfDefines.h"
#include "PdfElement.h"
#include "PdfName.h"

namespace PoDoFo {

class PdfString;

/** This class provides access to the documents
 *  info dictionary, which provides information
 *  about the PDF document.
 */
class PdfInfo : public PdfElement {
 public:
    /** Create a new PdfInfo object
     *  \param pParent the parent of this object
     */
    PdfInfo( PdfVecObjects* pParent );

    /** Create a PdfInfo object from an existing
     *  object in the PDF file.
     *  \param pObject must be an info dictionary.
     */
    PdfInfo( PdfObject* pObject );

    /** Destructor
     */
    ~PdfInfo();

    /** Set the author of the document.
     *  \param sAuthor author
     */
    void SetAuthor( const PdfString & sAuthor );

    /** Get the author of the document
     *  \returns the author
     */
    inline const PdfString & GetAuthor() const;

    /** Set the creator of the document.
     *  Typically the name of the application using the library.
     *  \param sCreator creator
     */
    void SetCreator( const PdfString & sCreator );

    /** Get the creator of the document
     *  \returns the creator
     */
    inline const PdfString & GetCreator() const;

    /** Set keywords for this document
     *  \param sKeywords a list of keywords
     */
    void SetKeywords( const PdfString & sKeywords );

    /** Get the keywords of the document
     *  \returns the keywords
     */
    inline const PdfString & GetKeywords() const;

    /** Set the subject of the document.
     *  \param sSubject subject
     */
    void SetSubject( const PdfString & sSubject );

    /** Get the subject of the document
     *  \returns the subject
     */
    inline const PdfString & GetSubject() const;

    /** Set the title of the document.
     *  \param sTitle title
     */
    void SetTitle( const PdfString & sTitle );

    /** Get the title of the document
     *  \returns the title
     */
    inline const PdfString & GetTitle() const;

 private:
    /** Add the initial document information to the dictionary.
     *  \param bModify if true a ModDate will be added instead
     *                 of a CreationDate.
     */
    void Init( bool bModify = false );

    /** Get a value from the info dictionary as name
     *  \para rName the key to fetch from the info dictionary
     *  \return a value from the info dictionary
     */
    const PdfString & GetStringFromInfoDict( const PdfName & rName ) const;

};

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfInfo::GetAuthor() const
{
    return this->GetStringFromInfoDict( PdfName("Author") );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfInfo::GetCreator() const
{
    return this->GetStringFromInfoDict( PdfName("Creator") );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfInfo::GetKeywords() const
{
    return this->GetStringFromInfoDict( PdfName("Keywords") );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfInfo::GetSubject() const
{
    return this->GetStringFromInfoDict( PdfName("Subject") );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfInfo::GetTitle() const
{
    return this->GetStringFromInfoDict( PdfName("Title") );
}

};


#endif // _PDF_INFO_H_
