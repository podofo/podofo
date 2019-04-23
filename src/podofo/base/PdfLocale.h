/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#ifndef PODOFO_PDFLOCALE_H
#define PODOFO_PDFLOCALE_H

#include <ios>

namespace PoDoFo {

/**
 * The locale to use for PDF I/O . See PoDoFo::PdfLocaleImbue() .
 */
static const char PdfIOLocale[] = "C";

/**
 * Imbue the passed stream with a locale that will be safe to do
 * I/O of the low level PDF format with. 
 *
 * PDF document structure I/O is done with the C++ standard library
 * IOStreams code. By default, this will adapt to the current locale.
 * That's not good at all when doing I/O of PDF data structures, which
 * follow POSIX/english locale conventions irrespective of runtime locale.
 * Make sure to to call this function on any stream you intend to use for
 * PDF I/O. Avoid using this stream for anything that should be done in the
 * regional locale.
 *
 * \warning This method may throw ePdfError_InvalidDeviceOperation
 *          if your STL does not support the locale string in PdfIOLocale .
 *
 * If you fail to call this on a stream you use for PDF I/O you will encounter
 * problems like German and other European users getting numbers in the format
 * "10110,4" or even "10.110,4" instead of "10110.4" .
 */
void PODOFO_API PdfLocaleImbue(std::ios_base&);

};

#endif
