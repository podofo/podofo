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

#ifndef _PDF_INFO_H_
#define _PDF_INFO_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfName.h"
#include "podofo/base/PdfDate.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfString;

/** This class provides access to the documents
 *  info dictionary, which provides information
 *  about the PDF document.
 */
class PODOFO_DOC_API PdfInfo : public PdfElement {
 public:
    /**
     * Enum to specifiy the initial information of the 
     * info dictionary.
     */
    enum EPdfInfoInitial {
        ePdfInfoInitial_WriteCreationTime     = 0x01, ///< Write the creation time (current time). Default for new documents.
        ePdfInfoInitial_WriteModificationTime = 0x02, ///< Write the modification time (current time). Default for loaded documents.
        ePdfInfoInitial_WriteProducer         = 0x04  ///< Write producer key. Default for new documents.
    };

    /** Create a new PdfInfo object
     *  \param pParent the parent of this object
     *  \param eInitial which information should be 
     *         writting initially to the information dictionary
     */
    PdfInfo( PdfVecObjects* pParent, 
             int eInitial = ePdfInfoInitial_WriteCreationTime | ePdfInfoInitial_WriteProducer );

    /** Create a PdfInfo object from an existing
     *  object in the PDF file.
     *  \param pObject must be an info dictionary.
     *  \param eInitial which information should be 
     *         writting initially to the information
     */
    PdfInfo( PdfObject* pObject, int eInitial = ePdfInfoInitial_WriteModificationTime );

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

    // Peter Petrov 27 April 2008
    /** Set the producer of the document.
     *  \param sProducer producer
     */
    void SetProducer( const PdfString & sProducer );

    // Peter Petrov 27 April 2008
    /** Get the producer of the document
     *  \returns the producer
     */
    inline const PdfString & GetProducer() const;

    /** Set the trapping state of the document.
     *  \param sTrapped trapped
     */
    void SetTrapped( const PdfName & sTrapped );
    
    /** Get the trapping state of the document
     *  \returns the title
     */
    inline const PdfName & GetTrapped() const;

    /** Get creation date of document
     *  \return creation date
     */
    inline PdfDate GetCreationDate() const;

    /** Get modification date of document
     *  \return modification date
     */
    inline PdfDate GetModDate() const;

    /** Set custom info key.
     * \param sName Name of the key.
     * \param sValue Value of the key.
     */
    void SetCustomKey(const PdfName &sName, const PdfString &sValue);
 private:
    /** Add the initial document information to the dictionary.
     *  \param eInitial which information should be 
     *         writting initially to the information
     */
    void Init( int eInitial );

    /** Get a value from the info dictionary as name
     *  \para rName the key to fetch from the info dictionary
     *  \return a value from the info dictionary
     */
    const PdfString & GetStringFromInfoDict( const PdfName & rName ) const;

     /** Get a value from the info dictionary as name
     *  \para rName the key to fetch from the info dictionary
     *  \return a value from the info dictionary
     */
    const PdfName & GetNameFromInfoDict( const PdfName & rName ) const;

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

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfInfo::GetProducer() const
{
    return this->GetStringFromInfoDict( PdfName("Producer") );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfName & PdfInfo::GetTrapped() const
{
	return this->GetNameFromInfoDict( PdfName("Trapped") );
}


// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfDate PdfInfo::GetCreationDate() const
{
    return PdfDate(this->GetStringFromInfoDict(PdfName("CreationDate")));
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfDate PdfInfo::GetModDate() const
{
    return PdfDate(this->GetStringFromInfoDict(PdfName("ModDate")));
}

};


#endif // _PDF_INFO_H_
