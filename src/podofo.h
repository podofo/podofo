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

#ifndef _PODOFO_H_
#define _PODOFO_H_

/** This file can be used in client applications to include
 *  all files required by PoDoFo at once.
 *
 */

#include "PdfDefines.h"

/**
 * Version number of this release
 */
#define PODOFO_MAJOR 0
#define PODOFO_MINOR 4
#define PODOFO_REVISION 0

#include "PdfAction.h"
#include "PdfAnnotation.h"
#include "PdfArray.h"
#include "PdfCanvas.h"
#include "PdfData.h"
#include "PdfDataType.h"
#include "PdfDate.h"
#include "PdfDestination.h"
#include "PdfDocument.h"
#include "PdfElement.h"
#include "PdfError.h"
#include "PdfExtGState.h"
#include "PdfFileSpec.h"
#include "PdfFilter.h"
#include "PdfFont.h"
#include "PdfFontCache.h"
#include "PdfFontMetrics.h"

// Not considereed to be part of the public API
#include "PdfHintStream.h"
// --

#include "PdfImage.h"
#include "PdfInfo.h"
#include "PdfInputDevice.h"
#include "PdfMemStream.h"
#include "PdfName.h"
#include "PdfNamesTree.h"
#include "PdfObject.h"
#include "PdfOutlines.h"
#include "PdfOutputDevice.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"
#include "PdfPainter.h"
#include "PdfPainterMM.h"
#include "PdfParser.h"
#include "PdfParserObject.h"
#include "PdfRect.h"
#include "PdfRefCountedBuffer.h"
#include "PdfRefCountedInputDevice.h"
#include "PdfStream.h"
#include "PdfString.h"
#include "PdfTokenizer.h"
#include "PdfVariant.h"
#include "PdfVecObjects.h"
#include "PdfWriter.h"
#include "PdfXObject.h"
#include "PdfXRef.h"
#include "PdfXRefStream.h"

#if 0
#ifndef _PODOFO_NO_NAMESPACE_
using namespace PoDoFo;
#endif /* _PODOFO_NO_NAMESPACE_ */
#endif

#endif /* _PODOFO_H_ */


