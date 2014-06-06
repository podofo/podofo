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

#ifndef _PDF_CANVAS_H_
#define _PDF_CANVAS_H_

#include "PdfDefines.h"
#include "PdfArray.h"

namespace PoDoFo {

class PdfDictionary;
class PdfObject;
class PdfRect;
class PdfColor;

/** A interface that provides the necessary features 
 *  for a painter to draw onto a PdfObject.
 */
class PODOFO_API PdfCanvas {
 public:
    /** Virtual destructor
     *  to avoid compiler warnings
     */
    virtual ~PdfCanvas() {};

    /** Get access to the contents object of this page.
     *  If you want to draw onto the page, you have to add 
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    virtual PdfObject* GetContents() const = 0;

    /** Get access an object that you can use to ADD drawing to.
     *  If you want to draw onto the page, you have to add 
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    virtual PdfObject* GetContentsForAppending() const = 0;

    /** Get access to the resources object of this page.
     *  This is most likely an internal object.
     *  \returns a resources object
     */
    virtual PdfObject* GetResources() const = 0;

    /** Get the current page size in PDF Units
     *  \returns a PdfRect containing the page size available for drawing
     */
    virtual const PdfRect GetPageSize() const = 0;

    /** Get a reference to a static procset PdfArray.
     *  \returns a reference to a static procset PdfArray
     */
    static const PdfArray & GetProcSet();

	/** Register a colourspace for a (separation) colour in the resource dictionary 
	 *  of this page or XObbject so that it can be used for any following drawing 
	 *  operations.
     *  
     *  \param rColor reference to the PdfColor
     */
	void AddColorResource( const PdfColor & rColor );

	/** Register an object in the resource dictionary of this page or XObbject
     *  so that it can be used for any following drawing operations.
     *  
     *  \param rIdentifier identifier of this object, e.g. /Ft0
     *  \param rRef reference to the object you want to register
     *  \param rName register under this key in the resource dictionary
     */
    void AddResource( const PdfName & rIdentifier, const PdfReference & rRef, const PdfName & rName );

 private:
    /** The procset is the same for all 
     *  PdfContents objects
     */
    static PdfArray s_procset;
};

};

#endif /* _PDF_CANVAS_H_ */
