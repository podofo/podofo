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

/**
 * This file can be used in client applications to include
 * all files required by PoDoFo at once.
 *
 * Some symbols may be declared in the PoDoFo::NonPublic namespace.
 * Client applications must never rely on or use these symbols directly.
 * On supporting platforms they will be excluded from the DLL interface,
 * and they are not guaranteed to continue to exist.
 */

#include "PdfVersion.h"
#include "PdfDefines.h"
#include "PdfAction.h"
#include "PdfAcroForm.h"
#include "PdfAnnotation.h"
#include "PdfArray.h"
#include "PdfCanvas.h"
#include "PdfColor.h"
#include "PdfContentsTokenizer.h"
#include "PdfData.h"
#include "PdfDataType.h"
#include "PdfDate.h"
#include "PdfDestination.h"
#include "PdfDictionary.h"
#include "PdfDifferenceEncoding.h"
#include "PdfDocument.h"
#include "PdfElement.h"
#include "PdfEncoding.h"
#include "PdfEncodingFactory.h"
#include "PdfEncrypt.h"
#include "PdfError.h"
#include "PdfExtGState.h"
#include "PdfField.h"
#include "PdfFileSpec.h"
#include "PdfFilter.h"
#include "PdfFont.h"
#include "PdfFontCache.h"
#include "PdfFontFactory.h"
#include "PdfFontMetrics.h"
#include "PdfFontTTFSubset.h"
#include "PdfFunction.h"
#include "PdfImage.h"
#include "PdfInfo.h"
#include "PdfInputDevice.h"
#include "PdfMemDocument.h"
#include "PdfMemStream.h"
#include "PdfName.h"
#include "PdfNamesTree.h"
#include "PdfObject.h"
#include "PdfOutlines.h"
#include "PdfOutputDevice.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"
#include "PdfPagesTreeCache.h"
#include "PdfPainter.h"
#include "PdfPainterMM.h"
#include "PdfParser.h"
#include "PdfParserObject.h"
#include "PdfRect.h"
#include "PdfRefCountedBuffer.h"
#include "PdfRefCountedInputDevice.h"
#include "PdfShadingPattern.h"
#include "PdfStream.h"
#include "PdfStreamedDocument.h"
#include "PdfString.h"
#include "PdfTable.h"
#include "PdfTokenizer.h"
#include "PdfVariant.h"
#include "PdfVecObjects.h"
#include "PdfVersion.h"
#include "PdfWriter.h"
#include "PdfXObject.h"
#include "PdfXRef.h"
#include "PdfXRefStream.h"
#include "PdfXRefStreamParserObject.h"

#if 0
#ifndef _PODOFO_NO_NAMESPACE_
using namespace PoDoFo;
#endif /* _PODOFO_NO_NAMESPACE_ */
#endif

#endif /* _PODOFO_H_ */


