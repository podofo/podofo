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

#include "podofo-base.h"

// Include files from PoDoFo-doc
#include "doc/PdfAcroForm.h"
#include "doc/PdfAction.h"
#include "doc/PdfAnnotation.h"
#include "doc/PdfCMapEncoding.h"
#include "doc/PdfContents.h"
#include "doc/PdfDestination.h"
#include "doc/PdfDifferenceEncoding.h"
#include "doc/PdfDocument.h"
#include "doc/PdfElement.h"
#include "doc/PdfEncodingObjectFactory.h"
#include "doc/PdfExtGState.h"
#include "doc/PdfField.h"
#include "doc/PdfFileSpec.h"
#include "doc/PdfFontCache.h"
#include "doc/PdfFontCID.h"
#include "doc/PdfFontConfigWrapper.h"
#include "doc/PdfFontFactoryBase14Data.h"
#include "doc/PdfFontFactory.h"
#include "doc/PdfFont.h"
#include "doc/PdfFontMetricsBase14.h"
#include "doc/PdfFontMetricsFreetype.h"
#include "doc/PdfFontMetrics.h"
#include "doc/PdfFontMetricsObject.h"
#include "doc/PdfFontSimple.h"
#include "doc/PdfFontTrueType.h"
#include "doc/PdfFontTTFSubset.h"
#include "doc/PdfFontType1Base14.h"
#include "doc/PdfFontType1.h"
#include "doc/PdfFontType3.h"
#include "doc/PdfFunction.h"
#include "doc/PdfHintStream.h"
#include "doc/PdfIdentityEncoding.h"
#include "doc/PdfImage.h"
#include "doc/PdfInfo.h"
#include "doc/PdfMemDocument.h"
#include "doc/PdfNamesTree.h"
#include "doc/PdfOutlines.h"
#include "doc/PdfPage.h"
#include "doc/PdfPagesTreeCache.h"
#include "doc/PdfPagesTree.h"
#include "doc/PdfPainter.h"
#include "doc/PdfPainterMM.h"
#include "doc/PdfShadingPattern.h"
#include "doc/PdfSignatureField.h"
#include "doc/PdfSignOutputDevice.h"
#include "doc/PdfStreamedDocument.h"
#include "doc/PdfTable.h"
#include "doc/PdfTilingPattern.h"
#include "doc/PdfXObject.h"

#ifdef _PODOFO_NO_NAMESPACE_
using namespace PoDoFo;
#endif /* _PODOFO_NO_NAMESPACE_ */

#endif /* _PODOFO_H_ */


