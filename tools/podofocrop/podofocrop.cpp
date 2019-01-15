/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
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

#include <podofo.h>

#include <cstdlib>
#include <cstdio>

#include <vector>

#ifdef _WIN32
# include <windows.h>
# ifdef GetObject
#  undef GetObject
# endif 
#else
# include <unistd.h>
# include <sys/types.h> 
# include <sys/wait.h> 
#endif // _WIN32

using namespace PoDoFo;

#ifdef _HAVE_CONFIG
#include <config.h>
#endif // _HAVE_CONFIG

void print_help()
{
    printf("Usage: podofocrop input.pdf output.pdf\n");
    printf("       This tool will crop all pages.\n");
    printf("       It requires ghostscript to be in your PATH\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

void crop_page( PdfPage* pPage, const PdfRect & rCropBox ) 
{
    PdfVariant var;
    /*
    printf("%f %f %f %f\n",
           rCropBox.GetLeft(),
           rCropBox.GetBottom(),
           rCropBox.GetWidth(),
           rCropBox.GetHeight());
    */
    rCropBox.ToVariant( var );
    if (!pPage)
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle,
                                "crop_page: No page pointer given" );
    }
    pPage->GetObject()->GetDictionary().AddKey( PdfName("MediaBox"), var );
}

std::string get_ghostscript_output( const char* pszInput )
{
	std::string sOutput;
    const int lBufferLen = 256;
    char buffer[lBufferLen];

#ifdef _WIN32
    DWORD count;
	char cmd[lBufferLen];

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    // Fenster nicht sichtbar
    si.dwFlags=STARTF_USESHOWWINDOW;
    si.wShowWindow=SW_HIDE;

    // Ausgabe umleiten
    HANDLE pipe;
    CreatePipe(&pipe, 0, 0, 0 );
    si.dwFlags|=STARTF_USESTDHANDLES;
    //si.hStdOutput=pipe_wr;
	si.hStdError = pipe;

	
	_snprintf(cmd, lBufferLen, "gs -dSAFER -sDEVICE=bbox -sNOPAUSE -q %s -c quit", pszInput);
	printf("Running %s\n", cmd );
    if( !CreateProcessA( NULL, cmd, NULL, NULL, TRUE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi ) ) 
    {
		printf("CreateProcess failed.");
        exit(1);
    }

    while( ReadFile(pipe,buffer,lBufferLen,&count,NULL) && GetLastError() != ERROR_BROKEN_PIPE && count > 0)
    {
		printf("%s",buffer);
        sOutput.append( buffer, count );
    }

	// eigenes Handle schliessen
    CloseHandle(pipe);
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread ); 
#else
    pid_t pid;
    int p[2];
    int count;

    pipe( p );

    if( (pid = fork()) == 0 ) 
    {        
        // Child, launch ghostscript
        close( p[0] ); // Close unused read end

        dup2(p[1], 2); // redirect stderr to stdout
        dup2(p[1], 1); 

        //printf("HELLO\n");
        execlp( "gs", "gs", "-dSAFER", "-sDEVICE=bbox",
                          "-sNOPAUSE", "-q", pszInput, "-c", "quit", NULL );
        printf("Fatal error, cannot launch ghostscript\n");
        exit(0);
    }
    else
    {
        close( p[1] ); // Close unused write end

        while( (count = read( p[0], buffer, lBufferLen )) > 0 )
        {
            sOutput.append( buffer, count );
        }
        wait(NULL);
    }

#endif // _WIN32
	return sOutput;
}

std::vector<PdfRect> get_crop_boxes( const char* pszInput )
{
    std::vector<PdfRect> rects;
	std::string sOutput = get_ghostscript_output( pszInput );

	std::stringstream ss(sOutput);
    std::string sLine;
    PdfRect curRect;
    bool bHaveRect = false;
    while(std::getline(ss, sLine)) 
    {
        if( strncmp( "%%BoundingBox: ", sLine.c_str(), 15 ) == 0 )
        {
            int x, y, w, h;
            if( sscanf( sLine.c_str() + 15, "%i %i %i %i\n", &x, &y, &w, &h ) != 4 )
            {
                printf( "Failed to read bounding box's four numbers from '%s'\n", sLine.c_str() + 15 );
                exit( 1 );
            }
            curRect = PdfRect( static_cast<double>(x), 
                               static_cast<double>(y),
                               static_cast<double>(w-x),
                               static_cast<double>(h-y) );
            bHaveRect = true;
        } 
        else if( strncmp( "%%HiResBoundingBox: ", sLine.c_str(), 17 ) == 0 ) 
        {
            if( bHaveRect ) 
            {
                // I have no idea, while gs writes BoundingBoxes twice to stdout ..
                printf("Using bounding box: [ %f %f %f %f ]\n", 
                       curRect.GetLeft(),
                       curRect.GetBottom(),
                       curRect.GetWidth(),
                       curRect.GetHeight());
                rects.push_back( curRect );
                bHaveRect = false;
            }
        }
    }
	
    return rects;
}

int main( int argc, char* argv[] )
{
    PdfError::EnableDebug( false );

    if( argc != 3 )
    {
        print_help();
        exit( -1 );
    }
    
    const char* pszInput = argv[1];
    const char* pszOutput = argv[2];

    try {
        printf("Cropping file:\t%s\n", pszInput);
        printf("Writing to   :\t%s\n", pszOutput);
 
        std::vector<PdfRect> cropBoxes = get_crop_boxes( pszInput );

        PdfMemDocument doc;
        doc.Load( pszInput );

        if( static_cast<int>(cropBoxes.size()) != doc.GetPageCount() ) 
        {
            printf("Number of cropboxes obtained form ghostscript does not match with page count (%i, %i)\n",
                   static_cast<int>(cropBoxes.size()), doc.GetPageCount() );
            PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
        }

        for( int i=0;i<doc.GetPageCount(); i++ ) 
        {
            PdfPage* pPage = doc.GetPage( i ); 
            crop_page( pPage, cropBoxes[i] );
        }

        doc.Write( pszOutput );
        
    } catch( PdfError & e ) {
        fprintf( stderr, "Error: An error %i ocurred during croppping pages in the pdf file.\n", e.GetError() );
        e.PrintErrorMsg();
        return e.GetError();
    }
    
    return 0;
}

