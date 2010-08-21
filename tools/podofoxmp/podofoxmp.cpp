/***************************************************************************
 *   Copyright (C) 2010 by Ian Ashley                                      *
 *   Ian Ashley <Ian.Ashley@opentext.com>                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <iostream>
#include <cstdlib>
#include <cstdio>

#include <podofo.h>

using namespace std;

int main (int argc, char *argv[])
{
	PoDoFo::PdfError::EnableDebug(false);
	if (argc != 2 && argc != 4)
    {
		cout << "Syntax" << endl;
		cout << "  " << argv[0] << " <pdf file> - display the XMP in a file" << endl;
		cout << "or" << endl;
		cout << "  " << argv[0] << " <src pdf file> <xmp file> <new pdf file> - create a new PDF with the XMP in" << endl;
		return EXIT_FAILURE;
    }

	PoDoFo::PdfMemDocument *doc = new PoDoFo::PdfMemDocument(argv[1]);

	if (argc == 2)
    {
		PoDoFo::PdfObject *metadata;
		if ((metadata = doc->GetMetadata()) == NULL)
			cout << "No metadata" << endl;
		else
        {
			PoDoFo::PdfStream *str = metadata->GetStream();
			if (str != NULL)
            {
				char *buf;
				PoDoFo::pdf_long len;
	
				str->GetFilteredCopy(&buf, &len);
				for (PoDoFo::pdf_long i = 0; i < len; ++i)
					printf("%c", buf[i]);
				printf("\n");
				fflush(stdout);
				free(buf);
            }
        }
    }

	if (argc == 4)
    {
		char *xmpBuf;
		FILE *fp;

		if ((fp = fopen(argv[2], "rb")) == NULL)
			cout << "Cannot open " << argv[2] << endl;
		else
        {
			fseek(fp, 0, SEEK_END);
			long xmpLen = ftell(fp);
			xmpBuf = new char[xmpLen];
			fseek(fp, 0, SEEK_SET);
			fread(xmpBuf, 1, xmpLen, fp);
			fclose(fp);

			PoDoFo::PdfObject *metadata;
			if ((metadata = doc->GetMetadata()) != NULL)
				metadata->GetStream()->Set(xmpBuf, xmpLen, PoDoFo::TVecFilters());
			else
            {
				metadata = doc->GetObjects().CreateObject("Metadata");
				metadata->GetDictionary().AddKey(PoDoFo::PdfName("Subtype"), PoDoFo::PdfName("XML"));
				metadata->GetStream()->Set(xmpBuf, xmpLen, PoDoFo::TVecFilters());
				doc->GetCatalog()->GetDictionary().AddKey(PoDoFo::PdfName("Metadata"), metadata->Reference());
            }
			delete[] xmpBuf;

			doc->Write(argv[3]);
        }
    }

	delete doc;

	return EXIT_SUCCESS;
}
